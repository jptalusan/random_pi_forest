#include "myMosqConcrete.h"

myMosqConcrete::myMosqConcrete(const char* id, const char* _topic, const char* host, int port)
        : myMosq(id, _topic, host, port) {
    std::cout << "Hello world jp" << std::endl;
}

bool myMosqConcrete::receive_message(const struct mosquitto_message* message) {
    std::cout << "Master here!" << std::endl;
    return true;
}
