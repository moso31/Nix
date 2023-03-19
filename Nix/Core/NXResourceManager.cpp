#include "NXResourceManager.h"
#include "DirectResources.h"
#include "NXConverter.h"
#include "DirectXTex.h"
#include <fstream>

TextureNXInfo::TextureNXInfo(const TextureNXInfo& info) :
	nTexType(info.nTexType),
	TexFormat(info.TexFormat),
	Width(info.Width),
	Height(info.Height),
	bSRGB(info.bSRGB),
	bInvertNormalY(info.bInvertNormalY),
	bGenerateMipMap(info.bGenerateMipMap),
	bCubeMap(info.bCubeMap)
{
}
//
//TextureNXInfo::TextureNXInfo(const TextureNXInfo&& info) noexcept :
//	nTexType(info.nTexType),
//	TexFormat(info.TexFormat),
//	Width(info.Width),
//	Height(info.Height),
//	bSRGB(info.bSRGB),
//	bInvertNormalY(info.bInvertNormalY),
//	bGenerateMipMap(info.bGenerateMipMap),
//	bCubeMap(info.bCubeMap)
//{
//}
//
//TextureNXInfo& TextureNXInfo::operator=(TextureNXInfo&& info)
//{
//	nTexType = info.nTexType;
//	TexFormat = info.TexFormat;
//	Width = info.Width;
//	Height = info.Height;
//	bSRGB = info.bSRGB;
//	bInvertNormalY = info.bInvertNormalY;
//	bGenerateMipMap = info.bGenerateMipMap;
//	bCubeMap = info.bCubeMap;
//	return *this;
//}

NXResourceManager::NXResourceManager()
{
}

NXResourceManager::~NXResourceManager()
{
}

NXTexture2D* NXResourceManager::CreateTexture2D(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, nullptr, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);
	
	m_pTextureArray.push_back(pTexture2D);
	return pTexture2D;
}

NXTexture2D* NXResourceManager::CreateTexture2D(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, initData, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.push_back(pTexture2D);
	return pTexture2D;
}

NXTexture2D* NXResourceManager::CreateTexture2D(const std::string& DebugName, const std::filesystem::path& FilePath)
{
	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, FilePath);

	m_pTextureArray.push_back(pTexture2D);
	return pTexture2D;
}

NXTextureCube* NXResourceManager::CreateTextureCube(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTextureCube* pTextureCube = new NXTextureCube();
	pTextureCube->Create(DebugName, nullptr, TexFormat, Width, Height, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.push_back(pTextureCube);
	return pTextureCube;
}

NXTextureCube* NXResourceManager::CreateTextureCube(const std::string& DebugName, const std::wstring& strFilePath, UINT width, UINT height)
{
	NXTextureCube* pTextureCube = new NXTextureCube();
	pTextureCube->Create(DebugName, strFilePath, width, height);

	m_pTextureArray.push_back(pTextureCube);
	return pTextureCube;
}

NXTexture2DArray* NXResourceManager::CreateTexture2DArray(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2DArray* pTexture2DArray = new NXTexture2DArray();
	pTexture2DArray->Create(DebugName, nullptr, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	m_pTextureArray.push_back(pTexture2DArray);
	return pTexture2DArray;
}

void NXResourceManager::InitCommonRT()
{
	Vector2 sz = g_dxResources->GetViewSize();

	m_pCommonRT.resize(NXCommonRT_SIZE);

	m_pCommonRT[NXCommonRT_DepthZ] = NXResourceManager::GetInstance()->CreateTexture2D("Scene DepthZ RT", DXGI_FORMAT_R24G8_TYPELESS, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_DepthZ]->AddDSV();
	m_pCommonRT[NXCommonRT_DepthZ]->AddSRV();

	m_pCommonRT[NXCommonRT_MainScene] = NXResourceManager::GetInstance()->CreateTexture2D("Scene RT0", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_MainScene]->AddRTV();
	m_pCommonRT[NXCommonRT_MainScene]->AddSRV();

	m_pCommonRT[NXCommonRT_ShadowTest] = NXResourceManager::GetInstance()->CreateTexture2D("Shadow Test RT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pCommonRT[NXCommonRT_ShadowTest]->AddRTV();
	m_pCommonRT[NXCommonRT_ShadowTest]->AddSRV();

	// 现行G-Buffer结构如下：
	// RT0:		Position				R32G32B32A32_FLOAT
	// RT1:		Normal					R32G32B32A32_FLOAT
	// RT2:		Albedo					R10G10B10A2_UNORM
	// RT3:		Metallic+Roughness+AO	R10G10B10A2_UNORM
	// *注意：上述RT0、RT1现在用的是128位浮点数——这只是临时方案。RT2、RT3也有待商榷。

	m_pCommonRT[NXCommonRT_GBuffer0] = NXResourceManager::GetInstance()->CreateTexture2D("GBuffer RT0", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer0]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer0]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer1] = NXResourceManager::GetInstance()->CreateTexture2D("GBuffer RT1", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer1]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer1]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer2] = NXResourceManager::GetInstance()->CreateTexture2D("GBuffer RT2", DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer2]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer2]->AddSRV();

	m_pCommonRT[NXCommonRT_GBuffer3] = NXResourceManager::GetInstance()->CreateTexture2D("GBuffer RT3", DXGI_FORMAT_R10G10B10A2_UNORM, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_GBuffer3]->AddRTV();
	m_pCommonRT[NXCommonRT_GBuffer3]->AddSRV();

	m_pCommonRT[NXCommonRT_PostProcessing] = NXResourceManager::GetInstance()->CreateTexture2D("Post Processing", DXGI_FORMAT_R11G11B10_FLOAT, (UINT)sz.x, (UINT)sz.y, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, 0);
	m_pCommonRT[NXCommonRT_PostProcessing]->AddRTV();
	m_pCommonRT[NXCommonRT_PostProcessing]->AddSRV();
}

NXTexture2D* NXResourceManager::GetCommonRT(NXCommonRTEnum eRT)
{
	return m_pCommonRT[eRT];
}

void NXResourceManager::Release()
{
	for (auto pTex : m_pTextureArray) SafeRelease(pTex);
}

TextureNXInfo* NXTexture::LoadTextureNXInfo(const std::filesystem::path& filePath)
{
	if (filePath.empty())
		return nullptr;

	std::string strPath = filePath.string().c_str();
	std::string strNXInfoPath = strPath + ".nxInfo";

	std::ifstream ifs(strNXInfoPath, std::ios::binary);

	// nxInfo 路径如果没打开，就返回一个所有值都给默认值的 InfoData
	// 2023.3.18（不算读取失败，很多纹理没有nxInfo文件。）
	if (!ifs.is_open())
	{
		return nullptr;
	}

	std::string strIgnore;

	size_t nHashFile;
	ifs >> nHashFile;
	std::getline(ifs, strIgnore);

	// 如果 纹理所存的文件路径的哈希nHashPath 和 nxInfo存储的路径哈希nHashFile
	// 对不上，则读取失败，返回默认 InfoData
	size_t nHashPath = std::filesystem::hash_value(filePath);
	if (nHashFile != nHashPath)
	{
		printf("Warning: TextureInfoData of %s has founded, but couldn't be open. Consider delete that file.\n", filePath.string().c_str());
		return nullptr;
	}

	TextureNXInfo* pNXInfo = new TextureNXInfo();
	ifs >> pNXInfo->TexFormat >> pNXInfo->Width >> pNXInfo->Height;
	std::getline(ifs, strIgnore);

	ifs >> pNXInfo->nTexType >> pNXInfo->bSRGB >> pNXInfo->bInvertNormalY >> pNXInfo->bGenerateMipMap >> pNXInfo->bCubeMap;
	std::getline(ifs, strIgnore);

	ifs.close();

	return pNXInfo;
}

void NXTexture::RemoveRef()
{
	m_nRefCount--; 
	if (!m_nRefCount) Release();
}

void NXTexture::Release()
{
	SafeDelete(m_pTexNXInfo);
}

void NXTexture2D::Create(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	this->m_debugName = DebugName;
	this->m_width = Width;
	this->m_height = Height;
	this->m_arraySize = ArraySize;
	this->m_texFormat = TexFormat;
	this->m_mipLevels = MipLevels;

	D3D11_TEXTURE2D_DESC Desc;
	Desc.Format = TexFormat;
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.ArraySize = ArraySize;
	Desc.MipLevels = MipLevels;
	Desc.BindFlags = BindFlags;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CpuAccessFlags;
	Desc.SampleDesc.Count = SampleCount;
	Desc.SampleDesc.Quality = SampleQuality;
	Desc.MiscFlags = MiscFlags;

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&Desc, initData, &m_pTexture));
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	AddRef();
}

void NXTexture2D::Create(const std::string& DebugName, const std::filesystem::path& filePath)
{
	TexMetadata metadata;
	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();

	HRESULT hr;
	std::string strExtension = NXConvert::s2lower(filePath.extension().string());
	if (strExtension == ".hdr") 
		hr = LoadFromHDRFile(filePath.c_str(), &metadata, *pImage);
	else if (strExtension == ".dds") 
		hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &metadata, *pImage);
	else if (strExtension == ".tga")
		hr = LoadFromTGAFile(filePath.c_str(), &metadata, *pImage);
	else 
		hr = LoadFromWICFile(filePath.c_str(), WIC_FLAGS_NONE, &metadata, *pImage);

	if (FAILED(hr))
		return;

	m_texFilePath = filePath;

	SafeDelete(m_pTexNXInfo)
		m_pTexNXInfo = LoadTextureNXInfo(filePath);
	if (!m_pTexNXInfo)
		m_pTexNXInfo = new TextureNXInfo();

	// 如果读取的是arraySize/TextureCube，就只读取ArraySize[0]/X+面。
	if (metadata.arraySize > 1)
	{
		std::unique_ptr<ScratchImage> timage(new ScratchImage);
		timage->InitializeFromImage(*pImage->GetImage(0, 0, 0));
		metadata = timage->GetMetadata();
		pImage.swap(timage);
	}

	// --- Convert -----------------------------------------------------------------
	if (IsSRGB(metadata.format) != m_pTexNXInfo->bSRGB)
	{
		std::unique_ptr<ScratchImage> timage(new ScratchImage);

		DXGI_FORMAT tFormat = m_pTexNXInfo->bSRGB ? NXConvert::ForceSRGB(metadata.format) : NXConvert::ForceNoSRGB(metadata.format);
		TEX_FILTER_FLAGS texFlags = m_pTexNXInfo->bSRGB ? TEX_FILTER_SRGB_IN : TEX_FILTER_DEFAULT;
		hr = Convert(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), tFormat, texFlags, TEX_THRESHOLD_DEFAULT, *timage);
		if (SUCCEEDED(hr))
		{
			metadata.format = tFormat;
		}
		else
		{
			printf("Warning: [Convert] failed when loading NXTextureCube file: %ws\n", filePath.c_str());
		}
		pImage.swap(timage);
	}

	// --- Invert Y Channel --------------------------------------------------------
	if (m_pTexNXInfo->bInvertNormalY)
	{
		std::unique_ptr<ScratchImage> timage(new ScratchImage);

		HRESULT hr = TransformImage(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(),
			[&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t w, size_t y)
			{
				static const XMVECTORU32 s_selecty = { { { XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0 } } };
				UNREFERENCED_PARAMETER(y);

				for (size_t j = 0; j < w; ++j)
				{
					const XMVECTOR value = inPixels[j];
					const XMVECTOR inverty = XMVectorSubtract(g_XMOne, value);
					outPixels[j] = XMVectorSelect(value, inverty, s_selecty);
				}
			}, *timage);

		if (FAILED(hr))
		{
			printf("Warning: [InvertNormalY] failed when loading NXTextureCube file: %ws\n", filePath.c_str());
		}

		pImage.swap(timage);
	}

	if (m_pTexNXInfo->bGenerateMipMap && metadata.width >= 2 && metadata.height >= 2 && metadata.mipLevels == 1)
	{
		std::unique_ptr<ScratchImage> pImageMip = std::make_unique<ScratchImage>();
		HRESULT hr = GenerateMipMaps(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *pImageMip);
		if (SUCCEEDED(hr))
		{
			metadata.mipLevels = pImageMip->GetMetadata().mipLevels;
			pImage.swap(pImageMip);
		}
		else
		{
			printf("Warning: [GenerateMipMap] failed when loading NXTextureCube file: %ws\n", filePath.c_str());
		}
	}

	this->m_texFilePath = filePath.c_str();
	this->m_debugName = DebugName;
	this->m_width = (UINT)metadata.width;
	this->m_height = (UINT)metadata.height;
	this->m_arraySize = (UINT)metadata.arraySize;
	this->m_mipLevels = (UINT)metadata.mipLevels;
	this->m_texFormat = metadata.format;

	DirectX::CreateTextureEx(g_pDevice.Get(), pImage->GetImage(0, 0, 0), pImage->GetImageCount(), metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, false, CREATETEX_DEFAULT, (ID3D11Resource**)m_pTexture.GetAddressOf());
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	AddRef();
}

void NXTexture2D::AddSRV()
{
	ID3D11ShaderResourceView* pSRV;

	DXGI_FORMAT SRVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	CD3D11_SHADER_RESOURCE_VIEW_DESC Desc(D3D11_SRV_DIMENSION_TEXTURE2D, SRVFormat, 0, m_mipLevels);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexture.Get(), &Desc, &pSRV));

	std::string SRVDebugName = m_debugName + " SRV";
	pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());

	m_pSRVs.push_back(pSRV);
}

void NXTexture2D::AddRTV()
{
	ID3D11RenderTargetView* pRTV;

	CD3D11_RENDER_TARGET_VIEW_DESC Desc(D3D11_RTV_DIMENSION_TEXTURE2D, m_texFormat);
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexture.Get(), &Desc, &pRTV));

	std::string RTVDebugName = m_debugName + " RTV";
	pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)RTVDebugName.size(), RTVDebugName.c_str());

	m_pRTVs.push_back(pRTV);
}

void NXTexture2D::AddDSV()
{
	ID3D11DepthStencilView* pDSV;

	DXGI_FORMAT DSVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	CD3D11_DEPTH_STENCIL_VIEW_DESC Desc(D3D11_DSV_DIMENSION_TEXTURE2D, DSVFormat);
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(m_pTexture.Get(), &Desc, &pDSV));

	std::string DSVDebugName = m_debugName + " DSV";
	pDSV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DSVDebugName.size(), DSVDebugName.c_str());

	m_pDSVs.push_back(pDSV);
}

void NXTexture2D::AddUAV()
{
	ID3D11UnorderedAccessView* pUAV;

	DXGI_FORMAT UAVFormat = m_texFormat;

	CD3D11_UNORDERED_ACCESS_VIEW_DESC Desc(D3D11_UAV_DIMENSION_TEXTURE2D, UAVFormat);
	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), &Desc, &pUAV));

	std::string UAVDebugName = m_debugName + " UAV";
	pUAV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)UAVDebugName.size(), UAVDebugName.c_str());

	m_pUAVs.push_back(pUAV);
}

void NXTextureCube::Create(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	UINT ArraySize = 6;	// textureCube must be 6.

	this->m_debugName = DebugName;
	this->m_width = Width;
	this->m_height = Height;
	this->m_arraySize = ArraySize; 
	this->m_texFormat = TexFormat;
	this->m_mipLevels = MipLevels;

	D3D11_TEXTURE2D_DESC Desc;
	Desc.Format = TexFormat;
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.ArraySize = ArraySize;
	Desc.MipLevels = MipLevels;
	Desc.BindFlags = BindFlags;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CpuAccessFlags;
	Desc.SampleDesc.Count = SampleCount;
	Desc.SampleDesc.Quality = SampleQuality;
	Desc.MiscFlags = MiscFlags | D3D11_RESOURCE_MISC_TEXTURECUBE; // textureCube must keep D3D11_RESOURCE_MISC_TEXTURECUBE flag.

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&Desc, initData, &m_pTexture));
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	AddRef();
}

void NXTextureCube::Create(const std::string& DebugName, const std::wstring& FilePath, size_t width, size_t height)
{
	TexMetadata metadata;
	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();
	LoadFromDDSFile(FilePath.c_str(), DDS_FLAGS_NONE, &metadata, *pImage);
	if (IsCompressed(metadata.format))
	{
		auto img = pImage->GetImage(0, 0, 0);
		size_t nimg = pImage->GetImageCount();

		std::unique_ptr<ScratchImage> dcImage = std::make_unique<ScratchImage>();
		HRESULT hr = Decompress(img, nimg, metadata, DXGI_FORMAT_UNKNOWN /* picks good default */, *dcImage);
		if (SUCCEEDED(hr))
		{
			if (dcImage && dcImage->GetPixels())
				pImage.swap(dcImage);
		}
		else
		{
			printf("Warning: [Decompress] failure when loading NXTextureCube file: %ws\n", FilePath.c_str());
		}
	}

	bool bResize = width && height && width != metadata.width && height != metadata.height;
	if (bResize)
	{
		std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();
		HRESULT hr = Resize(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), width, height, TEX_FILTER_DEFAULT, *timage);
		if (SUCCEEDED(hr))
		{
			auto& tinfo = timage->GetMetadata();

			metadata.width = tinfo.width;
			metadata.height = tinfo.height;
			metadata.mipLevels = 1;

			pImage.swap(timage);
		}
		else
		{
			printf("Warning: [Resize] failure when loading NXTextureCube file: %ws\n", FilePath.c_str());
		}
	}

	bool bGenerateMipMap = false;
	if (bGenerateMipMap)
	{
		std::unique_ptr<ScratchImage> pImageMip = std::make_unique<ScratchImage>();
		HRESULT hr = GenerateMipMaps(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *pImageMip);
		if (SUCCEEDED(hr))
		{
			metadata.mipLevels = pImageMip->GetMetadata().mipLevels;
			pImage.swap(pImageMip);
		}
		else
		{
			printf("Warning: [GenerateMipMaps] failure when loading NXTextureCube file: %ws\n", FilePath.c_str());
		}
	}

	this->m_texFilePath = FilePath.c_str();
	this->m_debugName = DebugName;
	this->m_width = (UINT)metadata.width;
	this->m_height = (UINT)metadata.height;
	this->m_arraySize = (UINT)metadata.arraySize;
	this->m_texFormat = metadata.format;
	this->m_mipLevels = (UINT)metadata.mipLevels;

	DirectX::CreateTextureEx(g_pDevice.Get(), pImage->GetImages(), pImage->GetImageCount(), metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, CREATETEX_DEFAULT, (ID3D11Resource**)m_pTexture.GetAddressOf());
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	auto HDRPreviewInfo = metadata;
	HDRPreviewInfo.arraySize = 1;
	HDRPreviewInfo.mipLevels = 1;
	HDRPreviewInfo.miscFlags = 0;
	CreateShaderResourceView(g_pDevice.Get(), pImage->GetImage(0, 0, 0), 1, HDRPreviewInfo, &m_pSRVPreview2D);

	AddRef();
}

void NXTextureCube::AddSRV()
{
	ID3D11ShaderResourceView* pSRV = nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	Desc.Format = m_texFormat;
	Desc.TextureCube.MostDetailedMip = 0;
	Desc.TextureCube.MipLevels = m_mipLevels;

	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexture.Get(), &Desc, &pSRV));

	std::string SRVDebugName = m_debugName + " SRV";
	pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());

	m_pSRVs.push_back(pSRV);
}

void NXTextureCube::AddRTV(UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	ID3D11RenderTargetView* pRTV = nullptr;

	D3D11_RENDER_TARGET_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = mipSlice;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;

	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexture.Get(), &Desc, &pRTV));

	std::string RTVDebugName = m_debugName + " RTV";
	pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)RTVDebugName.size(), RTVDebugName.c_str());

	m_pRTVs.push_back(pRTV);
}

void NXTextureCube::AddDSV(UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = mipSlice;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
}

void NXTextureCube::AddUAV(UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = mipSlice;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
}


void NXTexture2DArray::Create(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	this->m_debugName = DebugName;
	this->m_width = Width;
	this->m_height = Height;
	this->m_arraySize = ArraySize;
	this->m_texFormat = TexFormat;
	this->m_mipLevels = MipLevels;

	D3D11_TEXTURE2D_DESC Desc;
	Desc.Format = TexFormat;
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.ArraySize = ArraySize;
	Desc.MipLevels = MipLevels;
	Desc.BindFlags = BindFlags;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CpuAccessFlags;
	Desc.SampleDesc.Count = SampleCount;
	Desc.SampleDesc.Quality = SampleQuality;
	Desc.MiscFlags = MiscFlags;

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&Desc, initData, &m_pTexture));
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	AddRef();
}

void NXTexture2DArray::AddSRV(UINT firstArraySlice, UINT arraySize)
{
	ID3D11ShaderResourceView* pSRV = nullptr;

	DXGI_FORMAT SRVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
		SRVFormat = DXGI_FORMAT_R32_FLOAT;

	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = SRVFormat; 
	Desc.Texture2DArray.MostDetailedMip = 0;
	Desc.Texture2DArray.MipLevels = m_mipLevels;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;

	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexture.Get(), &Desc, &pSRV));

	std::string SRVDebugName = m_debugName + " SRV";
	pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());

	m_pSRVs.push_back(pSRV);
}

void NXTexture2DArray::AddRTV(UINT firstArraySlice, UINT arraySize)
{
	ID3D11RenderTargetView* pRTV = nullptr;

	D3D11_RENDER_TARGET_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = 0;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;

	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexture.Get(), &Desc, &pRTV));

	std::string RTVDebugName = m_debugName + " RTV";
	pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)RTVDebugName.size(), RTVDebugName.c_str());

	m_pRTVs.push_back(pRTV);
}

void NXTexture2DArray::AddDSV(UINT firstArraySlice, UINT arraySize)
{
	ID3D11DepthStencilView* pDSV = nullptr;

	DXGI_FORMAT DSVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
		DSVFormat = DXGI_FORMAT_D32_FLOAT;

	D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = DSVFormat;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
	Desc.Texture2DArray.MipSlice = 0;
	Desc.Flags = 0;

	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(m_pTexture.Get(), &Desc, &pDSV));

	std::string DSVDebugName = m_debugName + " DSV";
	pDSV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DSVDebugName.size(), DSVDebugName.c_str());

	m_pDSVs.push_back(pDSV);
}

void NXTexture2DArray::AddUAV(UINT firstArraySlice, UINT arraySize)
{
	ID3D11UnorderedAccessView* pUAV = nullptr;

	DXGI_FORMAT UAVFormat = m_texFormat;

	D3D11_UNORDERED_ACCESS_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = UAVFormat;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
	Desc.Texture2DArray.MipSlice = 0;

	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), &Desc, &pUAV));

	std::string UAVDebugName = m_debugName + " UAV";
	pUAV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)UAVDebugName.size(), UAVDebugName.c_str());

	m_pUAVs.push_back(pUAV);
}
