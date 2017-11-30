#ifndef _RTS_NODE_HPP_
#define _RTS_NODE_HPP_

#include <string>
#include <sstream>
namespace RTs{

//
// RTs::Node ... 決定木 (Tree) を構成するノードのクラス．分岐ノードと末端ノードのいずれかになる．
//               分岐ノードでは *lChild と *rChild に子ノードのポインタを保持する．
//               末端ノードでは *distribution に配列を確保して，ヒストグラムを保持する．
//

class Node{
public:
	int feature_id;      // feature
	float threshold;     // threshold
	float *distribution; // distribution histogram
	Node *lChild;        // left Child node
	Node *rChild;        // right Child node
	
	Node(){
		distribution = NULL;
		lChild = NULL;
		rChild = NULL;
	}

	~Node(){
		if(distribution != NULL){
			delete [] distribution;
		}
		if(lChild != NULL){
			delete lChild;
		}
		if(rChild != NULL){
			delete rChild;
		}
	}

	std::string toString() {
		std::stringstream ss;
		if (distribution == NULL) {
			ss << "Ft_id: " << feature_id << ", " << "Thresh: " << threshold << ", NULL";
		} else {
			ss << feature_id << ", " << threshold << ", distrbution: \n\r\t";
			for (int i = 0; i < 10; ++i) {
				ss << *(distribution + i) << ", ";
			}
		}
		return std::string(ss.str());
	}

};

} // namespace RTs

#endif // _RTS_NODE_HPP_
