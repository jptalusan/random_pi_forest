#include "myMosqConcrete.h"

//#define DEBUG

void mqtt_subscriber_thread(std::string host, std::string topic, Utils::Configs c) {
    std::string id = c.nodeName;
    int port = 1883;
    const std::string message = "subscribe";
    myMosq* mymosq = new myMosqConcrete(id.c_str(), topic.c_str(), host.c_str(), port, c);
    std::string lastWillTopic("master/lastWill/" + c.nodeName);
    mymosq->setupLastWill(lastWillTopic, ("I am " + c.nodeName + " and i am gone, goodbye."));
    mymosq->connect();
    mymosq->subscribe_to_topic();
    mymosq->loop_start();

    while(1) {
        //infinite loop
    }
}

int main(){
    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");

    //MQTT Subscriber loop
    std::thread t1(mqtt_subscriber_thread, c.mqttBroker, c.topic, c);
    t1.join();
    //end of subscriber

    return 0;
}
