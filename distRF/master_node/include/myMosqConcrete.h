#ifndef MYMOSQCONCRETE_H
#define MYMOSQCONCRETE_H

#include <vector>
#include <string>
#include <float.h>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <functional>
#include <future>
#include <time.h>
#include <set>
#include <chrono>
#include <thread>

#include "mosqrf.h"
#include "utils.hpp"
#include "rts_forest.hpp"
#include "rts_tree.hpp"
#include "concurrency.h"

class myMosqConcrete : public myMosq {
    public:
    std::vector<int> publishedNodes;
    std::set<int> availableNodes;
    std::set<int> availableNodesAtEnd;
    std::function<void(int)> callback;
    Utils::Timer t;
    bool firstAckReceived;
    myMosqConcrete(const char* id, const char * topic, const char* host, int port);
    bool receive_message(const struct mosquitto_message* message);
    void checkNodePayload(int n, std::string str);
    void distributedTest();
    int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
    void addHandler(std::function<void(int)> c);
    void tester();
    void sendSlavesQuery(std::string msg);
};


#endif
