#ifndef MYMOSQCONCRETE_H
#define MYMOSQCONCRETE_H

#include "mosqrf.h"
#include "utils.hpp"
#include "rts_forest.hpp"
#include "rts_tree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <float.h>
#include <algorithm>
#include "concurrency.h"
#include <sys/types.h>
#include <dirent.h>

class myMosqConcrete : public myMosq {
    public:
    Utils::Configs c;
    bool isProcessing;
    myMosqConcrete(const char* id, const char * topic, const char* host, int port, Utils::Configs c);
    bool receive_message(const struct mosquitto_message* message);
    int train();
    void initiateTraining(const char* message);
};

#endif
