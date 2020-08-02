#pragma once
#include "Header.h"
#include <queue>

#define priority_quque_NXPhoton priority_queue<NXPhoton*, vector<NXPhoton*>, function<bool(NXPhoton* photonA, NXPhoton* photonB)>>

enum LocateFilter
{
	// 按何种方法统计kNN近邻点。
	Disk,
	Sphere
};

struct NXPhoton
{
	Vector3 position;
	Vector3 direction;
	Vector3 power;
	int depth;
};

struct NXKdTreeNode
{
	NXKdTreeNode();
	~NXKdTreeNode() {}

	shared_ptr<NXKdTreeNode> lc, rc;
	NXPhoton data;
	int dim;

	void Release();
};

class NXKdTree
{
public:
	NXKdTree();
	~NXKdTree();

	// 创建初始即平衡的kd树。
	void BuildBalanceTree(vector<NXPhoton>& data);
	shared_ptr<NXKdTreeNode> RecursiveBuild(size_t begin, size_t offset, vector<NXPhoton>& data);
	void GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_quque_NXPhoton& out_nearestPhotons, int maxLimit = 500, float range = 0.1f, LocateFilter locateFilter = Sphere);

	void Release();
	
private:
	void Locate(const Vector3& position, const Vector3& normal, const shared_ptr<NXKdTreeNode>& p, int maxLimit, float& out_mindistSqr, priority_quque_NXPhoton& out_nearestPhotons, LocateFilter locateFilter);

private:
	shared_ptr<NXKdTreeNode> pRoot;
	int m_x;
};
