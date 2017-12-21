#include "mosqrf.h"

myMosq::myMosq(const char* _id, const char* _topic, const char* _host, int _port) : mosquittopp(_id) {
    mosqpp::lib_init();
    this->keepalive = 60;
    this->id = _id;
    this->port = _port;
    this->host = _host;
    this->topic = _topic;
    connect_async(host, port, keepalive);
    loop_start();
}

myMosq::~myMosq() {
    loop_stop();
    mosqpp::lib_cleanup();
}

bool myMosq::send_message(const char* topic, const char* _message) {
    //this->message = _message;
    //publish(int* mid, char* topic, int payloadlen, void *payload, int qos, bool retain);
    int ret = publish(NULL, topic, strlen(_message), _message, 1, false);
    return ( ret == MOSQ_ERR_SUCCESS );
}

void myMosq::on_disconnect(int rc) {
    std::cout << ">> myMosq - disconnection(" << rc << ")" << std::endl;
}

void myMosq::on_connect(int rc) {
    if (rc == 0) {
        std::cout << ">> myMosq - connected with server" << std::endl;
    } else {
        std::cout << ">> myMosq - Impossible to connect with server(" << rc << ")" << std::endl;
    }
}

void myMosq::on_publish(int mid) {
    std::cout << ">> myMosq = Message (" << mid << ") succeed to be published " << std::endl;
    //delete message;
    }

bool myMosq::receive_message() {
    std::cout << "Received message()" << std::endl;
    return true;
}

/*
struct mosquitto_message{
        int mid;
        char *topic;
        void *payload;
        int payloadlen;
        int qos;
        bool retain;
};
*/
void myMosq::on_message(const struct mosquitto_message *message) {
    char* pchar = (char*)(message->payload);
    std::string str(pchar);
    std::cout << "Message from broker: " << str << std::endl;

    std::string topic(message->topic);

    //This is where we handle messages received from subscription
    //Need to count if all the slaves have responded before 
    //getting the testing the random forest model
    if (topic.find("master") != std::string::npos) {
        std::cout << "Received data from slave node: " << str << std::endl;
        std::cout << "Topic: " << topic << std::endl;

        //Parse message here and use that to get the name of the node (topic to publish to)
        writeToFile(str.c_str(), "output.txt");
        std::string reply = "ACK";
        publish(NULL, "slave/node1", reply.length(), reply.c_str(), 1, false);
    }
}

void myMosq::on_subscribe(int mid, int qos_count, const int* granted_qos) {
    std::cout << ">> subscription succeeded (" << mid << ") " << std::endl;
}

void myMosq::on_unsubscribe(int mid) {
    std::cout << ">> unsubscribed (" << mid << ") " << std::endl;
    loop_stop();
}

bool myMosq::subscribe_to_topic() {
    //subscribe(int* mid, char* sub, int qos);
    int ret = subscribe(NULL, this->topic, 1);
    return ( ret == MOSQ_ERR_SUCCESS );
}
std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  GetCurrentDir( buff, FILENAME_MAX );
  std::string current_working_dir(buff);
  return current_working_dir;
}


void writeToFile(const char* buffer, std::string fileName) {
    std::stringstream ss;
    ss <<  GetCurrentWorkingDir() << "/" << fileName;
    std::ofstream outfile(ss.str(), std::ofstream::binary);
    long size = strlen(buffer);
    outfile.write(buffer, size);
    //delete[] buffer;
    outfile.close();
}

char* fileToBuffer(std::string fileName) {
    std::ifstream ifs(fileName, std::ifstream::binary);
    std::filebuf* pbuf = ifs.rdbuf();
    std::size_t size = pbuf->pubseekoff(0, ifs.end, ifs.in);
    pbuf->pubseekpos (0, ifs.in);

    char* buffer = new char[size];
    pbuf->sgetn (buffer, size);
    ifs.close();

    int lines = std::count(buffer, buffer + size, '\n');

    std::cout << lines << std::endl;
    return buffer;
}
