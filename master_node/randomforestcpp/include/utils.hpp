#include <iostream>
#include <algorithm>
#include <string>
#include <map>
#include "rts_sample.hpp"
#include <chrono>

namespace Utils {
    class Parser {
    private:
        int numberOfFeatures = -1;
        int classColumn = -1;
    public:
        void setClassColumn(int classColumn) {
            this->classColumn = classColumn;
        }

        std::vector<RTs::Sample> readCSVToSamples(std::string fileName) {
            std::ifstream ifs(fileName, std::ifstream::binary);
            std::filebuf* pbuf = ifs.rdbuf();
            std::size_t size = pbuf->pubseekoff(0, ifs.end, ifs.in);
            pbuf->pubseekpos (0, ifs.in);
            std::cout << size << std::endl;
            char* buffer = new char[size];
            pbuf->sgetn (buffer, size);
            ifs.close();

            //int lines = std::count(buffer, buffer + size, '\n');
            // std::cout.write (buffer, size);

            std::ifstream infile(fileName);
            if (infile.good()) {
                std::string sLine;
                std::getline(infile, sLine);
                numberOfFeatures = std::count(sLine.begin(), sLine.end(), ',');
                
                std::cout << numberOfFeatures << std::endl;
            }
            infile.close();

             // std::cout << lines << std::endl;
            // std::cout << buffer[0] << std::endl;

            //Probably not needed to reserve lines
            std::ifstream t(fileName, std::ifstream::binary);
            std::stringstream stream;
            stream << t.rdbuf();
            std::string line;

            std::vector<RTs::Sample> samples;
            while (std::getline(stream, line)) {
                // std::cout << line << std::endl;
                std::stringstream ss(line);
                double i;
                int index = 0;
                RTs::Sample sample;
                sample.feature_vec.resize(numberOfFeatures);
                bool hasLabelBeenObtained = false;
                while (ss >> i) {
                    // std::cout << i << std::endl;
                    if (classColumn == index && !hasLabelBeenObtained) {
                        hasLabelBeenObtained = true;
                        sample.label = i;
                    } else {
                        sample.feature_vec[index] = i;
                        ++index;
                    }
                    if (ss.peek() == ',')
                        ss.ignore();
                }
                samples.push_back(sample);
            }
            delete[] buffer;
            return samples;
        }
    };

    class SCP {
    public:
        std::vector<std::string> nodeList;

        void setNodeList(std::vector<std::string> list) {
            nodeList = list;
        }

        std::vector<std::string> getNodeList() {
            return nodeList;
        }

        void getFiles() {
            std::stringstream ss;
            for(unsigned int i = 0; i < nodeList.size(); i++) {
                // std::cout << "Hello" << std::endl;
                ss.str(std::string());
                ss << "scp pi@" << nodeList[i] << ":/home/pi/RandomForest_for_smart_home_data/RTs_Forest.txt ./RTs_Forest_" <<  i << ".txt";
                std::string command = ss.str();
                //std::cout << command << std::endl;
                system(command.c_str());
            }
        }

        void deleteFiles() {
            std::stringstream ss;
            for(unsigned int i = 0; i < nodeList.size(); i++) {
                // std::cout << "Hello" << std::endl;
                ss.str(std::string());
                ss << "ssh pi@" << nodeList[i] << " ""rm -f /home/pi/RandomForest_for_smart_home_data/data/data.txt /home/pi/RandomForest_for_smart_home_data/RTs_Forest.txt";
                std::string command = ss.str();
                //std::cout << command << std::endl;
                system(command.c_str());
            }
        }

        void deleteLocalFiles() {
            std::stringstream ss;
            for(unsigned int i = 0; i < nodeList.size(); i++) {
                // std::cout << "Hello" << std::endl;
                ss.str(std::string());
                ss << "rm RTs_Forest_*.txt";
                std::string command = ss.str();
                std::cout << command << std::endl;
                system(command.c_str());
            }
        }
    };

    class TallyScores {
    private:
        std::vector<int> consolidateScores(std::vector<std::vector<int>> estimates) {
            std::vector<int> consolidated;

            for(unsigned int i = 0; i < estimates.size(); ++i) {
                consolidated.push_back(findMostCommonValue(estimates[i]));
            }
            return consolidated;
        }

        int findMostCommonValue(std::vector<int> classes) {
            std::map<int, int> mydict = {};
                int cnt = 0;
                int itm = 0;  // in Python you made this a string '', which seems like a bug

            for (auto&& item : classes) {
                mydict[item] = mydict.emplace(item, 0).first->second + 1;
                if (mydict[item] >= cnt) {
                    std::tie(cnt, itm) = std::tie(mydict[item], item);
                }
            }
            return itm;
        }

    public:
        void checkScores(std::vector<int> correctLabels, std::vector<std::vector<int>> classifications) {
            std::vector<int> consolidated = consolidateScores(classifications);
            int i = 0;
            int score = 0;
            std::for_each(consolidated.begin(), consolidated.end(), [&](int a) {
                std::cout << a;
                if (a == correctLabels[i]) {
                    ++score;
                }
                ++i;
            });

            std::cout << std::endl;
            std::for_each(correctLabels.begin(), correctLabels.end(), [](int a) {
                std::cout << a;
            });

            std::cout << std::endl << "Score: " << score << " / " << consolidated.size() << std::endl;
            float f = (float)score / (float)consolidated.size();
            std::cout << f << std::endl;
        }
    };

    class Timer {
    private:
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::high_resolution_clock::time_point endTime;
    public:
        void start() {
            startTime = std::chrono::high_resolution_clock::now();
            // std::cout << startTime.str() << std::endl;
        }

        void stop() {
            endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            // std::cout << endTime.str() << std::endl;
            std::cout << "Total time spent: " << duration << std::endl;
        }
    };
}