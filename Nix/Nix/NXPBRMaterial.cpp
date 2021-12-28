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

void NXPBRMaterial::SetTexAlbedo(const std::wstring TexFilePath)
{
	m_pTexAlbedo = LoadFromTexFile(TexFilePath);
}

void NXPBRMaterial::SetTexNormal(const std::wstring TexFilePath)
{
	m_pTexNormal = LoadFromTexFile(TexFilePath);
}

void NXPBRMaterial::SetTexMetallic(const std::wstring TexFilePath)
{
	m_pTexMetallic = LoadFromTexFile(TexFilePath);
}

void NXPBRMaterial::SetTexRoughness(const std::wstring TexFilePath)
{
	m_pTexRoughness = LoadFromTexFile(TexFilePath);
}

void NXPBRMaterial::SetTexAO(const std::wstring TexFilePath)
{
	m_pTexAmbientOcclusion = LoadFromTexFile(TexFilePath);
}

NXTexture2D* NXPBRMaterial::LoadFromTexFile(const std::wstring texFilePath)
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

	auto img = image->GetImage(0, 0, 0);
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = img->pixels;
	initData.SysMemPitch = static_cast<DWORD>(img->rowPitch);
	initData.SysMemSlicePitch = static_cast<DWORD>(img->slicePitch);

	NXTexture2D* pOutTex = NXResourceManager::GetInstance()->CreateTexture2D(m_name.c_str(), &initData, info.format, (UINT)info.width, (UINT)info.height, (UINT)info.arraySize, (UINT)info.mipLevels, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, (UINT)info.miscFlags);
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
