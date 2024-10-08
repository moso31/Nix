#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXAllocatorManager.h"

template<typename T>
class NXConstantBufferImpl
{
public:
	struct Data
	{
		T* cpuAddress;
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
	};

	typedef ShaderVisibleDescriptorTaskResult ViewData;

public:
	virtual void Create() { assert(false); };
	virtual void Create(uint32_t arraySize) { assert(false); };

	const T& Current()
	{
		return *m_data.Current().cpuAddress;
	}

	void Update(const T& newData)
	{
		T* currentData = m_data.Current().cpuAddress;
		memcpy(currentData, &newData, m_byteSize);
	}

	void Set(const T& newData)
	{
		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			T* currentData = m_data[i].cpuAddress;
			memcpy(currentData, &newData, m_byteSize);
		}
	}

protected:
	void CreateInternal(UINT byteSize)
	{
		byteSize = (byteSize + 255) & ~255;

		m_byteSize = byteSize;
		m_allByteSize = byteSize * MultiFrameSets_swapChainCount;

		for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
		{
			NXAllocator_CB->Alloc(byteSize, [this](const CommittedBufferAllocTaskResult& result) {
				Data& data = m_data[i];
				data.cpuAddress = reinterpret_cast<T*>(result.cpuAddress);
				data.gpuAddress = result.gpuAddress;

				NXAllocator_SRV->Alloc([data, byteSize](const ShaderVisibleDescriptorTaskResult& taskResult) {
					m_cbView = taskResult;
					D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
					cbvDesc.BufferLocation = data.gpuAddress;
					cbvDesc.SizeInBytes = byteSize;
					NXAllocator_CB->GetDevice()->CreateConstantBufferView(&cbvDesc, m_cbView.cpuHandle);
					});
				});
		}
	}

protected:
	MultiFrame<Data> m_data;
	MultiFrame<ViewData> m_cbView;
	UINT m_byteSize;
	UINT m_allByteSize;
};

template<typename T>
class NXConstantBuffer : public NXConstantBufferImpl<T>
{
public:
	void Create() override
	{
		CreateInternal(sizeof(T));
	}
};

template<typename T>
class NXConstantBuffer<std::vector<T>> : public NXConstantBufferImpl<std::vector<T>>
{
public:
	void Create(uint32_t arraySize) override
	{
		CreateInternal(sizeof(T) * arraySize);
	}

	void Set(uint32_t index, const T& newData)
	{
		m_data[index] = newData;
	}
};