#include "myMosqConcrete.h"

//#define DEBUG

// void mqtt_subscriber_thread(std::string host, std::string topic) {
void mqtt_subscriber_thread(myMosq* mymosq) {
    mymosq->subscribe_to_topic();
    while(1) {
        //infinite loop
    }
}

int main(int argc, char *argv[]){
    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");
    
    //Must have BATMAN installed
    std::string cmd("sudo batctl o");
    std::string cmdout = json->exec(cmd.c_str());
    std::cout << cmdout << std::endl;
    //might need to remove the threading here?
    //concurrency: Divides the csv file according to the number of nodes available
    std::vector<std::string> data = readFileToBuffer("cleaned.csv");
    //int numberOfNodes = c.nodeList.size();
    int numberOfNodes = std::count(cmdout.begin(), cmdout.end(), '*');

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
    std::cout << "Subscribing to " << c.topic << std::endl;
    int port = 1883;
    myMosq* mymosq = new myMosqConcrete(id.c_str(), c.topic.c_str(), host.c_str(), port);

    // int index = 0;
    // for (auto s : files) {
    //     std::cout << s << std::endl;
    //     char* buffer = fileToBuffer(s);
    //     std::stringstream ss;
    //     ss << "slave/node" << index;
    //     std::cout << "sending to :" << ss.str() << std::endl;
    //     mymosq->send_message(ss.str().c_str(), buffer);
    //     //delete[] buffer;
    //     ++index;
    // }

    int sizeOfFilesList = files.size();
    int index;
    #pragma omp parallel private(index) num_threads(sizeOfFilesList)
    {
        index = omp_get_thread_num();
        std::string s = files[index];
        std::cout << s << std::endl;
        char* buffer = fileToBuffer(s);
        std::stringstream ss;
        ss << "slave/node" << index;
        std::cout << "sending to :" << ss.str() << std::endl;
        mymosq->send_message(ss.str().c_str(), buffer);
    }
    //End of MQTT

    //MQTT Subscriber loop
    //You subscribe to your own node name
    // std::thread t1(mqtt_subscriber_thread, c.mqttBroker, c.nodeName);

    // std::thread t1(mqtt_subscriber_thread, mymosq);
    // t1.join();

    //end of subscriber
    #pragma omp task// num_threads(1)
    {
        mqtt_subscriber_thread(mymosq);
    }

    return 0;
}

