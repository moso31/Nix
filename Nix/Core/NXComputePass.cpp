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

	std::wstring csoName(NXConvert::s2ws(GetPassName()) + L" CSO");
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

void NXComputePass::SetOutput(NXRGResource* pRes, uint32_t slotIndex, bool IsUAVCounter)
{
	if (m_pOutRes.size() <= slotIndex) m_pOutRes.resize(slotIndex + 1);
	m_pOutRes[slotIndex] = { pRes, IsUAVCounter };
}

void NXComputePass::SetIndirectArguments(NXRGResource* pRes)
{
	m_pIndirectArgs = pRes;
}

void NXComputePass::RenderSetTargetAndState()
{
	auto pCmdList = m_commandCtx.cmdList.Current().Get();

	// DX12��Ҫ��ʱ�����������Դ״̬
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;

	for (int i = 0; i < (int)m_pInRes.size(); i++)
	{
		if (!m_pInRes[i]) continue; // �����пյ�������Դ

		auto pRes = m_pInRes[i]->GetResource();
		pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	}
	for (int i = 0; i < (int)m_pOutRes.size(); i++)
	{
		if (!m_pOutRes[i].pRes) continue; // �����пյ������Դ

		auto pRes = m_pOutRes[i].pRes->GetResource();
		pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		uavBarriers.push_back(NX12Util::BarrierUAV(pRes->GetD3DResource()));
		uavBarriers.push_back(NX12Util::BarrierUAV(pRes->GetD3DResourceUAVCounter()));
	}

	if (m_pIndirectArgs)
	{
		auto pIndiArgs = m_pIndirectArgs->GetResource();
		pIndiArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	}

	// uavBarrier��ȷ��GPU����Դ״̬��ʱ����
	pCmdList->ResourceBarrier((uint32_t)uavBarriers.size(), uavBarriers.data());
}

void NXComputePass::RenderBefore()
{
	auto pCmdList = m_commandCtx.cmdList.Current().Get();
	pCmdList->SetComputeRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pCSO.Get());

	for (int i = 0; i < (int)m_cbvManagements.size(); i++)
	{
		// ֧���Զ��󶨣����Զ��󶨣�autoUpdate����ͼ�������﷨�ǣ�
		// Ҳ�����ϲ�ӿ��Լ��ֶ����¸�������// ������״�����δ���������������ʣ��ٸ�
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
			if (!m_pInRes[i])
			{
				NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullSRV());
				continue;
			}

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
			if (!m_pOutRes[i].pRes)
			{
				NXShVisDescHeap->PushFluid({ NXAllocator_NULL->GetNullUAV() });
				continue;
			}

			auto pRes = m_pOutRes[i].pRes->GetResource();
			if (pRes->GetResourceType() == NXResourceType::Buffer)
			{
				if (!m_pOutRes[i].isUAVCounter)
					NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetUAV());
				else
					NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetUAVCounter());
			}
			else if (pRes->GetResourceType() != NXResourceType::None)
			{
				NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetUAV());
			}
		}

		if (!m_uavRanges.empty())
		{
			D3D12_GPU_DESCRIPTOR_HANDLE uavHandle0 = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(uavTableIdx, uavHandle0);
		}
	}
}

void NXComputePass::Render()
{
	auto pCmdList = m_commandCtx.cmdList.Current().Get();
	RenderSetTargetAndState();
	RenderBefore();

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

void NXComputePass::SetBufferAsIndirectArg(NXRGResource* pUAVCounterRes)
{
	assert(m_pIndirectArgs);
	auto pCmdList = m_commandCtx.cmdList.Current().Get();

	auto pUAVCounterBuffer = pUAVCounterRes->GetBuffer();
	pUAVCounterBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

	auto pIndirectArgsBuffer = m_pIndirectArgs->GetBuffer();
	pIndirectArgsBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

	pCmdList->CopyBufferRegion(pIndirectArgsBuffer->GetD3DResource(), 0, pUAVCounterBuffer->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));
}
