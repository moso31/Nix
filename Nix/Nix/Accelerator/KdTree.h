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
	unique_ptr<KdTreeNode> RecursiveBuild(vector<PhotonMap>::iterator& itBegin, vector<PhotonMap>::iterator& itEnd, vector<PhotonMap>& data);

private:
	unique_ptr<KdTreeNode> pRoot;
};
