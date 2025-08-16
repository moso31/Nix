#include "NXRenderPass.h"
#include "NXSamplerManager.h"

NXRenderPass::NXRenderPass(NXRenderPassType type) :
	NXRGPass(type),
	m_entryNameVS(L"VS"),
	m_entryNamePS(L"PS"),
	m_entryNameCS(L"CS")
{
}

void NXRenderPass::SetRootParamLayout(const NXRGRootParamLayout& layout)
{
	m_rootParamLayout = layout;

	m_srvRanges.clear();
	m_uavRanges.clear();
	m_rootParams.clear();
	for (int i = 0; i < m_rootParamLayout.cbvCount; i++)
	{
		// 默认slotIndex = i，可以通过 SetStaticRootParamCBV(, slotIdx, ) 方法修改
		m_rootParams.push_back(NX12Util::CreateRootParameterCBV(i, 0, D3D12_SHADER_VISIBILITY_ALL));
	};

	if (m_rootParamLayout.srvCount)
	{
		m_srvRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_rootParamLayout.srvCount, 0, 0));
		m_rootParams.push_back(NX12Util::CreateRootParameterTable(m_srvRanges, D3D12_SHADER_VISIBILITY_ALL));
	}

	if (m_rootParamLayout.uavCount)
	{
		m_uavRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, m_rootParamLayout.uavCount, 0, 0));
		m_rootParams.push_back(NX12Util::CreateRootParameterTable(m_uavRanges, D3D12_SHADER_VISIBILITY_ALL));
	}

	m_cbvManagements.resize(m_rootParamLayout.cbvCount);
}

void NXRenderPass::SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs)
{
	m_cbvManagements[rootParamIndex].autoUpdate = true;
	m_cbvManagements[rootParamIndex].multiFrameGpuVirtAddr = gpuVirtAddrs;
}

void NXRenderPass::SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs)
{
	SetStaticRootParamCBV(rootParamIndex, gpuVirtAddrs);
	m_rootParams[rootParamIndex].Descriptor.ShaderRegister = slotIndex;
}

void NXRenderPass::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc)
{
	m_staticSamplers.push_back(samplerDesc);
}

void NXRenderPass::AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW)
{
	auto& samplerDesc = NXSamplerManager::GetInstance()->CreateIso((int)m_staticSamplers.size(), 0, D3D12_SHADER_VISIBILITY_ALL, filter, addrUVW);
	m_staticSamplers.push_back(samplerDesc);
}
