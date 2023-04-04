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
		// �����Ѽ������������ҵ�ǰ�������еĻ��Ͳ���Create��
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
		m_pTextureArray.insert(pTexture2D); // 2023.3.26 �����ǿ�Ƽ��أ��Ͳ�Ӧ���뵽��Դ��������
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
	// �����Ѽ������������ҵ�ǰ�������еĻ��Ͳ���Create��
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



void NXTextureResourceManager::InitCommonRT()
{
	Vector2 sz = g_dxResources->GetViewSize();

	m_pCommonRT.resize(NXCommonRT_SIZE);

	m_pCommonRT[NXCommonRT_DepthZ] = CreateTexture2D("Scene DepthZ RT", DXGI_FORMAT_R24G8_TYPELESS, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_DepthZ]->AddDSV();
	m_pCommonRT[NXCommonRT_DepthZ]->AddSRV();

	m_pCommonRT[NXCommonRT_MainScene] = CreateTexture2D("Scene RT0", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_MainScene]->AddRTV();
	m_pCommonRT[NXCommonRT_MainScene]->AddSRV();

	m_pCommonRT[NXCommonRT_ShadowTest] = CreateTexture2D("Shadow Test RT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_ShadowTest]->AddRTV();
	m_pCommonRT[NXCommonRT_ShadowTest]->AddSRV();

	// ����G-Buffer�ṹ���£�
	// RT0:		Position				R32G32B32A32_FLOAT
	// RT1:		Normal					R32G32B32A32_FLOAT
	// RT2:		Albedo					R10G10B10A2_UNORM
	// RT3:		Metallic+Roughness+AO	R10G10B10A2_UNORM
	// *ע�⣺����RT0��RT1�����õ���128λ������������ֻ����ʱ������RT2��RT3Ҳ�д���ȶ��

	m_pCommonRT[NXCommonRT_GBuffer0] = CreateTexture2D("GBuffer RT0", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer0]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer0]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer1] = CreateTexture2D("GBuffer RT1", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer1]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer1]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer2] = CreateTexture2D("GBuffer RT2", DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer2]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer2]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer3] = CreateTexture2D("GBuffer RT3", DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer3]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer3]->AddSRV();

	m_pCommonRT[NXCommonRT_PostProcessing] = CreateTexture2D("Post Processing", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_PostProcessing]->AddRTV();
	m_pCommonRT[NXCommonRT_PostProcessing]->AddSRV();
}

NXTexture2D* NXTextureResourceManager::GetCommonRT(NXCommonRTEnum eRT)
{
	return m_pCommonRT[eRT];
}

void NXTextureResourceManager::InitCommonTextures()
{
	m_pCommonTex.resize(NXCommonTex_SIZE);

	// ��ʼ��������ͼ
	m_pCommonTex[NXCommonTex_White] = CreateTexture2D("White Texture", g_defaultTex_white_wstr);
	m_pCommonTex[NXCommonTex_Normal] = CreateTexture2D("Normal Texture", g_defaultTex_normal_wstr);
}

NXTexture2D* NXTextureResourceManager::GetCommonTextures(NXCommonTexEnum eTex)
{
	return m_pCommonTex[eTex];
}

TextureNXInfo* NXTextureResourceManager::LoadTextureInfo(const std::filesystem::path& texFilePath)
{
	auto pResult = new TextureNXInfo();

	std::string strPath = texFilePath.string().c_str();
	std::string strNXInfoPath = strPath + ".nxInfo";

	std::ifstream ifs(strNXInfoPath, std::ios::binary);

	// nxInfo ·�����û�򿪣��ͷ���һ������ֵ����Ĭ��ֵ�� info
	if (!ifs.is_open())
		return pResult;

	std::string strIgnore;

	size_t nHashFile;
	ifs >> nHashFile;
	std::getline(ifs, strIgnore);

	// �������Ԫ�ļ���������·����ϣ��������Դ��������ƥ�䣬�����Ԫ�ļ���ʧЧ�ġ�����Ĭ��Ԫ�ļ���
	size_t nHashPath = std::filesystem::hash_value(texFilePath);
	if (nHashFile != nHashPath)
	{
		printf("Warning: TextureInfoData of %s has founded, but couldn't be open. Consider delete that file.\n", strPath.c_str());
		return pResult;
	}

	// ��ͨ������ȫ����������ʹ��Ԫ�ļ��洢������
	ifs >> pResult->nTexType >> pResult->bSRGB >> pResult->bInvertNormalY >> pResult->bGenerateMipMap >> pResult->bCubeMap;
	std::getline(ifs, strIgnore);

	ifs.close();

	return pResult;
}

void NXTextureResourceManager::SaveTextureInfo(const TextureNXInfo* pInfo, const std::filesystem::path& texFilePath)
{
	if (texFilePath.empty())
	{
		printf("Warning: can't save TextureNXInfo for %s, path does not exist.\n", texFilePath.string().c_str());
		return;
	}

	std::string strPathInfo = texFilePath.string() + ".nxInfo";

	std::ofstream ofs(strPathInfo, std::ios::binary);

	// 2023.3.22
	// ������Դ��Ԫ�ļ���*.nxInfo���洢��
	// 1. �����ļ�·���Ĺ�ϣ
	// 2. (int)TextureType, (int)IsSRGB, (int)IsInvertNormalY, (int)IsGenerateCubeMap, (int)IsCubeMap

	size_t pathHashValue = std::filesystem::hash_value(texFilePath);
	ofs << pathHashValue << std::endl;

	ofs << pInfo->nTexType << ' ' << (int)pInfo->bSRGB << ' ' << (int)pInfo->bInvertNormalY << ' ' << (int)pInfo->bGenerateMipMap << ' ' << (int)pInfo->bCubeMap << std::endl;

	ofs.close();
}

void NXTextureResourceManager::ReleaseUnusedTextures()
{
	// �Ƴ��������ü���Ϊ0�����������ͷ����ǡ�
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

		// 2023.3.25 Ŀǰ��֧�� Texture2D �� Reload
		if (!pTex->Is2D()) continue;

		if (pTex->GetReloadingState() == NXTextureReloadingState::Texture_StartReload)
		{
			pTex->SwapToReloadingTexture();
			pTex->SetReloadingState(NXTextureReloadingState::Texture_Reloading);

			bool bAsync = true;
			if (bAsync)
			{
				auto LoadTextureAsyncTask = pTex->LoadTextureAsync(); // �첽����������
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
}