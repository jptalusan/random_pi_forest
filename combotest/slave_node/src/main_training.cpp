#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <float.h>
#include <algorithm>
#include "utils.hpp"
#include "rts_forest.hpp"
#include "rts_tree.hpp"
#include "concurrency.h"
#include "mosqrf.h"
#include <sys/types.h>
#include <dirent.h>

//#define DEBUG
int train();
std::vector<RTs::Sample> getSamples();
int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
void distributedTest();
void centralizedTest();

void mqtt_subscriber_thread(std::string host, std::string topic) {
    std::string id = "sub";
    int port = 1883;
    const std::string message = "subscribe";
    myMosq* mymosq = new myMosq(id.c_str(), topic.c_str(), host.c_str(), port);
    mymosq->subscribe_to_topic();
    mymosq->loop_start();
    while(1) {
        //infinite loop
    }
}

int main(int argc, char *argv[]){

    Utils::Json *json = new Utils::Json();
    Utils::Configs c = json->parseJsonFile("configs.json");

    //concurrency test
    std::vector<std::string> data = readFileToBuffer("cleaned.csv");
    int numberOfNodes = c.nodeList.size();
    std::vector<int> v(data.size());
    std::iota(v.begin(), v.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);
    
    concurrentReads(numberOfNodes, data, v);
    //end concurrency test code

    //Testing directory scraper
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
    std::string host = "localhost";
    std::string id = "testing";
    std::string topic = "master";
    int port = 1883;
    const std::string message = "testing!";
    myMosq* mymosq = new myMosq(id.c_str(), topic.c_str(), host.c_str(), port);

    int index = 0;
    for (auto s : files) {
        std::cout << s << std::endl;
        char* buffer = fileToBuffer(s);
        std::stringstream ss;
        ss << "slave/node" << index;
        //send_message(topic, message)
        mymosq->send_message(ss.str().c_str(), buffer);
        delete[] buffer;
        ++index;
    }
    //End of MQTT

    //MQTT Subscriber loop
    std::thread t1(mqtt_subscriber_thread, "localhost", "master");
    t1.join();
    //end of subscriber

/*
        //
        // サンプルデータの読み込み
        //
        char dir[255];
        getcwd(dir,255);
        std::cout << dir << std::endl;
        std::cout << argc << argv[0] << std::endl;
        Utils::Timer* t = new Utils::Timer();
        t->start();
        train();
        t->stop();
        delete t;
*/
        return 0;
}

//TODO: make arguments adjustable via argv and transfer code to pi to start distribution
int train() {
        Utils::Parser *p = new Utils::Parser();
        p->setClassColumn(1);
        std::vector<RTs::Sample> samples;
        //samples = p->readCSVToSamples("cleaned.csv");
        samples = p->readCSVToSamples("/home/pi/random_pi_forest/slave_node/data/data.txt");
        std::cout << samples[4].label << std::endl;
        //
        // Randomized Forest 生成
        //
        //  numClass = 10  //学習に用いるクラス
        //  numTrees = 5 //木の数
        //  maxDepth = 10 //木の深さ
        //  featureTrials = 50 //分岐ノード候補の数
        //  thresholdTrials = 5  //分岐ノード閾値検索の候補の数
        //  dataPerTree = .25f  //サブセットに分けるデータの割合
        //
        std::cout << "1_Randomized Forest generation" << std::endl;
        RTs::Forest rts_forest;
        if(!rts_forest.Learn(10, 5, 10, 50, 5, 1.0f, samples)){
                printf("Randomized Forest Failed generation\n");
                std::cerr << "RTs::Forest::Learn() failed." << std::endl;
                std::cerr.flush();
                return 1;
        }

        //
        // 学習結果の保存
        //

        std::cout << "2_Saving the learning result" << std::endl;
        if(rts_forest.Save("RTs_Forest.txt") == false){
                std::cerr << "RTs::Forest::Save() failed." << std::endl;
                std::cerr.flush();
                return 1;
        }
        return 0;
}
