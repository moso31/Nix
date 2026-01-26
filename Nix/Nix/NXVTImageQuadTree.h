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
    void Init(int capaSize)
    {
        m_data.clear();
        m_pos.assign(capaSize, -1);
    }

    void Add(int value)
    {
        assert(m_pos[value] == -1);
        m_data.push_back(value);
        m_pos[value] = m_data.size() - 1;
    }

    void Remove(int value)
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

    bool Empty()
    {
        return m_data.empty();
    }

    int GetLast()
    {
        return m_data.back();
    }

private:
    std::vector<int> m_data;
    std::vector<int> m_pos;
};

class NXVTImageQuadTree
{
public:
    constexpr static int VT_IMAGE_LEVELS = 12; // 2048x2048
    constexpr static int VT_IMAGE_SIZE = 2048; // 根节点大小

    NXVTImageQuadTree()
    {
        m_state[0].assign(1, NXVTImageNodeState::Free);
        m_freeNodes[0].Init(1);
        m_freeNodes[0].Add(0);
        for (int i = 1; i < VT_IMAGE_LEVELS; i++)
        {
            int nodeCnt = 1 << (2 * i);
            m_freeNodes[i].Init(nodeCnt);
            m_state[i].assign(nodeCnt, NXVTImageNodeState::Unused);
        }
    }

    // GUI可视化访问接口
    int GetLevelCount() const { return VT_IMAGE_LEVELS; }
    int GetImageSize() const { return VT_IMAGE_SIZE; }
    const std::vector<NXVTImageNodeState>& GetStateAtLevel(int level) const { return m_state[level]; }

    int SizeToLevel(int size) // 2048 = 0, 1 = 11
    {
        assert(size > 0);
        assert((size & (size - 1)) == 0); // 输入size必须是POT的

        int lg = 0;
        while (size > 1) { size >>= 1; lg++; }
        return (VT_IMAGE_LEVELS - 1) - lg;
    }

    // 返回的Int2 * 此节点的size(2048, 1024, ...) = 实际像素坐标
    const Int2 Alloc(int size)
    {
        int findLV, findIdx;
        if (!Alloc(size, findLV, findIdx))
            return Int2(-1);

        return VTImageIndexToPos(findIdx);
    }

    void Free(const Int2& pos, int size)
    {
        int idx = VTImagePosToIndex(pos);
        int level = SizeToLevel(size);
        Free(level, idx);
    }

private:
    bool Alloc(int size, int& findLV, int& findIdx)
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

    void Free(int freeLV, int freeIdx)
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

    bool FindFreeLevel(int targetLV, int& findLV, int& findIdx)
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

private:
    std::vector<NXVTImageNodeState> m_state[VT_IMAGE_LEVELS];
    NXVTNodeArray m_freeNodes[VT_IMAGE_LEVELS];
};
