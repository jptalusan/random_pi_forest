#ifndef _RTS_SAMPLE_HPP_
#define _RTS_SAMPLE_HPP_

#include "rts_feature.hpp"

namespace RTs{

//
// RTs::Sample ... 特徴ベクトルとラベルをセットとする教師信号のクラス．
//

class Sample{
public:
	Feature feature_vec; // 特徴ベクトル
	int label;           // ラベル
};

} // namespace RTs

#endif // _RTS_SAMPLE_HPP_
