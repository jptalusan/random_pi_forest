#ifndef MOSQRF_H
#define MOSQRF_H

#include <iostream>
#include "mosquittopp.h"
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <algorithm>
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
        bool receive_message();
        bool subscribe_to_topic();
};

std::string GetCurrentWorkingDir(void);
void writeToFile(const char* buffer, std::string fileName);
char* fileToBuffer(std::string filename);
#endif
