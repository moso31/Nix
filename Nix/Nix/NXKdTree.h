#pragma once
#include "Header.h"
#include <queue>

template <typename T> using priority_queue_distance_cartesian = std::priority_queue<T, std::vector<T>, std::function<bool(const T& valA, const T& valB)>>;
//template <typename T> using priority_queue_distance_harmonic = std::priority_queue<T, vector<T>, function<bool(const T& valA)>>;

enum LocateFilter
{
	// 按何种方法统计kNN近邻点。
	Disk,
	Sphere
};

template <typename T>
struct NXKdTreeNode
{
	NXKdTreeNode() : lc(nullptr), rc(nullptr) {}
	~NXKdTreeNode() {}

	void Release()
	{
		if (lc) lc->Release();
		if (rc) rc->Release();
		delete this;
	}

	NXKdTreeNode<T>* lc; 
	NXKdTreeNode<T>* rc;
	T data;
	int dim;
};

template <typename T>
class NXKdTree
{
public:
	NXKdTree() {}
	~NXKdTree() {}

	// 创建初始即平衡的kd树。
	void BuildBalanceTree(std::vector<T>& data)
	{
		pRoot = RecursiveBuild(0, data.size(), data);
	}

	NXKdTreeNode<T>* RecursiveBuild(size_t begin, size_t offset, std::vector<T>& data)
	{
		if (offset < 1)
			return nullptr;

		NXKdTreeNode<T>* node;
		if (offset == 1)
		{
			// leaf node
			node = new NXKdTreeNode<T>();
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
		std::nth_element(itLeft, itSplit, itRight, [maxExtent](T& a, T& b)
			{
				return a.position[maxExtent] < b.position[maxExtent];
			});

		node = new NXKdTreeNode<T>();
		node->data = *itSplit;
		node->dim = maxExtent;
		node->lc = RecursiveBuild(begin, mid - begin, data);
		node->rc = RecursiveBuild(mid + 1, begin + offset - mid - 1, data);
		return node;
	}
	
	void Locate(const Vector3& position, const Vector3& normal, NXKdTreeNode<T>* p, int maxLimit, float& out_mindist2, priority_queue_distance_cartesian<T>& out_nearest, LocateFilter locateFilter)
	{
		if (!p) return;

		if (p->lc || p->rc)
		{
			int dim = p->dim;
			float disPlane = position[dim] - p->data.position[dim];
			float disPlane2 = disPlane * disPlane;
			if (disPlane < 0.0f)
			{
				if (p->lc) Locate(position, normal, p->lc, maxLimit, out_mindist2, out_nearest, locateFilter);
				if (disPlane2 < out_mindist2) // 说明另一侧可能有更近点，需要进一步检查另一侧的子树
					if (p->rc) Locate(position, normal, p->rc, maxLimit, out_mindist2, out_nearest, locateFilter);
			}
			else
			{
				if (p->rc) Locate(position, normal, p->rc, maxLimit, out_mindist2, out_nearest, locateFilter);
				if (disPlane2 < out_mindist2) // 说明另一侧可能有更近点，需要进一步检查另一侧的子树
					if (p->lc) Locate(position, normal, p->lc, maxLimit, out_mindist2, out_nearest, locateFilter);
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
			out_nearest.push(p->data);
			if (maxLimit != -1 && out_nearest.size() > maxLimit)
			{
				out_nearest.pop();
				out_mindist2 = Vector3::DistanceSquared(position, out_nearest.top().position);
			}
		}
	}

	void GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_queue_distance_cartesian<T>& out_nearest, int maxLimit = 500, float range = 0.1f, LocateFilter locateFilter = Sphere)
	{
		out_distSqr = range;
		Locate(position, normal, pRoot, maxLimit, out_distSqr, out_nearest, locateFilter);
	}

	void Release()
	{
		SafeRelease(pRoot);
	}

private:
	NXKdTreeNode<T>* pRoot;
	int m_x;
};
