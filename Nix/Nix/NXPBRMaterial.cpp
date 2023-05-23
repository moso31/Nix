#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXConverter.h"
#include "NXResourceManager.h"
#include "NXSubMesh.h"

#include "ShaderComplier.h"
#include "NXHLSLGenerator.h"
#include "GlobalBufferManager.h"
#include "NXRenderStates.h"

NXMaterial::NXMaterial(const std::string& name, const NXMaterialType type, const std::string& filePath) :
	m_name(name),
	m_type(type),
	m_filePath(filePath),
	m_pathHash(std::filesystem::hash_value(filePath)),
	m_RefSubMeshesCleanUpCount(0)
{
}

void NXMaterial::Update()
{
	// 材质只需要把自己的数据提交给GPU就行了。
	g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, m_cbData.get(), 0, 0);
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

void NXMaterial::SetTex2D(NXTexture2D*& pTex2D, const std::wstring& texFilePath)
{
	if (pTex2D) 
		pTex2D->RemoveRef();

	pTex2D = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, texFilePath);
}

NXPBRMaterialBase::NXPBRMaterialBase(const std::string& name, const NXMaterialType type, const std::string& filePath) :
	NXMaterial(name, type, filePath),
	m_pTexAlbedo(nullptr),
	m_pTexNormal(nullptr),
	m_pTexMetallic(nullptr),
	m_pTexRoughness(nullptr),
	m_pTexAmbientOcclusion(nullptr)
{
}

void NXPBRMaterialBase::SetTexAlbedo(const std::wstring& texFilePath)
{
	SetTex2D(m_pTexAlbedo, texFilePath);
}

void NXPBRMaterialBase::SetTexNormal(const std::wstring& texFilePath)
{
	SetTex2D(m_pTexNormal, texFilePath);
}

void NXPBRMaterialBase::SetTexMetallic(const std::wstring& texFilePath)
{
	SetTex2D(m_pTexMetallic, texFilePath);
}

void NXPBRMaterialBase::SetTexRoughness(const std::wstring& texFilePath)
{
	SetTex2D(m_pTexRoughness, texFilePath);
}

void NXPBRMaterialBase::SetTexAO(const std::wstring& texFilePath)
{
	SetTex2D(m_pTexAmbientOcclusion, texFilePath);
}

void NXPBRMaterialBase::Release()
{
	if (m_pTexAlbedo)			m_pTexAlbedo->RemoveRef();
	if (m_pTexNormal)			m_pTexNormal->RemoveRef();
	if (m_pTexMetallic)			m_pTexMetallic->RemoveRef();
	if (m_pTexRoughness)		m_pTexRoughness->RemoveRef();
	if (m_pTexAmbientOcclusion) m_pTexAmbientOcclusion->RemoveRef();
}

void NXPBRMaterialBase::ReloadTextures()
{
	SetTexAlbedo(m_pTexAlbedo->GetFilePath());
	SetTexNormal(m_pTexNormal->GetFilePath());
	SetTexMetallic(m_pTexMetallic->GetFilePath());
	SetTexRoughness(m_pTexRoughness->GetFilePath());
	SetTexAO(m_pTexAmbientOcclusion->GetFilePath());
}

NXPBRMaterialStandard::NXPBRMaterialStandard(const std::string& name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const std::string& filePath) :
	NXPBRMaterialBase(name, NXMaterialType::PBR_STANDARD, filePath)
{
	m_cbData = std::make_unique<CBufferMaterialStandard>(albedo, normal, metallic, roughness, ao);
	InitConstantBuffer();
}

void NXPBRMaterialStandard::InitConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferMaterialStandard);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cb));
}

NXPBRMaterialTranslucent::NXPBRMaterialTranslucent(const std::string& name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, const std::string& filePath) :
	NXPBRMaterialBase(name, NXMaterialType::PBR_TRANSLUCENT, filePath)
{
	m_cbData = std::make_unique<CBufferMaterialTranslucent>(albedo, normal, metallic, roughness, ao, opacity);
	InitConstantBuffer();
}

void NXPBRMaterialTranslucent::InitConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferMaterialTranslucent);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cb));
}

NXPBRMaterialSubsurface::NXPBRMaterialSubsurface(const std::string& name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, const Vector3& Subsurface, const std::string& filePath) :
	NXPBRMaterialBase(name, NXMaterialType::PBR_SUBSURFACE, filePath)
{
	m_cbData = std::make_unique<CBufferMaterialSubsurface>(albedo, normal, metallic, roughness, ao, opacity, Subsurface);
	InitConstantBuffer();
}

void NXPBRMaterialSubsurface::InitConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferMaterialSubsurface);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cb));
}

void NXCustomMaterial::SetShaderFilePath(const std::filesystem::path& path)
{
	m_nslFilePath = path;
}

void NXCustomMaterial::LoadShaderCode()
{
	std::string strShader;

	// 读取 nsl 文件，获取 nsl shader.
	assert(LoadShaderStringFromFile(strShader));

	// 将 nsl shader 拆成 params 和 code 两部分
	ExtractShaderData(strShader, m_nslParams, m_nslCode);
}

void NXCustomMaterial::ConvertNSLToHLSL(std::string& oHLSLHead, std::string& oHLSLBody)
{
	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	ProcessShaderParameters(m_nslParams, oHLSLHead);

	// 将 nsl code 转换成 DX 可以编译的 hlsl 代码
	ProcessShaderCode(m_nslCode, oHLSLBody);
}

void NXCustomMaterial::ConvertGUIDataToHLSL(std::string& oHLSLHead, std::string& oHLSLBody, const std::vector<NXGUICBufferData>& cbDataGUI, const std::vector<NXGUITextureData>& texDataGUI, const std::vector<NXGUISamplerData>& samplerDataGUI)
{
	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	ProcessShaderParameters(m_nslParams, oHLSLHead);

	// 将 nsl code 转换成 DX 可以编译的 hlsl 代码
	ProcessShaderCode(m_nslCode, oHLSLBody);
}

bool NXCustomMaterial::CompileShader(const std::string& strHLSLHead, const std::string& strHLSLBody, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
	std::string strGBufferShader;
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strHLSLHead, strHLSLBody, strGBufferShader);

	ComPtr<ID3D11VertexShader> pNewVS;
	ComPtr<ID3D11PixelShader>  pNewPS;
	ComPtr<ID3D11InputLayout>  pNewIL;

	HRESULT hrVS = NXShaderComplier::GetInstance()->CompileVSILByCode(strGBufferShader, "VS", &pNewVS, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &pNewIL, oErrorMessageVS);
	HRESULT hrPS = NXShaderComplier::GetInstance()->CompilePSByCode(strGBufferShader, "PS", &pNewPS, oErrorMessagePS);

	if (FAILED(hrVS) || FAILED(hrPS))
		return false;

	m_pVertexShader = pNewVS;
	m_pPixelShader = pNewPS;
	m_pInputLayout = pNewIL;
	return true;
}

void NXCustomMaterial::InitShaderResources()
{
	for (auto& texInfo : m_texInfos)
	{
		if (texInfo.pTexture)
			texInfo.pTexture->RemoveRef();
		texInfo.pTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(texInfo.name, g_defaultTex_white_str);
	}

	for (auto& ssInfo : m_samplerInfos)
	{
		ssInfo.pSampler = NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create();
	}

	UpdateCBData();
}

void NXCustomMaterial::UpdateCBData()
{
	auto& cbElems = m_cbInfo.elems;

	int cbArraySize = 0;
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& cb = cbElems[m_cbSortedIndex[i]];
		cbArraySize += (int)cb.type;
	}
	cbArraySize += (4 - cbArraySize % 4) % 4;	// 16 bytes align

	m_cbufferData.clear();
	m_cbufferData.reserve(cbArraySize);
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& cb = cbElems[m_cbSortedIndex[i]];
		switch (cb.type)
		{
		case NXCBufferInputType::Float:
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			break;
		case NXCBufferInputType::Float2:
		{
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 1]);
			break;
		}
		case NXCBufferInputType::Float3:
		{
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 1]);
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 2]);
			break;
		}
		case NXCBufferInputType::Float4:
		{
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 1]);
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 2]);
			m_cbufferData.push_back(m_cbInfoMemory[cb.memoryIndex + 3]);
			break;
		}
		default:
			break;
		}
	}

	// 基于 m_cbufferData 创建常量缓冲区
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = (UINT)(cbArraySize * sizeof(float));
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = m_cbufferData.data();

	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_cb));
}

NXCustomMaterial::NXCustomMaterial(const std::string& name) :
	NXMaterial(name, NXMaterialType::CUSTOM, "")
{
}

void NXCustomMaterial::Render()
{
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	for (auto& texInfo : m_texInfos)
	{
		if (texInfo.pTexture)
		{
			ID3D11ShaderResourceView* pSRV = texInfo.pTexture->GetSRV();
			if (pSRV) g_pContext->PSSetShaderResources(texInfo.slotIndex, 1, &pSRV);
		}
	}

	for (auto& ssInfo : m_samplerInfos)
	{
		if (ssInfo.slotIndex)
		{
			ID3D11SamplerState* pSampler = ssInfo.pSampler.Get();
			if (pSampler) g_pContext->PSSetSamplers(ssInfo.slotIndex, 1, &pSampler);
		}
	}

	g_pContext->PSSetConstantBuffers(m_cbInfo.slotIndex, 1, m_cb.GetAddressOf());
}

void NXCustomMaterial::Update()
{
	if (m_cb)
		g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, m_cbufferData.data(), 0, 0);
}

void NXCustomMaterial::Release()
{
}

void NXCustomMaterial::ReloadTextures()
{
}

void NXCustomMaterial::SetCBInfoMemoryData(UINT memoryIndex, UINT count, const float* newData)
{
	count = min(count, (UINT)m_cbInfoMemory.size() - memoryIndex);
	std::copy(newData, newData + count, m_cbInfoMemory.begin() + memoryIndex);
}

void NXCustomMaterial::GenerateInfoBackup()
{
#if 0
	m_cbInfoBackup = m_cbInfo;
	m_cbInfoMemoryBackup = m_cbInfoMemory;
	m_cbSortedIndexBackup = m_cbSortedIndex;
	m_texInfosBackup = m_texInfos;
	m_samplerInfosBackup = m_samplerInfos;
#else
	m_cbInfoBackup.elems.clear();
	m_cbInfoBackup.elems.reserve(m_cbInfo.elems.size());
	std::copy(m_cbInfo.elems.begin(), m_cbInfo.elems.end(), std::back_inserter(m_cbInfoBackup.elems));
	m_cbInfoBackup.slotIndex = m_cbInfo.slotIndex;

	m_cbInfoMemoryBackup.clear();
	m_cbInfoMemoryBackup.reserve(m_cbInfoMemory.size());
	std::copy(m_cbInfoMemory.begin(), m_cbInfoMemory.end(), std::back_inserter(m_cbInfoMemoryBackup));

	m_cbSortedIndexBackup.clear();
	m_cbSortedIndexBackup.reserve(m_cbSortedIndex.size());
	std::copy(m_cbSortedIndex.begin(), m_cbSortedIndex.end(), std::back_inserter(m_cbSortedIndexBackup));

	m_texInfosBackup.clear();
	m_texInfosBackup.reserve(m_texInfos.size());
	std::copy(m_texInfos.begin(), m_texInfos.end(), std::back_inserter(m_texInfosBackup));

	m_samplerInfosBackup.clear();
	m_samplerInfosBackup.reserve(m_samplerInfos.size());
	std::copy(m_samplerInfos.begin(), m_samplerInfos.end(), std::back_inserter(m_samplerInfosBackup));
#endif
}

void NXCustomMaterial::RecoverInfosBackup()
{
	std::swap(m_cbInfo.slotIndex, m_cbInfoBackup.slotIndex);
	m_cbInfo.elems.swap(m_cbInfoBackup.elems);
	m_cbInfoMemory.swap(m_cbInfoMemoryBackup);
	m_cbSortedIndex.swap(m_cbSortedIndexBackup);
	m_texInfos.swap(m_texInfosBackup);
	m_samplerInfos.swap(m_samplerInfosBackup);

	// 清空上述所有vector
	m_cbInfoBackup.elems.clear();
	m_cbInfoMemoryBackup.clear();
	m_cbSortedIndexBackup.clear();
	m_texInfosBackup.clear();
	m_samplerInfosBackup.clear();
}

bool NXCustomMaterial::LoadShaderStringFromFile(std::string& oShader)
{
	if (!std::filesystem::exists(m_nslFilePath))
	{
		printf("Error: Shader file not found: %s\n", m_nslFilePath.string().c_str());
		return false;
	}

	std::ifstream shaderFile(m_nslFilePath);
	if (!shaderFile.is_open())
	{
		printf("Error: Unable to open shader file: %s\n", m_nslFilePath.string().c_str());
		return false;
	}

	oShader = std::string(std::istreambuf_iterator<char>(shaderFile), std::istreambuf_iterator<char>());
	shaderFile.close();
	return true;
}

void NXCustomMaterial::ExtractShaderData(const std::string& shader, std::string& nslParams, std::string& nslCode)
{
	// 查找 Params 和 Code 块的开始和结束位置
	const auto paramsStart = shader.find("Params");
	const auto codeStart = shader.find("Code");
	const auto paramsEnd = codeStart - 1;
	const auto codeEnd = shader.size();

	// 提取 Params 和 Code 块
	nslParams = shader.substr(paramsStart, paramsEnd - paramsStart);
	nslCode = shader.substr(codeStart, codeEnd - codeStart);
}

void NXCustomMaterial::ProcessShaderParameters(const std::string& nslParams, std::string& oHLSLHeadCode)
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

		// 先找Params
		if (type == "Params")
		{
			inParam = true;
			continue;
		}

		if (!inParam) continue;

		// 再找左括号
		if (type == "{")
		{
			inParamBrace = true;
			continue;
		}

		if (!inParamBrace) continue;

		// 进入Param内部

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

				std::ostringstream strMatName;
				strMatName << "Mat_" << std::filesystem::hash_value(m_nslFilePath);

				// 处理 CBuffer。内部需要按照 packing rules 的建议对 CBuffer 进行排序。
				std::ostringstream strMatStruct;
				strMatStruct << "struct " << strMatName.str();
				strMatStruct << "\n{\n";
				ProcessShaderCBufferParam(in, strMatStruct);
				strMatStruct << "};\n";

				out << strMatStruct.str();
				out << typeToPrefix[type] << " CBuffer_" << strMatName.str() << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << "\n{\n";
				out << "\t" << strMatName.str() << " " << name << ";\n";
				out << "}\n";
			}
			else if (type == "Tex2D")
			{
				m_texInfos.push_back({ name, nullptr, typeToRegisterIndex[type] });

				out << typeToPrefix[type] << " " << name << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << ";\n";
			}
			else if (type == "SamplerState")
			{
				m_samplerInfos.push_back({ name, nullptr, typeToRegisterIndex[type] });

				out << typeToPrefix[type] << " " << name << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << ";\n";
			}

			continue;
		}
	}

	oHLSLHeadCode = std::move(out.str());
}

void NXCustomMaterial::ProcessShaderCBufferParam(std::istringstream& in, std::ostringstream& out)
{
	using namespace NXConvert;

	auto& cbElems = m_cbInfo.elems;

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

		// 找左括号
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

		// 找左括号
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

		if (type == "float")
		{
			cbElems.push_back({ name, NXCBufferInputType::Float, pOffset });
			m_cbInfoMemory.push_back(0.0f);
			pOffset++;
		}
		else if (type == "float2")
		{
			for (int i = 0; i < 2; i++) m_cbInfoMemory.push_back(0.0f);
			cbElems.push_back({ name, NXCBufferInputType::Float2, pOffset });
			pOffset += 2;
		}
		else if (type == "float3")
		{
			for (int i = 0; i < 3; i++) m_cbInfoMemory.push_back(0.0f);
			cbElems.push_back({ name, NXCBufferInputType::Float3, pOffset });
			pOffset += 3;
		}
		else if (type == "float4")
		{
			for (int i = 0; i < 4; i++) m_cbInfoMemory.push_back(0.0f);
			cbElems.push_back({ name, NXCBufferInputType::Float4, pOffset });
			pOffset += 4;
		}
	}

	SortShaderCBufferParam();

	// 给 CBuffer 填充变量
	for (int i = 0; i < m_cbSortedIndex.size(); i++)
	{
		auto cb = cbElems[m_cbSortedIndex[i]];
		switch (cb.type)
		{
		case NXCBufferInputType::Float:    out << "\tfloat "; break;
		case NXCBufferInputType::Float2:   out << "\tfloat2 "; break;
		case NXCBufferInputType::Float3:   out << "\tfloat3 "; break;
		case NXCBufferInputType::Float4:   out << "\tfloat4 "; break;
		default: break;
		}
		out << cb.name << ";\n";
	}
}

void NXCustomMaterial::ProcessShaderCode(const std::string& nslCode, std::string& oHLSLBodyCode)
{
	using namespace NXConvert;

	std::istringstream in(nslCode);
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

		// 先找Code
		if (type == "Code")
		{
			inCode = true;
			continue;
		}

		if (!inCode) continue;

		// 再找左括号
		if (type == "{")
		{
			inCodeBrace = true;
			continue;
		}

		if (!inCodeBrace) continue;

		// 进入Code内部
		if (type == "}")
		{
			inCode = false;
			inCodeBrace = false;
			continue;
		}

		out << line << "\n";
	}

	oHLSLBodyCode = std::move(out.str());
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
	// 采用三轮遍历的方法，第一轮填充float3/float4, 第二轮填充float2, 第三轮填充float。

	// 第一轮遍历
	std::vector<int> float3Indices; // 记录一下 float3 的索引，第三轮遍历要用。
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

	// 第二轮遍历
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& elem = cbElems[i];
		if (elem.type == NXCBufferInputType::Float2)
			push_back_cbSortedFunc(i);
	}

	// 第三轮遍历
	int traverse_3rd_count = 0;
	int offset = 1;
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& elem = cbElems[i];
		if (elem.type == NXCBufferInputType::Float)
		{
			// 优先填充 Vector3 的剩余内存
			if (traverse_3rd_count < float3Indices.size())
				insert_cbSortedFunc(float3Indices[traverse_3rd_count] + offset++, i);
			else push_back_cbSortedFunc(i);

			traverse_3rd_count++;
		}
	}
}

