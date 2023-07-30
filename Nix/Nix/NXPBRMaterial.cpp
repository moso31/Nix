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
#include "NXSamplerStates.h"
#include "NXGUIMaterial.h"
#include "NXGUICommon.h"

NXMaterial::NXMaterial(const std::string& name, const std::filesystem::path& filePath) :
	m_name(name),
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
	NXShaderComplier::GetInstance()->CompileVSIL(".\\Shader\\GBufferEasy.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(".\\Shader\\GBufferEasy.fx", "PS", &m_pPixelShader);

	InitConstantBuffer();
}

void NXEasyMaterial::InitConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBufferData);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cb));
}

void NXEasyMaterial::Update()
{
	g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, &m_cbData, 0, 0);
}

void NXEasyMaterial::Render()
{
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(m_pInputLayout.Get());

	ID3D11ShaderResourceView* pSRV = m_pTexture->GetSRV();
	if (pSRV) g_pContext->PSSetShaderResources(1, 1, &pSRV);

	ID3D11SamplerState* pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap);
	if (pSampler) g_pContext->PSSetSamplers(0, 1, &pSampler);

	g_pContext->PSSetConstantBuffers(3, 1, m_cb.GetAddressOf());
}

void NXCustomMaterial::LoadShaderCode()
{
	std::string strShader;

	// 读取 nsl 文件，获取 nsl shader.
	bool bLoadSuccess = LoadShaderStringFromFile(strShader);
	assert(bLoadSuccess);

	// 将 nsl shader 拆成 params 和 code 两部分
	ExtractShaderData(strShader, m_nslParams, m_nslFuncs);
}

void NXCustomMaterial::ConvertNSLToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody)
{
	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	ProcessShaderParameters(m_nslParams, oHLSLHead);

	// 将 nsl funcs 转换成 DX 可以编译的 hlsl 代码
	ProcessShaderFunctions(m_nslFuncs, oHLSLFuncs);

	// 将 nsl code 转换成 DX 可以编译的 hlsl 代码
	ProcessShaderMainFunc(oHLSLBody);
}

void NXCustomMaterial::ConvertGUIDataToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody, const std::vector<NXGUICBufferData>& cbDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues)
{
	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	ProcessShaderParameters(m_nslParams, oHLSLHead, cbDefaultValues, texDefaultValues, samplerDefaultValues);

	// 将 nsl funcs 转换成 DX 可以编译的 hlsl 代码
	ProcessShaderFunctions(m_nslFuncs, oHLSLFuncs);

	// 将 nsl code 转换成 DX 可以编译的 hlsl 代码
	ProcessShaderMainFunc(oHLSLBody);
}

bool NXCustomMaterial::CompileShader(const std::string& strGBufferShader, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
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

bool NXCustomMaterial::Recompile(const std::string& nslParams, const std::vector<std::string>& nslFuncs, const std::vector<NXGUICBufferData>& cbDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues, std::vector<NXHLSLCodeRegion>& oShaderFuncRegions, std::string& oErrorMessageVS, std::string& oErrorMessagePS)
{
	// 备份材质信息，方便编译失败时还原数据
	GenerateInfoBackup();

	// 构建 NSLParam 代码
	m_nslParams = nslParams;

	// 构建 NSLFunc 代码
	m_nslFuncs.assign(nslFuncs.begin(), nslFuncs.end());

	// 将 NSL 转换成 HLSL
	std::string strHLSLHead, strHLSLBody;
	std::vector<std::string> strHLSLFuncs;
	ConvertGUIDataToHLSL(strHLSLHead, strHLSLFuncs, strHLSLBody, cbDefaultValues, texDefaultValues, samplerDefaultValues);

	// 将 HLSL 组合成 GBuffer!
	std::string strGBufferShader;
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(strHLSLHead, strHLSLFuncs, strHLSLBody, strGBufferShader, oShaderFuncRegions);

	// 最后编译
	bool bCompileSuccess = CompileShader(strGBufferShader, oErrorMessageVS, oErrorMessagePS);

	// 如果编译失败，则用备份数据恢复材质
	if (!bCompileSuccess)
		RecoverInfosBackup();

	return bCompileSuccess;
}

void NXCustomMaterial::InitShaderResources()
{
	// 反序列化
	Deserialize();

	// 请求更新一次CBufferData
	RequestUpdateCBufferData();
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

	m_cbData.clear();
	m_cbData.reserve(cbArraySize);
	for (int i = 0; i < cbElems.size(); i++)
	{
		auto& cb = cbElems[m_cbSortedIndex[i]];
		switch (cb.type)
		{
		case NXCBufferInputType::Float:
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			break;
		case NXCBufferInputType::Float2:
		{
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 1]);
			break;
		}
		case NXCBufferInputType::Float3:
		{
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 1]);
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 2]);
			break;
		}
		case NXCBufferInputType::Float4:
		{
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 0]);
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 1]);
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 2]);
			m_cbData.push_back(m_cbInfoMemory[cb.memoryIndex + 3]);
			break;
		}
		default:
			break;
		}
	}

	// 基于 m_cbData 创建常量缓冲区
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = (UINT)(cbArraySize * sizeof(float));
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = m_cbData.data();

	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_cb));
}

NXCustomMaterial::NXCustomMaterial(const std::string& name, const std::filesystem::path& path) :
	NXMaterial(name, path)
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
		auto pSampler = NXSamplerManager::Get(ssInfo.filter, ssInfo.addressU, ssInfo.addressV, ssInfo.addressW);
		g_pContext->PSSetSamplers(ssInfo.slotIndex, 1, &pSampler);
	}

	g_pContext->PSSetConstantBuffers(m_cbInfo.slotIndex, 1, m_cb.GetAddressOf());
}

void NXCustomMaterial::Update()
{
	if (m_bIsDirty)
	{
		UpdateCBData();
		m_bIsDirty = false;
	}

	if (m_cb)
		g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, m_cbData.data(), 0, 0);
}

void NXCustomMaterial::SetTexture(NXTexture* pTexture, const std::filesystem::path& texFilePath)
{
	auto it = std::find_if(m_texInfos.begin(), m_texInfos.end(), [pTexture](const NXMaterialTextureInfo& texInfo) { return texInfo.pTexture == pTexture; });
	if (it != m_texInfos.end())
	{
		auto& pTex = it->pTexture;
		if (pTex)
			pTex->RemoveRef();

		pTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, texFilePath);
	}
}

void NXCustomMaterial::RemoveTexture(NXTexture* pTexture)
{
	auto it = std::find_if(m_texInfos.begin(), m_texInfos.end(), [pTexture](const NXMaterialTextureInfo& texInfo) { return texInfo.pTexture == pTexture; });
	if (it != m_texInfos.end())
	{
		auto& pTex = it->pTexture;
		if (pTex)
			pTex->RemoveRef();

		// Get tex by NXTextureType
		if (pTex->GetSerializationData().m_textureType == NXTextureType::NormalMap)
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

	bool needClear = false; // 不清空也没事，清空了调试方便点
	if (needClear)
	{
		// 清空上述所有vector
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
	// 保存到文件
	std::ofstream outputFile(m_filePath);

	if (!outputFile.is_open())
	{
		outputFile.close();
		return;
	}

	// 拼接字符串
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

	serializer.String("n0Path", n0Path);

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
		auto strInfoPath = deserializer.String("n0Path");
		if (strInfoPath == nxInfoPath)
		{
			// textures
			auto texArray = deserializer.Array("textures");
			for (auto& tex : texArray)
			{
				auto objName = deserializer.String(tex, "name");
				auto objPath = deserializer.String(tex, "path");

				auto texInfo = std::find_if(m_texInfos.begin(), m_texInfos.end(), [objName](const NXMaterialTextureInfo& texInfo) { return texInfo.name == objName; });
				if (texInfo == m_texInfos.end()) continue;

				texInfo->guiType = (NXGUITextureType)deserializer.Int(tex, "guiType", (int)NXGUITextureType::Unknown);
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
						if (cbElem.type == objType)
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
	// 查找 Params 和 主函数 的开始和结束位置
	size_t paramsStart = shader.find("Params");
	size_t codeStart = shader.find("Code");
	size_t funcStart = shader.find("Func:");
	size_t paramsEnd = codeStart - 1;
	size_t codeEnd = funcStart - 1;

	// 提取 Params 和 Funcs
	nslParams = shader.substr(paramsStart, paramsEnd - paramsStart);
	nslFuncs.push_back(shader.substr(codeStart, codeEnd - codeStart)); // 提取第一个函数。必然是主函数

	// 查找 Func 块
	// 并跳过函数之前的 Funcs: 和 空白字符
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

		// 提取 Func 块
		nslFuncs.push_back(shader.substr(funcStart, endPos - funcStart + 1));

		// 查找下一个 Func 块
		funcStart = shader.find("Func:", endPos + 1);
	}
}

void NXCustomMaterial::ProcessShaderParameters(const std::string& nslParams, std::string& oHLSLHeadCode, const std::vector<NXGUICBufferData>& cbDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues)
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
				strMatName << "Mat_" << std::filesystem::hash_value(m_filePath);

				// 处理 CBuffer。内部需要按照 packing rules 的建议对 CBuffer 进行排序。
				std::ostringstream strMatStruct;
				strMatStruct << "struct " << strMatName.str();
				strMatStruct << "\n{\n";
				ProcessShaderCBufferParam(in, strMatStruct, cbDefaultValues);
				strMatStruct << "};\n";

				out << strMatStruct.str();
				out << typeToPrefix[type] << " CBuffer_" << strMatName.str() << " : register(" << typeToRegisterPrefix[type] << typeToRegisterIndex[type]++ << ")";
				out << "\n{\n";
				out << "\t" << strMatName.str() << " " << "m" << ";\n";
				out << "}\n";
			}
			else if (type == "Tex2D")
			{
				// 如果默认值vector中存储的纹理信息不是空的，就优先在vector中匹配同名的NXTexture指针
				NXTexture* pTexValue = nullptr;
				NXGUITextureType guiType = NXGUITextureType::Unknown;
				if (!texDefaultValues.empty())
				{
					auto it = std::find_if(texDefaultValues.begin(), texDefaultValues.end(),
						[&name, this](const NXGUITextureData& texDisplay) { return texDisplay.name == name; }
					);

					// 若能匹配某个 NXTexture*，使用该 NXTexture* 作为新 Shader 的默认值；否则使用 nullptr
					if (it != texDefaultValues.end())
					{
						if (it->pTexture)
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

void NXCustomMaterial::ProcessShaderCBufferParam(std::istringstream& in, std::ostringstream& out, const std::vector<NXGUICBufferData>& cbDefaultValues)
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

		Vector4 cbValue(0.0f);
		NXGUICBufferStyle cbGUIStyle = NXGUICBufferStyle::Unknown;
		Vector2 cbGUIParams;
		if (!cbDefaultValues.empty())
		{
			// 如果是从GUI传过来的，则使用GUI中的值
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
