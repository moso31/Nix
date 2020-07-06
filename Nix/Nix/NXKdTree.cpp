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
	auto itRight = itLeft + (offset - 1);
	auto itSplit = it0 + mid;

	Vector3 vMax(-FLT_MAX), vMin(FLT_MAX);
	for (auto it = itLeft; it <= itRight; it++)
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

void NXKdTree::GetNearest(const Vector3& position, float& out_distSqr, priority_quque_NXPhoton& out_nearestPhotons, float range)
{
	out_distSqr = range;
	Locate(position, pRoot, out_distSqr, out_nearestPhotons);
}

void NXKdTree::Release()
{
	pRoot->Release();
	pRoot.reset();
}

void NXKdTree::Locate(const Vector3& position, const shared_ptr<NXKdTreeNode>& p, float& out_mindistSqr, priority_quque_NXPhoton& out_nearestPhotons)
{
	if (p->lc || p->rc)
	{
		int dim = p->dim;
		float disPlane = position[dim] - p->data.position[dim];
		float disPlaneSqr = disPlane * disPlane;
		if (disPlane < 0.0f)
		{
			if (p->lc) Locate(position, p->lc, out_mindistSqr, out_nearestPhotons);
			if (disPlaneSqr < out_mindistSqr) // 说明另一侧可能有更近点，需要进一步检查另一侧的子树
				if (p->rc) Locate(position, p->rc, out_mindistSqr, out_nearestPhotons);
		}
		else
		{
			if (p->rc) Locate(position, p->rc, out_mindistSqr, out_nearestPhotons);
			if (disPlaneSqr < out_mindistSqr) // 说明另一侧可能有更近点，需要进一步检查另一侧的子树
				if (p->lc) Locate(position, p->lc, out_mindistSqr, out_nearestPhotons);
		}

		float disSqr = Vector3::DistanceSquared(position, p->data.position);
		int sz = 500;
		if (out_nearestPhotons.size() < sz || disSqr < out_mindistSqr)
		{
			out_mindistSqr = disSqr;
			out_nearestPhotons.push(&p->data);
			if (out_nearestPhotons.size() > 500)
				out_nearestPhotons.pop();
		}
	}
}