#pragma once
#include "BaseDefs/Math.h"

template<typename T>
struct NXQuadTreeNode
{
	NXQuadTreeNode(NXQuadTreeNode* pParent) {}

	NXQuadTreeNode* m_pParent = nullptr;
	NXQuadTreeNode* m_pChilds[4] = {};
	T* m_Data;
};

template<typename T>
class NXQuadTree
{
public:
	NXQuadTree() 
	{
		m_pRootNode = new NXQuadTreeNode(nullptr);
	}

	virtual ~NXQuadTree() {}

private:
	NXQuadTreeNode<T>* m_pRootNode;
};
