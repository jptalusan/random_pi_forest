#include "myMosqConcrete.h"

//Add flag that while processing, should not accept more message or do not process incoming
myMosqConcrete::myMosqConcrete(const char* id, const char* _topic, const char* host, int port, Utils::Configs c)
        : myMosq(id, _topic, host, port) {
    this->c = c;
    std::cout << "Setup of mosquitto." << std::endl;
}

bool myMosqConcrete::receive_message(const struct mosquitto_message* message) {
    std::cout << "Slave node start processing!" << std::endl;
    char *pchar = (char*)(message->payload);
    writeToFile(pchar, "data.txt");
    // サンプルデータの読み込み
    //
    Utils::Timer* t = new Utils::Timer();
    t->start();
    train(c);
    t->stop();
    delete t;

    std::cout << "Slave node done training, written to data.txt" << std::endl;

    //Train first before sending
    //Read dataN.txt files to buffer and publish via MQTT.
    //send to master topic in localhost
    char dir[255];
    getcwd(dir,255);
    std::stringstream ss;
    ss << dir << "/RTs_Forest.txt";
    char* buffer = fileToBuffer(ss.str());

    std::string topic("master/");
    topic += c.nodeName;

    //remove "slave/" from part of name (what a hack)
    int indexToRemove = topic.find("slave/");
    topic.erase(indexToRemove, 6);

    std::cout << "Publishing to topic: " << topic << std::endl;
    this->send_message(topic.c_str(), buffer);
    delete[] buffer;
    //End of MQTT

    return true;
}


//TODO: make arguments adjustable via argv and transfer code to pi to start distribution
int train(Utils::Configs c) {
    std::cout << "start training" << std::endl;
    Utils::Parser *p = new Utils::Parser();
    //can also put the class column info into config
    p->setClassColumn(1);
    std::vector<RTs::Sample> samples;

    char dir[255];
    getcwd(dir,255);
    std::stringstream ss;
    ss << dir << "/data.txt";
    std::cout << ss.str() << std::endl;
    samples = p->readCSVToSamples(ss.str());
    std::cout << samples[4].label << std::endl;

    std::cout << "1_Randomized Forest generation" << std::endl;
    RTs::Forest rts_forest;
    if(!rts_forest.Learn(c.numClass,
                    c.numTrees,
                    c.maxDepth,
                    c.featureTrials,
                    c.thresholdTrials,
                    c.dataPerTree, 
                    samples)){
        printf("Randomized Forest Failed generation\n");
        std::cerr << "RTs::Forest::Learn() failed." << std::endl;
        std::cerr.flush();
        return 1;
    }

    std::cout << "2_Saving the learning result" << std::endl;
    if(rts_forest.Save("RTs_Forest.txt") == false){
        std::cerr << "RTs::Forest::Save() failed." << std::endl;
        std::cerr.flush();
        return 1;
    }

    return 0;
}
