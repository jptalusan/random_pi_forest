#include "myMosqConcrete.h"
//SLAVE
//Add flag that while processing, should not accept more message or do not process incoming
myMosqConcrete::myMosqConcrete(const char* id, const char* _topic, const char* host, int port, Utils::Configs c)
        : myMosq(id, _topic, host, port) {
    this->c = c;
    //mosquitto_will_set(myMosq, NULL, 0, NULL, 0, false)
    std::cout << "Setup of mosquitto." << std::endl;
    updateFlask("flask/query/" + c.nodeName, "available");
}

// struct mosquitto_message{
//         int mid;
//         char *topic;
//         void *payload;
//         int payloadlen;
//         int qos;
//         bool retain;
// }
bool myMosqConcrete::receive_message(const struct mosquitto_message* message) {
    char *pchar = (char*)(message->payload);
    std::string receivedTopic(message->topic);
    std::string nodeTopic = c.nodeName;
    std::string msg(pchar);

    std::string forPrint = "";
    if (message->payloadlen > 10) {
        forPrint = msg.substr(0, 10);
    } else {
        forPrint = msg;
    }
    std::cout << "t: " << receivedTopic << ", msg: " << forPrint << std::endl;
    
    std::string node("slave");
    if (receivedTopic.find(node) != std::string::npos) { //All messages for slave nodes
        unsigned int nodeIndex = receivedTopic.find("node");
        if (nodeIndex != std::string::npos) { //Specifically for nodes
            std::string nodeName(receivedTopic.substr(nodeIndex, receivedTopic.length()));
            if (c.nodeName == nodeName) { //If for this node
                std::cout << "nodename found in topic." << std::endl;
                if (receivedTopic.find("train") != std::string::npos) {
                    std::cout << "train found in topic." << std::endl;
                    //OR PUT THIS IN THREAD!
                    // initiateTraining(msg);
                    std::thread t(&myMosqConcrete::initiateTraining, this, msg);
                    t.detach();
                    updateFlask("flask/query/" + c.nodeName, "busy");
                }
            } //end for this node
        } else if (receivedTopic.find("query/master") != std::string::npos) { //Broadcast message from master
            std::string topic("master/ack/" + c.nodeName);
            this->send_message(topic.c_str(), "ack");
        } else if (receivedTopic.find("query/flask") != std::string::npos) {
            updateFlask("flask/query/" + c.nodeName, this->isProcessing ? "busy" : "available");
        } else if (receivedTopic.find("config") != std::string::npos) {
            //jq command: jq \'. + {{{0}}}\' configs.json > configs.tmp && mv configs.tmp configs.json

            const std::string& command("jq '. + {" + msg + "}' configs.json > configs.tmp && mv configs.tmp configs.json");
            std::cout << "command: " << command << std::endl;
            Utils::Command::exec(command.c_str());
            Utils::Json json = Utils::Json();
            this->c = json.parseJsonFile("configs.json");
            std::cout << Utils::Command::exec("cat configs.json");
            //Utils::Command::exec("jq ");
            // std::cout << Utils::Command::exec("ls") << std::endl;
        } else { //Not for this node
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void myMosqConcrete::initiateTraining(const std::string pchar) {
    this->isProcessing = true;
    Utils::Command::exec("rm data* RTs_Forest*");
    std::cout << "initiateTraining()";
    writeToFile(pchar, "data.txt");
    Utils::Timer* t = new Utils::Timer();
    t->start();
    train();
    t->stop();
    delete t;

    std::cout << "Slave node done training, written to RTs_Forest.txt" << std::endl;

    //Train first before sending
    //Read dataN.txt files to buffer and publish via MQTT.
    //send to master topic in localhost
    char dir[255];
    getcwd(dir,255);
    std::stringstream ss;
    ss << dir << "/RTs_Forest.txt";
    std::string buffer = fileToBuffer(ss.str());

    std::string topic("master/forest/" + c.nodeName);
    
    std::cout << "Publishing to topic: " << topic << std::endl;
    this->send_message(topic.c_str(), buffer.c_str());

    this->isProcessing = false;
    updateFlask("flask/query/" + c.nodeName, "availabile");
}

//TODO: make arguments adjustable via argv and transfer code to pi to start distribution
int myMosqConcrete::train() {
    std::cout << "start training" << std::endl;
    Utils::Parser *p = new Utils::Parser();
    //can also put the class column info into config
    std::cout << "classificationColumn: " << this->c.classificationColumn << std::endl;
    p->setClassColumn(c.classificationColumn);
    std::vector<RTs::Sample> samples;

    char dir[255];
    getcwd(dir,255);
    std::stringstream ss;
    ss << dir << "/data.txt";
    std::cout << ss.str() << std::endl;
    samples = p->readCSVToSamples(ss.str());
    
    std::cout << "1_Randomized Forest generation" << std::endl;
    RTs::Forest rts_forest;
    if(!rts_forest.Learn(
                    this->c.numClass,
                    this->c.numTrees,
                    this->c.maxDepth,
                    this->c.featureTrials,
                    this->c.thresholdTrials,
                    this->c.dataPerTree, 
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

void myMosqConcrete::updateFlask(std::string topic, std::string availability) {
    std::vector<std::pair<std::string, std::string>> kv;
    std::string ipaddress = Utils::Command::exec("hostname -I");
    kv.push_back(std::make_pair("ipaddress", ipaddress.substr(0, ipaddress.find(" "))));
    kv.push_back(std::make_pair("status", availability));
    //TODO: Get the file in here? or assigned by master
    kv.push_back(std::make_pair("datafile", "empty"));
    kv.push_back(std::make_pair("nodename", c.nodeName));
    kv.push_back(std::make_pair("master", c.masterNode));
    std::string json = Utils::Json::createJsonFile(kv);
    
    this->send_message(topic.c_str(), json.c_str());
}
