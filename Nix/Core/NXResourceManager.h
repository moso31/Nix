#pragma once
#include "NXInstance.h"

class NXTexture2D;

enum NXCommonRTEnum
{
    NXCommonRT_DepthZ,

    // ����G-Buffer�ṹ���£�
    // RT0:		Position				R32G32B32A32_FLOAT
    // RT1:		Normal					R32G32B32A32_FLOAT
    // RT2:		Albedo					R10G10B10A2_UNORM
    // RT3:		Metallic+Roughness+AO	R10G10B10A2_UNORM
    // *ע�⣺����RT0��RT1�����õ���128λ������������ֻ����ʱ������RT2��RT3Ҳ�д���ȶ��
    NXCommonRT_GBuffer0,
    NXCommonRT_GBuffer1,
    NXCommonRT_GBuffer2,
    NXCommonRT_GBuffer3,

    NXCommonRT_SIZE,
};

// by Moso31 2021.12.25
// NX Resource ����/���� ��Դ�����ࡣ
// ��Ҫְ��
// 1. ����һ������
// 2. ���������� ��Ⱦ�ж��pass�ظ�ʹ�õ�CommonRT��
class NXResourceManager : public NXInstance<NXResourceManager>
{
public:
    NXResourceManager();
	~NXResourceManager();

    NXTexture2D* CreateTexture2D(std::string DebugName,
        DXGI_FORMAT TexFormat,
        UINT Width,
        UINT Height,
        UINT ArraySize = 1,
        UINT MipLevels = 0,
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE Usage = D3D11_USAGE_DEFAULT,
        UINT CpuAccessFlags = 0,
        UINT SampleCount = 1,
        UINT SampleQuality = 0,
        UINT MiscFlags = 0);

    NXTexture2D* CreateTexture2D(std::string DebugName,
        const D3D11_SUBRESOURCE_DATA* initData,
        DXGI_FORMAT TexFormat,
        UINT Width,
        UINT Height,
        UINT ArraySize = 1,
        UINT MipLevels = 0,
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE Usage = D3D11_USAGE_DEFAULT,
        UINT CpuAccessFlags = 0,
        UINT SampleCount = 1,
        UINT SampleQuality = 0,
        UINT MiscFlags = 0);

    void InitCommonRT();
    NXTexture2D* GetCommonRT(NXCommonRTEnum eRT);

    void Release();

private:
    std::vector<NXTexture2D*> m_pCommonRT;
};

class NXTexture2D
{
public:
    NXTexture2D();
    ~NXTexture2D() {}

    void Create(std::string DebugName,
        const D3D11_SUBRESOURCE_DATA* initData,
        DXGI_FORMAT TexFormat,
        UINT Width,
        UINT Height,
        UINT ArraySize,
        UINT MipLevels,
        UINT BindFlags,
        D3D11_USAGE Usage,
        UINT CpuAccessFlags,
        UINT SampleCount,
        UINT SampleQuality,
        UINT MiscFlags);

    void CreateSRV();
    void CreateRTV();
    void CreateDSV();
    void CreateUAV();

    ID3D11Texture2D*            GetTex() { return pTexture.Get(); }
    ID3D11RenderTargetView*     GetRTV() { return pRTV.Get(); }
    ID3D11ShaderResourceView*   GetSRV() { return pSRV.Get(); }
    ID3D11DepthStencilView*     GetDSV() { return pDSV.Get(); }
    ID3D11UnorderedAccessView*  GetUAV() { return pUAV.Get(); }

private:
    std::string DebugName;
    ComPtr<ID3D11Texture2D> pTexture;

    ComPtr<ID3D11ShaderResourceView> pSRV;
    ComPtr<ID3D11RenderTargetView> pRTV;
    ComPtr<ID3D11DepthStencilView> pDSV;
    ComPtr<ID3D11UnorderedAccessView> pUAV;

    DXGI_FORMAT TexFormat;
    UINT Width;
    UINT Height;
    UINT ArraySize;
    UINT MipLevels;
};