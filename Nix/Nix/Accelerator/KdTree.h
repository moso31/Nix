#pragma once
#include "Header.h"

struct PhotonMap;

struct KdTreeNode
{
	unique_ptr<KdTreeNode> lc, rc;
};

class KdTree
{
public:
	KdTree();
	~KdTree();

	// 创建初始即平衡的kd树。
	void BuildBalanceTree(vector<PhotonMap>& data);

	void RecursiveBuildBalanceTree(vector<PhotonMap>::iterator& itBegin, vector<PhotonMap>::iterator& itEnd);

private:
	unique_ptr<KdTreeNode> pRoot;
};
