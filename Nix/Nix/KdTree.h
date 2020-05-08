#pragma once
#include "Header.h"

struct NXPhoton;

struct KdTreeNode
{
	unique_ptr<KdTreeNode> lc, rc;
};

class NXKdTree
{
public:
	NXKdTree();
	~NXKdTree();

	// 创建初始即平衡的kd树。
	void BuildBalanceTree(vector<NXPhoton>& data);
	unique_ptr<KdTreeNode> RecursiveBuild(vector<NXPhoton>::iterator& itBegin, vector<NXPhoton>::iterator& itEnd, vector<NXPhoton>& data);

private:
	unique_ptr<KdTreeNode> pRoot;
};
