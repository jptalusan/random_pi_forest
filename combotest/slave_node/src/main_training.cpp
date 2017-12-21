#include "myMosqConcrete.h"

//#define DEBUG
int train();
std::vector<RTs::Sample> getSamples();
int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
void distributedTest();
void centralizedTest();

void mqtt_subscriber_thread(std::string host, std::string topic, Utils::Configs c) {
    std::string id = c.nodeName;
    int port = 1883;
    const std::string message = "subscribe";
    myMosq* mymosq = new myMosqConcrete(id.c_str(), topic.c_str(), host.c_str(), port, c);
    mymosq->subscribe_to_topic();
    mymosq->loop_start();
    while(1) {
        //infinite loop
    }
}

int main(int argc, char *argv[]){
    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");

    //MQTT Subscriber loop
    std::thread t1(mqtt_subscriber_thread, c.mqttBroker, c.nodeName, c);
    t1.join();
    //end of subscriber

    return 0;
}
