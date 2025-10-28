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

Ntr<NXTexture> NXTextureResourceManager::CreateTextureAuto(const std::string& name, const std::filesystem::path& path, bool bForce, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	if (NXConvert::IsDDSFileExtension(path.extension().string()))
	{
		// 如果是DDS文件，需要判断是2D、2DArray、Cube中的哪种
		DirectX::TexMetadata metaData;
		if (NXConvert::GetMetadataFromFile(path, metaData))
		{
			if (metaData.IsCubemap())
			{
				// Cubemap
				return CreateTextureCube(name, path, flags, bAutoMakeViews);
			}
			else if (metaData.arraySize > 1)
			{
				// 2DArray
				return CreateTexture2DArray(name, path, flags, bAutoMakeViews);
			}
		}
	}

	return CreateTexture2D(name, path, bForce, flags, bAutoMakeViews);
}

Ntr<NXTexture2D> NXTextureResourceManager::CreateTexture2D(const std::string& name, const std::filesystem::path& filePath, bool bForce, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	if (!bForce)
	{
		// 先在已加载纹理里面找当前纹理，有的话就不用Create了
		for (auto pTexture : m_pTextureArrayInternal)
		{
			auto& pTex2D = pTexture.As<NXTexture2D>();
			if (pTex2D.IsValid() && !filePath.empty() 
				&& std::filesystem::hash_value(filePath) == std::filesystem::hash_value(pTexture->GetFilePath()) 
				&& !pTexture->IsSubRegion())
			{
				return pTex2D;
			}
		}
	}

	// 如果路径不存在，直接返回空指针
	if (!std::filesystem::exists(filePath))
	{
		return nullptr;
	}

	Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
	std::string strExt = filePath.extension().string();
	if (NXConvert::IsImageFileExtension(strExt))
	{
		pTexture2D->Create(name, filePath, flags);
	}
	else if (NXConvert::IsRawFileExtension(strExt))
	{
		pTexture2D->CreateHeightRaw(name, filePath, flags);
	}

	// 文件Tex2D的autoMakeView：只创建一个SRV
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

	// 2DRT的automakeView：创建一个SRV，如果作为RT使用，就创建RTV；如果作为DS使用，就创建DSV
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

Ntr<NXTexture2D> NXTextureResourceManager::CreateUAVTexture(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
	pTexture2D->CreateUAVTexture(name, fmt, width, height, flags);

	// 2DRT的automakeView：创建一个SRV，如果作为RT使用，就创建RTV；如果作为DS使用，就创建DSV
	if (bAutoMakeViews)
	{
		uint32_t uavCount = flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS? 1 : 0;
		uint32_t srvCount = 1;

		pTexture2D->SetViews(srvCount, 0, 0, uavCount);

		if (uavCount) pTexture2D->SetUAV(0);
		pTexture2D->SetSRV(0);
	}

	m_pTextureArrayInternal.push_back(pTexture2D);
	return pTexture2D;
}

Ntr<NXTexture2D> NXTextureResourceManager::CreateTexture2DSubRegion(const std::string& name, const std::filesystem::path& filePath, const Int2& subRegionXY, const Int2& subRegionSize, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	// 暂不判重，目前只有流式加载会用到这个接口
	
	// 如果路径不存在，直接返回空指针
	if (!std::filesystem::exists(filePath))
	{
		return nullptr;
	}

	Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
	std::string strExt = filePath.extension().string();
	if (NXConvert::IsImageFileExtension(strExt))
	{
		pTexture2D->Create(name, filePath, flags, true, subRegionXY, subRegionSize);
	}
	else if (NXConvert::IsRawFileExtension(strExt))
	{
		pTexture2D->CreateHeightRaw(name, filePath, flags, true, subRegionXY, subRegionSize);
	}

	// 文件Tex2D的autoMakeView：只创建一个SRV
	if (bAutoMakeViews)
	{
		pTexture2D->SetViews(1, 0, 0, 0);
		pTexture2D->SetSRV(0);
	}

	return pTexture2D;
}

Ntr<NXTextureCube> NXTextureResourceManager::CreateTextureCube(const std::string& name, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
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
	pTextureCube->Create(name, filePath, flags);

	// 文件 cubemap的automakeView：创建一个SRV
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

Ntr<NXTexture2DArray> NXTextureResourceManager::CreateTexture2DArray(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2DArray> pTexture2DArray(new NXTexture2DArray());
	pTexture2DArray->Create(debugName, filePath, flags);
	if (bAutoMakeViews)
	{
		pTexture2DArray->SetViews(1, 0, 0, 0);
		pTexture2DArray->SetSRV(0, 0, pTexture2DArray->GetArraySize());
	}
	m_pTextureArrayInternal.push_back(pTexture2DArray);
	return pTexture2DArray;
}

Ntr<NXTexture2DArray> NXTextureResourceManager::CreateTexture2DArray(const std::string& debugName, const std::wstring& filePath, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2DArray> pTexture2DArray(new NXTexture2DArray());
	pTexture2DArray->Create(debugName, filePath, texFormat, width, height, arraySize, mipLevels, flags);
	if (bAutoMakeViews)
	{
		pTexture2DArray->SetViews(1, 0, 0, 0);
		pTexture2DArray->SetSRV(0, 0, pTexture2DArray->GetArraySize());
	}
	m_pTextureArrayInternal.push_back(pTexture2DArray);
	return pTexture2DArray;
}

Ntr<NXTexture2DArray> NXTextureResourceManager::CreateRenderTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize, UINT mipLevels, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2DArray> pTexture2DArray(new NXTexture2DArray());
	pTexture2DArray->CreateRT(debugName, texFormat, width, height, arraySize, mipLevels, flags);
	m_pTextureArrayInternal.push_back(pTexture2DArray);
	return pTexture2DArray;
}

Ntr<NXTexture2DArray> NXTextureResourceManager::CreateUAVTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize, UINT mipLevels, D3D12_RESOURCE_FLAGS flags, bool bAutoMakeViews)
{
	Ntr<NXTexture2DArray> pTexture2DArray(new NXTexture2DArray());
	pTexture2DArray->CreateUAVTexture(debugName, texFormat, width, height, arraySize, mipLevels, flags);

	// 2DArray UAV的automakeView：总是创建一个SRV，如果允许UAV访问就创建UAV
	if (bAutoMakeViews)
	{
		uint32_t uavCount = flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ? 1 : 0;
		uint32_t srvCount = 1;

		pTexture2DArray->SetViews(srvCount, 0, 0, uavCount);

		if (uavCount) pTexture2DArray->SetUAV(0, 0, arraySize);
		pTexture2DArray->SetSRV(0, 0, arraySize);
	}

	m_pTextureArrayInternal.push_back(pTexture2DArray);
	return pTexture2DArray;
}

