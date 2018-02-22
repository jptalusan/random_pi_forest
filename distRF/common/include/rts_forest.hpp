#ifndef _RTS_FOREST_HPP_
#define _RTS_FOREST_HPP_

#include "rts_tree.hpp"

namespace RTs{

//
// RTs::Forest ... 複数の決定木 (Tree) を束ねるクラス．
//

class Forest{
private:
	int numClass;        // 識別するクラス数
	int numTrees;        // 決定木の数
	int maxDepth;        // 決定木の最大の深さ
	int featureTrials;   // 特徴量のランダム選択回数
	int thresholdTrials; // 閾値のランダム選択回数
	float dataPerTree;   // 各 Tree に与える教師信号のサブセットの割合

	std::vector<Tree *> trees; // 決定木
	float *histogram;

public:
	// コンストラクタ
	Forest();

	// デストラクタ
	~Forest();

	// 教師信号による Randomized Forest を生成する．
	bool Learn(const int _numClass,
	           const int _numTrees,
	           const int _maxDepth,
	           const int _featureTrials,
	           const int _thresholdTrials,
	           const float _dataPerTree,
	           std::vector<Sample> &samples);

	// 識別対象の特徴ベクトルから，対応するヒストグラムを求める．
	const float *EstimateClass(const Feature &feature_vec);

	// Forest 全体を保存する
	bool Save(const std::string &filename);

	// Forest 全体を読みこむ
	bool Load(const std::string &filename);

	std::vector<Tree *> getTrees() {
		return trees;
	}

	std::vector<int> RunThroughAllTrees(const Feature &feature_vec);

	bool ConcatenateTrees(std::vector<Tree *> trees);
};

} // namespace RTs

#endif // _RTS_FOREST_HPP_
