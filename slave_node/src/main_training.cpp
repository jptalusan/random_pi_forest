#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <float.h>
#include <algorithm>
#include "SCP_utils.hpp"
#include "rts_forest.hpp"
#include "rts_tree.hpp"
#include "csv.h"

#define CSV_IO_NO_THREAD
//#define DEBUG
int train();
std::vector<RTs::Sample> getSamples();
int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
void distributedTest();
void centralizedTest();

int main(int argc, char *argv[]){
        //
        // サンプルデータの読み込み
        //
        std::cout << argc << argv[0] << std::endl;
        train();
        return 0;
}

//TODO: make arguments adjustable via argv and transfer code to pi to start distribution
int train() {
        Utils::Parser *p = new Utils::Parser();
        p->setClassColumn(1);
        p->setDelimiter(',');
        //std::vector<RTs::Sample> samples = p->readCSVToSamples("/home/pi/RandomForest_for_smart_home_data/data/data.txt")
        std::vector<RTs::Sample> samples;
        samples = p->readCSVToSamples("cleaned.csv");
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
