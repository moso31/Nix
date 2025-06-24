#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXResourceManager.h"
#include "NXAllocatorManager.h"
#include "NXGPUTerrainManager.h"

#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXSamplerManager.h"
#include "NXRenderStates.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXSSSDiffuseProfile.h"
#include "NXPSOManager.h"
#include "NXCodeProcessHelper.h"

NXMaterial::NXMaterial(const std::string& name, const std::filesystem::path& filePath) :
	NXObject(name),
	m_filePath(filePath),
	m_RefSubMeshesCleanUpCount(0)
{
}

void NXMaterial::RemoveSubMesh(NXSubMeshBase* pRemoveSubmesh)
{
	m_pRefSubMeshes.erase(
		std::remove(m_pRefSubMeshes.begin(), m_pRefSubMeshes.end(), pRemoveSubmesh)
	);
}

void NXMaterial::AddSubMesh(NXSubMeshBase* pSubMesh)
{
	m_pRefSubMeshes.push_back(pSubMesh);
}

void NXMaterial::UpdatePSORenderStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& oPSODesc)
{
	oPSODesc.DepthStencilState = NXDepthStencilState<>::Create();
	oPSODesc.BlendState = NXBlendState<>::Create();
	oPSODesc.RasterizerState = NXRasterizerState<>::Create();
}

NXEasyMaterial::NXEasyMaterial(const std::string& name, const std::filesystem::path& filePath) :
	NXMaterial(name, filePath)
{
	Init();
	m_pTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, filePath);
}

void NXEasyMaterial::Init()
{
	ComPtr<IDxcBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(".\\Shader\\GBufferEasy.fx", L"VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(".\\Shader\\GBufferEasy.fx", L"PS", pPSBlob.GetAddressOf());

	// b0, t1, s0
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParams = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL)
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams, staticSamplers);

	DXGI_FORMAT fmtGBuffers[] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};
	DXGI_FORMAT fmtDepthZ = DXGI_FORMAT_R24G8_TYPELESS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPNTT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() }; 
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = _countof(fmtGBuffers);
	for (int i = 0; i < _countof(fmtGBuffers); i++)
		psoDesc.RTVFormats[i] = fmtGBuffers[i];
	psoDesc.DSVFormat = NXConvert::DXGINoTypeless(fmtDepthZ, true);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	UpdatePSORenderStates(psoDesc);

	m_pPSO = NXPSOManager::GetInstance()->Create(psoDesc, m_name + "_PSO");
}

void NXEasyMaterial::Render(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCommandList->SetPipelineState(m_pPSO.Get());

	NXShVisDescHeap->PushFluid(m_pTexture->GetSRV());
	auto& srvHandle = NXShVisDescHeap->Submit();
	pCommandList->SetGraphicsRootDescriptorTable(2, srvHandle);
}

void NXCustomMaterial::LoadAndCompile(const std::filesystem::path& nslFilePath)
{
	m_nslPath = nslFilePath;

	// 构建前先释放旧数据
	m_materialDatas.Destroy();

	if (LoadShaderCode())
	{
		InitShaderResources();

		std::string strHLSL = NXCodeProcessHelper::BuildHLSL(nslFilePath, m_materialDatas, m_codeBlocks, m_bEnableTerrainGPUInstancing);
		std::string strErrMsgVS, strErrMsgPS;
		CompileShader(strHLSL, strErrMsgVS, strErrMsgPS);
	}
	else
	{
		m_bCompileSuccess = false;
	}
}

bool NXCustomMaterial::LoadShaderCode()
{
	std::string strShader;

	// 读取 nsl 文件
	bool bLoadSuccess = LoadShaderStringFromFile(strShader);

	if (bLoadSuccess)
	{
		// 解析 nsl 文件
		NXCodeProcessHelper::ExtractShader(strShader, m_materialDatas, m_codeBlocks);
	}

	return bLoadSuccess;
}

void NXCustomMaterial::CompileShader(const std::string& strGBufferShader, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
	std::wstring strEnableGPUInstancing = m_bEnableTerrainGPUInstancing ? L"1" : L"0";
	ComPtr<IDxcBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->AddMacro(L"GPU_INSTANCING", strEnableGPUInstancing);
	HRESULT hrVS = NXShaderComplier::GetInstance()->CompileVSByCode(strGBufferShader, L"VS", pVSBlob.GetAddressOf(), oErrorMessageVS);
	NXShaderComplier::GetInstance()->AddMacro(L"GPU_INSTANCING", strEnableGPUInstancing);
	HRESULT hrPS = NXShaderComplier::GetInstance()->CompilePSByCode(strGBufferShader, L"PS", pPSBlob.GetAddressOf(), oErrorMessagePS);
	m_bCompileSuccess = SUCCEEDED(hrVS) && SUCCEEDED(hrPS);
	
	// 如果JIT编译OK，就可以构建shader了。首先重新构建根签名和PSO。
	if (m_bCompileSuccess)
	{
		auto& txArr = m_materialDatas.GetTextures();
		auto& ssArr = m_materialDatas.GetSamplers();

		// b3, t0..., s...
		// t0~tN：nsl材质文件生成的所有纹理
		// s0~sM：nsl材质文件生成的所有纹理
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		ranges.reserve(txArr.size());

		// 每张tex指定了slotIndex 所以还是得用for循环
		for (int i = 0; i < txArr.size(); i++)
			ranges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i));

		if (m_bEnableTerrainGPUInstancing)
		{
			ranges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1)); // t0, space1 = GPU Terrain Patch Buffer;
		}

		std::vector<D3D12_ROOT_PARAMETER> rootParams = {
			NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL), // b0, space0
			NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL), // b1
			NX12Util::CreateRootParameterCBV(0, 1, D3D12_SHADER_VISIBILITY_ALL), // b0, space1 用户自定义参数总是放在space1
			NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL), // t..., space0
		};

		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
		staticSamplers.reserve(ssArr.size());
		for (int i = 0; i < ssArr.size(); i++)
			staticSamplers.push_back(NXSamplerManager::GetInstance()->Create(i, 0, D3D12_SHADER_VISIBILITY_ALL, ssArr[i])); // s...

		m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams, staticSamplers);

		DXGI_FORMAT fmtGBuffers[] =
		{
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM
		};
		DXGI_FORMAT fmtDepthZ = DXGI_FORMAT_R24G8_TYPELESS;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_pRootSig.Get();
		psoDesc.InputLayout = NXGlobalInputLayout::layoutPNTT_GPUInstancing;
		psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.NumRenderTargets = _countof(fmtGBuffers);
		for (int i = 0; i < _countof(fmtGBuffers); i++)
			psoDesc.RTVFormats[i] = fmtGBuffers[i];
		psoDesc.DSVFormat = NXConvert::DXGINoTypeless(fmtDepthZ, true);
		psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		UpdatePSORenderStates(psoDesc);

		m_pPSO = NXPSOManager::GetInstance()->Create(psoDesc, m_name + "_PSO");
	}
}

bool NXCustomMaterial::Recompile(const NXMaterialData& guiData, const NXMaterialCode& code, const NXMaterialData& guiDataBackup, const NXMaterialCode& codeBackup, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
	auto codeCopy = code;
	std::string strHLSL = NXCodeProcessHelper::BuildHLSL(m_nslPath, guiData, codeCopy, m_bEnableTerrainGPUInstancing);
	CompileShader(strHLSL, oErrorMessageVS, oErrorMessagePS);

	m_materialDatas.Destroy();
	if (m_bCompileSuccess)
	{
		m_materialDatas = guiData.Clone();
		m_codeBlocks = codeCopy;
	}
	else
	{
		// 如果编译失败，则用备份数据恢复材质
		m_materialDatas = guiDataBackup.Clone();
		m_codeBlocks = codeBackup;
	}

	return m_bCompileSuccess;
}

void NXCustomMaterial::InitShaderResources()
{
	// 反序列化
	Deserialize();

	// 请求更新一次CBufferData
	RequestUpdateCBufferData(true);
}

void NXCustomMaterial::UpdateCBData(bool rebuildCB)
{
	std::vector<float> cbData;
	int alignCheck = 0;

	for (auto& cb : m_materialDatas.GetCBuffers())
	{
		if (alignCheck + cb->size <= 4)
		{
			for (int j = 0; j < cb->size; j++) cbData.push_back(cb->data[j]);
		}
		else
		{
			while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align
			for (int j = 0; j < cb->size; j++) cbData.push_back(cb->data[j]);
		}

		alignCheck = (alignCheck + cb->size) % 4;
	}

	while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

	// material settings
	cbData.push_back(reinterpret_cast<float&>(m_materialDatas.Settings().shadingModel));
	while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

	// sss Profile
	UINT sssGBufferIndex = (UINT)m_sssProfileGBufferIndexInternal;
	cbData.push_back(reinterpret_cast<float&>(sssGBufferIndex));
	while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

	// 重建整个CBuffer
	if (rebuildCB)
	{
		m_cbData.Recreate(cbData.size());
	}

	m_cbData.Set(cbData);
}

void NXCustomMaterial::UpdatePSORenderStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& oPSODesc)
{
	// TODO: 根据材质的属性，设置各种渲染状态
	// 现在的材质系统非常简单，只基于 ShadingModel 设了个模板状态=Replace（因为3S用的着），其他的就啥都没管了……
	// 将来会有更多的属性，比如alphaTest blend、双面、深度是否写入等等

	NXShadingModel shadingModel = GetShadingModel();
	if (shadingModel == NXShadingModel::SubSurface)
	{
		oPSODesc.DepthStencilState = NXDepthStencilState<true, true, D3D12_COMPARISON_FUNC_LESS, true, 0xff, 0xff, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS>::Create();
	}
	else
	{
		oPSODesc.DepthStencilState = NXDepthStencilState<>::Create();
	}

	oPSODesc.BlendState = NXBlendState<>::Create();
	oPSODesc.RasterizerState = NXRasterizerState<>::Create();
}

NXCustomMaterial::NXCustomMaterial(const std::string& name, const std::filesystem::path& path) :
	NXMaterial(name, path),
	m_bEnableTerrainGPUInstancing(true)
{
}

void NXCustomMaterial::Render(ID3D12GraphicsCommandList* pCommandList)
{
	if (!m_pRootSig || !m_pPSO) return;

	pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCommandList->SetPipelineState(m_pPSO.Get());

	for (auto& texData : m_materialDatas.GetTextures())
	{
		auto& pTex = texData->pTexture;
		if (pTex.IsValid())
			NXShVisDescHeap->PushFluid(pTex->GetSRV());
	}

	if (m_bEnableTerrainGPUInstancing)
	{
		auto pTerrainPatchBuffer = NXGPUTerrainManager::GetInstance()->GetTerrainPatcherBuffer();
		if (pTerrainPatchBuffer.IsValid())
		{
			NXShVisDescHeap->PushFluid(pTerrainPatchBuffer->GetSRV());
		}
	}

	auto& srvHandle = NXShVisDescHeap->Submit();
	pCommandList->SetGraphicsRootDescriptorTable(3, srvHandle); // t...

	if (!m_bIsDirty)
	{
		auto gpuHandle = m_cbData.CurrentGPUAddress();
		pCommandList->SetGraphicsRootConstantBufferView(2, gpuHandle); 
	}
}

void NXCustomMaterial::Update()
{
	if (m_bIsDirty)
	{
		UpdateCBData(m_bNeedRebuildCB);
		m_bIsDirty = false;
	}
}

void NXCustomMaterial::SetTexture(const Ntr<NXTexture>& pTexture, const std::filesystem::path& texFilePath)
{
	NXMatDataTexture* texData = m_materialDatas.FindTexture(pTexture);
	if (texData)
	{
		auto newTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, texFilePath);
		texData->pTexture = pTexture;
		texData->SyncLink();
	}
}


void NXCustomMaterial::RemoveTexture(const Ntr<NXTexture>& pTexture)
{
	NXMatDataTexture* texData = m_materialDatas.FindTexture(pTexture);
	if (texData)
	{
		auto pTex = texData->pTexture;
		if (pTex->GetSerializationData().m_textureType == NXTextureMode::NormalMap)
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Normal);
		else
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
	}
}

void NXCustomMaterial::SetCBInfoMemoryData()
{
	RequestUpdateCBufferData(false);
}

NXShadingModel NXCustomMaterial::GetShadingModel()
{
	return (NXShadingModel)m_materialDatas.Settings().shadingModel;
}

void NXCustomMaterial::RequestUpdateCBufferData(bool bNeedRebuildCB)
{
	m_bIsDirty = true;
	m_bNeedRebuildCB = bNeedRebuildCB;
}

void NXCustomMaterial::SaveToNSLFile()
{
	NXCodeProcessHelper::SaveToNSLFile(m_nslPath, m_materialDatas, m_codeBlocks);
}

void NXCustomMaterial::Serialize()
{
	using namespace rapidjson;
	std::string n0Path = m_filePath.string() + ".n0";
	NXSerializer serializer;
	serializer.StartObject();

	serializer.StartArray("textures");
	for (auto& tx : m_materialDatas.GetTextures())
	{
		serializer.StartObject();
		serializer.String("name", tx->name);
		serializer.String("path", tx->pTexture->GetFilePath().string());
		serializer.EndObject();
	}
	serializer.EndArray();

	serializer.StartArray("samplers");
	for (auto& ss : m_materialDatas.GetSamplers())
	{
		serializer.StartObject();
		serializer.String("name", ss->name);
		serializer.Int("filter", (int)ss->filter);
		serializer.Int("addressU", (int)ss->addressU);
		serializer.Int("addressV", (int)ss->addressV);
		serializer.Int("addressW", (int)ss->addressW);
		serializer.EndObject();
	}
	serializer.EndArray();

	serializer.StartArray("cbuffer");
	for (auto& cb : m_materialDatas.GetCBuffers())
	{
		serializer.StartObject();
		serializer.String("name", cb->name);
		serializer.Int("type", (int)cb->size);
		serializer.Int("guiStyle", (int)cb->gui.style);

		serializer.StartArray("guiParams");
		serializer.PushFloat(cb->gui.params[0]);
		serializer.PushFloat(cb->gui.params[1]);
		serializer.EndArray();

		serializer.StartArray("values");
		for (int j = 0; j < (int)cb->size; j++)
			serializer.PushFloat(cb->data[j]);
		serializer.EndArray();

		serializer.EndObject();
	}
	serializer.EndArray();

	// cbuffer sets
	{
		serializer.Uint("shadingModel", m_materialDatas.Settings().shadingModel);
		serializer.String("sssProfilePath", m_sssProfilePath.string());
	}

	serializer.EndObject();

	serializer.SaveToFile(n0Path.c_str());
}

void NXCustomMaterial::Deserialize()
{
	using namespace rapidjson;
	std::string nxInfoPath = m_filePath.string() + ".n0";
	NXDeserializer deserializer;
	bool bJsonExist = deserializer.LoadFromFile(nxInfoPath.c_str());
	if (bJsonExist)
	{
		// textures
		auto& texArray = deserializer.Array("textures");
		for (auto& tex : texArray)
		{
			auto& objName = deserializer.String(tex, "name");
			std::filesystem::path objPath = deserializer.String(tex, "path");

			auto* txData = m_materialDatas.FindTextureByName(objName);
			if (txData)
			{
				txData->pTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureAuto(objName, objPath);
			}
		}

		// samplers
		auto ssArray = deserializer.Array("samplers");
		for (auto& ss : ssArray)
		{
			auto& objName = deserializer.String(ss, "name");
			auto objFilter = deserializer.Int(ss, "filter");
			auto objAddressU = deserializer.Int(ss, "addressU");
			auto objAddressV = deserializer.Int(ss, "addressV");
			auto objAddressW = deserializer.Int(ss, "addressW");

			auto* ssData = m_materialDatas.FindSamplerByName(objName);
			if (ssData)
			{
				ssData->filter = (NXSamplerFilter)objFilter;
				ssData->addressU = (NXSamplerAddressMode)objAddressU;
				ssData->addressV = (NXSamplerAddressMode)objAddressV;
				ssData->addressW = (NXSamplerAddressMode)objAddressW;
			}
		}

		// cbuffer
		auto cbArray = deserializer.Array("cbuffer");
		for (auto& cb : cbArray)
		{
			auto objName = deserializer.String(cb, "name");
			auto* cbData = m_materialDatas.FindCBufferByName(objName);
			if (cbData)
			{
				cbData->name = objName;

				auto& objValues = deserializer.Array(cb, "values");
				if (!objValues.Empty())
				{
					cbData->size = objValues.Size();
					for (int i = 0; i < cbData->size; i++)
					{
						cbData->data[i] = objValues[i].GetFloat();
					}
				}

				auto objGUIStyle = deserializer.Int(cb, "guiStyle", (int)NXGUICBufferStyle::Unknown);
				cbData->gui.style = (NXGUICBufferStyle)objGUIStyle;

				auto objGUIParams = deserializer.Array(cb, "guiParams");
				if (!objGUIParams.Empty())
				{
					cbData->gui.params[0] = objGUIParams[0].GetFloat();
					cbData->gui.params[1] = objGUIParams[1].GetFloat();
				}
			}
		}

		auto& cbSets = m_materialDatas.Settings();
		cbSets.shadingModel = deserializer.Uint("shadingModel");

		if (cbSets.shadingModel == 2)
		{
			m_sssProfilePath = deserializer.String("sssProfilePath");
		}
	}
}

bool NXCustomMaterial::LoadShaderStringFromFile(std::string& oShader)
{
	if (!std::filesystem::exists(m_filePath))
	{
		printf("Error: Shader file not found: %s\n", m_filePath.string().c_str());
		return false;
	}

	std::ifstream shaderFile(m_filePath);
	if (!shaderFile.is_open())
	{
		printf("Error: Unable to open shader file: %s\n", m_filePath.string().c_str());
		return false;
	}

	oShader = std::string(std::istreambuf_iterator<char>(shaderFile), std::istreambuf_iterator<char>());
	shaderFile.close();
	return true;
}
