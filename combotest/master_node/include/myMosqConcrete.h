#ifndef MYMOSQCONCRETE_H
#define MYMOSQCONCRETE_H

#include "mosqrf.h"

class myMosqConcrete : public myMosq {
    public:
    myMosqConcrete(const char* id, const char * topic, const char* host, int port);
    bool receive_message(const struct mosquitto_message* message);
};

#endif
