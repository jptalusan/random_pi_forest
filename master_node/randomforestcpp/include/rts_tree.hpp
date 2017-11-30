#ifndef _RTS_TREE_HPP_
#define _RTS_TREE_HPP_

#include "rts_feature.hpp"
#include "rts_sample.hpp"
#include "rts_node.hpp"

namespace RTs{

//
// RTs::Tree ... 決定木 (Tree) のクラス．
//

class Tree{
private:
	int numClass;
	Node* root;

	// 再帰処理で Tree (= Node の階層) を生成する．学習処理のメイン部分．
	Node* build(const int maxDepth,
	            const int featureTrials,
	            const int thresholdTrials,
	            const std::vector<const Sample *> &samples,
	            const std::vector<float> &inverse_label_freq,
	            const int depth);

	// 再帰処理で末端ノードを探し，たどり着いた末端ノードのヒストグラムをゼロ初期化する．
	bool clearHistograms(Node *node);

	// 再帰処理で学習データを処理して末端ノードを探し，たどり着いた末端ノードのヒストグラムに投票する．
	bool voteHistograms(const Sample &sample,
	                    const std::vector<float> &inverse_label_freq,
	                    Node *node);

	// 再帰処理で末端ノードを探し，たどり着いた末端ノードのヒストグラムを正規化する．
	bool normalizeHistograms(Node *node);

	// 再帰処理で末端ノードを探し，たどり着いた末端ノードのヒストグラムを返す．識別処理に相当する．
	const float *traversal(Node *node,
	                       const Feature &feature_vec);

	// 再帰処理でノードを保存する
	bool save(std::ofstream &ofs,
	          Node *node);

	// 再帰処理でノードを読み込む
	Node* load(std::ifstream &ifs);

public:
	// コンストラクタ
	Tree(const int _numClass);

	// デストラクタ
	~Tree();

	// 指定されたパラメータで学習データから Tree を生成する．再帰処理で build() を実行する起点．
	bool BuildTree(const int maxDepth,
	               const int featureTrials,
	               const int thresholdTrials,
	               const std::vector<const Sample *> &samples,
	               const std::vector<float> &inverse_label_freq);

	// 構築済みの Tree 構造に対して学習データを与え，ヒストグラムを生成する．
	// clearHistograms(), voteHistograms(), normalizeHistograms() を実行する．
	bool BuildHistograms(const std::vector<const Sample *> &samples,
	                     const std::vector<float> &inverse_label_freq);

	// 識別対象の特徴ベクトルから，対応するヒストグラムを求める．再帰処理で traversal() を実行する起点．
	const float *Traversal(const Feature &feature_vec);

	// Tree を保存する．再帰処理で save() を実行する起点．
	bool Save(std::ofstream &ofs);

	// Tree を読み込む．再帰処理で load() を実行する起点．
	bool Load(std::ifstream &ifs);
};

} // namespace RTs

#endif // _RTS_TREE_HPP_
