#pragma once
#include "BaseDefs/DX12.h"
#include <unordered_map>
#include "NXTextureDefinitions.h"

struct NXRGHandle
{
	NXRGHandle() : index(-1), version(-1) {}
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

	NXRGHandle GetAncestor()
	{
		return NXRGHandle(index, 0);
	}

	uint16_t index;
	uint16_t version;
	static std::uint16_t s_nextIndex;
	static std::unordered_map<uint16_t, uint16_t> s_maxVersions;
};

struct NXRGLifeTime
{
	int start;  // 资源首次使用的时间层级
	int end;	// 资源最后使用的时间层级
};

enum class NXRGResourceUsage
{
	None,
	RenderTarget, // 用于纹理RT
	DepthStencil, // 用于纹理DS
	ShaderResource, // 用于SRV
	UnorderedAccess, // 用于UAV
};

struct NXRGDescription
{
	// note: import资源不应该使用这个！

	NXResourceType resourceType; // 资源类型
	NXRGResourceUsage usage; // 资源用途

	union // 纹理或buffer参数
	{
		struct
		{
			DXGI_FORMAT format; // 资源格式
			uint32_t width;    // 宽度
			uint32_t height;   // 高度
			uint32_t arraySize; // 数组大小
			uint32_t mipLevels; // MIP等级数
		} tex;
		struct
		{
			uint32_t stride;   // buffer元素大小
			uint32_t arraySize; // buffer元素总数
			// 总byteSize = stride * arraySize
		} buf;
	};

	bool operator==(const NXRGDescription& other) const
	{
		if (resourceType != other.resourceType || usage != other.usage)
			return false;

		if (resourceType == NXResourceType::Tex2D || resourceType == NXResourceType::Tex2DArray || resourceType == NXResourceType::TexCube || resourceType == NXResourceType::Tex3D || resourceType == NXResourceType::Tex1D)
		{
			return tex.format == other.tex.format &&
				tex.width == other.tex.width &&
				tex.height == other.tex.height &&
				tex.arraySize == other.tex.arraySize &&
				tex.mipLevels == other.tex.mipLevels;
		}
		else if (resourceType == NXResourceType::Buffer)
		{
			return buf.stride == other.buf.stride &&
				buf.arraySize == other.buf.arraySize;
		}

		assert(false);
		return false;
	}

	bool operator!=(const NXRGDescription& other) const
	{
		return !(*this == other);
	}
};

// 给map/set的哈希函数
namespace std 
{
	template<class T>
	inline void hash_combine(std::size_t& seed, const T& v) noexcept 
	{
		seed ^= std::hash<T>{}(v)+0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
	}

	template<>
	struct hash<NXRGHandle> 
	{
		size_t operator()(const NXRGHandle& handle) const 
		{
			return (static_cast<size_t>(handle.index) << 16) | handle.version;
		}
	};

	template<>
	struct hash<NXRGDescription> 
	{
		size_t operator()(const NXRGDescription& desc) const noexcept 
		{
			size_t seed = 0;
			hash_combine(seed, static_cast<int>(desc.resourceType));
			hash_combine(seed, static_cast<int>(desc.usage));

			if (desc.resourceType == NXResourceType::Tex2D ||
				desc.resourceType == NXResourceType::Tex2DArray ||
				desc.resourceType == NXResourceType::TexCube ||
				desc.resourceType == NXResourceType::Tex3D)
			{
				// 若 DXGI_FORMAT 的 std::hash 不可用，可改成 static_cast<int>(desc.tex.format)
				hash_combine(seed, static_cast<int>(desc.tex.format));
				hash_combine(seed, desc.tex.width);
				hash_combine(seed, desc.tex.height);
				hash_combine(seed, desc.tex.arraySize);
				hash_combine(seed, desc.tex.mipLevels);
			}
			else if (desc.resourceType == NXResourceType::Buffer)
			{
				hash_combine(seed, desc.buf.stride);
				hash_combine(seed, desc.buf.arraySize);
			}

			return seed;
		}
	};
}
