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
	Vector3 vMax(FLT_MIN), vMin(FLT_MAX);

	vector<PhotonMap>::iterator itBegin = data.begin();
	vector<PhotonMap>::iterator itEnd = data.end();
	int idxBegin = 0, idxEnd = data.size();
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

	RecursiveBuildBalanceTree(itBegin, itLcR);
	RecursiveBuildBalanceTree(itRcL, itEnd);
}
