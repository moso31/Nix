#include "NXPBRMaterial.h"
#include <memory>
#include "NXReflection.h"
#include "DirectXTex.h"

NXPBRMaterial::NXPBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR) :
	m_albedo(albedo),
	m_metallic(metallic),
	m_roughness(roughness),
	m_reflectivity(reflectivity),
	m_refractivity(refractivity),
	m_IOR(IOR),
	m_pTexAlbedo(nullptr),
	m_pTexNormal(nullptr),
	m_pTexMetallic(nullptr),
	m_pTexRoughness(nullptr),
	m_pTexAmbientOcclusion(nullptr),
	m_pSRVAlbedo(nullptr),
	m_pSRVNormal(nullptr),
	m_pSRVMetallic(nullptr),
	m_pSRVRoughness(nullptr),
	m_pSRVAmbientOcclusion(nullptr)
{
	// F0: 入射角度为0度时的Fresnel反射率。
	m_F0 = Vector3::Lerp(Vector3(0.04f), albedo, metallic); 
}

void NXPBRMaterial::CalcSampleProbabilities(float F)
{
	m_sampleProbs.Diffuse  = (1.0f - F) * (1.0f - m_refractivity);
	m_sampleProbs.Specular = F * (1.0f - m_reflectivity);
	m_sampleProbs.Reflect  = F * m_reflectivity;
	m_sampleProbs.Refract  = (1.0f - F) * m_refractivity;
}

ConstantBufferMaterial NXPBRMaterial::GetConstantBuffer()
{
	ConstantBufferMaterial cb;
	cb.albedo = m_albedo;
	cb.metallic = m_metallic;
	cb.roughness = m_roughness;
	return cb;
}

void NXPBRMaterial::SetTexAlbedo(const std::wstring& TexFilePath)
{
	SetTex(TexFilePath, m_pTexAlbedo, m_pSRVAlbedo);
}

void NXPBRMaterial::SetTexNormal(const std::wstring& TexFilePath)
{
	SetTex(TexFilePath, m_pTexNormal, m_pSRVNormal);
}

void NXPBRMaterial::SetTexMetallic(const std::wstring& TexFilePath)
{
	SetTex(TexFilePath, m_pTexMetallic, m_pSRVMetallic);
}

void NXPBRMaterial::SetTexRoughness(const std::wstring& TexFilePath)
{
	SetTex(TexFilePath, m_pTexRoughness, m_pSRVRoughness);
}

void NXPBRMaterial::SetTexAO(const std::wstring& TexFilePath)
{
	SetTex(TexFilePath, m_pTexAmbientOcclusion, m_pSRVAmbientOcclusion);
}

void NXPBRMaterial::SetTex(const std::wstring& TexFilePath, ID3D11Texture2D*& pTex, ID3D11ShaderResourceView*& pSRV)
{
	TexMetadata info;
	std::unique_ptr<ScratchImage> image = std::make_unique<ScratchImage>(); //(new (std::nothrow) ScratchImage);
	auto hr = LoadFromWICFile(TexFilePath.c_str(), WIC_FLAGS_NONE, &info, *image);

	CD3D11_TEXTURE2D_DESC descTex(info.format, (UINT)info.width, (UINT)info.height, (UINT)info.arraySize, (UINT)info.mipLevels, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, (UINT)info.miscFlags);
	auto img = image->GetImage(0, 0, 0);
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = img->pixels;
	initData.SysMemPitch = static_cast<DWORD>(img->rowPitch);
	initData.SysMemSlicePitch = static_cast<DWORD>(img->slicePitch);
	g_pDevice->CreateTexture2D(&descTex, &initData, &pTex);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, info.format, 0, (UINT)info.mipLevels, 0, (UINT)info.arraySize);
	g_pDevice->CreateShaderResourceView(pTex, &descSRV, &pSRV);
}

void NXPBRMaterial::Release()
{
	if (m_pTexAlbedo) m_pTexAlbedo->Release();
	if (m_pSRVAlbedo) m_pSRVAlbedo->Release();

	if (m_pTexNormal) m_pTexNormal->Release();
	if (m_pSRVNormal) m_pSRVNormal->Release();

	if (m_pTexMetallic) m_pTexMetallic->Release();
	if (m_pSRVMetallic) m_pSRVMetallic->Release();

	if (m_pTexRoughness) m_pTexRoughness->Release();
	if (m_pSRVRoughness) m_pSRVRoughness->Release();

	if (m_pTexAmbientOcclusion) m_pTexAmbientOcclusion->Release();
	if (m_pSRVAmbientOcclusion) m_pSRVAmbientOcclusion->Release();
}
