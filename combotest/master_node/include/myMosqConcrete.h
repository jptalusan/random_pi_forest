#ifndef MYMOSQCONCRETE_H
#define MYMOSQCONCRETE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <float.h>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>

#include "mosqrf.h"
#include "utils.hpp"
#include "rts_forest.hpp"
#include "rts_tree.hpp"
#include "concurrency.h"

class myMosqConcrete : public myMosq {
    public:
    std::vector<int> publishedNodes;
    myMosqConcrete(const char* id, const char * topic, const char* host, int port);
    bool receive_message(const struct mosquitto_message* message);
    void checkNodePayload(int n, std::string str, std::string topic);
    void distributedTest();
    int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);

};


#endif
