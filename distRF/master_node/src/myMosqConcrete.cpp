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
            f();
            break;
        }
    }
}

void myMosqConcrete::queryNodesTimeout(std::function<void(int)> f, int TIMEOUT) {
    auto timeStart = clock();
    while (true)
    {
        if ((clock() - timeStart) / CLOCKS_PER_SEC >= TIMEOUT) {// time in seconds
            std::cout << "Timed out querynodes: Proceeding with processing." << std::endl;
            int nodesToUse = 0;
            if (availableNodes.size() < unsigned(c.numberOfNodes)) {
                std::cout << "number of available nodes is less than number of nodes on config file." << std::endl;
                nodesToUse = availableNodes.size();
            } else {
                nodesToUse = c.numberOfNodes;
            }
            f(nodesToUse);
            break;
        }
    }
}

void myMosqConcrete::reset() {
    publishedNodes.clear();
    availableNodes.clear();
    availableNodesAtEnd.clear();
    nodes.clear();
    firstAckReceived = false;
    hasFailed = false;
    stopThreadFlag = false;
    firstAckReceivedNodes = false;
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
    
    std::string node("node");
    if (topic.find(node) != std::string::npos) { //All messages from slave nodes
        int nodeIndex = topic.find(node) + node.length();
        int nodeNumber = std::stoi(topic.substr(nodeIndex, topic.length()));
        printf("Received message from node:%d\n", nodeNumber);
        if (topic.find("forest/node") != std::string::npos) {

            //std::thread t(&myMosqConcrete::checkNodePayload, this, nodeNumber, );
            // stopThreadFlag = true;
            #pragma omp task
            {
                checkNodePayload(nodeNumber, msg);
            }
        } else if (topic.find("lastWill/node") != std::string::npos) {
            //TODO: Send data to next node in line (and continue waiting for timeout if not appear)
            std::cout << topic << ":" << msg << std::endl;
        } else if (topic.find("ack/node") != std::string::npos) { //Receive ack messages to count and call callback after timeout
            //TODO: Just list all available, even reserves and store that information into struct with datatext name and node name (list
            //OR: just move the sending of data.txt from main to here, easier)
            std::cout << topic << ":" << msg << std::endl;
            // std::thread t(&myMosqConcrete::timeout, this, this->callback, 10);

            availableNodes.insert(nodeNumber);
            // nodes.insert(std::make_pair(nodeNumber, "true"));
            std::cout << "out numberOfAvailableNodes: " << availableNodes.size() << std::endl;
            if (!firstAckReceivedNodes) {
                std::cout << "trigger thread for availablenodes callback" << std::endl;
                firstAckReceivedNodes = true;
                std::thread t(&myMosqConcrete::queryNodesTimeout, this, this->callback, 10);
                t.detach();
            }
        } else {
            std::cout << "ELSE" << std::endl;
        }
    } else { //flask messages
        return false;
    }

    if (publishedNodes.size() == unsigned(c.numberOfNodes)) {
        distributedTest();
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

    if (std::find(publishedNodes.begin(), publishedNodes.end(), n) == publishedNodes.end()) {
        publishedNodes.push_back(n);
        std::stringstream rts;
        rts << "RTs_Forest_" << n << ".txt";
        writeToFile(str, rts.str());
    }
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
    p->setClassColumn(1);
    std::vector<RTs::Sample> samples = p->readCSVToSamples("cleaned.csv");

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
