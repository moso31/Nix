#include "NXKdTree.h"

NXKdTreeNode::NXKdTreeNode() :
	lc(nullptr),
	rc(nullptr)
{
}

void NXKdTreeNode::Release()
{
	if (lc)
	{
		lc->Release();
		lc.reset();
	}
	if (rc)
	{
		rc->Release();
		rc.reset();
	}
}

NXKdTree::NXKdTree()
{
}

NXKdTree::~NXKdTree()
{
}

void NXKdTree::BuildBalanceTree(vector<NXPhoton>& data)
{
	pRoot = RecursiveBuild(0, data.size(), data);
}

shared_ptr<NXKdTreeNode> NXKdTree::RecursiveBuild(size_t begin, size_t offset, vector<NXPhoton>& data)
{
	if (offset < 1)
		return nullptr;

	unique_ptr<NXKdTreeNode> node;
	if (offset == 1)
	{
		// leaf node
		node = make_unique<NXKdTreeNode>();
		node->data = data[begin];
		node->dim = -1;
		node->lc = nullptr;
		node->rc = nullptr;
		return node;
	} 

	size_t mid = begin + ((offset - 1) >> 1);
	auto it0 = data.begin();
	auto itLeft = it0 + begin;
	auto itRight = itLeft + offset;
	auto itSplit = it0 + mid;

	Vector3 vMax(-FLT_MAX), vMin(FLT_MAX);
	for (auto it = itLeft; it < itRight; it++)
	{
		vMax = Vector3::Max(vMax, it->position);
		vMin = Vector3::Min(vMin, it->position);
	}

	AABB aabb;
	aabb.Center = (vMax + vMin) * 0.5f;
	aabb.Extents = (vMax - vMin) * 0.5f;
	int maxExtent = aabb.GetMaximumExtent();
	std::nth_element(itLeft, itSplit, itRight, [maxExtent](NXPhoton& a, NXPhoton& b)
		{
			return a.position[maxExtent] < b.position[maxExtent];
		});

	node = make_unique<NXKdTreeNode>();
	node->data = *itSplit;
	node->dim = maxExtent;
	node->lc = RecursiveBuild(begin, mid - begin, data);
	node->rc = RecursiveBuild(mid + 1, begin + offset - mid - 1, data);
	return node;
}

void NXKdTree::GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_quque_NXPhoton& out_nearestPhotons, int maxLimit, float range, LocateFilter locateFilter)
{
	out_distSqr = range;
	Locate(position, normal, pRoot, maxLimit, out_distSqr, out_nearestPhotons, locateFilter);
}

void NXKdTree::Release()
{
	pRoot->Release();
	pRoot.reset();
}

void NXKdTree::Locate(const Vector3& position, const Vector3& normal, const shared_ptr<NXKdTreeNode>& p, int maxLimit, float& out_mindist2, priority_quque_NXPhoton& out_nearestPhotons, LocateFilter locateFilter)
{
	if (p->lc || p->rc)
	{
		int dim = p->dim;
		float disPlane = position[dim] - p->data.position[dim];
		float disPlane2 = disPlane * disPlane;
		if (disPlane < 0.0f)
		{
			if (p->lc) Locate(position, normal, p->lc, maxLimit, out_mindist2, out_nearestPhotons, locateFilter);
			if (disPlane2 < out_mindist2) // 说明另一侧可能有更近点，需要进一步检查另一侧的子树
				if (p->rc) Locate(position, normal, p->rc, maxLimit, out_mindist2, out_nearestPhotons, locateFilter);
		}
		else
		{
			if (p->rc) Locate(position, normal, p->rc, maxLimit, out_mindist2, out_nearestPhotons, locateFilter);
			if (disPlane2 < out_mindist2) // 说明另一侧可能有更近点，需要进一步检查另一侧的子树
				if (p->lc) Locate(position, normal, p->lc, maxLimit, out_mindist2, out_nearestPhotons, locateFilter);
		}
	}

	Vector3 nearestDir = p->data.position - position;
	if (locateFilter == Disk)
	{
		if (nearestDir.Dot(normal) > 1e-4f) return;
	}

	float dist2 = nearestDir.LengthSquared();
	if (dist2 < out_mindist2)
	{
		out_nearestPhotons.push(&p->data);
		if (maxLimit != -1 && out_nearestPhotons.size() > maxLimit)
		{
			out_nearestPhotons.pop();
			out_mindist2 = Vector3::DistanceSquared(position, out_nearestPhotons.top()->position);
		}
	}
}