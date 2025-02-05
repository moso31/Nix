#include "NXTextureResourceManager.h"
#include "BaseDefs/NixCore.h"
#include "NXTexture.h"
#include "DirectResources.h"
#include "NXResourceManager.h"

void NXTextureResourceManager::InitCommonTextures()
{
	m_pCommonTex.resize(NXCommonTex_SIZE);

	m_pCommonTex[NXCommonTex_White] = CreateTexture2D("White Texture", g_defaultTex_white_wstr);
	m_pCommonTex[NXCommonTex_Normal] = CreateTexture2D("Normal Texture", g_defaultTex_normal_wstr);

	// Noise 2D Gray, 64x64
	Ntr<NXTexture2D> pTex(new NXTexture2D());
	pTex->CreateNoise("Noise2DGray 64x64", 64, 1);
	pTex->SetViews(1, 0, 0, 0);
	pTex->SetSRV(0);
	m_pCommonTex[NXCommonTex_Noise2DGray_64x64] = pTex;
}

Ntr<NXTexture2D> NXTextureResourceManager::GetCommonTextures(NXCommonTexEnum eTex)
{
	return m_pCommonTex[eTex];
}

void NXTextureResourceManager::OnReload()
{
	for (auto& pTex : m_pTextureArrayInternal)
	{
		if (pTex.IsNull()) continue;

		// 2023.3.25 目前仅支持 Texture2D 的 Reload
		if (pTex.As<NXTexture2D>().IsNull()) continue;

		pTex->ReloadCheck();
	}
}

void NXTextureResourceManager::Release()
{
}

Ntr<NXTexture2D> NXTextureResourceManager::CreateTexture2D(const std::string& name, const std::filesystem::path& filePath, bool bForce, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	if (!bForce)
	{
		// 先在已加载纹理里面找当前纹理，有的话就不用Create了
		for (auto pTexture : m_pTextureArrayInternal)
		{
			auto& pTex2D = pTexture.As<NXTexture2D>();
			if (pTex2D.IsValid() && !filePath.empty() && std::filesystem::hash_value(filePath) == std::filesystem::hash_value(pTexture->GetFilePath()))
			{
				return pTex2D;
			}
		}
	}

	Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
	pTexture2D->Create(name, filePath, flags);

	if (bAutoMakeViews)
	{
		pTexture2D->SetViews(1, 0, 0, 0);
		pTexture2D->SetSRV(0);
	}

	if (!bForce)
		m_pTextureArrayInternal.push_back(pTexture2D); // 2023.3.26 如果是强制加载，就不应加入到资源数组里面
	return pTexture2D;
}

Ntr<NXTexture2D> NXTextureResourceManager::CreateRenderTexture(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
	pTexture2D->CreateRenderTexture(name, fmt, width, height, flags);

	if (bAutoMakeViews)
	{
		uint32_t rtvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? 1 : 0;
		uint32_t dsvCount = flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ? 1 : 0;
		uint32_t srvCount = 1;

		pTexture2D->SetViews(srvCount, rtvCount, dsvCount, 0);

		if (rtvCount) pTexture2D->SetRTV(0);
		if (dsvCount) pTexture2D->SetDSV(0);
		pTexture2D->SetSRV(0);
	}

	m_pTextureArrayInternal.push_back(pTexture2D);
	return pTexture2D;
}

Ntr<NXTextureCube> NXTextureResourceManager::CreateTextureCube(const std::string& name, const std::wstring& filePath, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	// 先在已加载纹理里面找当前纹理，有的话就不用Create了
	for (auto& pTexture : m_pTextureArrayInternal)
	{
		auto& pTexCube = pTexture.As<NXTextureCube>();
		if (pTexCube.Ptr() && !filePath.empty() && std::filesystem::hash_value(filePath) == std::filesystem::hash_value(pTexture->GetFilePath()))
		{
			return pTexCube;
		}
	}

	Ntr<NXTextureCube> pTextureCube = new NXTextureCube();
	pTextureCube->Create(name, filePath, width, height, flags);

	if (bAutoMakeViews)
	{
		pTextureCube->SetViews(1, 0, 0, 0);
		pTextureCube->SetSRV(0);
	}

	m_pTextureArrayInternal.push_back(pTextureCube);
	return pTextureCube;
}

Ntr<NXTextureCube> NXTextureResourceManager::CreateTextureCube(const std::string& name, DXGI_FORMAT texFormat, UINT width, UINT height, UINT mipLevels, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTextureCube> pTextureCube(new NXTextureCube());
	pTextureCube->Create(name, texFormat, width, height, mipLevels, flags);
	m_pTextureArrayInternal.push_back(pTextureCube);
	return pTextureCube;
}

Ntr<NXTexture2DArray> NXTextureResourceManager::CreateTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize, UINT mipLevels, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2DArray> pTexture2DArray(new NXTexture2DArray());
	pTexture2DArray->Create(debugName, texFormat, width, height, arraySize, mipLevels, flags);
	m_pTextureArrayInternal.push_back(pTexture2DArray);
	return pTexture2DArray;
}

