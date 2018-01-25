#ifndef MOSQRF_H
#define MOSQRF_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "mqttInterface.h"
#include "mosquittopp.h"

#define GetCurrentDir getcwd

class myMosq : public mosqpp::mosquittopp {
    private:
        const char* host;
        const char* id;
        const char* topic;
        int port;
        int keepalive;

        const char* message;

        void on_connect(int rc);
        void on_disconnect(int rc);

        void on_message(const struct mosquitto_message *message);
        void on_publish(int mid);

        void on_subscribe(int mid, int qos_count, const int* granted_qos);
        void on_unsubscribe(int mid);

    public:
        myMosq(const char* id, const char* _topic, const char* host, int port);
        ~myMosq();
        bool send_message(const char* topic, const char* _message);
        virtual bool receive_message(const struct mosquitto_message* message) = 0;
        bool subscribe_to_topic();
        void connect();
        int setupLastWill(const std::string topic, const std::string msg);
};

std::string GetCurrentWorkingDir(void);
void writeToFile(const std::string buffer, std::string fileName);
std::string fileToBuffer(std::string filename);
#endif
