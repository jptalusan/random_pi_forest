#include "myMosqConcrete.h"

myMosqConcrete::myMosqConcrete(const char* id, const char* _topic, const char* host, int port)
        : myMosq(id, _topic, host, port), publishedNodes(0) {
    std::cout << "Master node mqtt setup" << std::endl;
    t = Utils::Timer();
    t.start();
    firstAckReceived = false;
    hasFailed = false;
}

void myMosqConcrete::tester() {
    auto timeStart = clock();
    while (true)
    {
        if ((clock() - timeStart) / CLOCKS_PER_SEC >= 10) {// time in seconds
            this->callback(availableNodes.size());
            break;
        }
    }
}

//This will check if all nodes are still active, or one died in transit.
void myMosqConcrete::checker() {
    auto timeStart = clock();
    while (true)
    {
        if ((clock() - timeStart) / CLOCKS_PER_SEC >= 5) {// time in seconds 
            std::cout << "Something went wrong, need to re-do all." << std::endl;
            this->reset();
            sendSlavesQuery("start");
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

    if (message->payloadlen < 15) {
        printf("From broker (%s, %s)\n", message->topic, pchar);
    }
    //Should also check the values of the message...

    std::string node("node");
    //fix next time for now hardcoded
    if (topic.find(node) != std::string::npos) {
        int nodeIndex = topic.find(node) + node.length();
        int nodeNumber = std::stoi(topic.substr(nodeIndex, topic.length()));
        printf("Received message from node:%d\n", nodeNumber);
        if (topic.find("forest/node") != std::string::npos) {
            //TODO: Check the effect of this
            #pragma omp task// num_threads(1)
            {
                checkNodePayload(nodeNumber, msg);
            }
        } else if (topic.find("ack/node") != std::string::npos) {
            if (msg == "start") {
                availableNodes.insert(nodeNumber);
                nodes.insert(std::make_pair(nodeNumber, "true"));
                std::cout << "out numberOfAvailableNodes: " << availableNodes.size() << std::endl;
                if (!firstAckReceived) {
                    firstAckReceived = true;
                    std::thread t(&myMosqConcrete::tester, this);
                    t.detach();
                }
            } else if (msg == "end") {
                availableNodesAtEnd.insert(nodeNumber);
            }
        } else if (topic.find("lastWill/node") != std::string::npos) {
                nodes[nodeNumber] = false;
        }
    } else {
        return false;
    }

    std::cout << "Published nodes size: " << publishedNodes.size() << std::endl;
    std::cout << "nodes size: " << nodes.size() << std::endl;
    for (auto p : nodes) {
        std::cout << p.first << ":" << p.second << ", ";
    }
    std::cout << std::endl;

    //TODO: check if any node sent a last will if they did, trigger reset and query start again
    for (auto p : nodes) {
        if (p.second == false) {
            std::cout << "node: " << p.first << " has failed." << std::endl;
            hasFailed = true;
            break;
        }
    }

    if (publishedNodes.size() == nodes.size() && !hasFailed) {
        distributedTest();
        t.stop();
        this->reset();
    } else {
        if (hasFailed) {
            hasFailed = false;
            // sendSlavesQuery("end");
            //TODO: Find something better to use than sleep here
            // std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // std::thread t(&myMosqConcrete::checker, this);
            // t.detach();
            this->reset();
            sendSlavesQuery("start");
        }
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
    //Have to loop through all of the node list
    std::vector<std::vector<int>> scoreVectors;
    std::vector<int> correctLabel;
    std::vector<RTs::Forest> randomForests;
    //Assume to read RTs_Forest.txt
    char dir[255];
    getcwd(dir,255);
    // std::cout << dir << std::endl;


    for (unsigned int i = 0; i < publishedNodes.size(); ++i) {
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
        for (unsigned int j = 0; j < publishedNodes.size(); ++j) {
            RTs::Feature f = samples[i].feature_vec;
            const float* histo = randomForests[j].EstimateClass(f);
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
