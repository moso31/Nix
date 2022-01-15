#include "NXPBRMaterial.h"
#include <memory>
#include <direct.h>
#include "DirectXTex.h"
#include "NXResourceManager.h"

NXPBRMaterial::NXPBRMaterial(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao) :
	m_cbData(albedo, normal, metallic, roughness, ao),
	m_name(name),
	m_pTexAlbedo(nullptr),
	m_pTexNormal(nullptr),
	m_pTexMetallic(nullptr),
	m_pTexRoughness(nullptr),
	m_pTexAmbientOcclusion(nullptr)
{
	InitConstantBuffer();
}

void NXPBRMaterial::Update()
{
	// 材质只需要把自己的数据提交给GPU就行了。
	g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, &m_cbData, 0, 0);
}

void NXPBRMaterial::SetTexAlbedo(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexAlbedo) delete m_pTexAlbedo;
	m_pTexAlbedo = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterial::SetTexNormal(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexNormal) delete m_pTexNormal;
	m_pTexNormal = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterial::SetTexMetallic(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexMetallic) delete m_pTexMetallic;
	m_pTexMetallic = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterial::SetTexRoughness(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexRoughness) delete m_pTexRoughness;
	m_pTexRoughness = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

void NXPBRMaterial::SetTexAO(const std::wstring TexFilePath, bool GenerateMipMap)
{
	if (m_pTexAmbientOcclusion) delete m_pTexAmbientOcclusion;
	m_pTexAmbientOcclusion = LoadFromTexFile(TexFilePath, GenerateMipMap);
}

NXTexture2D* NXPBRMaterial::LoadFromTexFile(const std::wstring texFilePath, bool GenerateMipMap)
{
	TexMetadata info;
	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>(); 

	HRESULT hr;
	std::wstring suffix = texFilePath.substr(texFilePath.find(L"."));
	if (_wcsicmp(suffix.c_str(), L".dds") == 0)
	{
		hr = LoadFromDDSFile(texFilePath.c_str(), DDS_FLAGS_NONE, &info, *pImage);
	}
	else if (_wcsicmp(suffix.c_str(), L".tga") == 0)
	{
		hr = LoadFromTGAFile(texFilePath.c_str(), &info, *pImage);
	}
	else if (_wcsicmp(suffix.c_str(), L".hdr") == 0)
	{
		hr = LoadFromHDRFile(texFilePath.c_str(), &info, *pImage);
	}
	else
	{
		hr = LoadFromWICFile(texFilePath.c_str(), WIC_FLAGS_NONE, &info, *pImage);
	}

	// 自动生成MipMap
	if (GenerateMipMap && info.width >= 2 && info.height >= 2 && info.mipLevels == 1)
	{
		std::unique_ptr<ScratchImage> pImageMip = std::make_unique<ScratchImage>();
		hr = GenerateMipMaps(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *pImageMip);
		info.mipLevels = pImageMip->GetMetadata().mipLevels;
		if (SUCCEEDED(hr))
			pImage.swap(pImageMip);
	}

	D3D11_SUBRESOURCE_DATA* pImageData = new D3D11_SUBRESOURCE_DATA[info.mipLevels];
	for (size_t i = 0; i < info.mipLevels; i++)
	{
		auto img = pImage->GetImage(i, 0, 0);
		D3D11_SUBRESOURCE_DATA& pData = pImageData[i];
		pData.pSysMem = img->pixels;
		pData.SysMemPitch = static_cast<DWORD>(img->rowPitch);
		pData.SysMemSlicePitch = static_cast<DWORD>(img->slicePitch);
	}

	NXTexture2D* pOutTex = NXResourceManager::GetInstance()->CreateTexture2D(m_name.c_str(), pImageData, info.format, (UINT)info.width, (UINT)info.height, (UINT)info.arraySize, (UINT)info.mipLevels, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, (UINT)info.miscFlags);

	delete[] pImageData;

	pOutTex->CreateSRV();
	return pOutTex;
}

void NXPBRMaterial::Release()
{
	SafeDelete(m_pTexAlbedo);
	SafeDelete(m_pTexNormal);
	SafeDelete(m_pTexMetallic);
	SafeDelete(m_pTexRoughness);
	SafeDelete(m_pTexAmbientOcclusion);
}

void NXPBRMaterial::InitConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferMaterial);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cb));
}
