#pragma once 
#include "NXVirtualTextureCommon.h"

enum class NXVTImageNodeState
{
    Unused, // 未分配
    Allocated, // 已分配
    Split, // 已被拆散（仅父节点）
    Free, // 已被标记但尚未分配
};

class NXVTNodeArray
{
public:
    void Init(int capaSize);
    void Add(int value);
    void Remove(int value);
    bool Empty();
    int GetLast();

private:
    std::vector<int> m_data;
    std::vector<int> m_pos;
};

class NXVTImageQuadTree
{
public:
    constexpr static int VT_IMAGE_LEVELS = 12; // 2048x2048
    constexpr static int VT_IMAGE_SIZE = 2048; // 根节点大小

    NXVTImageQuadTree();

    // GUI可视化访问接口
    int GetLevelCount() const { return VT_IMAGE_LEVELS; }
    int GetImageSize() const { return VT_IMAGE_SIZE; }
    const std::vector<NXVTImageNodeState>& GetStateAtLevel(int level) const { return m_state[level]; }

    // 输入node在四叉树中的角坐标位置/大小，返回对应的Sector
    const Int2& GetSector(const Int2& virtImageMip0Pos, const int virtImgLog2Size);

    int SizeToLevel(int size); // 2048 = 0, 1 = 11

    // 返回的Int2 * 此节点的size(2048, 1024, ...) = 实际像素坐标
    const Int2 Alloc(int size, const Int2& sectorID);

    void Free(const Int2& pos, int size);

private:
    // 尝试在四叉树里分配一个node
    // findLV表示返回的分配等级
    // findIdx相当于二维pos的mortoncode
    bool Alloc(int size, int& findLV, int& findIdx);
    void Free(int freeLV, int freeIdx);
    bool FindFreeLevel(int targetLV, int& findLV, int& findIdx);

private:
    std::vector<NXVTImageNodeState> m_state[VT_IMAGE_LEVELS];
    std::vector<Int2> m_node2sectors[VT_IMAGE_LEVELS]; // 每个节点记一下自己的sectors
    NXVTNodeArray m_freeNodes[VT_IMAGE_LEVELS];
};
