#include <math.h>
#include <string.h>
#include <float.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// #include "rts_feature.hpp"
// #include "rts_sample.hpp"
#include "rts_tree.hpp"
//#define DEBUG
namespace RTs{


/*!
@brief 再帰処理で Node の階層を生成する関数．:1
@param[in] maxDepth 決定木の階層の最大数
@param[in] featureTrials 特徴量ランダム選択の試行回数
@param[in] thresholdTrials しきい値ランダム選択の試行回数
@param[in] samples 教師信号
@param[in] inverse_label_freq ラベルの頻度の逆数 (重み付けに使う)
@param[in] depth 決定木の内に今から生成する Node の深さ
@return node
*/

Node *Tree::build(const int maxDepth,
                  const int featureTrials,
                  const int thresholdTrials,
                  const std::vector<const Sample *> &samples,
                  const std::vector<float> &inverse_label_freq,
                  const int depth){


	// std::cout << "		*Tree::build:再帰処理で Node の階層を生成する関数．:1" << std::endl;
	if(samples.empty()){
		std::cout << "		samples.empty()" << std::endl;
		return NULL;
	}

	// 特徴量の次元数．ちょっとダサいが，サンプル中の特徴量の次元数を参照する．
	// std::cout << "		140_特徴量の次元数．ちょっとダサいが，サンプル中の特徴量の次元数を参照する" << std::endl;
	unsigned int feature_dim = samples[0]->feature_vec.size();
	// std::cout << "		特徴量の次元数=" << feature_dim << std::endl;

	// 後のしきい値ランダム生成で特徴量の各次元の最大値，最小値が必要なので，ここで先に求めておく
	// std::cout << "		Since the maximum value and the minimum value of each dimension of the feature quantity are necessary in the threshold value random generation after 141 _, it is obtained here first" << std::endl;
	std::vector<float> feature_min_values(feature_dim);
	std::vector<float> feature_max_values(feature_dim);
	for(unsigned int f=0; f<feature_dim; ++f){
		feature_min_values[f] = feature_max_values[f] = samples[0]->feature_vec[f];
	}
	//skip 0 since it was already assigned above.
	for(unsigned int s=1; s<samples.size(); ++s) {
		const Feature *feature_vec = &(samples[s]->feature_vec);
		if(feature_vec->size() != feature_dim){
			// エラー：特徴量の次元数が合っていない??
			std::cout << "エラー：特徴量の次元数が合っていない??" << std::endl;
			return NULL;
		}
		// std::cout << "i: " << s << std::endl;

		for(unsigned int f=0; f<feature_dim; ++f){
			float feature_value = (*feature_vec)[f];
			// std::cout << feature_value << std::endl;
			if(feature_value < feature_min_values[f]){
				feature_min_values[f] = feature_value;
			}
			else if(feature_max_values[f] < feature_value){
				feature_max_values[f] = feature_value;
			}
		}

	//debug
		// std::cout << "Min and max" << std::endl;
		// for (std::vector<float>::const_iterator i = feature_min_values.begin(); i != feature_min_values.end(); ++i)
	 //    	std::cout << *i << ' ' << std::endl;
	 //    std::cout << std::endl;

		// for (std::vector<float>::const_iterator i = feature_max_values.begin(); i != feature_max_values.end(); ++i)
	 //    	std::cout << *i << ' '  << std::endl;

	}
	//Obtained the min and max of each feature (why?!)

	// A histogram creation buffer for evaluating segmentation results
	std::vector<float> lDistribution(numClass, 0);
	std::vector<float> rDistribution(numClass, 0);

	// Buffer for holding the division result
	std::vector<const Sample *> lSamples;
	std::vector<const Sample *> rSamples;
	// 必要になり得るサイズの最大値分のバッファを確保しておく
	lSamples.reserve(samples.size());
	rSamples.reserve(samples.size());

	// Search division results that maximize information gain
	// std::cout << "		142_情報利得を最大化する分割結果を探索する" << std::endl;
	double bestGain = -FLT_MAX;
	float bestThreshold = 0;
	int bestFeature = 0;
	std::vector<const Sample *> bestlSamples;
	std::vector<const Sample *> bestrSamples;
	bestlSamples.reserve(samples.size());
	bestrSamples.reserve(samples.size());

	// std::cout << "		143_分岐につかう特徴量の次元をランダムに決定" << std::endl;
	for(int ft=0; ft<featureTrials; ++ft){

		// Randomly determine the dimensions of features used for branching
		int rand_feature_id = (int)(rand() % feature_dim);

		for(int tt=0; tt<thresholdTrials; ++tt){

			// The threshold used for branching is randomly determined between the maximum and minimum of the feature amount "rand_feature_id"
			float rand_threshold = feature_min_values[rand_feature_id]
			                      +(float)rand()/(float)(RAND_MAX)
			                       *(feature_max_values[rand_feature_id] - feature_min_values[rand_feature_id]);

			//
			// 上記のランダムなしきい値で分割を試す
			//

			lDistribution.clear();
			lDistribution.resize(numClass, 0);
			rDistribution.clear();
			rDistribution.resize(numClass, 0);
			lSamples.clear();
			rSamples.clear();

			//for all samples, pick a random feature id and then get the value, compare to threshold
			//if less -> left, if more -> right (binary tree)
			//increment distribution for that particular label (Scoring for determining which is the right label)
			for(unsigned int s=0; s<samples.size(); ++s){
				int label = samples[s]->label;
				float feature_val = samples[s]->feature_vec[rand_feature_id];
				if(feature_val < rand_threshold){
					lSamples.push_back(samples[s]);
					lDistribution[label] += inverse_label_freq[label];
				}
				else{
					rSamples.push_back(samples[s]);
					rDistribution[label] += inverse_label_freq[label];
				}
			}

			//
			// 分割結果の情報利得の計算
			//

			float lTotal = 0;
			float rTotal = 0;
			for(int c=0; c<numClass; ++c){
				lTotal += lDistribution[c];
				rTotal += rDistribution[c];
			}

			if(0 < lTotal){
				for(int c=0; c<numClass; ++c){
					lDistribution[c] /= lTotal;
				}
			}
			if(0 < rTotal){
				for(int c=0; c<numClass; ++c){
					rDistribution[c] /= rTotal;
				}
			}

			float lEntropy = 0;
			float rEntropy = 0;
#define STRICT_CALCULATION
#ifdef STRICT_CALCULATION
#define LOG10_2 0.301029995663981198017 // = log10(2.0)
			for(int c=0; c<numClass; ++c){
				if(0 < lDistribution[c]){
					lEntropy += lDistribution[c]*log10f(lDistribution[c])/LOG10_2;
				}
				if(0 < rDistribution[c]){
					rEntropy += rDistribution[c]*log10f(rDistribution[c])/LOG10_2;
				}
			}

			double gain = (lSamples.size()*lEntropy + rSamples.size()*rEntropy)/(lSamples.size() + rSamples.size());
#else // STRICT_CALCULATION
			//Since only the magnitude relation for finding the maximum value of gain is
			// important in the first place, it is always good to ignore such coefficients
			for(int c=0; c<numClass; ++c){
				if(0 < lDistribution[c]){
					lEntropy += lDistribution[c]*log10f(lDistribution[c]);
				}
				if(0 < rDistribution[c]){
					rEntropy += rDistribution[c]*log10f(rDistribution[c]);
				}
			}

			//I dont get these blocks of code, why only multiply entropy with the size?
			double gain = lSamples.size()*lEntropy + rSamples.size()*rEntropy;
#endif // STRICT_CALCULATION

			//
			// 情報利得の最大値を探す
			//
			//std::cout << "144 _ Search for maximum value of information gain" << std::endl;

			if(bestGain < gain){
				bestGain = gain;
				bestFeature = rand_feature_id;
				bestThreshold = rand_threshold;
				bestlSamples = lSamples;
				bestrSamples = rSamples;
			}
		}

		//std::cout << "		145_I am looking for an optimal branching function"
		// std::cout<< "145_[" << ft+1 << "/" << featureTrials << "]" << "gain = " << bestGain << "\r\n";
		// std::cout.flush();
	}
	// std::cout << std::endl;

	// 最良だったパラメータをノードに保存
	// std::cout << "		146_Save best parameters to node" << std::endl;
	Node *node = new Node();
	node->feature_id = bestFeature;
	node->threshold = bestThreshold;

	//
	// 今回で末端ノードに到達したかどうかチェック
	//
	// std::cout << "		147 _ Check if it reached the terminal node this time" << std::endl;
	if(-FLT_MIN < bestGain || maxDepth < depth || bestlSamples.empty() || bestrSamples.empty()){
		// 末端ノードに到達したので，ヒストグラム用のメモリを確保．
		// (Since the contents of the histogram will be generated later, we do not mind here)
		// std::cout << "		148_Since it reached end node, securing memory for histogram" << std::endl;
		// std::cout << "		leaf node !" << std::endl;
		node->distribution = new float[numClass];
	}
	else{
		// Since it is still a branch node, it creates child nodes
		// std::cout << "		149_まだ分岐ノードなので，子ノードを生成する" << std::endl;
		node->lChild = build(maxDepth, featureTrials, thresholdTrials, bestlSamples, inverse_label_freq, depth+1);
		node->rChild = build(maxDepth, featureTrials, thresholdTrials, bestrSamples, inverse_label_freq, depth+1);
		if(node->lChild == NULL || node->rChild == NULL){
			// 子ノードの生成過程で何かエラーが起きた模様
			delete node;
			node = NULL;
		}
	}

	return node;

}


/*!
@brief 再帰処理で末端ノードを探し，末端ノードのヒストグラムをゼロ初期化する関数．:2
@param[in] node 今からチェックする Node
@return succeed or not
*/

bool Tree::clearHistograms(Node *node){
#ifdef DEBUG
	std::cout << "Tree::clearHistogramsFunction: A function that finds a terminal node by recursive processing and zero initializes the histogram of the terminal node.＊" << std::endl;
#endif
	if(node == NULL){
		return false;
	}

	if(node->distribution == NULL){
		// このノードは分岐ノードなので，決定木の子ノードへ進む
#ifdef DEBUG
		std::cout << "		Since this node is a branching node, proceed to the child node of the decision tree" << std::endl;
#endif
		if(clearHistograms(node->lChild) == false
		|| clearHistograms(node->rChild) == false){
			return false;
		}
	}
	else{
		// Since this node is a terminal node, clear the histogram to zero
#ifdef DEBUG
		std::cout << "		Since this node is a terminal node, clear the histogram to zero" << std::endl;
#endif
		memset(node->distribution, 0, numClass*sizeof(float));
	}

	return true;

}


/*!
@brief A function to vote the teacher signal on the histogram of the terminal node.:3
@param[in] sample 今から投票する教師信号
@param[in] inverse_label_freq ラベルの頻度の逆数 (重み付けに使う)
@param[in] node 今からチェックする Node
@return succeed or not
*/

bool Tree::voteHistograms(const Sample &sample,
                          const std::vector<float> &inverse_label_freq,
                          Node *node){
	if(node == NULL){
		return false;
	}

	//std::cout << "Tree::voteHistograms function: A function that votes a teacher signal into a histogram of a terminal node.:3＊" << std::endl;
	if(node->distribution == NULL){
		// このノードは分岐ノードなので，決定木の子ノードへ進む
		if(sample.feature_vec[node->feature_id] < node->threshold){
			return voteHistograms(sample, inverse_label_freq, node->lChild);
		}
		else{
			return voteHistograms(sample, inverse_label_freq, node->rChild);
		}
	}
	else{
		// Since this node is a terminal node, it votes on the histogram
		int label = sample.label;
		float weight = inverse_label_freq[label];
		node->distribution[label] += weight;
	}

	return true;

}


/*!
@brief 再帰処理で末端ノードを探し，末端ノードのヒストグラムを正規化する関数．:4
@param[in] node 今からチェックする Node
@return succeed or not
*/

bool Tree::normalizeHistograms(Node *node){
#ifdef DEBUG
	std::cout << "Tree::normalizeHistograms function: a function that finds the terminal node by recursive processing and normalizes the histogram of the terminal node:4＊" << std::endl;
#endif
	if(node == NULL){
		return false;
	}

	if(node->distribution == NULL){
		// Since this node is a branching node, proceed to the child node of the decision tree
		// i think this can be done in parallel
		if(normalizeHistograms(node->lChild) == false
		|| normalizeHistograms(node->rChild) == false){
			return false;
		}
	}
	else{
		// Since this node is a terminal node, normalize the histogram
		double total_dist = 0;
		for(int c=0; c<numClass; ++c){
			total_dist += node->distribution[c];
		}
		for(int c=0; c<numClass; ++c){
			node->distribution[c] /= total_dist;
		}
	}

	return true;

}


/*!
@brief A function that finds the terminal node by recursive processing and returns the histogram of the terminal node.:5
@param[in] node 今からチェックする Node
@param[in] feature_vec 識別対象の特徴量
@return histogram
*/
//uses the feature vector as input for the decision tree
const float *Tree::traversal(Node *node,
                             const Feature &feature_vec){
#ifdef DEBUG
	std::cout << "*Tree::traversal: A function that finds a terminal node by recursive processing and returns a histogram of the terminal node.:5＊" << std::endl;
#endif
	if(node == NULL){
		return NULL;
	}

	if(node->distribution == NULL){
		// Since this node is a branching node, proceed to the child node of the decision tree
		if(feature_vec[node->feature_id] < node->threshold){
			return traversal(node->lChild, feature_vec);
		}
		else{
			return traversal(node->rChild, feature_vec);
		}
	}

	// このノードは末端ノードなのでヒストグラムを返す
	return node->distribution;
}


/*!
@brief 再帰処理で決定木構造の全てのノードを保存する関数．:6
@param[in] ofs 出力ストリーム
@param[in] node 今から保存する Node
@return succeed or not
*/

bool Tree::save(std::ofstream &ofs,
                Node *node){
#ifdef DEBUG
	std::cout << "		Tree::save関数:再帰処理で決定木構造の全てのノードを保存する関数:6" << std::endl;
#endif
	if(node == NULL){
		return false;
	}

	if(node->distribution == NULL){
		// このノードは分岐ノード
		ofs << node->feature_id << " "
		    << node->threshold << " "
		    << false
		    << std::endl;
		if(ofs.fail()){
			return false;
		}

		// 分岐ノードなので再帰処理で子ノードを書き込む
		if(save(ofs, node->lChild) == false
		|| save(ofs, node->rChild) == false){
			return false;
		}
	}
	else{
		// このノードは末端ノード
		ofs << node->feature_id << " "
		    << node->threshold << " "
		    << true
		    << std::endl;
		if(ofs.fail()){
			return false;
		}

		// 末端ノードなのでヒストグラムを書き込む
		for(int c=0; c<numClass; ++c){
			ofs << node->distribution[c] << " ";
			if(ofs.fail()){
				return false;
			}
		}
		ofs << std::endl;
		if(ofs.fail()){
			return false;
		}
	}

	return true;

}


/*!
@brief 再帰処理で決定木構造の全てのノードを読みこむ関数．:7
@param[in] ifs 入力ストリーム
@return node (root node)
*/

Node *Tree::load(std::ifstream &ifs){
#ifdef DEBUG
	std::cout << "		*Tree::load: A function that reads all the nodes of the decision tree structure recursively:7" << std::endl;
#endif
	Node *node = new Node();
	bool isLeafNode;

	ifs >> node->feature_id
	    >> node->threshold
	    >> isLeafNode;
	if(ifs.fail()){
		delete node;
		return NULL;
	}

	//Leaf node == terminal node
#ifdef DEBUG
	if (!isLeafNode) {
		std::cout << node->toString() << std::endl;
	}
#endif
	if(isLeafNode == true){
		//Leaf node == terminal node

		// Since this node is an end node, it reads a histogram
		node->distribution = new float[numClass];
		for(int c=0; c<numClass; ++c){
			ifs >> node->distribution[c];
			if(ifs.fail()){
				delete node;
				return NULL;
			}
		}
#ifdef DEBUG
		std::cout << (isLeafNode?"Leaf node":"Not leaf node") << " ";
		std::cout << node->toString() << std::endl;
#endif
	}
	else{
		// Since this node is a branch node, it reads a child node
		// left nodes are filled first
		node->lChild = load(ifs);
		node->rChild = load(ifs);
		if(node->lChild == NULL || node->rChild == NULL){
			delete node;
			return NULL;
		}
	}


	return node;

}


/*!
@brief コンストラクタ:8
@param[in] _numClass 識別するクラス数
*/

Tree::Tree(const int _numClass){
	numClass = _numClass;
	root = NULL;
}


/*!
@brief デストラクタ:9
*/

Tree::~Tree(){
	if(root != NULL){
		delete root;
	}
}


/*!
@brief 指定されたパラメータで学習データから決定木構造を生成する関数．:10
@param[in] maxDepth 決定木の階層の最大数
@param[in] featureTrials 特徴量ランダム選択の試行回数
@param[in] thresholdTrials しきい値ランダム選択の試行回数
@param[in] samples 教師信号
@param[in] inverse_label_freq ラベルの頻度の逆数 (重み付けに使う)
*/

bool Tree::BuildTree(const int maxDepth,
                     const int featureTrials,
                     const int thresholdTrials,
                     const std::vector<const Sample *> &samples,
                     const std::vector<float> &inverse_label_freq){

	// std::cout << "		Tree::BuildTree関数:指定されたパラメータで学習データから決定木構造を生成する関数:10" << std::endl;
	if(root != NULL){
		// std::cout << "		root != NULL" << std::endl;
		delete root;
	}
	// std::cout << "		build関数へ．" << std::endl;
	root = build(maxDepth, featureTrials, thresholdTrials, samples, inverse_label_freq, 0);


	if(root == NULL){
		// 何かエラーが起きた場合に NULL が返ってくる
		return false;
	}

	return true;

}


/*!
@brief A function to generate a histogram of the end node by learning data using the constructed decision tree structure: 11.
@param[in] samples 今から投票する教師信号
@param[in] inverse_label_freq (Inverse of label frequency (used for weighting))
@param[in] node 今からチェックする Node
@return succeed or not
*/

bool Tree::BuildHistograms(const std::vector<const Sample *> &samples,
                           const std::vector<float> &inverse_label_freq){

	// std::cout << "		Tree::BuildHistogramsFunction: A function that generates a histogram of a terminal node by learning data using a constructed decision tree structure:11" << std::endl;

	if(root == NULL){
		std::cout << "		root == NULL" << std::endl;
		return false;
	}

	// 全ての末端ノードのヒストグラムをゼロクリアする
	// std::cout << "		Clear the histogram of all the terminal nodes to zero" << std::endl;
	if(clearHistograms(root) == false){
		std::cout << "		clearHistograms(root) == false" << std::endl;
		return false;
	}

	// 学習データを１つづつヒストグラムに投票する
	// std::cout << "		Vote the learning data one by one on the histogram" << std::endl;
	for(unsigned int s=0; s<samples.size(); ++s){
		if(voteHistograms(*samples[s], inverse_label_freq, root) == false){
			return false;
		}
	}

	// 全ての末端ノードのヒストグラムを正規化する
	// std::cout << "		Normalize the histogram of all the terminal nodes" << std::endl;
	if(normalizeHistograms(root) == false){
		return false;
	}

	return true;

}


/*!
@brief 識別対象の特徴ベクトルから，対応するヒストグラムを求める関数．:12
@param[in] feature_vec 特徴ベクトル
@return ヒストグラム (エラー時には NULL が返る)
*/
//Start at the root
const float *Tree::Traversal(const Feature &feature_vec){
#ifdef DEBUG
	std::cout << "		Tree::Traversal: for obtaining a corresponding histogram from the feature vector to be identified:12" << std::endl;
#endif
	if(root == NULL){
		return NULL;
	}

	return traversal(root, feature_vec);

}


/*!
@brief 決定木構造を保存する関数．:13
@param[in] ofs 出力ストリーム
@return succeed or not
*/

bool Tree::Save(std::ofstream &ofs){
#ifdef DEBUG
	std::cout << "		Tree::Save関数:決定木構造を保存する関数．" << std::endl;
#endif
	if(root == NULL){
		return false;
	}

	return save(ofs, root);

}


/*!
@brief 決定木構造を読みこむ関数．:14
@param[in] ifs 入力ストリーム
@return succeed or not
*/

bool Tree::Load(std::ifstream &ifs){
#ifdef DEBUG
	std::cout << "		Tree::Load関数:決定木構造を読みこむ関数．" << std::endl;
#endif
	if(root == NULL){
		delete root;
	}

	root = load(ifs);
	if(root == NULL){
		return false;
	}

	return true;

}


} // namespace RTs
