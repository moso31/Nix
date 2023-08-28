#include "NXTextureResourceManager.h"
#include "NXTexture.h"
#include "DirectResources.h"

NXTexture2D* NXTextureResourceManager::CreateTexture2D(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, nullptr, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.insert(pTexture2D);
	return pTexture2D;
}

NXTexture2D* NXTextureResourceManager::CreateTexture2D(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, initData, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.insert(pTexture2D);
	return pTexture2D;
}

NXTexture2D* NXTextureResourceManager::CreateTexture2D(const std::string& DebugName, const std::filesystem::path& filePath, bool bForce)
{
	if (!bForce)
	{
		// 先在已加载纹理里面找当前纹理，有的话就不用Create了
		for (auto pTexture : m_pTextureArray)
		{
			auto pTex2D = pTexture->Is2D();

			if (pTex2D && !filePath.empty() && std::filesystem::hash_value(filePath) == std::filesystem::hash_value(pTexture->GetFilePath()))
			{
				pTex2D->AddRef();
				return pTex2D;
			}
		}
	}

	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, filePath);
	pTexture2D->AddSRV();

	if (!bForce)
		m_pTextureArray.insert(pTexture2D); // 2023.3.26 如果是强制加载，就不应加入到资源数组里面
	return pTexture2D;
}

NXTextureCube* NXTextureResourceManager::CreateTextureCube(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTextureCube* pTextureCube = new NXTextureCube();
	pTextureCube->Create(DebugName, nullptr, TexFormat, Width, Height, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.insert(pTextureCube);
	return pTextureCube;
}

NXTextureCube* NXTextureResourceManager::CreateTextureCube(const std::string& DebugName, const std::wstring& filePath, UINT width, UINT height)
{
	// 先在已加载纹理里面找当前纹理，有的话就不用Create了
	for (auto pTexture : m_pTextureArray)
	{
		auto pTexCube = pTexture->IsCubeMap();
		if (pTexCube && !filePath.empty() && std::filesystem::hash_value(filePath) == std::filesystem::hash_value(pTexture->GetFilePath()))
		{
			pTexCube->AddRef();
			return pTexCube;
		}
	}

	NXTextureCube* pTextureCube = new NXTextureCube();
	pTextureCube->Create(DebugName, filePath, width, height);

	m_pTextureArray.insert(pTextureCube);
	return pTextureCube;
}

NXTexture2DArray* NXTextureResourceManager::CreateTexture2DArray(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2DArray* pTexture2DArray = new NXTexture2DArray();
	pTexture2DArray->Create(DebugName, nullptr, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.insert(pTexture2DArray);
	return pTexture2DArray;
}



void NXTextureResourceManager::InitCommonRT(const Vector2& rtSize)
{
	ResizeCommonRT(rtSize);
}

void NXTextureResourceManager::ResizeCommonRT(const Vector2& rtSize)
{
	for (auto& pRT : m_pCommonRT)
		if (pRT) pRT->RemoveRef();

	m_pCommonRT.clear();
	m_pCommonRT.resize(NXCommonRT_SIZE);

	m_pCommonRT[NXCommonRT_DepthZ] = CreateTexture2D("Scene DepthZ RT", DXGI_FORMAT_R24G8_TYPELESS, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_DepthZ]->AddDSV();
	m_pCommonRT[NXCommonRT_DepthZ]->AddSRV();

	m_pCommonRT[NXCommonRT_Lighting0] = CreateTexture2D("Lighting RT0", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_Lighting0]->AddRTV();
	m_pCommonRT[NXCommonRT_Lighting0]->AddSRV();

	m_pCommonRT[NXCommonRT_Lighting1] = CreateTexture2D("Lighting RT1", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_Lighting1]->AddRTV();
	m_pCommonRT[NXCommonRT_Lighting1]->AddSRV();

	m_pCommonRT[NXCommonRT_SSSLighting] = CreateTexture2D("SSS Lighting RT", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_SSSLighting]->AddRTV();
	m_pCommonRT[NXCommonRT_SSSLighting]->AddSRV();

	m_pCommonRT[NXCommonRT_ShadowTest] = CreateTexture2D("Shadow Test RT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_ShadowTest]->AddRTV();
	m_pCommonRT[NXCommonRT_ShadowTest]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer0] = CreateTexture2D("GBuffer RT0", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer0]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer0]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer1] = CreateTexture2D("GBuffer RT1", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer1]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer1]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer2] = CreateTexture2D("GBuffer RT2", DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer2]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer2]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer3] = CreateTexture2D("GBuffer RT3", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer3]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer3]->AddSRV();

	m_pCommonRT[NXCommonRT_PostProcessing] = CreateTexture2D("Post Processing", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)rtSize.x, (UINT)rtSize.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_PostProcessing]->AddRTV();
	m_pCommonRT[NXCommonRT_PostProcessing]->AddSRV();
}

NXTexture2D* NXTextureResourceManager::GetCommonRT(NXCommonRTEnum eRT)
{
	return m_pCommonRT.empty() ? nullptr : m_pCommonRT[eRT];
}

void NXTextureResourceManager::InitCommonTextures()
{
	m_pCommonTex.resize(NXCommonTex_SIZE);

	// 初始化常用贴图
	bool bIsCommonTexture = true;
	m_pCommonTex[NXCommonTex_White] = new NXTexture2D(bIsCommonTexture);
	m_pCommonTex[NXCommonTex_White]->Create("White Texture", g_defaultTex_white_wstr);
	m_pCommonTex[NXCommonTex_White]->AddSRV();

	m_pCommonTex[NXCommonTex_Normal] = new NXTexture2D(bIsCommonTexture);
	m_pCommonTex[NXCommonTex_Normal]->Create("Normal Texture", g_defaultTex_normal_wstr);
	m_pCommonTex[NXCommonTex_Normal]->AddSRV();
}

NXTexture2D* NXTextureResourceManager::GetCommonTextures(NXCommonTexEnum eTex)
{
	return m_pCommonTex[eTex];
}

void NXTextureResourceManager::ReleaseUnusedTextures()
{
	// 移除所有引用计数为0的纹理，并释放它们。
	std::erase_if(m_pTextureArray, [&](NXTexture* pTex) { 
		if (pTex->GetRef() == 0)
		{
			SafeRelease(pTex);
			return true;
		}
		return false;
	});
}

void NXTextureResourceManager::OnReload()
{
	for (auto pTex : m_pTextureArray)
	{
		if (!pTex) continue;

		// 2023.3.25 目前仅支持 Texture2D 的 Reload
		if (!pTex->Is2D()) continue;

		if (pTex->GetReloadingState() == NXTextureReloadingState::Texture_StartReload)
		{
			pTex->SwapToReloadingTexture();
			pTex->SetReloadingState(NXTextureReloadingState::Texture_Reloading);

			bool bAsync = true;
			if (bAsync)
			{
				auto LoadTextureAsyncTask = pTex->LoadTextureAsync(); // 异步加载纹理。
				LoadTextureAsyncTask.m_handle.promise().m_callbackFunc = [pTex]() { pTex->OnReloadFinish(); };
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
	for (auto pTex : m_pTextureArray) SafeRelease(pTex);
	for (auto pTex : m_pCommonTex) SafeRelease(pTex);
}

void NXTextureResourceManager::CreateTexture2D_Internal(const std::string& name, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	Ntr<NXTexture2D> pTexture2D = new NXTexture2D();
	pTexture2D->Create(name, nullptr, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArrayInternal.insert(pTexture2D);
}
