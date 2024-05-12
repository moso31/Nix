#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXConverter.h"
#include "NXSubMesh.h"
#include "NXResourceManager.h"
#include "NXAllocatorManager.h"

#include "ShaderComplier.h"
#include "NXHLSLGenerator.h"
#include "NXGlobalDefinitions.h"
#include "NXSamplerManager.h"
#include "NXRenderStates.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"
#include "NXSSSDiffuseProfile.h"

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

NXEasyMaterial::NXEasyMaterial(const std::string& name, const std::filesystem::path& filePath) :
	NXMaterial(name, filePath)
{
	Init();
	m_pTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, filePath);
}

void NXEasyMaterial::Init()
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(".\\Shader\\GBufferEasy.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(".\\Shader\\GBufferEasy.fx", "PS", pPSBlob.GetAddressOf());

	// b0, t1, s0
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParams = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL)
	};

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams, staticSamplers);

	Ntr<NXTexture2D> pGBuffers[] =
	{
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2),
		NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3),
	};
	Ntr<NXTexture2D> pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPNTT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() }; 
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = _countof(pGBuffers);
	for (int i = 0; i < _countof(pGBuffers); i++) 
		psoDesc.RTVFormats[i] = pGBuffers[i]->GetFormat();
	psoDesc.DSVFormat = NXConvert::DXGINoTypeless(pDepthZ->GetFormat(), true);
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXEasyMaterial::Render(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCommandList->SetPipelineState(m_pPSO.Get());

	auto srvHandle = NXGPUHandleHeap->SetFluidDescriptor(m_pTexture->GetSRV());
	pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);
}

void NXCustomMaterial::LoadAndCompile(const std::filesystem::path& nslFilePath)
{
	if (LoadShaderCode())
	{
		std::string strHLSLHead, strHLSLBody;
		std::vector<std::string> strHLSLFuncs;
		ConvertNSLToHLSL(strHLSLHead, strHLSLFuncs, strHLSLBody);

		std::vector<std::string> strHLSLTitles;
		strHLSLTitles.push_back("main()");
		for (int i = 0; i < strHLSLFuncs.size(); i++)
			strHLSLTitles.push_back(NXConvert::GetTitleOfFunctionData(strHLSLFuncs[i]));

		std::string strGBufferShader;
		NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strHLSLHead, strHLSLFuncs, strHLSLTitles, strHLSLBody, strGBufferShader, std::vector<NXHLSLCodeRegion>());

		std::string strErrMsgVS, strErrMsgPS;
		CompileShader(strGBufferShader, strErrMsgVS, strErrMsgPS);

		InitShaderResources();
	}
	else
	{
		m_bCompileSuccess = false;
	}
}

bool NXCustomMaterial::LoadShaderCode()
{
	std::string strShader;

	// ��ȡ nsl �ļ�����ȡ nsl shader.
	bool bLoadSuccess = LoadShaderStringFromFile(strShader);

	if (bLoadSuccess)
	{
		// �� nsl shader ��� params �� code ������
		ExtractShaderData(strShader, m_nslParams, m_nslFuncs);
	}

	return bLoadSuccess;
}

void NXCustomMaterial::ConvertNSLToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody)
{
	// �� nsl params ת���� DX ���Ա���� hlsl ���룬
	// ͬʱ������зּ𣬽� cb ���浽 m_cbInfo�������浽 m_texInfoMap�����������浽 m_ssInfoMap
	ProcessShaderParameters(m_nslParams, oHLSLHead);

	// �� nsl funcs ת���� DX ���Ա���� hlsl ����
	ProcessShaderFunctions(m_nslFuncs, oHLSLFuncs);

	// �� nsl code ת���� DX ���Ա���� hlsl ����
	ProcessShaderMainFunc(oHLSLBody);
}

void NXCustomMaterial::ConvertGUIDataToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody, const std::vector<NXGUICBufferData>& cbDefaultValues, const NXGUICBufferSetsData& cbSettingsDataGUI, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues)
{
	// �� nsl params ת���� DX ���Ա���� hlsl ���룬
	// ͬʱ������зּ𣬽� cb ���浽 m_cbInfo�������浽 m_texInfoMap�����������浽 m_ssInfoMap
	ProcessShaderParameters(m_nslParams, oHLSLHead, cbDefaultValues, cbSettingsDataGUI, texDefaultValues, samplerDefaultValues);

	// �� nsl funcs ת���� DX ���Ա���� hlsl ����
	ProcessShaderFunctions(m_nslFuncs, oHLSLFuncs);

	// �� nsl code ת���� DX ���Ա���� hlsl ����
	ProcessShaderMainFunc(oHLSLBody);
}

void NXCustomMaterial::CompileShader(const std::string& strGBufferShader, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	HRESULT hrVS = NXShaderComplier::GetInstance()->CompileVS(strGBufferShader, "VS", pVSBlob.GetAddressOf(), oErrorMessageVS);
	HRESULT hrPS = NXShaderComplier::GetInstance()->CompilePS(strGBufferShader, "PS", pPSBlob.GetAddressOf(), oErrorMessagePS);
	m_bCompileSuccess = SUCCEEDED(hrVS) && SUCCEEDED(hrPS);
	
	// ���JIT����OK���Ϳ��Թ���shader�ˡ��������¹�����ǩ����PSO��
	if (m_bCompileSuccess)
	{ 
		// b3, t0~tN, s0~sN.
		std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		ranges.reserve(m_texInfos.size());

		// t0~tN. ÿ��texָ����slotIndex ���Ի��ǵ���forѭ��
		for (const auto &texInfo : m_texInfos)
			ranges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, texInfo.slotIndex));

		std::vector<D3D12_ROOT_PARAMETER> rootParams = {
			NX12Util::CreateRootParameterCBV(3, 0, D3D12_SHADER_VISIBILITY_ALL), // b3
			NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL), // t0~tN
		};

		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
		staticSamplers.reserve(m_samplerInfos.size());
		for (int i = 0; i < m_samplerInfos.size(); i++)
			NXSamplerManager::GetInstance()->Create(i, 0, D3D12_SHADER_VISIBILITY_ALL, m_samplerInfos[i]); // s0~sN

		m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParams, staticSamplers);

		Ntr<NXTexture2D> pGBuffers[] =
		{
			NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer0),
			NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer1),
			NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer2),
			NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_GBuffer3),
		};
		Ntr<NXTexture2D> pDepthZ = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = m_pRootSig.Get();
		psoDesc.InputLayout = NXGlobalInputLayout::layoutPNTT;
		psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		psoDesc.RasterizerState = NXRasterizerState<>::Create();
		psoDesc.BlendState = NXBlendState<>::Create();
		psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.NumRenderTargets = 1;
		for (int i = 0; i < _countof(pGBuffers); i++)
			psoDesc.RTVFormats[i] = pGBuffers[i]->GetFormat();
		psoDesc.DSVFormat = pDepthZ->GetFormat();
		psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
		psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
	}
}

bool NXCustomMaterial::Recompile(const std::string& nslParams, const std::vector<std::string>& nslFuncs, const std::vector<std::string>& nslTitles, const std::vector<NXGUICBufferData>& cbDefaultValues, const NXGUICBufferSetsData& cbSettingDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues, std::vector<NXHLSLCodeRegion>& oShaderFuncRegions, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
	// ���ݲ�����Ϣ���������ʧ��ʱ��ԭ����
	GenerateInfoBackup();

	// ���� NSLParam ����
	m_nslParams = nslParams;

	// ���� NSLFunc ����
	m_nslFuncs.assign(nslFuncs.begin(), nslFuncs.end());

	// �� NSL ת���� HLSL
	std::string strHLSLHead, strHLSLBody;
	std::vector<std::string> strHLSLFuncs;
	ConvertGUIDataToHLSL(strHLSLHead, strHLSLFuncs, strHLSLBody, cbDefaultValues, cbSettingDefaultValues, texDefaultValues, samplerDefaultValues);

	// �� HLSL ��ϳ� GBuffer!
	std::string strGBufferShader;
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strHLSLHead, strHLSLFuncs, nslTitles, strHLSLBody, strGBufferShader, oShaderFuncRegions);

	// ������
	CompileShader(strGBufferShader, oErrorMessageVS, oErrorMessagePS);

	// �������ʧ�ܣ����ñ������ݻָ�����
	if (!m_bCompileSuccess)
		RecoverInfosBackup();

	return m_bCompileSuccess;
}

void NXCustomMaterial::InitShaderResources()
{
	// �����л�
	Deserialize();

	// �������һ��CBufferData
	RequestUpdateCBufferData();
}

void NXCustomMaterial::UpdateCBData()
{
	auto& cbElems = m_cbInfo.elems;
	auto& cbSets = m_cbInfo.sets;
	std::vector<float> cbData;

	int alignCheck = 0;
	cbData.clear();
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& cb = cbElems[m_cbSortedIndex[i]];
		int nType = (int)cb.type;

		// ����4float(16byte)�����顣
		if (alignCheck + nType < 4) // ����Ҫ padding
		{
			for(int j = 0; j < nType; j++) cbData.push_back(m_cbInfoMemory[cb.memoryIndex + j]);
			alignCheck += nType;
		}
		else if (alignCheck + nType == 4)
		{
			for (int j = 0; j < nType; j++) cbData.push_back(m_cbInfoMemory[cb.memoryIndex + j]);
			alignCheck = 0;
		}
		else // > 4���Ȳ������ _pad��������һ�� float
		{
			while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

			for (int j = 0; j < nType; j++) cbData.push_back(m_cbInfoMemory[cb.memoryIndex + j]);
			alignCheck = nType % 4;
		}
	}
	while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

	// material settings
	cbData.push_back(reinterpret_cast<float&>(cbSets.shadingModel));
	while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

	// sss Profile
	UINT sssGBufferIndex = (UINT)m_sssProfileGBufferIndexInternal;
	cbData.push_back(reinterpret_cast<float&>(sssGBufferIndex));
	while (cbData.size() % 4 != 0) cbData.push_back(0); // 16 bytes align

	// �ؽ�����CBuffer
	m_cbData.CreateFrameBuffers((UINT)(cbData.size() * sizeof(float)), NXCBufferAllocator, NXDescriptorAllocator);
	m_cbData.Set(cbData);
}

NXCustomMaterial::NXCustomMaterial(const std::string& name, const std::filesystem::path& path) :
	NXMaterial(name, path)
{
}

void NXCustomMaterial::Render(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCommandList->SetPipelineState(m_pPSO.Get());

	if (m_texInfos.empty()) return;

	auto& srvHandle0 = NXGPUHandleHeap->SetFluidDescriptor(m_texInfos[0].pTexture->GetSRV());
	for (auto& texInfo : m_texInfos)
	{
		if (texInfo.pTexture.IsValid())
		{
			NXGPUHandleHeap->SetFluidDescriptor(texInfo.pTexture->GetSRV());
		}
	}
	pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle0); // t0~tN.
}

void NXCustomMaterial::Update()
{
	if (m_bIsDirty)
	{
		UpdateCBData();
		m_bIsDirty = false;
	}
	else
	{
		m_cbData.UpdateBuffer();
	}
}

void NXCustomMaterial::SetTexture(const Ntr<NXTexture>& pTexture, const std::filesystem::path& texFilePath)
{
	auto it = std::find_if(m_texInfos.begin(), m_texInfos.end(), [pTexture](const NXMaterialTextureInfo& texInfo) { return texInfo.pTexture == pTexture; });
	if (it != m_texInfos.end())
	{
		it->pTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, texFilePath);
	}
}

void NXCustomMaterial::RemoveTexture(const Ntr<NXTexture>& pTexture)
{
	auto it = std::find_if(m_texInfos.begin(), m_texInfos.end(), [pTexture](const NXMaterialTextureInfo& texInfo) { return texInfo.pTexture == pTexture; });
	if (it != m_texInfos.end())
	{
		auto& pTex = it->pTexture;

		// Get tex by NXTextureMode
		if (pTex->GetSerializationData().m_textureType == NXTextureMode::NormalMap)
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_Normal);
		else
			pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White);
	}
}

void NXCustomMaterial::SetCBInfoMemoryData(UINT memoryIndex, UINT count, const float* newData)
{
	count = min(count, (UINT)m_cbInfoMemory.size() - memoryIndex);
	std::copy(newData, newData + count, m_cbInfoMemory.begin() + memoryIndex);

	RequestUpdateCBufferData();
}

void NXCustomMaterial::GenerateInfoBackup()
{
	m_cbInfoBackup.elems.assign(m_cbInfo.elems.begin(), m_cbInfo.elems.end());
	m_cbInfoBackup.slotIndex = m_cbInfo.slotIndex;
	m_cbInfoMemoryBackup.assign(m_cbInfoMemory.begin(), m_cbInfoMemory.end());
	m_cbSortedIndexBackup.assign(m_cbSortedIndex.begin(), m_cbSortedIndex.end());
	m_texInfosBackup.assign(m_texInfos.begin(), m_texInfos.end());
	m_samplerInfosBackup.assign(m_samplerInfos.begin(), m_samplerInfos.end());

	m_nslFuncsBackup.assign(m_nslFuncs.begin(), m_nslFuncs.end());
}

void NXCustomMaterial::RecoverInfosBackup()
{
	std::swap(m_cbInfo.slotIndex, m_cbInfoBackup.slotIndex);
	m_cbInfo.elems.swap(m_cbInfoBackup.elems);
	m_cbInfoMemory.swap(m_cbInfoMemoryBackup);
	m_cbSortedIndex.swap(m_cbSortedIndexBackup);
	m_texInfos.swap(m_texInfosBackup);
	m_samplerInfos.swap(m_samplerInfosBackup);
	m_nslFuncs.swap(m_nslFuncsBackup);

	bool needClear = false; // �����Ҳû�£�����˵��Է����
	if (needClear)
	{
		// �����������vector
		m_cbInfoBackup.elems.clear();
		m_cbInfoMemoryBackup.clear();
		m_cbSortedIndexBackup.clear();
		m_texInfosBackup.clear();
		m_samplerInfosBackup.clear();
		m_nslFuncsBackup.clear();
	}
}

void NXCustomMaterial::SaveToNSLFile()
{
	// ���浽�ļ�
	std::ofstream outputFile(m_filePath);

	if (!outputFile.is_open())
	{
		outputFile.close();
		return;
	}

	// ƴ���ַ���
	std::string content;
	content += m_nslParams;

	for (int i = 0; i < m_nslFuncs.size(); i++) 
	{
		if (i > 0) content += "Func:\n";
		content += m_nslFuncs[i];
		content += "\n";
	}

	outputFile << content;
	outputFile.close();
}

void NXCustomMaterial::Serialize()
{
	using namespace rapidjson;
	std::string n0Path = m_filePath.string() + ".n0";
	NXSerializer serializer;
	serializer.StartObject();

	serializer.StartArray("textures");
	for (auto& texInfo : m_texInfos)
	{
		serializer.StartObject();
		serializer.String("name", texInfo.name);
		serializer.String("path", texInfo.pTexture->GetFilePath().string());
		serializer.Uint("slotIndex", texInfo.slotIndex);
		serializer.Int("guiType", (int)texInfo.guiType);
		serializer.EndObject();
	}
	serializer.EndArray();

	serializer.StartArray("samplers");
	for (auto& ssInfo : m_samplerInfos)
	{
		serializer.StartObject();
		serializer.String("name", ssInfo.name);
		serializer.Uint("slotIndex", ssInfo.slotIndex);
		serializer.Int("filter", (int)ssInfo.filter);
		serializer.Int("addressU", (int)ssInfo.addressU);
		serializer.Int("addressV", (int)ssInfo.addressV);
		serializer.Int("addressW", (int)ssInfo.addressW);
		serializer.EndObject();
	}
	serializer.EndArray();

	serializer.StartArray("cbuffer");
	for (int i = 0; i < m_cbInfo.elems.size(); i++)
	{
		auto& cbInfo = m_cbInfo.elems[i];
		serializer.StartObject();
		serializer.String("name", cbInfo.name);
		serializer.Int("type", (int)cbInfo.type);
		serializer.Int("guiStyle", (int)cbInfo.style);

		serializer.StartArray("guiParams");
		serializer.PushFloat(cbInfo.guiParams[0]);
		serializer.PushFloat(cbInfo.guiParams[1]);
		serializer.EndArray();

		serializer.StartArray("values");
		for (int j = 0; j < (int)cbInfo.type; j++)
			serializer.PushFloat(m_cbInfoMemory[cbInfo.memoryIndex + j]);
		serializer.EndArray();

		serializer.EndObject();
	}
	serializer.EndArray();

	// cbuffer sets
	{
		serializer.Uint("shadingModel", m_cbInfo.sets.shadingModel);
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
		auto texArray = deserializer.Array("textures");
		for (auto& tex : texArray)
		{
			auto objName = deserializer.String(tex, "name");
			auto objPath = deserializer.String(tex, "path");

			auto texInfo = std::find_if(m_texInfos.begin(), m_texInfos.end(), [objName](const NXMaterialTextureInfo& texInfo) { return texInfo.name == objName; });
			if (texInfo == m_texInfos.end()) continue;

			texInfo->guiType = (NXGUITextureMode)deserializer.Int(tex, "guiType", (int)NXGUITextureMode::Unknown);
			SetTexture(texInfo->pTexture, objPath);
		}

		// samplers
		auto ssArray = deserializer.Array("samplers");
		for (auto& ss : ssArray)
		{
			auto objName = deserializer.String(ss, "name");
		}

		// cbuffer
		auto cbArray = deserializer.Array("cbuffer");
		for (auto& cb : cbArray)
		{
			auto objName = deserializer.String(cb, "name");

			for (int i = 0; i < m_cbInfo.elems.size(); i++)
			{
				auto& cbElem = m_cbInfo.elems[i];
				if (cbElem.name == objName)
				{
					auto objType = deserializer.Int(cb, "type", (int)NXCBufferInputType::Float);
					if ((int)cbElem.type == objType)
					{
						auto objGUIStyle = deserializer.Int(cb, "guiStyle", (int)NXGUICBufferStyle::Unknown);
						cbElem.style = (NXGUICBufferStyle)objGUIStyle;

						auto objValues = deserializer.Array(cb, "values");
						if (!objValues.Empty())
						{
							for (int j = 0; j < (int)cbElem.type; j++)
								m_cbInfoMemory[cbElem.memoryIndex + j] = objValues[j].GetFloat();
						}

						auto objParams = deserializer.Array(cb, "guiParams");
						if (!objParams.Empty())
						{
							cbElem.guiParams[0] = objParams[0].GetFloat();
							cbElem.guiParams[1] = objParams[1].GetFloat();
						}
					}
				}
			}
		}

		auto& cbSets = m_cbInfo.sets;
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

void NXCustomMaterial::ExtractShaderData(const std::string& shader, std::string& nslParams, std::vector<std::string>& nslFuncs)
{
	// ���� Params �� ������ �Ŀ�ʼ�ͽ���λ��
	size_t paramsStart = shader.find("Params");
	size_t codeStart = shader.find("Code");
	size_t funcStart = shader.find("Func:");
	size_t paramsEnd = codeStart - 1;
	size_t codeEnd = funcStart - 1;

	// ��ȡ Params �� Funcs
	nslParams = shader.substr(paramsStart, paramsEnd - paramsStart);
	nslFuncs.push_back(shader.substr(codeStart, codeEnd - codeStart)); // ��ȡ��һ����������Ȼ��������

	// ���� Func ��
	// ����������֮ǰ�� Funcs: �� �հ��ַ�
	while (funcStart != std::string::npos)
	{
		funcStart += 5; // 5 == sizeof("Func:")
		while (std::isspace(shader[funcStart])) funcStart++;

		size_t openBraceCount = 0;
		size_t closeBraceCount = 0;
		size_t endPos = funcStart;

		for (; endPos < shader.size(); ++endPos)
		{
			if (shader[endPos] == '{')
			{
				openBraceCount++;
			}
			else if (shader[endPos] == '}')
			{
				closeBraceCount++;

				if (openBraceCount == closeBraceCount)
				{
					break;
				}
			}
		}

		// ��ȡ Func ��
		nslFuncs.push_back(shader.substr(funcStart, endPos - funcStart + 1));

		// ������һ�� Func ��
		funcStart = shader.find("Func:", endPos + 1);
	}
}

void NXCustomMaterial::ProcessShaderParameters(const std::string& nslParams, std::string& oHLSLHeadCode, const std::vector<NXGUICBufferData>& cbDefaultValues, const NXGUICBufferSetsData& cbSettingDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues)
{
	using namespace NXConvert;

	m_cbInfo.elems.clear();
	m_texInfos.clear();
	m_samplerInfos.clear();

	std::map<std::string, std::string> typeToPrefix
	{
		{"Tex2D", "Texture2D"},
		{"SamplerState", "SamplerState"},
		{"CBuffer", "cbuffer"}
	};

	std::map<std::string, char> typeToRegisterPrefix
	{
		{"Tex2D", 't'},
		{"SamplerState", 's'},
		{"CBuffer", 'b'}
	};

	std::map<std::string, UINT> typeToRegisterIndex
	{
		{"Tex2D", 1},
		{"SamplerState", 0},
		{"CBuffer", 3}
	};

	std::istringstream in(nslParams);
	std::ostringstream out;

	std::string line;

	int cbIndex = 0;
	bool inParam = false;
	bool inParamBrace = false;
	while (std::getline(in, line))
	{
		std::string type, name;
		std::istringstream lineStream(line);
		std::getline(lineStream, type, ':');
		std::getline(lineStream, name, ':');
		type = Trim(type);
		name = Trim(name);

		// ����Params
		if (type == "Params")
		{
			inParam = true;
			continue;
		}

		if (!inParam) continue;

		// ����������
		if (type == "{")
		{
			inParamBrace = true;
			continue;
		}

		if (!inParamBrace) continue;

		// ����Param�ڲ�

		if (type == "}")
		{
			inParam = false;
			inParamBrace = false;
			continue;
		}

		if (typeToPrefix.find(type) != typeToPrefix.end())
		{
			if (type == "CBuffer")
			{
				m_cbInfo.slotIndex = typeToRegisterIndex[type];

				std::string strMatName("Mat_" + std::to_string(std::filesystem::hash_value(m_filePath)));

				// ���� CBuffer���ڲ���Ҫ���� packing rules �Ľ���� CBuffer ��������
				std::string strMatStruct;
				strMatStruct += "struct " + strMatName + "\n";
				strMatStruct += "{\n";
				ProcessShaderCBufferParam(in, strMatStruct, cbDefaultValues, cbSettingDefaultValues);
				strMatStruct += "};\n";

				out << strMatStruct;
				out << typeToPrefix[type] << " CBuffer_" << strMatName << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << "\n{\n";
				out << "\t" << strMatName << " " << "m" << ";\n";
				out << "}\n";
			}
			else if (type == "Tex2D")
			{
				// ���Ĭ��ֵvector�д洢��������Ϣ���ǿյģ���������vector��ƥ��ͬ����NXTextureָ��
				Ntr<NXTexture> pTexValue;
				NXGUITextureMode guiType = NXGUITextureMode::Unknown;
				if (!texDefaultValues.empty())
				{
					auto it = std::find_if(texDefaultValues.begin(), texDefaultValues.end(),
						[&name, this](const NXGUITextureData& texDisplay) { return texDisplay.name == name; }
					);

					// ����ƥ��ĳ�� NXTexture��ʹ�ø� NXTexture ��Ϊ�� Shader ��Ĭ��ֵ������ʹ�� nullptr
					if (it != texDefaultValues.end())
					{
						if (it->pTexture.IsValid())
						{
							pTexValue = it->pTexture;
							guiType = it->texType;
						}
					}
				}

				m_texInfos.push_back({ name, pTexValue, typeToRegisterIndex[type], guiType });

				out << typeToPrefix[type] << " " << name << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << ";\n";
			}
			else if (type == "SamplerState")
			{
				NXSamplerFilter filter = NXSamplerFilter::Linear;
				NXSamplerAddressMode addressU = NXSamplerAddressMode::Wrap;
				NXSamplerAddressMode addressV = NXSamplerAddressMode::Wrap;
				NXSamplerAddressMode addressW = NXSamplerAddressMode::Wrap;
				if (!samplerDefaultValues.empty())
				{
					auto it = std::find_if(samplerDefaultValues.begin(), samplerDefaultValues.end(),
						[&name, this](const NXGUISamplerData& samplerDisplay) { return samplerDisplay.name == name; }
					);

					if (it != samplerDefaultValues.end())
					{
						filter = it->filter;
						addressU = it->addressU;
						addressV = it->addressV;
						addressW = it->addressW;
					}
				}
				m_samplerInfos.push_back({ name, typeToRegisterIndex[type], filter, addressU, addressV, addressW });

				out << typeToPrefix[type] << " " << name << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << ";\n";
			}

			continue;
		}
	}

	oHLSLHeadCode = std::move(out.str());
}

void NXCustomMaterial::ProcessShaderCBufferParam(std::istringstream& in, std::string& outStr, const std::vector<NXGUICBufferData>& cbDefaultValues, const NXGUICBufferSetsData& cbSettingDefaultValues)
{
	using namespace NXConvert;

	auto& cbElems = m_cbInfo.elems;
	auto& cbSets = m_cbInfo.sets;

	std::istringstream inRecord(in.str());
	std::string line;
	bool inParamBrace = false;
	int cbElemCount = 0;
	int cbFloatCount = 0;
	while (std::getline(inRecord, line))
	{
		std::string type, name;
		std::istringstream lineStream(line);
		std::getline(lineStream, type, ':');
		std::getline(lineStream, name, ':');
		type = Trim(type);
		name = Trim(name);

		// ��������
		if (type == "{")
		{
			inParamBrace = true;
			continue;
		}

		if (!inParamBrace) continue;

		if (type == "}")
		{
			inParamBrace = false;
			break;
		}

		if		(type == "float")  { cbElemCount++; cbFloatCount++; }
		else if (type == "float2") { cbElemCount++; cbFloatCount += 2; }
		else if (type == "float3") { cbElemCount++; cbFloatCount += 3; }
		else if (type == "float4") { cbElemCount++; cbFloatCount += 4; }
	}

	cbElems.clear();
	cbElems.reserve(cbElemCount);

	m_cbInfoMemory.clear();
	m_cbInfoMemory.reserve(cbFloatCount);
	int pOffset = 0;

	inParamBrace = false;
	while (std::getline(in, line))
	{
		std::string type, name;
		std::istringstream lineStream(line);
		std::getline(lineStream, type, ':');
		std::getline(lineStream, name, ':');
		type = Trim(type);
		name = Trim(name);

		// ��������
		if (type == "{")
		{
			inParamBrace = true;
			continue;
		}

		if (!inParamBrace) continue;

		if (type == "}")
		{
			inParamBrace = false;
			break;
		}

		Vector4 cbValue(0.0f);
		NXGUICBufferStyle cbGUIStyle = NXGUICBufferStyle::Unknown;
		Vector2 cbGUIParams;
		if (!cbDefaultValues.empty())
		{
			// ����Ǵ�GUI�������ģ���ʹ��GUI�е�ֵ
			auto it = std::find_if(cbDefaultValues.begin(), cbDefaultValues.end(),
				[&name, this](const NXGUICBufferData& cbDisplay) { return cbDisplay.name == name; }
			);

			if (it != cbDefaultValues.end())
			{
				cbValue = it->data;
				cbGUIStyle = it->guiStyle;
				cbGUIParams = it->params;
			}
		}

		if (type == "float")
		{
			cbElems.push_back({ name, NXCBufferInputType::Float, pOffset, cbGUIStyle, cbGUIParams });
			m_cbInfoMemory.push_back(cbValue.x);
			pOffset++;
		}
		else if (type == "float2")
		{
			for (int i = 0; i < 2; i++) m_cbInfoMemory.push_back(cbValue[i]);
			cbElems.push_back({ name, NXCBufferInputType::Float2, pOffset, cbGUIStyle, cbGUIParams });
			pOffset += 2;
		}
		else if (type == "float3")
		{
			for (int i = 0; i < 3; i++) m_cbInfoMemory.push_back(cbValue[i]);
			cbElems.push_back({ name, NXCBufferInputType::Float3, pOffset, cbGUIStyle, cbGUIParams });
			pOffset += 3;
		}
		else if (type == "float4")
		{
			for (int i = 0; i < 4; i++) m_cbInfoMemory.push_back(cbValue[i]);
			cbElems.push_back({ name, NXCBufferInputType::Float4, pOffset, cbGUIStyle, cbGUIParams });
			pOffset += 4;
		}
	}

	cbSets = cbSettingDefaultValues.data;

	SortShaderCBufferParam();

	// �� CBuffer ������
	std::string strResult = "\t// params \n"; // �Ӹ�ע�ͷ������
	int alignCheck = 0;
	int paddingNum = 0;
	for (int i = 0; i < m_cbSortedIndex.size(); i++)
	{
		const auto& cb = cbElems[m_cbSortedIndex[i]];

		int nType = (int)(cb.type);

		// ����4float(16byte)�����顣
		if (alignCheck + nType < 4) // ����Ҫ padding
		{
			strResult += "\tfloat";
			if (nType > 1) strResult += std::to_string(nType);

			strResult += " " + cb.name;
			alignCheck += nType;
		}
		else if (alignCheck + nType == 4)
		{
			strResult += "\tfloat";
			if (nType > 1) strResult += std::to_string(nType);

			strResult += " " + cb.name;
			alignCheck = 0;
		}
		else // > 4���Ȳ������ _pad��������һ�� float
		{
			int nAlign = 4 - alignCheck;
			strResult += "\tfloat";
			if (nAlign > 1) strResult += std::to_string(nAlign);
			strResult += " _pad" + std::to_string(paddingNum++);
			strResult += ";\n";

			strResult += "\tfloat";
			if (nType > 1) strResult += std::to_string(nType);

			strResult += " " + cb.name;
			alignCheck = nType % 4;
		}

		strResult += ";\n";
	}

	if (alignCheck)
	{
		int nAlign = 4 - alignCheck;
		strResult += "\tfloat";
		if (nAlign > 1) strResult += std::to_string(nAlign);
		strResult += " _pad" + std::to_string(paddingNum++);
		strResult += ";\n";
	}

	strResult += "\t// settings\n"; // �Ӹ�ע�ͷ������
	strResult += "\tfloat shadingModel;\n";
	strResult += "\tfloat3 _pad" + std::to_string(paddingNum++) + ";\n";
	
	// ���ڴ��ݲ��ʵ��Զ�������
	strResult += "\t// customDatas\n"; // �Ӹ�ע�ͷ������
	strResult += "\tfloat4 customData0;\n";

	outStr += strResult;
}

void NXCustomMaterial::ProcessShaderMainFunc(std::string& oHLSLBodyCode)
{
	using namespace NXConvert;

	std::istringstream in(m_nslFuncs[0]);
	std::ostringstream out;

	std::string line;

	bool inCode = false;
	bool inCodeBrace = false;

	while (std::getline(in, line))
	{
		std::string type, name;
		std::istringstream lineStream(line);
		std::getline(lineStream, type, ':');
		std::getline(lineStream, name, ':');
		type = Trim(type);
		name = Trim(name);

		// ����Code
		if (type == "Code")
		{
			inCode = true;
			continue;
		}

		if (!inCode) continue;

		// ����������
		if (type == "{")
		{
			inCodeBrace = true;
			continue;
		}

		if (!inCodeBrace) continue;

		// ����Code�ڲ�
		if (type == "}")
		{
			inCode = false;
			inCodeBrace = false;
			continue;
		}

		out << line << "\n";
	}

	oHLSLBodyCode += std::move(out.str());
}

void NXCustomMaterial::ProcessShaderFunctions(const std::vector<std::string>& nslFuncs, std::vector<std::string>& oHLSLBodyFuncs)
{
	oHLSLBodyFuncs.reserve(nslFuncs.size());
	for (size_t i = 1; i < nslFuncs.size(); i++)
	{
		oHLSLBodyFuncs.push_back(nslFuncs[i] + "\n");
	}
}

const std::string& NXCustomMaterial::GetNSLFunc(UINT index)
{
	if (index < 0 || index >= m_nslFuncs.size()) return g_str_empty;
	return m_nslFuncs[index];
}

void NXCustomMaterial::SetNSLFunc(const std::string& nslFunc, UINT index)
{
	if (index < 0 || index >= m_nslFuncs.size()) return;
	m_nslFuncs[index] = nslFunc;
}

void NXCustomMaterial::SetNSLMainFunc(const std::string& nslFunc)
{
	if (m_nslFuncs.empty()) m_nslFuncs.emplace_back(nslFunc);
	else m_nslFuncs[0] = nslFunc;
}

void NXCustomMaterial::SortShaderCBufferParam()
{
	auto& cbElems = m_cbInfo.elems;

	m_cbSortedIndex.clear();
	m_cbSortedIndex.reserve(cbElems.size());

#if DEBUG
	std::vector<std::string> cbSortedName;
	cbSortedName.reserve(cbElems.size());
#endif

	auto push_back_cbSortedFunc = [&](int index)
	{
		m_cbSortedIndex.push_back(index);
#if DEBUG
		cbSortedName.push_back(cbElems[index].name);
#endif
	};

	auto insert_cbSortedFunc = [&](int insertIndex, int index)
	{
		m_cbSortedIndex.insert(m_cbSortedIndex.begin() + insertIndex, index);
#if DEBUG
		cbSortedName.insert(cbSortedName.begin() + insertIndex, cbElems[index].name);
#endif
	};

	// 2023.5.14
	// �������ֱ����ķ�������һ�����float3/float4, �ڶ������float2, ���������float��

	// ��һ�ֱ���
	std::vector<int> float3Indices; // ��¼һ�� float3 �������������ֱ���Ҫ�á�
	int traverse_1st_count = 0;
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& elem = cbElems[i];
		if (elem.type > NXCBufferInputType::Float2)
		{
			if (elem.type == NXCBufferInputType::Float3)
				float3Indices.push_back(traverse_1st_count);
			push_back_cbSortedFunc(i);
			traverse_1st_count++;
		}
	}

	// �ڶ��ֱ���
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& elem = cbElems[i];
		if (elem.type == NXCBufferInputType::Float2)
			push_back_cbSortedFunc(i);
	}

	// �����ֱ���
	int traverse_3rd_count = 0;
	int offset = 1;
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& elem = cbElems[i];
		if (elem.type == NXCBufferInputType::Float)
		{
			// ������� Vector3 ��ʣ���ڴ�
			if (traverse_3rd_count < float3Indices.size())
				insert_cbSortedFunc(float3Indices[traverse_3rd_count] + offset++, i);
			else push_back_cbSortedFunc(i);

			traverse_3rd_count++;
		}
	}
}
