#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXConverter.h"
#include "NXResourceManager.h"
#include "NXSubMesh.h"

NXMaterial::NXMaterial(const std::string name, const NXMaterialType type, const std::string& filePath) :
	m_name(name),
	m_type(type),
	m_filePath(filePath),
	m_pathHash(std::filesystem::hash_value(filePath)),
	m_RefSubMeshesCleanUpCount(0)
{
}

bool NXMaterial::IsPBRType()
{
	return m_type == NXMaterialType::PBR_STANDARD || m_type == NXMaterialType::PBR_TRANSLUCENT;
}

void NXMaterial::Update()
{
	// 材质只需要把自己的数据提交给GPU就行了。
	g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, m_cbData.get(), 0, 0);
}

NXTexture2D* NXMaterial::LoadFromTexFile(const std::wstring texFilePath, bool GenerateMipMap)
{
	NXTexture2D* pOutTex = NXResourceManager::GetInstance()->CreateTexture2D(m_name, texFilePath);
	pOutTex->AddSRV();
	return pOutTex;
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

NXPBRMaterialBase::NXPBRMaterialBase(const std::string name, const NXMaterialType type, const std::string& filePath) :
	NXMaterial(name, type, filePath),
	m_pTexAlbedo(nullptr),
	m_pTexNormal(nullptr),
	m_pTexMetallic(nullptr),
	m_pTexRoughness(nullptr),
	m_pTexAmbientOcclusion(nullptr)
{
}

void NXPBRMaterialBase::SetTexAlbedo(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexAlbedo) m_pTexAlbedo->RemoveRef();
	m_pTexAlbedo = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterialBase::SetTexNormal(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexNormal) m_pTexNormal->RemoveRef();
	m_pTexNormal = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterialBase::SetTexMetallic(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexMetallic) m_pTexMetallic->RemoveRef();
	m_pTexMetallic = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterialBase::SetTexRoughness(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexRoughness) m_pTexRoughness->RemoveRef();
	m_pTexRoughness = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterialBase::SetTexAO(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexAmbientOcclusion) m_pTexAmbientOcclusion->RemoveRef();
	m_pTexAmbientOcclusion = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterialBase::Release()
{
	m_pTexAlbedo->RemoveRef();
	m_pTexNormal->RemoveRef();
	m_pTexMetallic->RemoveRef();
	m_pTexRoughness->RemoveRef();
	m_pTexAmbientOcclusion->RemoveRef();
}

NXPBRMaterialStandard::NXPBRMaterialStandard(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const std::string& filePath) :
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

NXPBRMaterialTranslucent::NXPBRMaterialTranslucent(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, const std::string& filePath) :
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
