#pragma once
#include "BaseDefs/Math.h"

struct NXQuadTreeNode
{
	NXQuadTreeNode(NXQuadTreeNode* pParent) :
		m_pParent(pParent),
		m_pChilds{ nullptr, nullptr, nullptr, nullptr }
	{
	}

	NXQuadTreeNode* m_pParent;
	NXQuadTreeNode* m_pChilds[4]; // 0=左下, 1=右下, 2=左上, 3=右上，和Nix坐标系的XZ正方向一致
};

class NXQuadTree
{
public:
	NXQuadTree(const Vector2& regionCenter, const Vector2& regionSize, int depth) :
		m_regionCenter(regionCenter),
		m_regionSize(regionSize),
		m_depth(depth)
	{
		m_pRootNode = new NXQuadTreeNode(nullptr);
	}

	virtual ~NXQuadTree() {}

	void AddChilds(NXQuadTreeNode* pNode)
	{
		for (int i = 0; i < 4; i++) if (pNode->m_pChilds[i]) return;
		for (int i = 0; i < 4; i++)
		{
			pNode->m_pChilds[i] = new NXQuadTreeNode(pNode);
		}
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

	// get对应位置的四叉树节点
	NXQuadTreeNode* Get(const Vector2& coord)
	{
		Get_Internal(m_pRootNode, coord, m_regionCenter, m_regionSize);
	}

private:
	NXQuadTreeNode* Get_Internal(NXQuadTreeNode* pNode, const Vector2& coord, const Vector2& nodeCenter, const Vector2& nodeSize)
	{
		Vector2 childSize = nodeSize * Vector2(0.5f, 0.5f);
		if (coord.x < nodeCenter.x && coord.y < nodeCenter.y) // 左下
		{
			Vector2 childCenter(nodeCenter.x - childSize.x, nodeCenter.y - childSize.y);
			if (pNode->m_pChilds[0]) 
				return Get_Internal(pNode->m_pChilds[0], coord, childCenter, childSize);
		}
		else if (coord.x >= nodeCenter.x && coord.y < nodeCenter.y) // 右下
		{
			Vector2 childCenter(nodeCenter.x + childSize.x, nodeCenter.y - childSize.y);
			if (pNode->m_pChilds[1]) 
				return Get_Internal(pNode->m_pChilds[1], coord, childCenter, childSize);
		}
		else if (coord.x < nodeCenter.x && coord.y >= nodeCenter.y) // 左上
		{
			Vector2 childCenter(nodeCenter.x - childSize.x, nodeCenter.y + childSize.y);
			if (pNode->m_pChilds[2]) 
				return Get_Internal(pNode->m_pChilds[2], coord, childCenter, childSize);
		}
		else // 右上
		{
			Vector2 childCenter(nodeCenter.x + childSize.x, nodeCenter.y + childSize.y);
			if (pNode->m_pChilds[3]) 
				return Get_Internal(pNode->m_pChilds[3], coord, childCenter, childSize);
		}

		return pNode;
	}

private:
	Vector2 m_regionCenter; // 四叉树中心点
	Vector2 m_regionSize; // 四叉树区域大小
	int m_depth;

	NXQuadTreeNode* m_pRootNode;
};
