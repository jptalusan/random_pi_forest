#include "myMosqConcrete.h"
//MASTER
//#define DEBUG
void mqtt_subscriber_thread(std::shared_ptr<myMosqConcrete> mymosq) {
    mymosq->subscribe_to_topic();
    while(1) {
        //infinite loop
    }
}

std::shared_ptr<myMosqConcrete> m;

void implementRF(int numberOfNodes) {
    std::cout << Utils::Command::exec("rm data* RTs_Forest*") << std::endl;
    std::vector<std::string> data = readFileToBuffer("cleaned.csv");

    std::vector<int> v(data.size());
    std::iota(v.begin(), v.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);
    
    concurrentReads(numberOfNodes, data, v);
    //end concurrency test code, saves it into X number of dataN.txt files

    //Testing directory scraper (lists all the dataN.txt files)

    std::vector<std::string> files = Utils::FileList::listFilesWithNameAndExtension("data", ".txt");
    // std::string name = ".";
    // DIR* dirp = opendir(name.c_str());
    // struct dirent * dp;
    // while ((dp = readdir(dirp)) != NULL) {
    //     //std::cout << dp->d_name;
    //     std::string s(dp->d_name);
    //     if (s.find("data") != std::string::npos &&
    //         s.find(".txt") != std::string::npos)
    //         files.push_back(s);
    // }
    // closedir(dirp);
    //end scraper
    
    int sizeOfFilesList = files.size();
    std::cout << "sizeOfFilesList: " << sizeOfFilesList << std::endl;
    #pragma omp parallel num_threads(sizeOfFilesList)
    {
        int index = omp_get_thread_num();
        std::string s(files[index]);
        char* buffer = fileToBuffer(s);
        std::stringstream ss;
        ss << "slave/node" << index;
        // std::cout << "sending " << s << " to :" << ss.str() << std::endl;
        m->send_message(ss.str().c_str(), buffer);
    }
    //End of MQTT

    //end of subscriber
}

void testCallback(int i) {
    std::cout << "Called back, total number of available nodes: " << i << std::endl;
    implementRF(3); //TODO: Fix this, should use c.numberOfNode
}

int main(){
    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");

    //Read dataN.txt files to buffer and publish via MQTT.
    //send to master topic in localhost
    std::string host = c.mqttBroker;
    std::string id = "testing";
    std::cout << "Subscribing to " << c.topic << std::endl;
    int port = 1883;
    std::shared_ptr<myMosqConcrete> mymosq (new myMosqConcrete(id.c_str(), c.topic.c_str(), host.c_str(), port, c));

    m = mymosq;
    //callback for number of nodes query
    mymosq->addHandler(testCallback);

    //TODO: maybe change the topic for master?
    std::string lastWillTopic("master/lastWill/" + c.nodeName);
    mymosq->setupLastWill(lastWillTopic, "I am master and i am gone, goodbye.");
    mymosq->connect();

    mymosq->sendSlavesQuery("start");
    // std::cout << Utils::Command::exec("rm data* RTs_Forest*") << std::endl;
    // std::vector<std::string> data = readFileToBuffer("cleaned.csv");

    // std::vector<int> v(data.size());
    // std::iota(v.begin(), v.end(), 0);
    // std::random_device rd;
    // std::mt19937 g(rd());
    // std::shuffle(v.begin(), v.end(), g);
    
    // std::cout << "Concurrent reads(" << c.numberOfNodes << ")" << std::endl;
    // concurrentReads(c.numberOfNodes, data, v);
    // //end concurrency test code, saves it into X number of dataN.txt files

    // //Testing directory scraper (lists all the dataN.txt files)
    // std::vector<std::string> files;
    // std::string name = ".";
    // DIR* dirp = opendir(name.c_str());
    // struct dirent * dp;
    // while ((dp = readdir(dirp)) != NULL) {
    //     //std::cout << dp->d_name;
    //     std::string s(dp->d_name);
    //     if (s.find("data") != std::string::npos &&
    //         s.find(".txt") != std::string::npos)
    //         files.push_back(s);
    // }
    // closedir(dirp);
    // //end scraper
    
    // int sizeOfFilesList = files.size();
    // #pragma omp parallel num_threads(sizeOfFilesList)
    // {
    //     int index = omp_get_thread_num();
    //     std::string s(files[index]);
    //     char* buffer = fileToBuffer(s);
    //     std::stringstream ss;
    //     ss << "slave/node" << index;
    //     // std::cout << "sending " << s << " to :" << ss.str() << std::endl;
    //     mymosq->send_message(ss.str().c_str(), buffer);
    // }

    #pragma omp task// num_threads(1)
    {
        mqtt_subscriber_thread(mymosq);
    }
    
    return 0;
}

