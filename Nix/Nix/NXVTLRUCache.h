#pragma once
#include <unordered_map>
#include <vector>
#include <list>
#include "NXVirtualTextureCommon.h"

/* 
2026.1.30 为方便日后梳理，用个具体例子说明下lru cache是怎么运作的：

以一个capacity=5的情况为例，假设
初始：
umap: [a]=0 [b]=1 [c]=2 [d]=3 [e]=4
vector: [0]=a [1]=b [2]=c [3]=d [4]=e
lru(list): 0->1->2->3->4

接下来示例touch和insert两种情况。

touch(d)：
umap找到d，所以不需要移除，但[d]=3
umap: [a]=0 [b]=1 [c]=2 [d]=3 [e]=4
vector: [0]=a [1]=b [2]=c [3]=d [4]=e
lru(list)：3->0->1->2->4

insert(f)：
umap找不到，需要移除
确定末尾：lru(list)=4
umap.remove(vector[4])
lru更新：4->3->0->1->2
vector[4]=f
umap[f]=4;

最终：
umap: [a]=0 [b]=1 [c]=2 [d]=3 [f]=4
vector: [0]=a [1]=b [2]=c [3]=d [4]=f
lru(list)维护：4->3->0->1->2
*/

class NXVTLRUCache
{
public:
	NXVTLRUCache(int capacity) 
	{
		m_vector.resize(capacity);
		m_slot2ListIt.resize(capacity);
		uint64_t invalidValue = UINT64_MAX;
		for (uint64_t i = 0; i < capacity; i++)
		{
			uint64_t lruKey = invalidValue - i;
			m_umap[lruKey] = i;
			m_vector[i] = lruKey;

			auto it = m_lruList.insert(m_lruList.end(), i); // 末尾插入 slot，并拿到迭代器
			m_slot2ListIt[i] = it;
		}
	}

	~NXVTLRUCache() {}

	bool Find(uint64_t key)
	{
		return m_umap.contains(key);
	}

	size_t Touch(uint64_t key)
	{
		size_t slot = m_umap[key];
		m_lruList.splice(m_lruList.begin(), m_lruList, m_slot2ListIt[slot]);
		return slot;
	}

	size_t Insert(uint64_t key)
	{
		size_t lastSlot = m_lruList.back();
		m_umap.erase(m_vector[lastSlot]);

		m_vector[lastSlot] = key;
		m_umap[key] = lastSlot;
		m_lruList.splice(m_lruList.begin(), m_lruList, m_slot2ListIt[lastSlot]);

		return lastSlot;
	}

private:
	std::unordered_map<uint64_t, size_t> m_umap;
	std::vector<uint64_t> m_vector;
	std::list<size_t> m_lruList;
	std::vector<std::list<size_t>::iterator> m_slot2ListIt;
};
