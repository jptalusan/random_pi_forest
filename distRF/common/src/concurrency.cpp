#include "concurrency.h"

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

//How to split the buffer to send to the nodes more efficiently
//now converts to string (per line) so that it would be accurate
std::vector<std::string> readFileToBuffer(std::string fileName) {
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

void concurrentReads(int numberOfThreads, std::vector<std::string> vec, std::vector<int> randomData) {
    if (vec.size() == 0) {
        return;
    }

    int division = vec.size() / numberOfThreads;
    int id, total, data;
    #pragma omp parallel private(data, id, total) num_threads(numberOfThreads)
    {
        id = omp_get_thread_num();
        total = omp_get_num_threads();
        data = id;
        std::cout << "Thread: " << id << " of: " << total << ", with division: " << division << " and data: " << data << std::endl;
        saveStringVecToFile(vec, division * id, division * (id + 1), id, randomData);
    }

    std::cout << "End of Parallel for loop." << std::endl;
}

void saveStringVecToFile(const std::vector<std::string>& sVec, int start, int end, int count, const std::vector<int>& randomData) {
    std::cout << "saveStringVecToFile: " << sVec.size() << ", " << start << ", " << end << ", " << count << std::endl;
    std::stringstream ss;
    ss << "data" << start << ".txt";
    std::string fileName = ss.str();
    std::ofstream outfile(fileName);
    for (int i = start; i < end; ++i) {
        outfile << sVec[randomData[i]] << std::endl;
    }
    outfile.close();
    //Probably should return some bool if error?
}
