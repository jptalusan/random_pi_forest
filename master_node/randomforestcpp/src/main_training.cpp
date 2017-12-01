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

//#define DEBUG
int train();
int getClassNumberFromHistogram(int numberOfClasses, const float* histogram);
void distributedTest();
void centralizedTest();

int main(int argc, char *argv[]){
	if (argc < 2) {
		std::cout << "rf_exe [test|train (cent|dist)]" << std::endl;
	} else {
		std::string arg1(argv[1]);
		std::string arg2 = "";
		if (argc > 2) {
			arg2 = argv[2];
		}
		Utils::Timer* t = new Utils::Timer();
		std::cout << argc << ": " << arg1 << " " << arg2 << " " << std::endl;
		if (arg1 == "train") {
			t->start();
			train();
			t->stop();
		} else if (arg1 == "test" && arg2 == "dist" && argc == 3) {
			t->start();
			distributedTest();
			t->stop();
		} else if (arg1 == "test" && arg2 == "cent" && argc == 3) {
			t->start();
			centralizedTest();
			t->stop();
		} else {
			std::cout << "rf_exe [test|train]" << std::endl;
		}
		delete t;
	}
	return 0;
}

std::vector<RTs::Sample> getSamples() {
	Utils::Parser *p = new Utils::Parser();
	p->setClassColumn(1);
	return p->readCSVToSamples("cleaned.csv");
}

void centralizedTest() {
	char dir[255];
	getcwd(dir,255);
	std::cout << dir << std::endl;

	RTs::Forest rts_forest;
	std::stringstream ss;
	ss << "RTs_Forest.txt";
	rts_forest.Load(ss.str());

	Utils::Parser *p = new Utils::Parser();
	p->setClassColumn(1);
	//p->setNumberOfFeatures(54);
	std::vector<RTs::Sample> samples = p->readCSVToSamples("cleaned.csv");

	int score = 0;
	for (unsigned int i = 0; i < samples.size(); ++i) {
		RTs::Feature f = samples[i].feature_vec;
    	const float* histo = rts_forest.EstimateClass(f);
    	int classification = getClassNumberFromHistogram(10, histo);

    	if (classification == samples[i].label) {
    		++score;
    	}
	}
	float f = (float) score / (float) samples.size();
	std::cout << score << "/" << samples.size() << ": " << f << std::endl;
}

void distributedTest() {
	std::vector<std::string> nodeList = {
		"163.221.68.236",
		"163.221.68.245",
		"163.221.68.246"
	};

	Utils::SCP *u = new Utils::SCP();
	u->setNodeList(nodeList);
	u->getFiles();
	//u->deleteFiles();
	
	//Have to loop through all of the node list
	std::vector<std::vector<int>> scoreVectors;
	std::vector<int> correctLabel;
	std::vector<RTs::Forest> randomForests;
	//Assume to read RTs_Forest.txt
	char dir[255];
	getcwd(dir,255);
	std::cout << dir << std::endl;


	for (unsigned int i = 0; i < nodeList.size(); ++i) {
		RTs::Forest rts_forest;
		std::stringstream ss;
		ss << "RTs_Forest_" << i << ".txt";
		rts_forest.Load(ss.str());
		randomForests.push_back(rts_forest);
		ss.str(std::string());
	}
	//todo: process the rts_forest, load fxn already created the node
	// read the csv file here
	std::vector<RTs::Sample> samples = getSamples();
	

	//Too many loops for testing
	//Need to change checkScores func to just accept the samples vector (too large? const)
	std::for_each(samples.begin(), samples.end(), [&](const RTs::Sample& s) {
		correctLabel.push_back(s.label);
	});

	for (unsigned int i = 0; i < samples.size(); ++i) {
		std::vector<int> nodeListSamples(0, 3);
		scoreVectors.push_back(nodeListSamples);
		for (unsigned int j = 0; j < nodeList.size(); ++j) {
	    	RTs::Feature f = samples[i].feature_vec;
	    	const float* histo = randomForests[j].EstimateClass(f);

	    	//std::cout << getClassNumberFromHistogram(10, histo) << std::endl;

    		scoreVectors[i].push_back(getClassNumberFromHistogram(10, histo));
    	}
	}

 	Utils::TallyScores *ts = new Utils::TallyScores();
 	ts->checkScores(correctLabel, scoreVectors);
 	//u->deleteLocalFiles();
 	delete u;
 	delete ts;
}

int getClassNumberFromHistogram(int numberOfClasses, const float* histogram) {
	float biggest = -FLT_MAX;
	int index = -1;

	//since classes start at 1
	for (int i = 0; i < numberOfClasses; ++i) {
		float f = *(histogram + i);
		if (f > biggest) {
			index = i;
			biggest = f;
		}
	}
	return index;
}

//TODO: make arguments adjustable via argv and transfer code to pi to start distribution
int train() {
	char dir[255];
	getcwd(dir,255);
	std::cout << dir << std::endl;

	std::vector<RTs::Sample> samples = getSamples();
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
	if(!rts_forest.Learn(9, 5, 10, 50, 5, 1.0f, samples)){
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