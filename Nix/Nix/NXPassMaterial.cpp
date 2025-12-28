#include "NXPassMaterial.h"
#include "BaseDefs/DX12.h"
#include "NXSamplerManager.h"
#include "NXSubMeshGeometryEditor.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXConverter.h"
#include "NXAllocatorManager.h"
#include "NXShaderVisibleDescriptorHeap.h"
#include "NXBuffer.h"
#include "NXRenderStates.h"

NXGraphicPassMaterial::NXGraphicPassMaterial(const std::string& name, const std::filesystem::path& shaderPath) : 
	NXPassMaterial(name, shaderPath),
	m_stencilRef(0),
	m_rtSubMeshName("_RenderTarget"),
	m_dsvFormat(DXGI_FORMAT_UNKNOWN),
	m_psoDesc({})
{
	m_psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	m_psoDesc.BlendState = NXBlendState<>::Create();
	m_psoDesc.RasterizerState = NXRasterizerState<>::Create();
	m_psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.SampleDesc.Quality = 0;
	m_psoDesc.SampleMask = UINT_MAX;
	m_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

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

NXPassMaterial::NXPassMaterial(const std::string& name, const std::filesystem::path& shaderPath) : 
	NXMaterial(name),
	m_shaderFilePath(shaderPath),
	m_entryNameVS(L"VS"),
	m_entryNamePS(L"PS"),
	m_entryNameCS(L"CS")
{
}

void NXGraphicPassMaterial::FinalizeLayout()
{
	// cbuffer 需要初始置空（通过判空确定控制权，nullptr = 其他逻辑手动控制，非空 = 交给NXRG(execute)控制）
	m_cbuffers.resize(m_layout.cbvSpaceNum);
	for (int i = 0; i < m_layout.cbvSpaceNum; ++i)
	{
		auto& cbvSlotNum = m_layout.cbvSlotNum[i];
		m_cbuffers[i].resize(cbvSlotNum);
		for (int j = 0; j < cbvSlotNum; ++j)
		{
			m_cbuffers[i][j] = nullptr;
		}
	}

	// 纹理不需要初始化，预留出size即可
	m_pInTexs.resize(m_layout.srvSpaceNum);
	for (int i = 0; i < m_layout.srvSpaceNum; ++i)
	{
		auto& srvSlotNum = m_layout.srvSlotNum[i];
		m_pInTexs[i].resize(srvSlotNum);
	}
	m_pOutRTs.resize(m_rtFormats.size());
}

// NXPassMaterial implementations
void NXPassMaterial::SetShaderFilePath(const std::filesystem::path& shaderFilePath)
{
	m_shaderFilePath = shaderFilePath;
}

void NXPassMaterial::SetConstantBuffer(int spaceIndex, int slotIndex, const NXConstantBufferImpl* pCBuffer)
{
	if (spaceIndex >= m_layout.cbvSpaceNum || slotIndex >= m_layout.cbvSlotNum[spaceIndex]) return;
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
		for (int slot = 0; slot < m_layout.cbvSlotNum[space]; ++slot)
        {
			// 每个CBV占用一个根参数
            m_rootParams.push_back(NX12Util::CreateRootParameterCBV(slot, space, D3D12_SHADER_VISIBILITY_ALL));
        }
    }

    for (int space = 0; space < m_layout.srvSpaceNum; ++space)
    {
		// 每个space占用一个描述符Table，并占用一个根参数。
        m_srvRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_layout.srvSlotNum[space], 0, space));
        m_rootParams.push_back(NX12Util::CreateRootParameterTable(1, &m_srvRanges.back(), D3D12_SHADER_VISIBILITY_ALL));
    }

    for (int space = 0; space < m_layout.uavSpaceNum; ++space)
    {
		// 每个space占用一个描述符Table，并占用一个根参数。
        m_uavRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, m_layout.uavSlotNum[space], 0, space));
        m_rootParams.push_back(NX12Util::CreateRootParameterTable(1, &m_uavRanges.back(), D3D12_SHADER_VISIBILITY_ALL));
    }

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);
}

void NXGraphicPassMaterial::SetInputTex(int spaceIndex, int slotIndex, const Ntr<NXResource>& pTex)
{
	if (spaceIndex >= m_layout.srvSpaceNum || slotIndex >= m_layout.srvSlotNum[spaceIndex]) return;
	m_pInTexs[spaceIndex][slotIndex] = pTex;
}

void NXGraphicPassMaterial::SetOutputRT(int index, const Ntr<NXResource>& pRT)
{
	if (index >= m_rtFormats.size()) return;
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
	m_psoDesc.DSVFormat = NXConvert::TypelessToDSVFormat(m_dsvFormat);

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

	// DX12需要及时更新纹理的资源状态
	for (int i = 0; i < (int)m_pInTexs.size(); i++)
	{
		auto& pSpaceTexs = m_pInTexs[i];
		for (int j = 0; j < (int)pSpaceTexs.size(); j++)
		{
			auto& pSlotTex = pSpaceTexs[j];
			if (pSlotTex.IsValid())
			{
				pSlotTex->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		}
	}
	for (int i = 0; i < (int)m_pOutRTs.size(); i++)
		m_pOutRTs[i]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (m_pOutDS.IsValid())
		m_pOutDS->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	pCmdList->OMSetRenderTargets((UINT)ppRTVs.size(), ppRTVs.data(), false, m_pOutDS.IsValid() ? &m_pOutDS.As<NXTexture2D>()->GetDSV() : nullptr);
}

void NXGraphicPassMaterial::RenderBefore(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->OMSetStencilRef(m_stencilRef);

	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	int rootParameterIndex = 0;
	for (int i = 0; i < (int)m_layout.cbvSpaceNum; i++)
	{
		for (int j = 0; j < (int)m_layout.cbvSlotNum[i]; j++)
		{
			if (m_cbuffers[i][j])
			{
				const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbuffers[i][j]->GetFrameGPUAddresses().Current();
				pCmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuVirtAddr);
			}
			rootParameterIndex++;
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
				if (pRes.IsValid())
					NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetSRV());
				else
					NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullSRV());
			}
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0 = NXShVisDescHeap->Submit();

            // 按照当前规则，每个srv space占用一个描述符表
            pCmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvHandle0);
			rootParameterIndex++;
		}
	}
}

void NXComputePassMaterial::SetInput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes)
{
	if (spaceIndex >= m_layout.srvSpaceNum || slotIndex >= m_layout.srvSlotNum[spaceIndex]) return;
	m_pInRes[spaceIndex][slotIndex] = pRes;
}

void NXComputePassMaterial::SetOutput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes, bool isUAVCounter)
{
	if (spaceIndex >= m_layout.uavSpaceNum || slotIndex >= m_layout.uavSlotNum[spaceIndex]) return;
	m_pOutRes[spaceIndex][slotIndex] = NXResourceUAV(pRes, isUAVCounter);
}

void NXComputePassMaterial::SetOutput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes, int mipSlice)
{
	if (spaceIndex >= m_layout.uavSpaceNum || slotIndex >= m_layout.uavSlotNum[spaceIndex]) return;
	m_pOutRes[spaceIndex][slotIndex] = NXResourceUAV(pRes, mipSlice);
}

NXComputePassMaterial::NXComputePassMaterial(const std::string& name, const std::filesystem::path& shaderPath) : 
	NXPassMaterial(name, shaderPath),
	m_csoDesc({})
{
	m_csoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void NXComputePassMaterial::FinalizeLayout()
{
	// cbuffer 需要初始置空（通过判空确定控制权，nullptr = 其他逻辑手动控制，非空 = 交给NXRG(execute)控制）
	m_cbuffers.resize(m_layout.cbvSpaceNum);
	for (int i = 0; i < m_layout.cbvSpaceNum; ++i)
	{
		auto& cbvSlotNum = m_layout.cbvSlotNum[i];
		m_cbuffers[i].resize(cbvSlotNum);
		for (int j = 0; j < cbvSlotNum; ++j)
		{
			m_cbuffers[i][j] = nullptr;
		}
	}

	// 纹理不需要初始化，预留出size即可
	m_pInRes.resize(m_layout.srvSpaceNum);
	for (int i = 0; i < m_layout.srvSpaceNum; ++i)
	{
		auto& srvSlotNum = m_layout.srvSlotNum[i];
		m_pInRes[i].resize(srvSlotNum);
	}
	m_pOutRes.resize(m_layout.uavSpaceNum);
	for (int i = 0; i < m_layout.uavSpaceNum; ++i)
	{
		auto& uavSlotNum = m_layout.uavSlotNum[i];
		m_pOutRes[i].resize(uavSlotNum);
	}
}

void NXComputePassMaterial::Compile()
{
	InitRootParams();

	ComPtr<IDxcBlob> pCSBlob;
	NXShaderComplier::GetInstance()->CompileCS(m_shaderFilePath, m_entryNameCS, pCSBlob.GetAddressOf());

	m_csoDesc.pRootSignature = m_pRootSig.Get();
	m_csoDesc.CS = { pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize() };
	
	NXGlobalDX::GetDevice()->CreateComputePipelineState(&m_csoDesc, IID_PPV_ARGS(&m_pPSO));

	std::wstring csoName(NXConvert::s2ws(m_name) + L" CSO");
	m_pPSO->SetName(csoName.c_str());
}

void NXComputePassMaterial::Render(ID3D12GraphicsCommandList* pCmdList)
{
	RenderSetTargetAndState(pCmdList);
	RenderBefore(pCmdList);
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
			if (pRes.IsValid())
			{
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
	int rootParameterIndex = 0;
	for (int i = 0; i < (int)m_layout.cbvSpaceNum; i++)
	{
		for (int j = 0; j < (int)m_layout.cbvSlotNum[i]; j++)
		{
			if (m_cbuffers[i][j])
			{
				const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbuffers[i][j]->GetFrameGPUAddresses().Current();
				pCmdList->SetComputeRootConstantBufferView(rootParameterIndex, gpuVirtAddr);
			}
			rootParameterIndex++;
		}
	}

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
			pCmdList->SetComputeRootDescriptorTable(rootParameterIndex, srvHandle);
			rootParameterIndex++;
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
				if (pRes.IsValid())
				{
					if (pRes->GetResourceType() == NXResourceType::Buffer)
					{
						auto pBuffer = pRes.As<NXBuffer>();
						// 根据NXResourceUAV的标志选择UAV描述符
						if (pSpaceOuts[j].useBufferUAVCounter)
						{
							NXShVisDescHeap->PushFluid(pBuffer->GetUAVCounter());
						}
						else
						{
							NXShVisDescHeap->PushFluid(pBuffer->GetUAV());
						}
					}
					else if (pRes->GetResourceType() == NXResourceType::Tex2D)
					{
						auto pTexture2D = pRes.As<NXTexture2D>();
						// 根据NXResourceUAV的mipSlice选择UAV描述符
						if (pSpaceOuts[j].texMipSlice >= 0)
						{
							NXShVisDescHeap->PushFluid(pTexture2D->GetUAV((uint32_t)pSpaceOuts[j].texMipSlice));
						}
						else
						{
							NXShVisDescHeap->PushFluid(pTexture2D->GetUAV());
						}
					}
					else if (pRes->GetResourceType() != NXResourceType::None)
					{
						NXShVisDescHeap->PushFluid(pRes.As<NXTexture>()->GetUAV());
					}
				}
				else
				{
					NXShVisDescHeap->PushFluid(NXAllocator_NULL->GetNullUAV());
				}
			}

			D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = NXShVisDescHeap->Submit();
			pCmdList->SetComputeRootDescriptorTable(rootParameterIndex, uavHandle);
			rootParameterIndex++;
		}
	}
}

void NXReadbackPassMaterial::Render(ID3D12GraphicsCommandList* pCmdList)
{
	// 在这里维护CPUData（m_pOutData）的大小
	auto pGPUBuffer = m_pReadbackBuffer.As<NXBuffer>();
	auto stride = pGPUBuffer->GetStride();
	auto byteSize = pGPUBuffer->GetByteSize();
	if (m_pOutData->GetStride() != stride || m_pOutData->GetByteSize() != byteSize)
		m_pOutData->Create(stride, byteSize / stride);

	NXReadbackContext ctx(pGPUBuffer->GetName() + "_Buffer");
	if (NXReadbackSys->BuildTask(pGPUBuffer->GetByteSize(), ctx))
	{
		// 从（一般是主渲染cmdList）将RT拷到readback ringbuffer（ctx.pResource）
		pGPUBuffer->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pCmdList->CopyBufferRegion(ctx.pResource, ctx.pResourceOffset, pGPUBuffer->GetD3DResource(), 0, pGPUBuffer->GetByteSize());

		NXReadbackSys->FinishTask(ctx, [this, ctx]() {
			// 这时候对应的ringBuffer还不会释放 放心用
			uint8_t* pData = ctx.pResourceData + ctx.pResourceOffset;
			m_pOutData->CopyDataFromGPU(pData);
			});
	}
}
