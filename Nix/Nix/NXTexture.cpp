#include "NXTexture.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"

TextureNXInfo::TextureNXInfo(const TextureNXInfo& info) :
	nTexType(info.nTexType),
	//TexFormat(info.TexFormat),
	//Width(info.Width),
	//Height(info.Height),
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


void NXTexture::SwapToReloadingTexture()
{
	if (m_reloadingState == NXTextureReloadingState::Texture_StartReload)
	{
		InternalReload(NXResourceManager::GetInstance()->GetCommonTextures(NXCommonTex_White));
	}

	if (m_reloadingState == NXTextureReloadingState::Texture_FinishReload)
	{
		InternalReload(m_pReloadingTexture);
		SafeRelease(m_pReloadingTexture);
	}
}

void NXTexture::InternalReload(NXTexture* pReloadTexture)
{
	m_pTexture = pReloadTexture->m_pTexture;

	auto& pReloadSRVs = pReloadTexture->m_pSRVs;
	m_pSRVs.resize(pReloadSRVs.size());

	auto& pReloadDSVs = pReloadTexture->m_pDSVs;
	m_pDSVs.resize(pReloadDSVs.size());

	auto& pReloadRTVs = pReloadTexture->m_pRTVs;
	m_pRTVs.resize(pReloadRTVs.size());

	auto& pReloadUAVs = pReloadTexture->m_pUAVs;
	m_pUAVs.resize(pReloadUAVs.size());

	for (size_t i = 0; i < pReloadSRVs.size(); i++) m_pSRVs[i] = pReloadSRVs[i];
	for (size_t i = 0; i < pReloadDSVs.size(); i++) m_pDSVs[i] = pReloadDSVs[i];
	for (size_t i = 0; i < pReloadRTVs.size(); i++) m_pRTVs[i] = pReloadRTVs[i];
	for (size_t i = 0; i < pReloadUAVs.size(); i++) m_pUAVs[i] = pReloadUAVs[i];
}

NXTextureReloadTask NXTexture::LoadTextureAsync()
{
	co_await NXTextureAwaiter();

	// 从这里开始异步加载...
	LoadTextureSync();
}

void NXTexture::LoadTextureSync()
{
	m_pReloadingTexture = NXResourceManager::GetInstance()->CreateTexture2D(m_debugName, m_texFilePath, true); // 将最后的参数设为true，以强制读取硬盘纹理
}

void NXTexture::AddRef()
{
	m_nRefCount++;
}

void NXTexture::RemoveRef()
{
	m_nRefCount--;
	if (!m_nRefCount) Release();
}

void NXTexture::Release()
{
	SafeDelete(m_pInfo);
}

void NXTexture::OnReload()
{
	if (m_reloadingState == NXTextureReloadingState::Texture_None)
	{
		m_reloadingState = NXTextureReloadingState::Texture_StartReload;
		return;
	}
}

void NXTexture::OnReloadFinish()
{
	m_reloadingState = NXTextureReloadingState::Texture_FinishReload;
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

NXTexture2D* NXTexture2D::Create(const std::string& DebugName, const std::filesystem::path& filePath)
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
	{
		pImage.reset();
		return nullptr;
	}

	m_texFilePath = filePath;
	m_pInfo = NXResourceManager::GetInstance()->LoadTextureInfo(filePath);

	// 如果读取的是arraySize/TextureCube，就只读取ArraySize[0]/X+面。
	if (metadata.arraySize > 1)
	{
		std::unique_ptr<ScratchImage> timage(new ScratchImage);
		timage->InitializeFromImage(*pImage->GetImage(0, 0, 0));
		metadata = timage->GetMetadata();
		pImage.swap(timage);
	}

	// --- Convert -----------------------------------------------------------------
	if (IsSRGB(metadata.format) != m_pInfo->bSRGB)
	{
		std::unique_ptr<ScratchImage> timage(new ScratchImage);

		DXGI_FORMAT tFormat = m_pInfo->bSRGB ? NXConvert::ForceSRGB(metadata.format) : NXConvert::ForceLinear(metadata.format);
		TEX_FILTER_FLAGS texFlags = m_pInfo->bSRGB ? TEX_FILTER_SRGB_IN : TEX_FILTER_DEFAULT;
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
	if (m_pInfo->bInvertNormalY)
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

	if (m_pInfo->bGenerateMipMap && metadata.width >= 2 && metadata.height >= 2 && metadata.mipLevels == 1)
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

	pImage.reset();
	return this;
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
