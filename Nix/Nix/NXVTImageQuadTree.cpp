#include "NXVTImageQuadTree.h"

void NXVTNodeArray::Init(int capaSize)
{
    m_data.clear();
    m_pos.assign(capaSize, -1);
}

void NXVTNodeArray::Add(int value)
{
    assert(m_pos[value] == -1);
    m_data.push_back(value);
    m_pos[value] = m_data.size() - 1;
}

void NXVTNodeArray::Remove(int value)
{
    assert(m_pos[value] != -1);
    int i = m_pos[value];
    int last = m_data.size() - 1;

    if (i != last)
    {
        int t = m_data[last];
        m_data[i] = t;
        m_pos[t] = i;
    }

    m_pos[value] = -1;
    m_data.pop_back();
}

bool NXVTNodeArray::Empty()
{
    return m_data.empty();
}

int NXVTNodeArray::GetLast()
{
    return m_data.back();
}

NXVTImageQuadTree::NXVTImageQuadTree()
{
    m_state[0].assign(1, NXVTImageNodeState::Free);
    m_freeNodes[0].Init(1);
    m_freeNodes[0].Add(0);
    m_node2sectors[0].push_back(Int2(INT_MIN));
    for (int i = 1; i < VT_IMAGE_LEVELS; i++)
    {
        int nodeCnt = 1 << (2 * i);
        m_freeNodes[i].Init(nodeCnt);
        m_state[i].assign(nodeCnt, NXVTImageNodeState::Unused);
        m_node2sectors[i].assign(nodeCnt, Int2(INT_MIN)); // sectorID可能是负的 用INT_MIN表示无效
    }
}

const Int2& NXVTImageQuadTree::GetSector(const Int2& virtImageMip0Pos, const int virtImgLog2Size)
{
    int lv = VT_IMAGE_LEVELS - 1 - virtImgLog2Size;
    int index = VTImagePosToIndex(virtImageMip0Pos >> virtImgLog2Size);
    return m_node2sectors[lv][index];
}

int NXVTImageQuadTree::SizeToLevel(int size)
{
    assert(size > 0);
    assert((size & (size - 1)) == 0); // 输入size必须是POT的

    int lg = 0;
    while (size > 1) { size >>= 1; lg++; }
    return (VT_IMAGE_LEVELS - 1) - lg;
}

const Int2 NXVTImageQuadTree::Alloc(int size, const Int2& sectorID)
{
    int findLV, findIdx;
    if (!Alloc(size, findLV, findIdx))
    {
        return Int2(-1);
    }

    m_node2sectors[findLV][findIdx] = sectorID; 
    return VTImageIndexToPos(findIdx);
}

void NXVTImageQuadTree::Free(const Int2& pos, int size)
{
    int idx = VTImagePosToIndex(pos);
    int level = SizeToLevel(size);
    Free(level, idx);

    m_node2sectors[level][idx] = Int2(INT_MIN);
}

bool NXVTImageQuadTree::Alloc(int size, int& findLV, int& findIdx)
{
    int targetLV = SizeToLevel(size);
    if (FindFreeLevel(targetLV, findLV, findIdx))
    {
        int idx = findIdx;
        for (int i = findLV; i < targetLV; i++)
        {
            m_state[i][idx] = NXVTImageNodeState::Split;
            m_freeNodes[i].Remove(idx);
            idx <<= 2;
            for (int j = idx; j < idx + 4; j++)
            {
                m_state[i + 1][j] = NXVTImageNodeState::Free;
                m_freeNodes[i + 1].Add(j);
            }
        }

        m_state[targetLV][idx] = NXVTImageNodeState::Allocated;
        m_freeNodes[targetLV].Remove(idx);
        findLV = targetLV;
        findIdx = idx;
        return true;
    }

    return false;
}

void NXVTImageQuadTree::Free(int freeLV, int freeIdx)
{
    assert(m_state[freeLV][freeIdx] == NXVTImageNodeState::Allocated);
    m_state[freeLV][freeIdx] = NXVTImageNodeState::Free;
    m_freeNodes[freeLV].Add(freeIdx);

    int lv = freeLV;
    int idx = freeIdx;
    while (lv > 0)
    {
        idx = idx & ~0x3;
        bool allFree = true;
        for (int i = idx; i < idx + 4; i++)
        {
            if (m_state[lv][i] != NXVTImageNodeState::Free)
            {
                allFree = false;
                break;
            }
        }

        if (!allFree) break;

        for (int i = idx; i < idx + 4; i++)
        {
            m_state[lv][i] = NXVTImageNodeState::Unused;
            m_freeNodes[lv].Remove(i);
        }

        idx >>= 2;
        assert(m_state[lv - 1][idx] == NXVTImageNodeState::Split);
        m_state[lv - 1][idx] = NXVTImageNodeState::Free;
        m_freeNodes[lv - 1].Add(idx);

        lv--;
    }
}

bool NXVTImageQuadTree::FindFreeLevel(int targetLV, int& findLV, int& findIdx)
{
    for (int i = targetLV; i >= 0; i--)
    {
        if (m_freeNodes[i].Empty()) continue;

        findLV = i;
        findIdx = m_freeNodes[i].GetLast();
        return true;
    }

    return false;
}
