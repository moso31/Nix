#pragma once
#include <bitset>
#include "NXVirtualTextureCommon.h"

struct NXQuadTreeNodeAtlas
{
	NXQuadTreeNodeAtlas* parent = nullptr; 
	NXQuadTreeNodeAtlas* childs[4] = {};
	bool isImage = false; // 当前节点是否是图像
	int subImages = 0; // 当前节点下有多少子图像
	Int2 data = {};
};

class NXQuadTreeAtlas
{
public:
	NXQuadTreeAtlas(int atlasSize) : 
		m_atlasSize(atlasSize)
	{
		m_rootNode = new NXQuadTreeNodeAtlas();
		int levelCount = std::log2(atlasSize) + 1;
	}

	void Insert(int size)
	{
		int depth = m_atlasSize / size - 1;
	}

private:
	int m_atlasSize;
	NXQuadTreeNodeAtlas* m_rootNode = nullptr; // 根节点
};

class NXVirtualImageAtlas
{
	const static int VIRTUALIMAGE_ATLAS_SIZE = 2048; // Atlas的大小

public:
	NXVirtualImageAtlas()
	{
	}

	void InsertImage(NXSectorInfo& sectorInfo, int virtImgSize)
	{

	}

	void RemoveImage();

private:
};
