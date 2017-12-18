#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include "utils.h"

std::vector<std::string> nodeList;

void hello() {
    std::cout << "Hello from thread " << std::this_thread::get_id() << std::endl;
}

int generateUniqueNoInRange(std::vector<int> iv, int start, int end) {
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<> distr(start, end - 1); // define the range

    int a = distr(eng);
    if (std::find(iv.begin(), iv.end(), a) != iv.end()) {
        return generateUniqueNoInRange(iv, start, end);
    } else {
        return a;
    }
}

std::vector<std::string> bufferTest(std::string fileName) {
    std::ifstream ifs(fileName, std::ifstream::binary);
    std::filebuf* pbuf = ifs.rdbuf();
    std::size_t size = pbuf->pubseekoff(0, ifs.end, ifs.in);
    pbuf->pubseekpos (0, ifs.in);

    char* buffer = new char[size];
    pbuf->sgetn (buffer, size);
    ifs.close();

    int lines = std::count(buffer, buffer + size, '\n');

    //Probably not needed to reserve lines
    std::vector<std::string> linesVec;
    linesVec.reserve(lines);
    std::ifstream t(fileName, std::ifstream::binary);
    std::stringstream stream;
    stream << t.rdbuf();
    std::string line;
    while (std::getline(stream, line)) {
        linesVec.push_back(line);
    }

    delete[] buffer;
    return linesVec;
}

bool sendFileViaScp(std::string filename, int count) {
    //scp -rv test2.txt pi@163.221.68.203:/home/pi/
    std::stringstream ss;
    ss << filename << " pi@" << nodeList[count] << ":/home/pi/random_pi_forest/slave_node/data/data.txt";
    std::string command = "scp -rv " + ss.str();
    //std::cout << command << std::endl;
    system(command.c_str());
    return true;
}

void sampleFunction(std::vector<std::string> sVec, int start, int end, int count, std::vector<int> randomData) {
    std::stringstream ss;
    ss << "data" << start << ".txt";
    std::string fileName = ss.str();
    std::ofstream outfile(fileName);
    for (int i = start; i < end; ++i) {
        //std::cout << sVec[i] << std::endl;
        outfile << sVec[randomData[i]] << std::endl;
    }
    outfile.close();
    sendFileViaScp(fileName, count);
}

void concurrentReads(int numberOfThreads, std::vector<std::string> vec, std::vector<int> randomData) {
    if (vec.size() == 0) {
        return;
    }

    //minus 1 to remove header
    int division = (vec.size() - 1) / numberOfThreads;
    //fix load balancing here
    std::vector<std::thread> threads;
    for (int i = 0; i < numberOfThreads; ++i) {
        threads.push_back(std::thread(sampleFunction, vec, division * i, division * ( i + 1 ), i, randomData));
    }

    for (auto& thread : threads) {
        thread.join();
    }
    
}

void populateNodeList(std::string fileName) {
    std::ifstream infile(fileName);
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string node;
        if (!(iss >> node)) { break; } // error
        // std::cout << node << std::endl;
        nodeList.push_back(node);
    }
}

int main(int argc, char* argv[]) {

    using namespace std;

    const std::string DFLT_SERVER_ADDRESS   { "172.27.0.1:1883" };
    const std::string DFLT_CLIENT_ID        { "async_publish" };

    const string TOPIC { "hello" };

    const char* PAYLOAD1 = "Hello World!";
    const char* PAYLOAD2 = "Hi there!";
    const char* PAYLOAD3 = "Is anyone listening?";
    const char* PAYLOAD4 = "Someone is always listening.";

    const char* LWT_PAYLOAD = "Last will and testament.";

    const int  QOS = 1;

    const auto TIMEOUT = std::chrono::seconds(10);

    string  address  = (argc > 1) ? string(argv[1]) : DFLT_SERVER_ADDRESS,
            clientID = (argc > 2) ? string(argv[2]) : DFLT_CLIENT_ID;

    cout << "Initializing for server '" << address << "'..." << endl;
    mqtt::async_client client(address, clientID);

    Utils::callback cb;
    client.set_callback(cb);

    mqtt::connect_options conopts;
    mqtt::message willmsg(TOPIC, LWT_PAYLOAD, 1, true);
    mqtt::will_options will(willmsg);
    conopts.set_will(will);

    cout << "  ...OK" << endl;

try {
        cout << "\nConnecting..." << endl;
        mqtt::token_ptr conntok = client.connect(conopts);
        cout << "Waiting for the connection..." << endl;
        conntok->wait();
        cout << "  ...OK" << endl;

        // First use a message pointer.

        cout << "\nSending message..." << endl;
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, PAYLOAD1);
        pubmsg->set_qos(QOS);
        client.publish(pubmsg)->wait_for(TIMEOUT);
        cout << "  ...OK" << endl;

        // Now try with itemized publish.

        cout << "\nSending next message..." << endl;
        mqtt::delivery_token_ptr pubtok;
        pubtok = client.publish(TOPIC, PAYLOAD2, strlen(PAYLOAD2), QOS, false);
        cout << "  ...with token: " << pubtok->get_message_id() << endl;
        cout << "  ...for message with " << pubtok->get_message()->get_payload().size()
            << " bytes" << endl;
        pubtok->wait_for(TIMEOUT);
        cout << "  ...OK" << endl;

        // Now try with a listener

        cout << "\nSending next message..." << endl;
        Utils::action_listener listener;
        pubmsg = mqtt::make_message(TOPIC, PAYLOAD3);
        pubtok = client.publish(pubmsg, nullptr, listener);
        pubtok->wait();
        cout << "  ...OK" << endl;

        // Finally try with a listener, but no token

        cout << "\nSending final message..." << endl;
        Utils::delivery_action_listener deliveryListener;
        pubmsg = mqtt::make_message(TOPIC, PAYLOAD4);
        client.publish(pubmsg, nullptr, deliveryListener);

        while (!deliveryListener.is_done()) {
            this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        cout << "OK" << endl;

        // Double check that there are no pending tokens

        auto toks = client.get_pending_delivery_tokens();
        if (!toks.empty())
            cout << "Error: There are pending delivery tokens!" << endl;

        // Disconnect
        cout << "\nDisconnecting..." << endl;
        conntok = client.disconnect();
        conntok->wait();
        cout << "  ...OK" << endl;
    }
    catch (const mqtt::exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }



/*
std::string arg1 = "";
    std::string arg2 = "";
    if (argc == 3) {
        arg1 = argv[1];
        arg2 = argv[2];
        populateNodeList(arg2);
    } else if (argc == 4) {

        arg1 = argv[1];
        arg2 = argv[2];
        populateNodeList(arg2);
        std::for_each(nodeList.begin(), nodeList.end(), [&](std::string s){
            std::string command = "scp -rv " + arg1 + " pi@" + s + ":/home/pi/random_pi_forest/slave_node/data/data.txt";
            std::cout << command << std::endl;
            system(command.c_str());
        });
        return 0;
    } else {
        std::cout << "./exe filename.csv nodeList.txt" << std::endl;
        return 0;
    }
    
    std::vector<std::string> data = bufferTest(arg1);
    int numberOfNodes = nodeList.size();

    std::vector<int> v(data.size());
    std::iota(v.begin(), v.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());
 
    std::shuffle(v.begin(), v.end(), g);
    concurrentReads(numberOfNodes, data, v);
*/
    return 0;
}
