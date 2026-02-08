#include "NXNullDescriptor.h"
#include "NXGlobalDefinitions.h"

NXNullDescriptor::NXNullDescriptor()
{
	m_pNullAllocator = std::make_unique<DescriptorAllocator<false>>(NXGlobalDX::GetDevice(), 100, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// m_nullUAV
	m_pNullAllocator->Alloc([&](const D3D12_CPU_DESCRIPTOR_HANDLE& res) {
		m_nullUAV = res;
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = 0;
		uavDesc.Buffer.StructureByteStride = 1;
		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, m_nullUAV);
		});

	// m_nullSRV
	m_pNullAllocator->Alloc([&](const D3D12_CPU_DESCRIPTOR_HANDLE& res) {
		m_nullSRV = res;
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = 0;
		srvDesc.Buffer.StructureByteStride = 1;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		NXGlobalDX::GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, m_nullSRV);
		});
}

void NXNullDescriptor::ExecuteTasks() // 注：这个方法目前约定是在主线程仅调用一次的。
{
	m_pNullAllocator->ExecuteTasks();
}
