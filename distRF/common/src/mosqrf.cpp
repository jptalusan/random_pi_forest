#include "mosqrf.h"

myMosq::myMosq(const char* _id, const char* _topic, const char* _host, int _port) : mosquittopp(_id) {
    mosqpp::lib_init();
    this->keepalive = 60;
    this->id = _id;
    this->port = _port;
    this->host = _host;
    this->topic = _topic;
}

myMosq::~myMosq() {
    loop_stop();
    mosqpp::lib_cleanup();
}

bool myMosq::send_message(const char* topic, const char* _message) {
    //this->message = _message;
    //publish(int* mid, char* topic, int payloadlen, void *payload, int qos, bool retain);
    int ret = publish(NULL, topic, strlen(_message), _message, 2, false);
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

int myMosq::setupLastWill(const std::string topic, const std::string lastWillMessage) {
    int rc;

    std::cout << "Last will topic:" << topic << ", msg:" << lastWillMessage<< std::endl;
    
    rc = will_set(topic.c_str(), lastWillMessage.length(), lastWillMessage.c_str(), 2, false);

    return rc;
}

void myMosq::connect() {
    connect_async(this->host, this->port, this->keepalive);
    loop_start();
}
/*
bool myMosq::receive_message(const struct mosquitto_message *message) {
    std::cout << "Received message()" << std::endl;
    char* pchar = (char*)(message->payload);
    std::string str(pchar);
    std::cout << "Message from broker: " << str << std::endl;
    mqttif->receive_message();
    return true;
}
*/

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
    receive_message(message);
}

void myMosq::on_subscribe(int mid, int qos_count, const int* granted_qos) {
    std::cout << ">> subscription succeeded (" << mid << ") with qos: " << *granted_qos << ", and count of: " << qos_count << std::endl;
}

void myMosq::on_unsubscribe(int mid) {
    std::cout << ">> unsubscribed (" << mid << ") " << std::endl;
    loop_stop();
}

bool myMosq::subscribe_to_topic() {
    //subscribe(int* mid, char* sub, int qos);
    int ret = subscribe(NULL, this->topic, 2);
    return ( ret == MOSQ_ERR_SUCCESS );
}
std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  GetCurrentDir( buff, FILENAME_MAX );
  std::string current_working_dir(buff);
  return current_working_dir;
}

void writeToFile(const std::string buffer, std::string fileName) {
    std::stringstream ss;
    ss <<  GetCurrentWorkingDir() << "/" << fileName;
    std::ofstream outfile(ss.str());
    outfile << buffer;
    // unsigned size = buffer.length();
    // outfile.write(buffer, size);
    //delete[] buffer;
    outfile.close();
}

std::string fileToBuffer(std::string fileName) {
    std::ifstream ifs(fileName);
    std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    return content;
}
