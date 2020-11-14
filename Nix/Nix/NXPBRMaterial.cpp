#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "NXReflection.h"
#include "DirectXTex.h"

NXPBRMaterial::NXPBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR) :
	m_albedo(albedo),
	m_metallic(metallic),
	m_roughness(roughness),
	m_reflectivity(reflectivity),
	m_refractivity(refractivity),
	m_IOR(IOR)
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

void NXPBRMaterial::SetTexAlbedo(const std::wstring TexFilePath)
{
	SetTex(TexFilePath, m_pTexAlbedo, m_pSRVAlbedo);
}

void NXPBRMaterial::SetTexNormal(const std::wstring TexFilePath)
{
	SetTex(TexFilePath, m_pTexNormal, m_pSRVNormal);
}

void NXPBRMaterial::SetTexMetallic(const std::wstring TexFilePath)
{
	SetTex(TexFilePath, m_pTexMetallic, m_pSRVMetallic);
}

void NXPBRMaterial::SetTexRoughness(const std::wstring TexFilePath)
{
	SetTex(TexFilePath, m_pTexRoughness, m_pSRVRoughness);
}

void NXPBRMaterial::SetTexAO(const std::wstring TexFilePath)
{
	SetTex(TexFilePath, m_pTexAmbientOcclusion, m_pSRVAmbientOcclusion);
}

void NXPBRMaterial::SetTex(const std::wstring texFilePath, ComPtr<ID3D11Texture2D>& pTex, ComPtr<ID3D11ShaderResourceView>& pSRV)
{
	TexMetadata info;
	std::unique_ptr<ScratchImage> image = std::make_unique<ScratchImage>(); 

	HRESULT hr;
	std::wstring suffix = texFilePath.substr(texFilePath.find(L"."));
	if (_wcsicmp(suffix.c_str(), L".dds") == 0)
	{
		hr = LoadFromDDSFile(texFilePath.c_str(), DDS_FLAGS_NONE, &info, *image);
	}
	else if (_wcsicmp(suffix.c_str(), L".tga") == 0)
	{
		hr = LoadFromTGAFile(texFilePath.c_str(), &info, *image);
	}
	else if (_wcsicmp(suffix.c_str(), L".hdr") == 0)
	{
		hr = LoadFromHDRFile(texFilePath.c_str(), &info, *image);
	}
	else
	{
		hr = LoadFromWICFile(texFilePath.c_str(), WIC_FLAGS_NONE, &info, *image);
	}

	//std::wstring assetPath = L"D:\\NixAssets\\";
	//size_t assetFolderIndex = texFilePath.find(assetPath) + assetPath.length();
	//std::wstring texFolderPath = texFilePath.substr(assetFolderIndex, texFilePath.rfind(L"\\") - assetFolderIndex);
	////std::wstring texFolderPath = texFilePath.substr(assetFolderIndex, texFilePath.find(L".png") - assetFolderIndex);
	//std::wstring ddsFolderPath = L"D:\\Caches\\" + texFolderPath + L"\\";
	//std::wstring ddsFilePath = ddsFolderPath + L".dds";

	//hr = CreateDirectory(ddsFolderPath.c_str(), nullptr);
	//if (SUCCEEDED(hr))
	//{
	//	hr = SaveToDDSFile(image->GetImage(0, 0, 0), image->GetImageCount(), info, DDS_FLAGS_NONE, ddsFilePath.c_str());
	//}

	CD3D11_TEXTURE2D_DESC descTex(info.format, (UINT)info.width, (UINT)info.height, (UINT)info.arraySize, (UINT)info.mipLevels, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, (UINT)info.miscFlags);
	auto img = image->GetImage(0, 0, 0);
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = img->pixels;
	initData.SysMemPitch = static_cast<DWORD>(img->rowPitch);
	initData.SysMemSlicePitch = static_cast<DWORD>(img->slicePitch);
	g_pDevice->CreateTexture2D(&descTex, &initData, &pTex);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, info.format, 0, (UINT)info.mipLevels, 0, (UINT)info.arraySize);
	g_pDevice->CreateShaderResourceView(pTex.Get(), &descSRV, &pSRV);
}

void NXPBRMaterial::Release()
{
}
