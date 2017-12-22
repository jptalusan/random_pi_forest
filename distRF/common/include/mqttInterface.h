#ifndef MQTTINTERFACE_H
#define MQTTINTERFACE_H
#include <string>

//Setup how a node handles a message or subscribes to which topics
class mqttInterface {
    public:
        virtual bool receive_message() = 0;
};

#endif
