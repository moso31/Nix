#include "NXResourceManager.h"
#include "DirectResources.h"

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

	return pTexture2D;
}

NXTexture2D* NXResourceManager::CreateTexture2D(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2D* pTexture2D = new NXTexture2D();
	pTexture2D->Create(DebugName, initData, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	return pTexture2D;
}

NXTexture2DArray* NXResourceManager::CreateTexture2DArray(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	NXTexture2DArray* pTexture2DARray = new NXTexture2DArray();
	pTexture2DARray->Create(DebugName, nullptr, TexFormat, Width, Height, ArraySize, MipLevels, BindFlags, Usage, CpuAccessFlags, SampleCount, SampleQuality, MiscFlags);

	return pTexture2DARray;
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
	for (auto pRT : m_pCommonRT) SafeDelete(pRT);
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
