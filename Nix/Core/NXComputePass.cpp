#include "NXComputePass.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXRGResource.h"

NXComputePass::NXComputePass() :
	NXRenderPass(NXRenderPassType::ComputePass),
	m_csoDesc({}),
	m_pIndirectArgs(nullptr)
{
	m_csoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void NXComputePass::InitCSO()
{
	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);

	if (m_pIndirectArgs)
	{
		InitCommandSignature();
	}

	ComPtr<IDxcBlob> pCSBlob;
	NXShaderComplier::GetInstance()->CompileCS(m_shaderFilePath, m_entryNameCS, pCSBlob.GetAddressOf());

	m_csoDesc.pRootSignature = m_pRootSig.Get();
	m_csoDesc.CS = { pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize() };
	NXGlobalDX::GetDevice()->CreateComputePipelineState(&m_csoDesc, IID_PPV_ARGS(&m_pCSO));

	std::wstring csoName(NXConvert::s2ws(m_passName) + L" CSO");
	m_pCSO->SetName(csoName.c_str());
}

void NXComputePass::InitCommandSignature()
{
	D3D12_INDIRECT_ARGUMENT_DESC indirectArgDesc = {};
	indirectArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

	D3D12_COMMAND_SIGNATURE_DESC desc = {};
	desc.pArgumentDescs = &indirectArgDesc;
	desc.NumArgumentDescs = 1;
	desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
	desc.NodeMask = 0;

	m_pCommandSig = NX12Util::CreateCommandSignature(NXGlobalDX::GetDevice(), desc, nullptr);
}

void NXComputePass::SetThreadGroups(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	m_threadGroupX = threadGroupX;
	m_threadGroupY = threadGroupY;
	m_threadGroupZ = threadGroupZ;
}

void NXComputePass::SetInput(NXRGResource* pRes, uint32_t slotIndex)
{
	if (m_pInRes.size() <= slotIndex) m_pInRes.resize(slotIndex + 1);
	m_pInRes[slotIndex] = pRes;
}

void NXComputePass::SetOutput(NXRGResource* pRes, uint32_t slotIndex)
{
	if (m_pOutRes.size() <= slotIndex) m_pOutRes.resize(slotIndex + 1);
	m_pOutRes[slotIndex] = pRes;
}

void NXComputePass::SetIndirectArguments(NXRGResource* pRes)
{
	m_pIndirectArgs = pRes;
}

void NXComputePass::RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList)
{
	// DX12需要及时更新纹理的资源状态
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;

	for (int i = 0; i < (int)m_pInRes.size(); i++)
	{
		auto pRes = m_pInRes[i]->GetResource();
		pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	}
	for (int i = 0; i < (int)m_pOutRes.size(); i++)
	{
		auto pRes = m_pOutRes[i]->GetResource();
		pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		uavBarriers.push_back(NX12Util::BarrierUAV(pRes->GetD3DResource()));
	}

	if (m_pIndirectArgs)
	{
		auto pIndiArgs = m_pIndirectArgs->GetResource();
		pIndiArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	}

	// uavBarrier，确保GPU侧资源状态及时更新
	pCmdList->ResourceBarrier((uint32_t)uavBarriers.size(), uavBarriers.data());
}

void NXComputePass::RenderBefore(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->SetComputeRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pCSO.Get());

	for (int i = 0; i < (int)m_cbvManagements.size(); i++)
	{
		// 支持自动绑定，但自动绑定（autoUpdate）是图方便搞的语法糖，
		// 也许让上层接口自己手动更新更清晰。// 保持现状，如果未来觉得这样不合适，再改
		if (m_cbvManagements[i].autoUpdate)
		{
			const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbvManagements[i].multiFrameGpuVirtAddr->Current();
			pCmdList->SetComputeRootConstantBufferView(i, gpuVirtAddr);
		}
	}

	uint32_t srvTableIdx = (uint32_t)m_cbvManagements.size();
	uint32_t uavTableIdx = m_srvRanges.empty() ? srvTableIdx : srvTableIdx + 1;

	// compute pass input
	if (!m_pInRes.empty())
	{
		for (int i = 0; i < (int)m_pInRes.size(); i++)
		{
			auto pRes = m_pInRes[i]->GetResource();
			if (pRes->GetResourceType() == NXResourceType::Buffer)
			{
				NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetSRV());
			}
			else if (pRes->GetResourceType() != NXResourceType::None)
			{
				NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetSRV());
			}
		}

		if (!m_srvRanges.empty())
		{
			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0 = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(srvTableIdx, srvHandle0);
		}
	}

	// compute pass output
	if (!m_pOutRes.empty())
	{
		for (int i = 0; i < (int)m_pOutRes.size(); i++)
		{
			auto pRes = m_pOutRes[i]->GetResource();
			if (pRes->GetResourceType() == NXResourceType::Buffer)
			{
				NXShVisDescHeap->PushFluid(Ntr<NXBuffer>(pRes)->GetUAV());
			}
			else if (pRes->GetResourceType() != NXResourceType::None)
			{
				NXShVisDescHeap->PushFluid(Ntr<NXTexture>(pRes)->GetUAV());
			}
		}

		if (!m_uavRanges.empty())
		{
			D3D12_GPU_DESCRIPTOR_HANDLE uavHandle0 = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(uavTableIdx, uavHandle0);
		}
	}
}

void NXComputePass::Render(ID3D12GraphicsCommandList* pCmdList)
{
	RenderSetTargetAndState(pCmdList);
	RenderBefore(pCmdList);

	if (!m_pIndirectArgs)
	{
		pCmdList->Dispatch(m_threadGroupX, m_threadGroupY, m_threadGroupZ);
	}
	else
	{
		auto pIndiArg = m_pIndirectArgs->GetResource().As<NXBuffer>();
		pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pIndiArg->GetD3DResource(), 0, nullptr, 0);
	}
}

void NXComputePass::CopyUAVCounterTo(ID3D12GraphicsCommandList* pCmdList, NXRGResource* pUAVCounterRes)
{
	assert(m_pIndirectArgs);

	auto pUAVCounterBuffer = pUAVCounterRes->GetBuffer();
	pUAVCounterBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

	auto pIndirectArgsBuffer = m_pIndirectArgs->GetBuffer();
	pIndirectArgsBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	pCmdList->CopyBufferRegion(pIndirectArgsBuffer->GetD3DResource(), 0, pUAVCounterBuffer->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));
}
