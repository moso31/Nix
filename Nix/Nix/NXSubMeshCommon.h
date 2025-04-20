#pragma once
#include <string>
#include <span>
#include <atomic>
#include <future>
#include "BaseDefs/DX12.h"

template<typename T>
class NXSubMesh;

enum NXSubMeshReloadState
{
	None,		// 正常状态
	Start,		// A->Default 状态
	Replacing,  // Default->B 状态
	Finish,		// B 状态
};


enum class NXMeshViewType
{
	VERTEX, // vbv
	INDEX, // ibv
};

struct NXRawMeshView
{
	NXRawMeshView(const std::span<const std::byte>& span, uint32_t stride, NXMeshViewType GPUDataType) :
		span(span), stride(stride), GPUDataType(GPUDataType) {}

	// 这里只记录视图，实体存在NXSubMeshBase
	std::span<const std::byte> span; 
	uint32_t stride; 

	// 记录view的类型是vbv还是ibv（鬼知道为啥DX12非得给这bufferview做区分。。）
	NXMeshViewType GPUDataType; 

	// 对应的实际资源gpu地址
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress; 
};

class NXMeshViews
{
public:
	NXMeshViews(const std::string& name, const std::vector<NXRawMeshView>& views) :
		m_name(name),
		m_views(views),
		m_indexCount(0),
		m_pending((int)views.size()),
		m_loadFuture(m_loadPromise.get_future())
	{
		// 记录索引总数量
		for (auto view : m_views)
		{
			if (view.GPUDataType == NXMeshViewType::INDEX)
				m_indexCount += view.span.size_bytes() / view.stride;
		}
	}

	void ProcessOne()
	{
		if (--m_pending == 0)
		{
			m_loadPromise.set_value();
		}
	}

	void WaitLoadComplete() const
	{
		m_loadFuture.wait();
	}

	bool GetVBV(int index, D3D12_VERTEX_BUFFER_VIEW& out) const
	{
		if (index >= m_views.size() || m_views[index].GPUDataType != NXMeshViewType::VERTEX)
			return false;

		WaitLoadComplete();
		auto& view = m_views[index];
		out.BufferLocation = view.gpuAddress;
		out.StrideInBytes = view.stride;
		out.SizeInBytes = (uint32_t)view.span.size_bytes();
		return true;
	}

	bool GetIBV(int index, D3D12_INDEX_BUFFER_VIEW& out) const
	{
		if (index >= m_views.size() || m_views[index].GPUDataType != NXMeshViewType::INDEX)
			return false;

		WaitLoadComplete();
		auto& view = m_views[index];
		out.BufferLocation = view.gpuAddress;
		out.Format = DXGI_FORMAT_R32_UINT; // 目前只支持R32_UINT，将来有16的需求再说
		out.SizeInBytes = (uint32_t)view.span.size_bytes();
		return true;
	}

	uint32_t GetIndexCount() const { return m_indexCount; }

private:
	std::string m_name;
	const std::vector<NXRawMeshView>& m_views; 
	std::atomic<int> m_pending;

	uint32_t m_indexCount;
	std::promise<void> m_loadPromise;
	std::future<void> m_loadFuture;
};

enum NXPlaneAxis
{
	POSITIVE_X,
	POSITIVE_Y,
	POSITIVE_Z,
	NEGATIVE_X,
	NEGATIVE_Y,
	NEGATIVE_Z
};
