#ifndef MYMOSQCONCRETE_H
#define MYMOSQCONCRETE_H

#include <vector>
#include <string>
#include <float.h>
#include <algorithm>
#include <functional>
#include <future>
#include <time.h>
#include <set>
#include <chrono>
#include <thread>
#include <map>

#include "mosqrf.h"
#include "utils.hpp"
#include "rts_forest.hpp"
#include "rts_tree.hpp"
#include "concurrency.h"
#include "nodeclass.h"

//MASTER
class myMosqConcrete : public myMosq {
    public:
    std::vector<int> publishedNodes;
    std::set<int> availableNodes;
    std::set<int> availableNodesAtEnd;
    std::map<int, bool> nodes;
    std::function<void(int)> callback;
    std::atomic<bool> stopThreadFlag;
    std::vector<NodeClass> nodeClassList;
    Utils::Configs c;
    Utils::Timer t;
    bool firstAckReceived;
    bool firstAckReceivedNodes;
    bool hasFailed;
    myMosqConcrete(const char* id, const char * topic, const char* host, int port, Utils::Configs c);
    bool receive_message(const struct mosquitto_message* message);
    void checkNodePayload(int n, std::string str);
    void distributedTest();
    int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
    void addHandler(std::function<void(int)> c);
    void publishedNodesTimeout(std::function<void()> f, int timeout);
    void queryNodesTimeout(std::function<std::vector<NodeClass>()> f, int timeout);
    void checker();
    void sendSlavesQuery(std::string msg);
    void reset();
    std::vector<NodeClass> generateNodeAndDataList();
};


#endif
