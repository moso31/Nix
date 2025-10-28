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
	InitRootParams();

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

void NXComputePass::SetInput(NXRGResource* pRes, uint32_t slotIndex, uint32_t spaceIndex)
{
	if (m_pInRes.size() <= spaceIndex) m_pInRes.resize(spaceIndex + 1);
	auto& pSpaceIns = m_pInRes[spaceIndex];

	if (pSpaceIns.size() <= slotIndex) pSpaceIns.resize(slotIndex + 1);
	pSpaceIns[slotIndex] = pRes;

	if (m_rootParamLayout.srvCount.size() <= spaceIndex) m_rootParamLayout.srvCount.resize(spaceIndex + 1);
	m_rootParamLayout.srvCount[spaceIndex] = (uint32_t)pSpaceIns.size();
}

void NXComputePass::SetOutput(NXRGResource* pRes, uint32_t slotIndex, uint32_t spaceIndex, bool IsUAVCounter)
{
	if (m_pOutRes.size() <= spaceIndex) m_pOutRes.resize(spaceIndex + 1);
	auto& pSpaceOuts = m_pOutRes[spaceIndex];
	if (pSpaceOuts.size() <= slotIndex) pSpaceOuts.resize(slotIndex + 1);
	pSpaceOuts[slotIndex] = { pRes, IsUAVCounter };

	if (m_rootParamLayout.uavCount.size() <= spaceIndex) m_rootParamLayout.uavCount.resize(spaceIndex + 1);
	m_rootParamLayout.uavCount[spaceIndex] = (uint32_t)pSpaceOuts.size();
}

void NXComputePass::SetIndirectArguments(NXRGResource* pRes)
{
	m_pIndirectArgs = pRes;
}

void NXComputePass::RenderSetTargetAndState()
{
	auto pCmdList = m_commandCtx.cmdList.Current().Get();

	// DX12需要及时更新纹理的资源状态
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;

	for (int i = 0; i < (int)m_pInRes.size(); i++)
	{
		auto& pSpaceIns = m_pInRes[i];
		for (int j = 0; j < (int)pSpaceIns.size(); j++)
		{
			if (!pSpaceIns[j]) continue; // 可能有空的输入资源
			auto pRes = pSpaceIns[j]->GetResource();
			pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
	}
	for (int i = 0; i < (int)m_pOutRes.size(); i++)
	{
		auto& pSpaceOuts = m_pOutRes[i];
		for (int j = 0; j < (int)pSpaceOuts.size(); j++)
		{
			if (!pSpaceOuts[j].pRes) continue; // 可能有空的输出资源

			auto pRes = pSpaceOuts[j].pRes->GetResource();
			pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			uavBarriers.push_back(NX12Util::BarrierUAV(pRes->GetD3DResource()));
			uavBarriers.push_back(NX12Util::BarrierUAV(pRes->GetD3DResourceUAVCounter()));
		}
	}

	if (m_pIndirectArgs)
	{
		auto pIndiArgs = m_pIndirectArgs->GetResource();
		pIndiArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	}

	// uavBarrier，确保GPU侧资源状态及时更新
	pCmdList->ResourceBarrier((uint32_t)uavBarriers.size(), uavBarriers.data());
}

void NXComputePass::RenderBefore()
{
	auto pCmdList = m_commandCtx.cmdList.Current().Get();
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

	// 描述符表的根参数索引位置，计算srv从第几个索引开始，uav从第几个索引开始
	uint32_t srvTableIdx = (uint32_t)m_cbvManagements.size();
	uint32_t uavTableIdx = srvTableIdx + (uint32_t)m_rootParamLayout.srvCount.size();

	// compute pass input
	if (!m_pInRes.empty())
	{
		for (int i = 0; i < (int)m_pInRes.size(); i++)
		{
			auto& pSpaceIns = m_pInRes[i];
			for (int j = 0; j < (int)pSpaceIns.size(); j++)
			{
				if (!pSpaceIns[j])
				{
					NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullSRV());
					continue;
				}

				auto pRes = pSpaceIns[j]->GetResource();
				if (pRes->GetResourceType() == NXResourceType::Buffer)
				{
					NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetSRV());
				}
				else if (pRes->GetResourceType() != NXResourceType::None)
				{
					NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetSRV());
				}
			}

			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0 = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(srvTableIdx + i, srvHandle0);
		}
	}

	// compute pass output
	if (!m_pOutRes.empty())
	{
		for (int i = 0; i < (int)m_pOutRes.size(); i++)
		{
			auto& pSpaceOuts = m_pOutRes[i];
			for (int j = 0; j < (int)pSpaceOuts.size(); j++)
			{
				auto& resUAV = pSpaceOuts[j];
				if (!resUAV.pRes)
				{
					NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullUAV());
					continue;
				}
				auto pRes = resUAV.pRes->GetResource();
				if (pRes->GetResourceType() == NXResourceType::Buffer)
				{
					if (!resUAV.isUAVCounter)
						NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetUAV());
					else
						NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetUAVCounter());
				}
				else if (pRes->GetResourceType() != NXResourceType::None)
				{
					NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetUAV());
				}
			}

			D3D12_GPU_DESCRIPTOR_HANDLE uavHandle0 = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(uavTableIdx + i, uavHandle0);
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
