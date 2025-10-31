#include "NXPassMaterial.h"
#include "BaseDefs/DX12.h"
#include "Core/NXSamplerManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "Core/ShaderComplier.h"
#include "Core/NXGlobalDefinitions.h"
#include "Core/NXConverter.h"
#include "Core/NXAllocatorManager.h"
#include "Core/NXShaderVisibleDescriptorHeap.h"
#include "NXBuffer.h"
#include "NXGlobalDefinitions.h"
#include "ShaderComplier.h"

void NXGraphicPassMaterial::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc)
{
	m_psoDesc.InputLayout = desc;
}

void NXGraphicPassMaterial::SetBlendState(const D3D12_BLEND_DESC& desc)
{
	m_psoDesc.BlendState = desc;
}

void NXGraphicPassMaterial::SetRasterizerState(const D3D12_RASTERIZER_DESC& desc)
{
	m_psoDesc.RasterizerState = desc;
}

void NXGraphicPassMaterial::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc)
{
	m_psoDesc.DepthStencilState = desc;
}

void NXGraphicPassMaterial::SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask)
{
	m_psoDesc.SampleDesc.Count = Count;
	m_psoDesc.SampleDesc.Quality = Quality;
	m_psoDesc.SampleMask = Mask;
}

void NXGraphicPassMaterial::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	m_psoDesc.PrimitiveTopologyType = type;
}

// NXGraphicPassMaterial constructor
NXGraphicPassMaterial::NXGraphicPassMaterial(const std::string& name, const std::filesystem::path& shaderPath)
	: NXPassMaterial(name, shaderPath),
	m_stencilRef(0),
	m_rtSubMeshName("_RenderTarget")
{
}

// NXPassMaterial implementations
void NXPassMaterial::SetShaderFilePath(const std::filesystem::path& shaderFilePath)
{
	m_shaderFilePath = shaderFilePath;
}

void NXPassMaterial::SetConstantBuffer(int spaceIndex, int slotIndex, NXConstantBufferImpl* pCBuffer)
{
	if (spaceIndex >= m_layout.cbvSpaceNum || slotIndex >= m_layout.cbvSlotNum) return;
	m_cbuffers[spaceIndex][slotIndex] = pCBuffer;
}

void NXPassMaterial::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler)
{
	m_staticSamplers.push_back(staticSampler);
}

void NXPassMaterial::AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW)
{
	auto& samplerDesc = NXSamplerManager::GetInstance()->CreateIso(static_cast<int>(m_staticSamplers.size()), 0, D3D12_SHADER_VISIBILITY_ALL, filter, addrUVW);
	m_staticSamplers.push_back(samplerDesc);
}

void NXPassMaterial::InitRootParams()
{
	m_rootParams.clear();
	m_srvRanges.clear();
	m_uavRanges.clear();

    for (int space = 0; space < m_layout.cbvSpaceNum; ++space)
    {
        for (int slot = 0; slot < m_layout.cbvSlotNum; ++slot)
        {
            m_rootParams.push_back(NX12Util::CreateRootParameterCBV(slot, space, D3D12_SHADER_VISIBILITY_ALL));
        }
    }

    for (int space = 0; space < m_layout.srvSpaceNum; ++space)
    {
        m_srvRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_layout.srvSlotNum, 0, space));
        m_rootParams.push_back(NX12Util::CreateRootParameterTable(1, &m_srvRanges.back(), D3D12_SHADER_VISIBILITY_ALL));
    }

    for (int space = 0; space < m_layout.uavSpaceNum; ++space)
    {
        m_uavRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, m_layout.uavSlotNum, 0, space));
        m_rootParams.push_back(NX12Util::CreateRootParameterTable(1, &m_uavRanges.back(), D3D12_SHADER_VISIBILITY_ALL));
    }

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);
}

void NXGraphicPassMaterial::SetLayout(int cbvSlotNum, int cbvSpaceNum, int srvSlotNum, int srvSpaceNum, const std::vector<DXGI_FORMAT>& rtFormats, DXGI_FORMAT dsvFormat)
{
	m_layout.cbvSlotNum = cbvSlotNum;
	m_layout.cbvSpaceNum = cbvSpaceNum;
	m_layout.srvSlotNum = srvSlotNum;
	m_layout.srvSpaceNum = srvSpaceNum;
	m_layout.rtNum = rtFormats.size();
	m_rtFormats = rtFormats;
	m_dsvFormat = dsvFormat;

	m_cbuffers.resize(m_layout.cbvSpaceNum);
	for (int i = 0; i < m_layout.cbvSpaceNum; ++i)
	{
		m_cbuffers[i].resize(m_layout.cbvSlotNum);
		for (int j = 0; j < m_layout.cbvSlotNum; ++j)
		{
			// cbuffer 指针需要初始化为 nullptr 才行
			m_cbuffers[i][j] = nullptr; 
		}
	}
	m_pInTexs.resize(m_layout.srvSpaceNum);
	for (int i = 0; i < m_layout.srvSpaceNum; ++i)
		m_pInTexs[i].resize(m_layout.srvSlotNum);
	m_pOutRTs.resize(m_layout.rtNum);
}

void NXGraphicPassMaterial::SetInputTex(int spaceIndex, int slotIndex, const Ntr<NXResource>& pTex)
{
	if (spaceIndex >= m_layout.srvSpaceNum || slotIndex >= m_layout.srvSlotNum) return;
	m_pInTexs[spaceIndex][slotIndex] = pTex;
}

void NXGraphicPassMaterial::SetOutputRT(int index, const Ntr<NXResource>& pRT)
{
	if (index >= m_layout.rtNum) return;
	m_pOutRTs[index] = pRT;
}

void NXGraphicPassMaterial::SetOutputDS(const Ntr<NXResource>& pDS)
{
	m_pOutDS = pDS;
}

void NXGraphicPassMaterial::Compile()
{
    InitRootParams();

	ComPtr<IDxcBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(m_shaderFilePath, m_entryNameVS, pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(m_shaderFilePath, m_entryNamePS, pPSBlob.GetAddressOf());

	m_psoDesc.pRootSignature = m_pRootSig.Get();
	m_psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	m_psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	m_psoDesc.NumRenderTargets = (UINT)m_pOutRTs.size();
	for (UINT i = 0; i < m_psoDesc.NumRenderTargets; i++)
		m_psoDesc.RTVFormats[i] = m_rtFormats[i];
	m_psoDesc.DSVFormat = m_dsvFormat;

	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pPSO));

	std::wstring psoName(NXConvert::s2ws(m_name) + L" PSO");
	m_pPSO->SetName(psoName.c_str());
}

void NXGraphicPassMaterial::Render(ID3D12GraphicsCommandList* pCmdList)
{
	RenderSetTargetAndState(pCmdList);
	RenderBefore(pCmdList);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews(m_rtSubMeshName);
	D3D12_VERTEX_BUFFER_VIEW vbv;
	if (meshView.GetVBV(0, vbv))
		pCmdList->IASetVertexBuffers(0, 1, &vbv);
	D3D12_INDEX_BUFFER_VIEW ibv;
	if (meshView.GetIBV(1, ibv))
		pCmdList->IASetIndexBuffer(&ibv);
	pCmdList->DrawIndexedInstanced(meshView.GetIndexCount(), 1, 0, 0, 0);
}

void NXGraphicPassMaterial::RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ppRTVs;
	for (auto& pTex : m_pOutRTs)
	{
		ppRTVs.push_back(pTex.As<NXTexture2D>()->GetRTV());
	}
	pCmdList->OMSetRenderTargets((UINT)ppRTVs.size(), ppRTVs.data(), true, m_pOutDS.IsValid() ? &m_pOutDS.As<NXTexture2D>()->GetDSV() : nullptr);

	// DX12需要及时更新纹理的资源状态
	for (int i = 0; i < (int)m_pInTexs.size(); i++)
	{
		auto& pSpaceTexs = m_pInTexs[i];
		for (int j = 0; j < (int)pSpaceTexs.size(); j++)
			pSpaceTexs[j]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	for (int i = 0; i < (int)m_pOutRTs.size(); i++)
		m_pOutRTs[i]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (m_pOutDS.IsValid())
		m_pOutDS->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void NXGraphicPassMaterial::RenderBefore(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->OMSetStencilRef(m_stencilRef);

	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	for (int i = 0; i < (int)m_layout.cbvSpaceNum; i++)
	{
		for (int j = 0; j < (int)m_layout.cbvSlotNum; j++)
		{
			if (m_cbuffers[i][j])
			{
				const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbuffers[i][j]->GetFrameGPUAddresses().Current();
				pCmdList->SetGraphicsRootConstantBufferView(i * m_layout.cbvSlotNum + j, gpuVirtAddr);
			}
		}
	}

	if (!m_pInTexs.empty())
	{
		for (int i = 0; i < (int)m_pInTexs.size(); i++)
		{
			auto& pSpaceTexs = m_pInTexs[i];
			for (int j = 0; j < (int)pSpaceTexs.size(); j++)
			{
				auto pRes = pSpaceTexs[j];
				NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetSRV());
			}
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0 = NXShVisDescHeap->Submit();

            // 按照当前规则，每个srv space占用一个描述符表
            int srvRootParamIndex = m_layout.cbvSlotNum * m_layout.cbvSpaceNum + i;
            pCmdList->SetGraphicsRootDescriptorTable(srvRootParamIndex, srvHandle0);
		}
	}
}

void NXComputePassMaterial::SetLayout(int cbvSlotNum, int cbvSpaceNum, int srvSlotNum, int srvSpaceNum, int uavSlotNum, int uavSpaceNum)
{
	m_layout.cbvSlotNum = cbvSlotNum;
	m_layout.cbvSpaceNum = cbvSpaceNum;
	m_layout.srvSlotNum = srvSlotNum;
	m_layout.srvSpaceNum = srvSpaceNum;
	m_layout.uavSlotNum = uavSlotNum;
	m_layout.uavSpaceNum = uavSpaceNum;

	m_cbuffers.resize(m_layout.cbvSpaceNum);
	m_pInRes.resize(m_layout.srvSpaceNum);
	for (int i = 0; i < m_layout.srvSpaceNum; ++i)
		m_pInRes[i].resize(m_layout.srvSlotNum);
	m_pOutRes.resize(m_layout.uavSpaceNum);
	for (int i = 0; i < m_layout.uavSpaceNum; ++i)
		m_pOutRes[i].resize(m_layout.uavSlotNum);
}

void NXComputePassMaterial::SetInput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes)
{
	if (spaceIndex >= m_layout.srvSpaceNum || slotIndex >= m_layout.srvSlotNum) return;
	m_pInRes[spaceIndex][slotIndex] = pRes;
}

void NXComputePassMaterial::SetOutput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes, bool isUAVCounter)
{
	if (spaceIndex >= m_layout.uavSpaceNum || slotIndex >= m_layout.uavSlotNum) return;
	m_pOutRes[spaceIndex][slotIndex] = NXResourceUAV(pRes, isUAVCounter);
}

void NXComputePassMaterial::SetIndirectArguments(const Ntr<NXResource>& pRes)
{
	m_pIndirectArgs = pRes;
}

NXComputePassMaterial::NXComputePassMaterial(const std::string& name, const std::filesystem::path& shaderPath)
	: NXPassMaterial(name, shaderPath),
	m_threadGroupX(1),
	m_threadGroupY(1),
	m_threadGroupZ(1)
{
	m_csoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void NXComputePassMaterial::SetThreadGroups(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	m_threadGroupX = threadGroupX;
	m_threadGroupY = threadGroupY;
	m_threadGroupZ = threadGroupZ;
}

void NXComputePassMaterial::SetBufferUAVCounterAsIndirectArgDispatchX(const Ntr<NXResource>& pRes, ID3D12GraphicsCommandList* pCmdList)
{
    // 虽然只是拷贝UAV计数器，但目前的设计不太灵活，要SetResourceState必须带着原始资源一起做...
	auto pBuffer = pRes.As<NXBuffer>();
	pBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

	auto pIndirectArgsBuffer = m_pIndirectArgs.As<NXBuffer>();
	pIndirectArgsBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

    // ...不过最终拷贝的时候，只拷贝UAV计数器即可。
	pCmdList->CopyBufferRegion(pIndirectArgsBuffer->GetD3DResource(), 0, pBuffer->GetD3DResourceUAVCounter(), 0, sizeof(uint32_t));
}

void NXComputePassMaterial::Compile()
{
	InitRootParams();

	if (m_pIndirectArgs.IsValid())
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

	ComPtr<IDxcBlob> pCSBlob;
	NXShaderComplier::GetInstance()->CompileCS(m_shaderFilePath, m_entryNameCS, pCSBlob.GetAddressOf());

	m_csoDesc.pRootSignature = m_pRootSig.Get();
	m_csoDesc.CS = { pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize() };
	
	NXGlobalDX::GetDevice()->CreateComputePipelineState(&m_csoDesc, IID_PPV_ARGS(&m_pPSO));

	std::wstring csoName(NXConvert::s2ws(m_name) + L" CSO");
	m_pPSO->SetName(csoName.c_str());
}

void NXComputePassMaterial::Update()
{
}

void NXComputePassMaterial::Render(ID3D12GraphicsCommandList* pCmdList)
{
	RenderSetTargetAndState(pCmdList);
	RenderBefore(pCmdList);

	if (!m_pIndirectArgs.IsValid())
	{
		pCmdList->Dispatch(m_threadGroupX, m_threadGroupY, m_threadGroupZ);
	}
	else
	{
		auto pIndirectBuffer = m_pIndirectArgs.As<NXBuffer>();
		pCmdList->ExecuteIndirect(m_pCommandSig.Get(), 1, pIndirectBuffer->GetD3DResource(), 0, nullptr, 0);
	}
}

void NXComputePassMaterial::Release()
{
}

void NXComputePassMaterial::RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList)
{
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;

	// 设置输入资源状态
	for (int space = 0; space < (int)m_pInRes.size(); space++)
	{
		auto& pSpaceIns = m_pInRes[space];
		for (int slot = 0; slot < (int)pSpaceIns.size(); slot++)
		{
			if (!pSpaceIns[slot].IsValid()) continue;
			pSpaceIns[slot]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
	}

	// 设置输出资源状态
	for (int space = 0; space < (int)m_pOutRes.size(); space++)
	{
		auto& pSpaceOuts = m_pOutRes[space];
		for (int slot = 0; slot < (int)pSpaceOuts.size(); slot++)
		{
			if (pSpaceOuts[slot].pRes.IsNull()) continue;
			auto pRes = pSpaceOuts[slot].pRes;
			pRes->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            
			uavBarriers.push_back(NX12Util::BarrierUAV(pRes->GetD3DResource()));
			if (pRes->GetResourceType() == NXResourceType::Buffer)
			{
				auto pBuffer = pRes.As<NXBuffer>();
				if (pBuffer->GetD3DResourceUAVCounter())
				{
					uavBarriers.push_back(NX12Util::BarrierUAV(pBuffer->GetD3DResourceUAVCounter()));
				}
			}
		}
	}

	// 间接参数资源状态（如果存在）
	if (m_pIndirectArgs.IsValid())
	{
		m_pIndirectArgs->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	}

	// 最后提交UAV屏障
	if (!uavBarriers.empty())
		pCmdList->ResourceBarrier((uint32_t)uavBarriers.size(), uavBarriers.data());
}

void NXComputePassMaterial::RenderBefore(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->SetComputeRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	// 绑定CBV
	for (int i = 0; i < (int)m_layout.cbvSpaceNum; i++)
	{
		for (int j = 0; j < (int)m_layout.cbvSlotNum; j++)
		{
			if (m_cbuffers[i][j])
			{
				const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbuffers[i][j]->GetFrameGPUAddresses().Current();
				pCmdList->SetComputeRootConstantBufferView(i * m_layout.cbvSlotNum + j, gpuVirtAddr);
			}
		}
	}

	// 描述符表的根参数索引位置
	uint32_t srvTableIdx = m_layout.cbvSpaceNum * m_layout.cbvSlotNum;
	uint32_t uavTableIdx = srvTableIdx + m_layout.srvSpaceNum;

	// 绑定SRV描述符表
	if (!m_pInRes.empty())
	{
		for (int i = 0; i < (int)m_pInRes.size(); i++)
		{
			auto& pSpaceIns = m_pInRes[i];
			for (int j = 0; j < (int)pSpaceIns.size(); j++)
			{
				if (!pSpaceIns[j].IsValid())
				{
					NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullSRV());
					continue;
				}

				auto pRes = pSpaceIns[j];
				if (pRes->GetResourceType() == NXResourceType::Buffer)
				{
					NXShVisDescHeap->PushFluid(pRes.As<NXBuffer>()->GetSRV());
				}
				else if (pRes->GetResourceType() != NXResourceType::None)
				{
					NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetSRV());
				}
			}

			D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(srvTableIdx + i, srvHandle);
		}
	}

	// 绑定UAV描述符表
	if (!m_pOutRes.empty())
	{
		for (int i = 0; i < (int)m_pOutRes.size(); i++)
		{
			auto& pSpaceOuts = m_pOutRes[i];
			for (int j = 0; j < (int)pSpaceOuts.size(); j++)
			{
				if (!pSpaceOuts[j].pRes.IsValid())
				{
					NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullUAV());
					continue;
				}

				auto pRes = pSpaceOuts[j].pRes;
				if (pRes->GetResourceType() == NXResourceType::Buffer)
				{
					auto pBuffer = pRes.As<NXBuffer>();
					// 根据NXResourceUAV的标志选择UAV描述符
					if (pSpaceOuts[j].isUAVCounter)
					{
						NXShVisDescHeap->PushFluid(pBuffer->GetUAVCounter());
					}
					else
					{
						NXShVisDescHeap->PushFluid(pBuffer->GetUAV());
					}
				}
				else if (pRes->GetResourceType() != NXResourceType::None)
				{
					NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetUAV());
				}
			}

			D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(uavTableIdx + i, uavHandle);
		}
	}
}