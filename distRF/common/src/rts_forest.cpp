#include <math.h>
#include <string.h>
#include <float.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "rts_feature.hpp"
#include "rts_sample.hpp"
#include "rts_node.hpp"
#include "rts_tree.hpp"
#include "rts_forest.hpp"

#include <algorithm>

namespace RTs{
//#define DEBUG

/*!
@brief コンストラクタ
*/
Forest::Forest(){
	numClass = 0;
	numTrees = 0;
	maxDepth = 0;
	featureTrials = 0;
	thresholdTrials = 0;
	dataPerTree = 0;
	histogram = NULL;
}

/*!
@brief デストラクタ
*/
Forest::~Forest(){
	//maybe should also delete tree?
	if(histogram != NULL){
		delete [] histogram;
	}
}

/*!
@brief 教師信号による Randomized Forest を生成する関数．:1
@param[in] _numClass 識別するクラス数
@param[in] _numTrees 決定木の数
@param[in] _maxDepth 決定木の階層の最大数
@param[in] _featureTrials 特徴量ランダム選択の試行回数
@param[in] _thresholdTrials しきい値ランダム選択の試行回数
@param[in] _dataPerTree 個々の決定木に使う教師信号のサブセットの割合
@param[in] samples 教師信号
@return succeed or not
*/

bool Forest::Learn(
		const int _numClass,
		const int _numTrees,
		const int _maxDepth,
		const int _featureTrials,
		const int _thresholdTrials,
		const float _dataPerTree,
		std::vector<Sample> &samples){
	numClass = _numClass;
	numTrees = _numTrees;
	maxDepth = _maxDepth;
	featureTrials = _featureTrials;
	thresholdTrials = _thresholdTrials;
	dataPerTree = _dataPerTree;

	for(unsigned int t=0; t<trees.size(); ++t){
		delete trees[t];
	}
	trees.clear();

	if(histogram != NULL){
		delete [] histogram;
		histogram = NULL;
	}
	// std::cout << "＊Forest::Learn関数：教師信号による Randomized Forest を生成する関数:1＊" << std::endl;

	// ラベルの頻度の逆数を生成 (ヒストグラムの重み付けに使う)
	// std::cout << "	10_ラベルの頻度の逆数を生成 (ヒストグラムの重み付けに使う)" << std::endl;

	std::vector<float> inverse_label_freq(numClass, 0);
	//debug
	// for (std::vector<float>::const_iterator i = inverse_label_freq.begin(); i != inverse_label_freq.end(); ++i)
 //    	std::cout << *i << ' ';

	// std::cout << "	クラス数" << numClass << std::endl;

	//Counting all samples available for each class
	//But what is this for?
	for(unsigned int s=0; s<samples.size(); ++s){
		if(samples[s].label < 0 || numClass <= samples[s].label){
			// エラー：学習データ内で不正なラベルが使われている
			std::cout << "Error: Invalid label used in training data" << std::endl;
			return false;
		}
		inverse_label_freq[samples[s].label] += 1.0f;
	}

	//debug
	// for (std::vector<float>::const_iterator i = inverse_label_freq.begin(); i != inverse_label_freq.end(); ++i)
 //    	std::cout << *i << ' ';

	// std::cout << std::endl;
	for(int c=0; c<numClass; ++c){
		if(0 < inverse_label_freq[c]){
			inverse_label_freq[c] = 1.0f/inverse_label_freq[c];
		}
	}

	//debug
	// for (std::vector<float>::const_iterator i = inverse_label_freq.begin(); i != inverse_label_freq.end(); ++i)
 //    	std::cout << *i << ' ';

	// std::cout << std::endl;
	// Tree が受け付けるのは "Sample *" の vector なので，ポインタの vector を生成
	// std::cout << "	11_Tree が受け付けるのは Sample * の vector なので，ポインタの vector を生成" << std::endl;
	std::vector<const Sample *> samples_pointers(samples.size());
	for(unsigned int s=0; s<samples.size(); ++s){
		samples_pointers[s] = &samples[s];
	}

	//決定木の数繰り返す＝分散処理する数
	for(int t=0; t<numTrees; ++t){
		// 決定木を新たに作成
		// std::cout << "	12_決定木を新たに作成する" << std::endl;
		trees.push_back(new Tree(numClass));
		//Treeクラスの初期化
		//numClass = クラス数

		// この決定木のためのサブセット学習データを生成
		//This is only training data? so 0.25 percent of data is training?
		//the 0.5 does not do anything
		// std::cout << "	13_この決定木のためのサブセット学習データを生成" << std::endl;
		std::vector<const Sample *> subset_samples_pointers((int)(samples_pointers.size()*dataPerTree + .5));

		//debug
		// std::cout << (samples_pointers.size()*dataPerTree + .5) << std::endl;
		// std::cout << samples_pointers.size() << std::endl;
		// std::cout << subset_samples_pointers.size() << std::endl;

		int number;
		//Shouldn't we also check for duplicates here so that subset_samples_pointers is unique
		for(unsigned int s=0; s<subset_samples_pointers.size(); ++s){
			number = ((float)rand()/(float)(RAND_MAX)) * ((int)samples_pointers.size() - 1);
			subset_samples_pointers[s] = samples_pointers[number];
		}

		//ここの処理をCMMで書く
		// サブセットを使って決定木を学習させる
		/*!
		@brief BuildTree = 指定されたパラメータで学習データから決定木構造を生成する関数．
		@param[in] maxDepth 決定木の階層の最大数
		@param[in] featureTrials 特徴量ランダム選択の試行回数
		@param[in] thresholdTrials しきい値ランダム選択の試行回数
		@param[in] samples 教師信号
		@param[in] inverse_label_freq ラベルの頻度の逆数 (重み付けに使う)
		*/

		//Build tree using subset samples but create histogram from all
		// std::cout << "	14_サブセットを使って決定木を学習させる" << std::endl;
		if(trees.back()->BuildTree(maxDepth, featureTrials, thresholdTrials,
		                          subset_samples_pointers, inverse_label_freq) == false){
			// エラー：決定木の学習中に何かエラーが起きた模様
			// std::cout << "	Error: Pattern that something error occurred while learning decision tree" << std::endl;
			for(unsigned int t=0; t<trees.size(); ++t){
				delete trees[t];
			}
			trees.clear();
			return false;
		}

		// 決定木の末端ノードのヒストグラムを作成
		// std::cout << "	15_ Create a histogram of the terminal nodes of the decision tree" << std::endl;
		if(trees.back()->BuildHistograms(samples_pointers, inverse_label_freq) == false){
			// Error: Something seems to have occurred during histogram creation
			for(unsigned int t=0; t<trees.size(); ++t){
				delete trees[t];
			}
			trees.clear();
			return false;
		}

	}

	return true;
}

/*!
@brief A corresponding histogram is obtained from feature vectors to be identified:2
@param[in] feature_vec The feature vector to be identified
return histogram
*/

const float* Forest::EstimateClass(const Feature &feature_vec){
#ifdef DEBUG
	std::cout << "＊Forest::EstimateClass： A corresponding histogram is obtained from feature vectors to be identified:2＊" << std::endl;
	std::cout << "Trees: " << trees.size() << std::endl;
#endif
	if(trees.empty()){
		// エラー：決定木が作成されていない
		std::cout << "Error: Decision tree not created" << std::endl;
		return NULL;
	}

	// Buffer for histogram output
	if(histogram == NULL){
		histogram = new float[numClass];
	}
	memset(histogram, 0, numClass*sizeof(float));

	//This counts the trees and then collates all the histograms and adds it up into a random forest histogram
	for(unsigned int t=0; t<trees.size(); ++t){
		const float *tree_histogram = trees[t]->Traversal(feature_vec);
		if(tree_histogram == NULL){
			// エラー：決定木をたどる過程でエラーが発生した模様
			return NULL;
		}

		for(int c=0; c<numClass; ++c){
			histogram[c] += tree_histogram[c];
		}
	}

	// ヒストグラムを正規化する
	float total = 0;
	for(int c=0; c<numClass; ++c){
		total += histogram[c];
	}
	for(int c=0; c<numClass; ++c){
		histogram[c] /= total;
	}

	return histogram;
}

/*!
@brief Forest 全体を保存する:3
@param[in] filename ファイル名
@return succeed or not
*/

bool Forest::Save(const std::string &filename){

	// std::cout << "＊Forest::Save： Forest 全体を保存する．:3＊" << std::endl;

	if(trees.empty()){
		// エラー：決定木が作成されていない
		return false;
	}

	std::ofstream ofs(filename.c_str());
	if(ofs.fail()){
		return false;
	}

	// まず Forest 内のパラメータを保存
	ofs << numClass << " "
	    << numTrees << " "
	    << maxDepth << " "
	    << featureTrials << " "
	    << thresholdTrials << " "
		<< dataPerTree << std::endl;
	if(ofs.fail()){
		return false;
	}

	for(unsigned int t=0; t<trees.size(); ++t){
		// 各 Tree の情報を保存
		if(trees[t]->Save(ofs) == false){
			return false;
		}
	}

	return true;
}

/*!
@brief Forest Load the whole:4
@param[in] filename ファイル名
@return succeed or not
*/

bool Forest::Load(const std::string &filename){
	// std::cout << "＊Forest::Load： Forest Load the whole．4＊" << std::endl;

	for(unsigned int t=0; t<trees.size(); ++t){
		delete trees[t];
	}
	trees.clear();

	std::ifstream ifs(filename.c_str());
	if(ifs.fail()){
		return false;
	}

	// まず Forest 内のパラメータを読み込み
	ifs >> numClass
	    >> numTrees
	    >> maxDepth
	    >> featureTrials
	    >> thresholdTrials
		>> dataPerTree;
	if(ifs.fail()){
		return false;
	}

	for(int t=0; t<numTrees; ++t){

		// 決定木を新たに作成
		trees.push_back(new Tree(numClass));

		// Tree の情報を読み込み
		if(trees.back()->Load(ifs) == false){
			return false;
		}
	}

	return true;
}


} // namespace RTs
