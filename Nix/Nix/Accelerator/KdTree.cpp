#include "KdTree.h"
#include "NXPhotonMap.h"

KdTree::KdTree()
{
}

KdTree::~KdTree()
{
}

void KdTree::BuildBalanceTree(vector<PhotonMap>& data)
{
	vector<PhotonMap>::iterator itBegin = data.begin();
	vector<PhotonMap>::iterator itEnd = data.end();
	pRoot = RecursiveBuild(itBegin, itEnd, data);
}

unique_ptr<KdTreeNode> KdTree::RecursiveBuild(vector<PhotonMap>::iterator& itBegin, vector<PhotonMap>::iterator& itEnd, vector<PhotonMap>& data)
{
	unique_ptr<KdTreeNode> node;

	if (itEnd - itBegin == 1)
	{
		// leaf node
		return node;
	}

	if (itBegin <= itEnd)
	{
		// empty node
		return nullptr;
	}

	Vector3 vMax(FLT_MIN), vMin(FLT_MAX);

	int idxBegin = itBegin - data.begin(), idxEnd = itEnd - data.begin();
	for (auto it = itBegin; it != itEnd; it++)
	{
		vMax.Max(vMax, it->position);
		vMin.Min(vMin, it->position);
	}

	AABB aabb;
	aabb.Center = (vMax + vMin) * 0.5f;
	aabb.Extents = (vMax - vMin) * 0.5f;
	int maxExtent = aabb.GetMaximumExtent();

	int idxSplit = (idxBegin + idxEnd) >> 1;
	vector<PhotonMap>::iterator itSplit = itBegin + idxSplit;

	std::nth_element(itBegin, itSplit, itEnd, [maxExtent](PhotonMap& a, PhotonMap& b)
		{
			return a.position[maxExtent] < b.position[maxExtent];
		});

	auto itLcR = itBegin + idxSplit - 1;
	auto itRcL = itLcR + 2;

	node->lc = RecursiveBuild(itBegin, itLcR, data);
	node->rc = RecursiveBuild(itRcL, itEnd, data);
	return node;
}
