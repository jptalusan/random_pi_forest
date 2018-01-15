#include "myMosqConcrete.h"

myMosqConcrete::myMosqConcrete(const char* id, const char* _topic, const char* host, int port)
        : myMosq(id, _topic, host, port), publishedNodes(0) {
    std::cout << "Master node mqtt setup" << std::endl;
    t = Utils::Timer();
    t.start();
    firstAckReceived = false;
}

void myMosqConcrete::tester() {
    auto timeStart = clock();
    while (true)
    {
        if ((clock() - timeStart) / CLOCKS_PER_SEC >= 5) {// time in seconds 
            this->callback(availableNodes.size());
            break;
        }
    }
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

    printf("From broker (%s, %s)", message->topic, pchar);
    //Should also check the values of the message...

    std::string node("node");
    //fix next time for now hardcoded
    if (topic.find(node) != std::string::npos) {
        int nodeIndex = topic.find(node) + node.length();
        int nodeNumber = std::stoi(topic.substr(nodeIndex, topic.length()));
        printf("Received message from node:%d\n", nodeNumber);
        if (topic.find("forest/node") != std::string::npos) {
            checkNodePayload(nodeNumber, msg);
        } else if (topic.find("ack/node") != std::string::npos) {
            if (msg == "start") {
                availableNodes.insert(nodeNumber);
            } else if (msg == "end") {
                availableNodesAtEnd.insert(nodeNumber);
            }

            std::cout << "out numberOfAvailableNodes: " << availableNodes.size() << std::endl;
            if (!firstAckReceived) {
                firstAckReceived = true;
                std::thread t(&myMosqConcrete::tester, this);
                t.detach();
            }
        }
    } else {
        return false;
    }

    std::cout << "Published nodes size: " << publishedNodes.size() << std::endl;
    
    //TODO: compare to size from query, only as fast as the slowest node
    //TODO: Should I make this a thread?
    if (publishedNodes.size() == availableNodes.size()) {
        distributedTest();
        t.stop();
    }

    return true;

}

void myMosqConcrete::sendSlavesQuery(std::string msg) {
    //Query for available nodes
    std::string queryTopic("slave/query");
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
        writeToFile(str.c_str(), rts.str());
    }
}

void myMosqConcrete::distributedTest() {
    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");

    std::vector<std::string> nodeList = c.nodeList;
    
    //Have to loop through all of the node list
    std::vector<std::vector<int>> scoreVectors;
    std::vector<int> correctLabel;
    std::vector<RTs::Forest> randomForests;
    //Assume to read RTs_Forest.txt
    char dir[255];
    getcwd(dir,255);
    std::cout << dir << std::endl;


    for (unsigned int i = 0; i < nodeList.size(); ++i) {
        RTs::Forest rts_forest;
        std::stringstream ss;
        ss << dir << "/RTs_Forest_" << i << ".txt";//double check
        rts_forest.Load(ss.str());
        randomForests.push_back(rts_forest);
        ss.str(std::string());
    }
    //todo: process the rts_forest, load fxn already created the node
    // read the csv file here
    Utils::Parser *p = new Utils::Parser();
    p->setClassColumn(1);
    std::vector<RTs::Sample> samples = p->readCSVToSamples("cleaned.csv");

    //Too many loops for testing
    //Need to change checkScores func to just accept the samples vector (too large? const)
    std::for_each(samples.begin(), samples.end(), [&](const RTs::Sample& s) {
        correctLabel.push_back(s.label);
    });

    for (unsigned int i = 0; i < samples.size(); ++i) {
        std::vector<int> nodeListSamples(0, 3);
        scoreVectors.push_back(nodeListSamples);
        for (unsigned int j = 0; j < nodeList.size(); ++j) {
            RTs::Feature f = samples[i].feature_vec;
            const float* histo = randomForests[j].EstimateClass(f);
            scoreVectors[i].push_back(getClassNumberFromHistogram(10, histo));
        }
    }

    Utils::TallyScores *ts = new Utils::TallyScores();
    ts->checkScores(correctLabel, scoreVectors);

    //TODO: Check if available nodes at this time is equal to the nodes at the start
    sendSlavesQuery("end");
    //TODO: Find something better to use than sleep here
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (availableNodes != availableNodesAtEnd) {
        //TODO: trigger the callback to start again
        printf("Not all nodes were availalbe. Try again.");
    } else {
        printf("This is ok.");
    }
    availableNodes.clear();
    availableNodesAtEnd.clear();
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
