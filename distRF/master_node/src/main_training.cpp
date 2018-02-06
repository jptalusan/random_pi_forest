#include "myMosqConcrete.h"
//MASTER
//#define DEBUG
void mqtt_subscriber_thread(std::shared_ptr<myMosqConcrete> mymosq) {
    mymosq->subscribe_to_topic();
    while(1) {
        //infinite loop
    }
}

// void testCallback(int i) {
//     std::cout << "Called back, total number of available nodes: " << i << std::endl;
//     implementRF(i);
// }

int main(){
    std::unique_ptr<Utils::Json> json(new Utils::Json());
    Utils::Configs c = json->parseJsonFile("configs.json");

    //Read dataN.txt files to buffer and publish via MQTT.
    //send to master topic in localhost
    std::string host = c.mqttBroker;
    std::string id = "testing";
    std::cout << "Subscribing to " << c.topic << std::endl;
    int port = 1883;
    std::shared_ptr<myMosqConcrete> mymosq (new myMosqConcrete(id.c_str(), c.topic.c_str(), host.c_str(), port, c));

    // m = mymosq;
    //callback for number of nodes query
    // mymosq->addHandler(testCallback);

    //TODO: maybe change the topic for master?
    std::string lastWillTopic("master/lastWill/" + c.nodeName);
    mymosq->setupLastWill(lastWillTopic, "I am master and i am gone, goodbye.");
    mymosq->connect();

    // mymosq->sendSlavesQuery("start");
    std::thread t1(mqtt_subscriber_thread, mymosq);
    t1.join();
    return 0;
}

