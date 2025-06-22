#pragma once
#include "BaseDefs/Math.h"

struct NXQuadTreeNode
{
	NXQuadTreeNode(NXQuadTreeNode* pParent) :
		m_pParent(pParent),
		m_pChilds{ nullptr, nullptr, nullptr, nullptr }
	{
	}

	AABB m_aabb; // 节点的包围盒
	NXQuadTreeNode* m_pParent;
	NXQuadTreeNode* m_pChilds[4]; // 0=左下, 1=右下, 2=左上, 3=右上，和Nix坐标系的XZ正方向一致
};

class NXQuadTree
{
public:
	NXQuadTree(const Vector3& pos, const Vector3& size) :
		m_pos(pos), m_size(size)
	{
		m_pRootNode = new NXQuadTreeNode(nullptr);
	}

	virtual ~NXQuadTree() {}

	// 构建四叉树
	void Build(int depth)
	{
		m_pRootNode->m_aabb.Center = m_pos;
		m_pRootNode->m_aabb.Extents = m_size;
		m_maxDepth = depth;
		
		BuildNode(m_pRootNode, 0);
	}

	// 释放四叉树
	void Destroy()
	{
		RemoveChilds(m_pRootNode);
	}

	// 获取一个节点基于距离的覆盖情况
	// return 0: 完全不相交 1：部分覆盖 2：完全覆盖
	int GetOverlapState(const Vector3& pos, const float radius, const NXQuadTreeNode* pNode)
	{
		// 目前暂时用不到Y轴
		Vector3 pMax = pNode->m_aabb.GetMax();
		Vector3 pMin = pNode->m_aabb.GetMin();
		float clampX = Clamp(pos.x, pMin.x, pMax.x);
		float clampZ = Clamp(pos.z, pMin.z, pMax.z);
		float dx = clampX - pos.x;
		float dz = clampZ - pos.z;
		float r2 = radius * radius;

		if (dx * dx + dz * dz > r2)
			return 0;

		float cornerX[4] = { pMin.x - pos.x, pMin.x - pos.x, pMax.x - pos.x, pMax.x - pos.x };
		float cornerZ[4] = { pMin.z - pos.z, pMax.z - pos.z, pMax.z - pos.z, pMin.z - pos.z };
		for (int i = 0; i < 4; i++)
		{
			if (cornerX[i] * cornerX[i] + cornerZ[i] * cornerZ[i] > r2)
				return 1;
		}

		return 2;
	}

private:
	void BuildNode(NXQuadTreeNode* pNode, int depth)
	{
		if (depth == m_maxDepth - 1) return;

		Vector3 halfSize = pNode->m_aabb.Extents * 0.5;
		Vector3 O = pNode->m_aabb.Center;
		Vector3 A = pNode->m_aabb.GetMin();
		Vector3 C = pNode->m_aabb.GetMax();
		// 目前暂时用不到Y轴
		O.y = A.y = C.y = 0.0f;
		Vector3 B(A.x, 0.0, C.z);
		Vector3 D(C.x, 0.0, A.z);

		// 1--2   B--C
		// |  | = |  |
		// 0--3   A--D
		auto pChild = new NXQuadTreeNode(pNode);
		pChild->m_aabb.Center = (A + O) * 0.5;
		pChild->m_aabb.Extents = halfSize;
		pNode->m_pChilds[0] = pChild;

		pChild = new NXQuadTreeNode(pNode);
		pChild->m_aabb.Center = (B + O) * 0.5;
		pChild->m_aabb.Extents = halfSize;
		pNode->m_pChilds[1] = pChild;

		pChild = new NXQuadTreeNode(pNode);
		pChild->m_aabb.Center = (C + O) * 0.5;
		pChild->m_aabb.Extents = halfSize;
		pNode->m_pChilds[2] = pChild;

		pChild = new NXQuadTreeNode(pNode);
		pChild->m_aabb.Center = (D + O) * 0.5;
		pChild->m_aabb.Extents = halfSize;
		pNode->m_pChilds[3] = pChild;

		BuildNode(pNode->m_pChilds[0], depth + 1);
		BuildNode(pNode->m_pChilds[1], depth + 1);
		BuildNode(pNode->m_pChilds[2], depth + 1);
		BuildNode(pNode->m_pChilds[3], depth + 1);
	}

	void RemoveChilds(NXQuadTreeNode* pNode)
	{
		for (int i = 0; i < 4; i++)
		{
			if (pNode->m_pChilds[i])
			{
				RemoveChilds(pNode->m_pChilds[i]);
				delete pNode->m_pChilds[i];
				pNode->m_pChilds[i] = nullptr;
			}
		}
	}

private:
	Vector3 m_pos;
	Vector3 m_size;
	int m_maxDepth;

	NXQuadTreeNode* m_pRootNode;
};
