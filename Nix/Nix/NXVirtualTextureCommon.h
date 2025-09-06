#pragma once
#include "BaseDefs/Math.h"
#include <bit>

struct NXSectorInfo
{
	Int2 position;
	int size; // virtual Image atlas �� size

	bool operator==(const NXSectorInfo& o) const noexcept {
		return position == o.position && size == o.size;
	}
};

// NXSectorInfo �Ĺ�ϣ��
// ������ֻ��32λ position��12λ �����ݵ�0-4095 ���Ը�������0-2047�ĵ��ηֱ��������ˣ�size��4λ����
template<>
struct std::hash<NXSectorInfo> {
	size_t operator()(const NXSectorInfo& v) const noexcept 
	{ 
		return (v.position.x << 20) ^ (v.position.y << 4) ^ std::countr_zero((uint32_t)v.size); // countr_zero = λ��-1 = log2 size
	}
};

// Int2 ����VT���ػ���ϣ�� 
struct Int2VTHasher {
	size_t operator()(const Int2& v) const noexcept
	{
		return (v.x << 16) ^ (v.y);
	}
};

struct NXVTAtlasQuadTreeNode
{
	NXVTAtlasQuadTreeNode* InsertImage(const NXSectorInfo& info)
	{
		int newImgSize = info.size;

		if (newImgSize > size) return nullptr;
		if (isImage) return nullptr;

		if (size == newImgSize)
		{
			// ����ͼ�񣬲���û����ͼ����ô���Է���
			if (!isImage && subImageNum == 0)
			{
				isImage = true;
				position = info.position;
				size = info.size;

				auto* p = pParent;
				while (p)
				{
					p->subImageNum++;
					p = p->pParent;
				}
				return this;
			}
			else return nullptr; // ���������������
		}

		// �����ǰ�ڵ㲻�У��ݹ��ӽڵ㿴���ܲ����ҵ��ɷ�������
		// �������Ѿ�������ڴ��
		for (int i = 0; i < 4; i++)
		{
			if (pChilds[i])
			{
				NXVTAtlasQuadTreeNode* p = pChilds[i]->InsertImage(info);
				if (p)
				{
					return p;
				}
			}
		}

		// ����ѷ���ռ���û�к��ʵĿɷ������򣬾͵�new���ڴ�
		for (int i = 0; i < 4; i++)
		{
			if (!pChilds[i])
			{
				NXVTAtlasQuadTreeNode* p = new NXVTAtlasQuadTreeNode();
				p->size = size / 2;
				p->pParent = this;
				p->nodeID = nodeID * 4 + i + 1;
				pChilds[i] = p;

				return p->InsertImage(info);
			}
		}

		return nullptr;
	}

	NXVTAtlasQuadTreeNode* pParent = nullptr;
	NXVTAtlasQuadTreeNode* pChilds[4] = { nullptr , nullptr , nullptr , nullptr };
	Int2 position = Int2(-1, -1);
	int size = -1;

	int nodeID = -1;
	int isImage = false;
	int subImageNum = 0;
};

class NXVTAtlasQuadTree
{
	static constexpr int NXVT_VIRTUALIMAGE_NODE_SIZE = 2048;

public:
	NXVTAtlasQuadTree()
	{
		pRoot = new NXVTAtlasQuadTreeNode();
		pRoot->size = NXVT_VIRTUALIMAGE_NODE_SIZE;
		pRoot->nodeID = 0;
	}

	NXVTAtlasQuadTreeNode* GetRootNode() { return pRoot; }
	
	NXVTAtlasQuadTreeNode* InsertImage(const NXSectorInfo& info)
	{
		return pRoot->InsertImage(info);
	}

	void RemoveImage(NXVTAtlasQuadTreeNode* pNode)
	{
		// ����ͼ����ɾ
		if (!pNode || !pNode->isImage)
			return;

		// �ȸ�����ǲ����������������pNode
		auto* p = pNode;
		while (p->pParent) p = p->pParent;
		if (p != pRoot)
			return;

		bool bFlag = false;
		auto* pRemove = pNode;
		for (auto* p = pNode->pParent; p; p = p->pParent)
		{
			p->subImageNum--;
			if (p->subImageNum == 0 && !bFlag)
				pRemove = p;
			else bFlag = true;

			assert(p->subImageNum >= 0);
		}

		// �����˲����Ƴ�pRoot����ΪpRootû�и��ڵ�
		if (pRemove->pParent)
		{
			int childIdx = (pRoot == pRemove->pParent ? 
				pRemove->nodeID : 
				pRemove->nodeID - pRemove->pParent->nodeID * 4) - 1;
			pRemove->pParent->pChilds[childIdx] = nullptr;
		}

		RemoveSubTree(pRemove);
	}

	void Release()
	{
		RemoveSubTree(pRoot);
		delete pRoot;
		pRoot = nullptr;
	}

private:
	void RemoveSubTree(NXVTAtlasQuadTreeNode* pNode)
	{
		for (int i = 0; i < 4; i++)
		{
			if (pNode->pChilds[i])
			{
				RemoveSubTree(pNode->pChilds[i]);
				pNode->pChilds[i] = nullptr;
			}
		}

		// ���ڵ㲻��ɾ ������һ��״̬
		if (pNode == pRoot)
		{
			pNode->isImage = false;
			pNode->subImageNum = 0;
		}
		else
		{
			delete pNode;
			pNode = nullptr;
		}
	}

private:
	NXVTAtlasQuadTreeNode* pRoot;
};

class NXVirtualTextureAtlas
{
public:
	NXVirtualTextureAtlas() :
		m_qtree(new NXVTAtlasQuadTree())
	{
	}
	~NXVirtualTextureAtlas() { Release(); }

	void InsertImage(const NXSectorInfo& info)
	{
		auto* pNode = m_qtree->InsertImage(info);
		if (pNode) m_allocatedNodes.push_back(pNode);
	}

	void RemoveImage(const NXSectorInfo& info)
	{
		auto secIt = std::find_if(m_allocatedNodes.begin(), m_allocatedNodes.end(), [&info](const NXVTAtlasQuadTreeNode* node) {
			return info.position == node->position && info.size == node->size; });

		if (secIt != m_allocatedNodes.end())
		{
			m_qtree->RemoveImage(*secIt);
			m_allocatedNodes.erase(secIt);
		}
	}

	void Release()
	{
		if (m_qtree)
		{
			m_qtree->Release();
			delete m_qtree;
			m_qtree = nullptr;
		}

		m_allocatedNodes.clear();
	}

	const std::vector<NXVTAtlasQuadTreeNode*>& GetNodes() { return m_allocatedNodes; }
	NXVTAtlasQuadTreeNode* GetRootNode() { return m_qtree->GetRootNode(); }

private:
	NXVTAtlasQuadTree* m_qtree;
	std::vector<NXVTAtlasQuadTreeNode*> m_allocatedNodes;
};
