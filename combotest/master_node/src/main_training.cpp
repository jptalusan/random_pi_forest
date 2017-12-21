#include "myMosqConcrete.h"

//#define DEBUG
int train(Utils::Configs c);
int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
void distributedTest();
void centralizedTest();

// void mqtt_subscriber_thread(std::string host, std::string topic) {
void mqtt_subscriber_thread(myMosq* mymosq) {
    // std::string id = "sub";
    // int port = 1883;
    // myMosq* mymosq = new myMosqConcrete(id.c_str(), topic.c_str(), host.c_str(), port);
    mymosq->subscribe_to_topic();
    // mymosq->loop_start();
    while(1) {
        //infinite loop
    }
}

int main(int argc, char *argv[]){
    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");

    //might need to remove the threading here?
    //concurrency: Divides the csv file according to the number of nodes available
    std::vector<std::string> data = readFileToBuffer("cleaned.csv");
    int numberOfNodes = c.nodeList.size();
    std::vector<int> v(data.size());
    std::iota(v.begin(), v.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);
    
    concurrentReads(numberOfNodes, data, v);
    //end concurrency test code, saves it into X number of dataN.txt files

    //Testing directory scraper (lists all the dataN.txt files)
    std::vector<std::string> files;
    std::string name = ".";
    DIR* dirp = opendir(name.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
        //std::cout << dp->d_name;
        std::string s(dp->d_name);
        if (s.find("data") != std::string::npos &&
            s.find(".txt") != std::string::npos)
            files.push_back(s);
    }
    closedir(dirp);
    //end scraper
    
    //Read dataN.txt files to buffer and publish via MQTT.
    //send to master topic in localhost
    std::string host = c.mqttBroker;
    std::string id = "testing";
    std::string topic = c.nodeName + "/+";
    std::cout << "Subscribing to " << topic << std::endl;
    int port = 1883;
    myMosq* mymosq = new myMosqConcrete(id.c_str(), topic.c_str(), host.c_str(), port);

    int index = 0;
    for (auto s : files) {
        std::cout << s << std::endl;
        char* buffer = fileToBuffer(s);
        std::stringstream ss;
        ss << "slave/node" << index;
        std::cout << "sending to :" << ss.str() << std::endl;
        mymosq->send_message(ss.str().c_str(), buffer);
        //delete[] buffer;
        ++index;
    }
    //End of MQTT

    //MQTT Subscriber loop
    //You subscribe to your own node name
    // std::thread t1(mqtt_subscriber_thread, c.mqttBroker, c.nodeName);
    std::thread t1(mqtt_subscriber_thread, mymosq);
    t1.join();
    //end of subscriber
/*
    if (argc < 2) {
        std::cout << "rf_exe [test|train (cent|dist)]" << std::endl;
    } else {
        std::string arg1(argv[1]);
        std::string arg2 = "";
        if (argc > 2) {
            arg2 = argv[2];
        }
        Utils::Timer* t = new Utils::Timer();
        std::cout << argc << ": " << arg1 << " " << arg2 << " " << std::endl;
        if (arg1 == "train") {
            t->start();
            train(c);
            t->stop();
        } else if (arg1 == "test" && arg2 == "dist" && argc == 3) {
            t->start();
            distributedTest();
            t->stop();
        } else if (arg1 == "test" && arg2 == "cent" && argc == 3) {
            t->start();
            centralizedTest();
            t->stop();
        } else {
            std::cout << "rf_exe [test|train]" << std::endl;
        }
        delete t;
    }
*/
    return 0;
}

