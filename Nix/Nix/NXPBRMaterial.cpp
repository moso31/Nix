#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXConverter.h"
#include "NXResourceManager.h"
#include "NXSubMesh.h"

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

void NXCustomMaterial::Render()
{
	for (auto param : m_texParams)
	{
		UINT paramSlot = ;// ...???
		auto pSRV = GetTexture2DParamSRV(param.first);
		if (pSRV)
			g_pContext->PSSetShaderResources(paramSlot, 1, &pSRV);
	}

	for (auto param : m_ssParams)
	{
		UINT paramSlot = ;// ...???
		auto pSampler = GetSamplerParam(param.first);
		if (pSampler)
			g_pContext->PSSetSamplers(paramSlot, 1, &pSampler);
	}

	for (auto param : m_cbParams)
	{
		UINT paramSlot = ;// ...???
		auto pCB = GetConstantBufferParam(param.first);
		if (pCB)
			g_pContext->PSSetConstantBuffers(paramSlot, 1, &pCB);
	}
}
