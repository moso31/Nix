#pragma once
#include "BaseDefs/DX12.h"
#include <unordered_map>

struct NXRGHandle
{
	NXRGHandle(uint16_t idx, uint16_t ver = 0) : index(idx), version(ver) {}

	static void Reset() 
	{
		s_nextIndex = 0;
		s_maxVersions.clear();
	}

	bool operator==(const NXRGHandle& other) const 
	{
		return index == other.index && version == other.version;
	}
	
	uint16_t index;
	uint16_t version;
	static std::uint16_t s_nextIndex;
	static std::unordered_map<uint16_t, uint16_t> s_maxVersions;
};

// 为 std::unordered_map 提供哈希函数
namespace std {
	template<>
	struct hash<NXRGHandle> {
		size_t operator()(const NXRGHandle& handle) const {
			return (static_cast<size_t>(handle.index) << 16) | handle.version;
		}
	};
}

extern uint32_t g_nextRGHandleIndex;

enum NXRGHandleFlags
{
	RG_None = 0,
	RG_RenderTarget = 1 << 0,
	RG_DepthStencil = 1 << 1
};
