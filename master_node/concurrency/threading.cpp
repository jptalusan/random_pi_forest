#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>

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
    std::string arg1 = "";
    std::string arg2 = "";
    if (argc == 3) {
        arg1 = argv[1];
        arg2 = argv[2];
        populateNodeList(arg2);
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

    return 0;
}
