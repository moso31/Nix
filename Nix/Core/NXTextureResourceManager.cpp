#include "NXTextureResourceManager.h"
#include "BaseDefs/NixCore.h"
#include "NXTexture.h"
#include "DirectResources.h"

void NXTextureResourceManager::InitCommonRT(const Vector2& rtSize)
{
	ResizeCommonRT(rtSize);
}

void NXTextureResourceManager::ResizeCommonRT(const Vector2& rtSize)
{
	m_pCommonRT.clear();
	m_pCommonRT.resize(NXCommonRT_SIZE);

	m_pCommonRT[NXCommonRT_DepthZ] = CreateRenderTexture("Scene DepthZ RT", DXGI_FORMAT_R24G8_TYPELESS, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	m_pCommonRT[NXCommonRT_DepthZ]->AddDSV();
	m_pCommonRT[NXCommonRT_DepthZ]->AddSRV();

	m_pCommonRT[NXCommonRT_DepthZ_R32] = CreateRenderTexture("Scene DepthZ R32 RT", DXGI_FORMAT_R32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_DepthZ_R32]->AddRTV();
	m_pCommonRT[NXCommonRT_DepthZ_R32]->AddSRV();

	m_pCommonRT[NXCommonRT_Lighting0] = CreateRenderTexture("Lighting RT0", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_Lighting0]->AddRTV();
	m_pCommonRT[NXCommonRT_Lighting0]->AddSRV();

	m_pCommonRT[NXCommonRT_Lighting1] = CreateRenderTexture("Lighting RT1", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_Lighting1]->AddRTV();
	m_pCommonRT[NXCommonRT_Lighting1]->AddSRV();

	m_pCommonRT[NXCommonRT_Lighting2] = CreateRenderTexture("Lighting RT2", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_Lighting2]->AddRTV();
	m_pCommonRT[NXCommonRT_Lighting2]->AddSRV();

	m_pCommonRT[NXCommonRT_SSSLighting] = CreateRenderTexture("SSS Lighting RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_SSSLighting]->AddRTV();
	m_pCommonRT[NXCommonRT_SSSLighting]->AddSRV();

	m_pCommonRT[NXCommonRT_ShadowTest] = CreateRenderTexture("Shadow Test RT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_ShadowTest]->AddRTV();
	m_pCommonRT[NXCommonRT_ShadowTest]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer0] = CreateRenderTexture("GBuffer RT0", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_GBuffer0]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer0]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer1] = CreateRenderTexture("GBuffer RT1", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_GBuffer1]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer1]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer2] = CreateRenderTexture("GBuffer RT2", DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_GBuffer2]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer2]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer3] = CreateRenderTexture("GBuffer RT3", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_GBuffer3]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer3]->AddSRV();

	m_pCommonRT[NXCommonRT_PostProcessing] = CreateRenderTexture("Post Processing", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_PostProcessing]->AddRTV();
	m_pCommonRT[NXCommonRT_PostProcessing]->AddSRV();

	m_pCommonRT[NXCommonRT_DebugLayer] = CreateRenderTexture("Debug Layer Out RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pCommonRT[NXCommonRT_DebugLayer]->AddRTV();
	m_pCommonRT[NXCommonRT_DebugLayer]->AddSRV();
}

Ntr<NXTexture2D> NXTextureResourceManager::GetCommonRT(NXCommonRTEnum eRT)
{
	return m_pCommonRT.empty() ? nullptr : m_pCommonRT[eRT];
}

void NXTextureResourceManager::InitCommonTextures()
{
	m_pCommonTex.reserve(NXCommonTex_SIZE);

	// NXCommonTex_White
	auto pTex = m_pCommonTex.emplace_back(new NXTexture2D());
	pTex->Create("White Texture", g_defaultTex_white_wstr);
	pTex->AddSRV();

	// NXCommonTex_Normal
	pTex = m_pCommonTex.emplace_back(new NXTexture2D());
	pTex->Create("Normal Texture", g_defaultTex_normal_wstr);
	pTex->AddSRV();

	// Noise 2D Gray, 64x64
	pTex = m_pCommonTex.emplace_back(new NXTexture2D());
	pTex->CreateNoise("Noise2DGray 64x64", 64, 1);
	pTex->AddSRV();
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

		if (pTex->GetReloadingState() == NXTextureReloadingState::Texture_StartReload)
		{
			pTex->SwapToReloadingTexture();
			pTex->SetReloadingState(NXTextureReloadingState::Texture_Reloading);

			bool bAsync = true;
			if (bAsync)
			{
				auto LoadTextureAsyncTask = pTex->LoadTextureAsync(); // 异步加载纹理。

				LoadTextureAsyncTask.m_handle.promise().m_callbackFunc = [&pTex]() { pTex->OnReloadFinish(); };
			}
			else
			{
				pTex->LoadTextureSync();
				pTex->OnReloadFinish();
			}

			continue;
		}

		if (pTex->GetReloadingState() == NXTextureReloadingState::Texture_FinishReload)
		{
			pTex->SwapToReloadingTexture();
			pTex->SetReloadingState(NXTextureReloadingState::Texture_None);
			continue;
		}
	}
}

void NXTextureResourceManager::Release()
{
}

Ntr<NXTexture2D> NXTextureResourceManager::CreateTexture2D(const std::string& name, const std::filesystem::path& filePath, bool bForce, D3D12_RESOURCE_FLAGS flags)
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
	pTexture2D->AddSRV();

	if (!bForce)
		m_pTextureArrayInternal.push_back(pTexture2D); // 2023.3.26 如果是强制加载，就不应加入到资源数组里面
	return pTexture2D;
}

Ntr<NXTexture2D> NXTextureResourceManager::CreateRenderTexture(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags)
{
	Ntr<NXTexture2D> pTexture2D(new NXTexture2D());
	pTexture2D->CreateRenderTexture(name, fmt, width, height, flags);
	m_pTextureArrayInternal.push_back(pTexture2D);
	return pTexture2D;
}

Ntr<NXTextureCube> NXTextureResourceManager::CreateTextureCube(const std::string& name, const std::wstring& filePath, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags)
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
	m_pTextureArrayInternal.push_back(pTextureCube);
	return pTextureCube;
}

Ntr<NXTextureCube> NXTextureResourceManager::CreateTextureCube(const std::string& name, DXGI_FORMAT texFormat, UINT width, UINT height, UINT mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	Ntr<NXTextureCube> pTextureCube(new NXTextureCube());
	pTextureCube->Create(name, texFormat, width, height, mipLevels, flags);
	m_pTextureArrayInternal.push_back(pTextureCube);
	return pTextureCube;
}

Ntr<NXTexture2DArray> NXTextureResourceManager::CreateTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize, UINT mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	Ntr<NXTexture2DArray> pTexture2DArray(new NXTexture2DArray());
	pTexture2DArray->Create(debugName, texFormat, width, height, arraySize, mipLevels, flags);
	m_pTextureArrayInternal.push_back(pTexture2DArray);
	return pTexture2DArray;
}

