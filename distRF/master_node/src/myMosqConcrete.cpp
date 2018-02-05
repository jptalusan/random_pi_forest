#include "myMosqConcrete.h"
//MASTER
myMosqConcrete::myMosqConcrete(const char* id, const char* _topic, const char* host, int port, Utils::Configs c)
        : myMosq(id, _topic, host, port), publishedNodes(0) {
    std::cout << "Master node mqtt setup" << std::endl;
    this->c = c;
    t = Utils::Timer();
    t.start();
    firstAckReceived = false;
    hasFailed = false;
    stopThreadFlag = false;
    firstAckReceivedNodes = false;
    std::string topic("flask/query/" + c.nodeName);
    updateFlask(topic, "available");
    this->isProcessing = false;
}

void myMosqConcrete::publishedNodesTimeout(std::function<void()> f, int TIMEOUT) {
    auto timeStart = clock();
    while (true)
    {
        //Stop timeout thread when a new forest is received. To prevent spurious timeouts and leaks?l
        if (stopThreadFlag) {
            firstAckReceived = false;
            break;
        }

        if ((clock() - timeStart) / CLOCKS_PER_SEC >= TIMEOUT) {// time in seconds
            std::cout << "Timed out publishedNodes: Proceeding with processing." << std::endl;
            std::string topic("flask/query/" + c.nodeName);
            updateFlask(topic, "busy");
            std::thread t(f);
            t.detach();
            // f();
            break;
        }
    }
}

void myMosqConcrete::queryNodesTimeout(std::function<std::vector<NodeClass>()> f, int TIMEOUT) {
    auto timeStart = clock();
    while (true)
    {
        if ((clock() - timeStart) / CLOCKS_PER_SEC >= TIMEOUT) {// time in seconds
            std::cout << "Timed out querynodes: Proceeding with processing." << std::endl;
            nodeClassList = f();
            break;
        }
    }
}

void myMosqConcrete::reset() {
    nodeClassList.clear();
    publishedNodes.clear();
    availableNodes.clear();
    availableNodesAtEnd.clear();
    nodes.clear();
    firstAckReceived = false;
    hasFailed = false;
    stopThreadFlag = false;
    firstAckReceivedNodes = false;
    this->isProcessing = false;
}

//Will look through all available nodes and only use the number stated by the configs
//Remaining nodes will be reserves in case the other nodes fail
std::vector<NodeClass> myMosqConcrete::generateNodeAndDataList() {
    std::cout << "Trying to identify reserves and used nodes..." << std::endl;
    std::cout << Utils::Command::exec("rm data* RTs_Forest*") << std::endl;
    std::cout << "trainingDataFileName: " << c.trainingDataFileName << std::endl;
    std::vector<std::string> data = readFileToBuffer(c.trainingDataFileName);

    std::vector<int> v(data.size());
    std::iota(v.begin(), v.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);
    
    int numberOfAvailableNodes = this->availableNodes.size();

    if (numberOfAvailableNodes < c.numberOfNodes) {
        std::cout << "Warning: number of available nodes are less than the setting in config.json, please adjust." << std::endl;
    } else {
        numberOfAvailableNodes = c.numberOfNodes;
    }

    concurrentReads(numberOfAvailableNodes, data, v);
    
    std::vector<std::string> files = Utils::FileList::listFilesWithNameAndExtension("data", ".txt");
    int sizeOfFilesList = files.size();

    std::set<int>::iterator it = this->availableNodes.begin();

    std::vector<NodeClass> ncVec;
    for (int i = 0; i < sizeOfFilesList; ++i) {
        NodeClass nc;
        nc.nodeNumber = *it;
        nc.dataTextfileName = files[i];
        ncVec.push_back(nc);
        this->availableNodes.erase(it);
        ++it;
    }

    for (auto n : this->availableNodes) {
        std::cout << "Reserves: " << n << std::endl;
    }

    for (auto nc : ncVec) {
        std::cout << "Used node: " << nc.nodeNumber << ", for file: " << nc.dataTextfileName << std::endl;
    }

    #pragma omp parallel num_threads(sizeOfFilesList)
    {
        int index = omp_get_thread_num();
        std::string buffer = fileToBuffer(ncVec[index].dataTextfileName);
        std::stringstream ss;
        ss << "slave/train/node" << ncVec[index].nodeNumber;
        send_message(ss.str().c_str(), buffer.c_str());
        index++;
    }
    return ncVec;
}

/*
struct mosquitto_message{
        int mid;
        char *topic;
        void *payload;
        int payloadlen;
        int qos;
        bool retain;
};
*/
bool myMosqConcrete::receive_message(const struct mosquitto_message* message) {
    std::string topic(message->topic);
    char* pchar = (char*)(message->payload);
    std::string msg(pchar);

    std::string forPrint = "";
    if (message->payloadlen > 10) {
        forPrint = msg.substr(0, 10);
    } else {
        forPrint = msg;
    }

    std::cout << "t: " << topic << ", msg: " << forPrint << std::endl;
    if (topic.find("master/start/flask") != std::string::npos) {
        sendSlavesQuery("start");
        return true;
    } else if (topic.find("query/flask") != std::string::npos) {
        std::string topic("flask/query/" + c.nodeName);
        updateFlask(topic, this->isProcessing ? "busy" : "available");
    } 

    std::string node("node");
    if (topic.find(node) != std::string::npos) { //All messages from slave nodes
        int nodeIndex = topic.find(node) + node.length();
        int receivedNodeNumber = std::stoi(topic.substr(nodeIndex, topic.length()));
        printf("Received message from node:%d\n", receivedNodeNumber);
        if (topic.find("forest/node") != std::string::npos) {
            #pragma omp task
            {
                checkNodePayload(receivedNodeNumber, msg);
            }
        } else if (topic.find("lastWill/node") != std::string::npos) {
            //Just exit and do nothing when no available nodes yet.
            if (availableNodes.size() == 0) {
                return false;
            }
            //TODO: Send data to next node in line (and continue waiting for timeout if not appear)
            //If node that died is on the list, then pass the data to the others and remove from list
            std::cout << topic << ":" << msg << std::endl;
            std::cout << "Received node number: " << receivedNodeNumber << std::endl;
            //debug list
            for (auto nc : this->nodeClassList) {
                std::cout << "node: " << nc.nodeNumber << ", file: " << nc.dataTextfileName << std::endl;
            }

            auto it = find_if(this->nodeClassList.begin(), this->nodeClassList.end(), [receivedNodeNumber](const NodeClass& n){
                return n.nodeNumber == receivedNodeNumber;
            });

            if (it != nodeClassList.end()) {
                std::cout << it->dataTextfileName << std::endl;
            } else {
                std::cout << "Not found!" << std::endl;
            }

            //get from reserve and then erase that node from reserve list, put it into nodeclasslist
            NodeClass replacementNode;
            if (this->availableNodes.size() > 0) {
                auto anIt = this->availableNodes.begin();
                std::cout << "Available node is: " << *anIt << std::endl;

                replacementNode.nodeNumber = *anIt;
                replacementNode.dataTextfileName = it->dataTextfileName;

                this->nodeClassList.erase(it);
                this->availableNodes.erase(anIt);
            } else {
                std::cout << "No more available reserves..." << std::endl;
            }

            std::cout << "New node: " << replacementNode.nodeNumber << " with text: " << replacementNode.dataTextfileName << std::endl;
            //Sending new message
            #pragma omp task
            {
                std::string buffer = fileToBuffer(replacementNode.dataTextfileName);
                std::stringstream ss;
                ss << "slave/train/node" << replacementNode.nodeNumber;
                send_message(ss.str().c_str(), buffer.c_str());
            }

            //debug
            this->nodeClassList.push_back(replacementNode);
            for (auto nc : this->nodeClassList) {
                std::cout << "node: " << nc.nodeNumber << ", file: " << nc.dataTextfileName << std::endl;
            }


        } else if (topic.find("ack/node") != std::string::npos) { //Receive ack messages to count and call callback after timeout
            //TODO: Just list all available, even reserves and store that information into struct with datatext name and node name (list
            //OR: just move the sending of data.txt from main to here, easier)
            this->availableNodes.insert(receivedNodeNumber);
            std::cout << "out numberOfAvailableNodes: " << this->availableNodes.size() << std::endl;
            if (!firstAckReceivedNodes) {
                std::cout << "trigger thread for availablenodes callback" << std::endl;
                firstAckReceivedNodes = true;
                std::function<std::vector<NodeClass>()> f = std::bind(&myMosqConcrete::generateNodeAndDataList, this);
                std::thread t(&myMosqConcrete::queryNodesTimeout, this, f, 10);
                t.detach();
            }
        } else {
            std::cout << "ELSE" << std::endl;
        }
    } else { //flask messages
        return false;
    }

    if (publishedNodes.size() == unsigned(c.numberOfNodes)) {
        std::string topic("flask/query/" + c.nodeName);
        this->isProcessing = true;
        updateFlask(topic, "busy");
        // distributedTest();
        std::thread t(&myMosqConcrete::distributedTest, this);
        t.detach();
        this->stopThreadFlag = true;
    } else {
        //TODO: Start timeout countdown, maybe 5 minutes? (only start this once) and reset flag in reset()
        //distributedTest should be called at the end of the countdown
        if (!firstAckReceived) {
            std::cout << "trigger thread for publishedNodes callback" << std::endl;
            firstAckReceived = true;
            std::function<void()> f = std::bind(&myMosqConcrete::distributedTest, this);
            std::thread t(&myMosqConcrete::publishedNodesTimeout, this, f, 300);
            t.detach();
        }
    }
    return true;
}

void myMosqConcrete::sendSlavesQuery(std::string msg) {
    //Query for available nodes
    std::string queryTopic("slave/query/master");
    send_message(queryTopic.c_str(), msg.c_str());
}

void myMosqConcrete::checkNodePayload(int n, std::string str) {//, std::string topic) {
    std::cout << "Master node is processing received forests..." << std::endl;
    std::stringstream ss;
    ss << "Receiving node " << n << " message...";
    std::cout << ss.str() << std::endl;
    this->isProcessing = true;
    //TODO: Maybe send update here and after the loop?
    if (std::find(publishedNodes.begin(), publishedNodes.end(), n) == publishedNodes.end()) {
        publishedNodes.push_back(n);
        std::stringstream rts;
        rts << "RTs_Forest_" << n << ".txt";
        writeToFile(str, rts.str());
    }
    this->isProcessing = false;
}

void myMosqConcrete::distributedTest() {
    this->stopThreadFlag = true;
    std::cout << "distributedTest()" << std::endl;
    //Have to loop through all of the node list
    std::vector<std::vector<int>> scoreVectors;
    std::vector<int> correctLabel;
    std::vector<RTs::Forest> randomForests;
    //Assume to read RTs_Forest.txt
    char dir[255];
    getcwd(dir,255);
    // std::cout << dir << std::endl;

    std::vector<std::string> files = Utils::FileList::listFilesWithNameAndExtension("RTs_Forest_", ".txt");

    #pragma omp parallel num_threads(files.size())
    {
        int i = omp_get_thread_num();
        RTs::Forest rts_forest;
        std::stringstream ss;
        //TODO: Someproblem here if the available RTs forests are not sequentially. Just use same as main, and find all text files named RTs_Forest instead.
        ss << dir << "/" << files[i];
        // ss << dir << "/RTs_Forest_" << i << ".txt";//double check
        rts_forest.Load(ss.str());
        #pragma omp critical
        {
            randomForests.push_back(rts_forest);
        }
        ss.str(std::string());
    }

    //TODO: Change to use configs
    Utils::Parser *p = new Utils::Parser();
    std::cout << "classificationColumn: " << c.classificationColumn << std::endl;
    p->setClassColumn(c.classificationColumn);
    std::cout << "trainingDataFileName: " << c.trainingDataFileName << std::endl;
    std::vector<RTs::Sample> samples = p->readCSVToSamples(c.trainingDataFileName);

    //Too many loops for testing
    //Need to change checkScores func to just accept the samples vector (too large? const)
    std::for_each(samples.begin(), samples.end(), [&](const RTs::Sample& s) {
        correctLabel.push_back(s.label);
    });

    //TODO: Break here if fail? then restart?
    for (unsigned int i = 0; i < samples.size(); ++i) {
        std::vector<int> nodeListSamples;
        scoreVectors.push_back(nodeListSamples);
        for (unsigned int j = 0; j < publishedNodes.size(); ++j) {
            RTs::Feature f = samples[i].feature_vec;
            const float* histo = randomForests[j].EstimateClass(f);
            //TODO: 10 is hard coded number of classes + 1
            scoreVectors[i].push_back(getClassNumberFromHistogram(10, histo));
        }
    }

    Utils::TallyScores *ts = new Utils::TallyScores();
    ts->checkScores(correctLabel, scoreVectors);
    delete ts;
    //Clear all variables? even for reserves?
    reset();
    this->isProcessing = false;
    updateFlask("flask/query/" + c.nodeName, "available");
    t.stop();
}

int myMosqConcrete::getClassNumberFromHistogram(int numberOfClasses, const float* histogram) {
    float biggest = -FLT_MAX;
    int index = -1;

    //since classes start at 1
    for (int i = 0; i < numberOfClasses; ++i) {
        float f = *(histogram + i);
        if (f > biggest) {
            index = i;
            biggest = f;
        }
    }
    return index;
}

void myMosqConcrete::addHandler(std::function<void(int)> callback) {
    printf("Handler added.\n");
    this->callback = callback;
}

void myMosqConcrete::updateFlask(std::string topic, std::string availability) {
    std::vector<std::pair<std::string, std::string>> kv;
        std::string ipaddress = Utils::Command::exec("hostname -I");
        kv.push_back(std::make_pair("ipaddress", ipaddress.substr(0, ipaddress.find(" "))));
        kv.push_back(std::make_pair("status", availability));
        kv.push_back(std::make_pair("datafile", c.trainingDataFileName));
        kv.push_back(std::make_pair("nodename", c.nodeName));
        std::string json = Utils::Json::createJsonFile(kv);
        
        this->send_message(topic.c_str(), json.c_str());
}