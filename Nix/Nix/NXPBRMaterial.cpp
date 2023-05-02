#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXConverter.h"
#include "NXResourceManager.h"
#include "NXSubMesh.h"

#include "ShaderComplier.h"
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
	NXHLSLGenerator::GetInstance()->LoadShaderFromFile(m_nslFilePath, strShader);
	NXHLSLGenerator::GetInstance()->ConvertShaderToHLSL(m_nslFilePath, strShader, m_nslParams, m_nslCode, m_srInfoArray);
}

void NXCustomMaterial::CompileShaders()
{
	std::string strGBufferShader;
	NXHLSLGenerator::GetInstance()->EncodeToGBufferShader(m_nslParams, m_nslCode, strGBufferShader);

	NXShaderComplier::GetInstance()->CompileVSILByCode(strGBufferShader, "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPNTT, ARRAYSIZE(NXGlobalInputLayout::layoutPNTT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePSByCode(strGBufferShader, "PS", &m_pPixelShader);
}

void NXCustomMaterial::BuildShaderParams()
{
	for (auto srInfo : m_srInfoArray)
	{
		std::string name = srInfo.first;
		if (srInfo.second.type == NXShaderInputType::Texture)
		{
			NXTexture2D* pDefaultTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(name, g_defaultTex_white_str);
			AddTexture2DParam(name, pDefaultTex);
			continue;
		}

		if (srInfo.second.type == NXShaderInputType::Sampler)
		{
			m_pSamplerStates[name].Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
			AddSamplerStateParam(name, m_pSamplerStates[name].Get());
			continue;
		}

		if (srInfo.second.type == NXShaderInputType::CBuffer)
		{
			// 遍历 cbInfoArray，从中读取自定义struct的结构，并使用该结构，构建ID3D11Buffer。
			auto cbInfoArray = srInfo.second.cbInfos;

			//UINT byteWidth = 
			GenerateCBufferDatas(cbInfoArray);

			//D3D11_BUFFER_DESC bufferDesc;
			//ZeroMemory(&bufferDesc, sizeof(bufferDesc));
			//bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			//bufferDesc.ByteWidth = byteWidth;
			//bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			//bufferDesc.CPUAccessFlags = 0;
			//NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cbuffers[name]));

			//AddConstantBufferParam(name, m_cbuffers[name].Get());
		}
	}
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

	for (auto param : m_texParams)
	{
		UINT paramSlot = m_srInfoArray[param.first].registerIndex;
		auto pSRV = GetTexture2DParamSRV(param.first);
		if (pSRV)
			g_pContext->PSSetShaderResources(paramSlot, 1, &pSRV);
	}

	for (auto param : m_ssParams)
	{
		UINT paramSlot = m_srInfoArray[param.first].registerIndex;
		auto pSampler = GetSamplerParam(param.first);
		if (pSampler)
			g_pContext->PSSetSamplers(paramSlot, 1, &pSampler);
	}

	for (auto param : m_cbParams)
	{
		UINT paramSlot = m_srInfoArray[param.first].registerIndex;
		auto pCB = GetConstantBufferParam(param.first);
		if (pCB)
			g_pContext->PSSetConstantBuffers(paramSlot, 1, &pCB);
	}
}

void NXCustomMaterial::Update()
{
	for (auto param : m_cbParams)
	{
		auto pCBData = GetConstantBufferParam(param.first);
		auto pCB = m_cbuffers[param.first].Get();

		if (pCBData && pCB)
			g_pContext->UpdateSubresource(pCB, 0, nullptr, pCBData, 0, 0);
	}
}

void NXCustomMaterial::Release()
{
}

void NXCustomMaterial::ReloadTextures()
{
}

void NXCustomMaterial::GenerateCBufferDatas(const NXCBufferInfoArray& cbInfos)
{
	static const UINT ALIGN_SIZE = 16;

	UINT cbDataIndex = 0;
	m_CBufferDatas.resize(cbInfos.size());
	
	// 材质可能有 n 组 cb，遍历它们
	for (auto cb : cbInfos)
	{
		NXCBufferInfo& cbInfo = cb.second;

		NXCustomCBuffer& oCBData = m_CBufferDatas[cbDataIndex++];
		oCBData.CBSlotIndex = cbInfo.cbSlotIndex;

		UINT AlignByteSize = 0;

		// 一个 cb 由 一堆 elems 组成
		for (NXCBufferElem cbElem : cbInfo.elems)
		{
			oCBData.ElementIndex.push_back(oCBData.DataSize);
			UINT ElemByteSize = 0;
			switch (cbElem.type)
			{
			case NXCBufferInputType::Float:
				ElemByteSize = sizeof(float);
				break;
			case NXCBufferInputType::Float2:
				ElemByteSize = sizeof(Vector2);
				break;
			case NXCBufferInputType::Float3:
				ElemByteSize = sizeof(Vector3);
				break;
			case NXCBufferInputType::Float4:
				ElemByteSize = sizeof(Vector4);
				break;
			case NXCBufferInputType::Float4x4:
				ElemByteSize = sizeof(Matrix);
				break;
			default:
				break;
			}

			if (AlignByteSize + ElemByteSize <= ALIGN_SIZE)
			{
				AlignByteSize += ElemByteSize;
			}
			else
			{
				AlignByteSize = ElemByteSize;
				oCBData.ByteSize += ALIGN_SIZE;
			}

			if (ElemByteSize == sizeof(Matrix))
			{
				oCBData.ByteSize += ALIGN_SIZE * 4;
				AlignByteSize = 0;
			}
		}

		if (AlignByteSize != 0)
		{
			oCBData.ByteSize += ALIGN_SIZE;
		}
	}
}
