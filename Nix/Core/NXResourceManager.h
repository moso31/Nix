#pragma once
#include "NXInstance.h"
#include <filesystem>

enum NXCommonRTEnum
{
    NXCommonRT_DepthZ,
    NXCommonRT_MainScene,

    // 屏幕空间阴影
    NXCommonRT_ShadowTest,

    // 现行G-Buffer结构如下：
    // RT0:		Position				R32G32B32A32_FLOAT
    // RT1:		Normal					R32G32B32A32_FLOAT
    // RT2:		Albedo					R10G10B10A2_UNORM
    // RT3:		Metallic+Roughness+AO	R10G10B10A2_UNORM
    // *注意：上述RT0、RT1现在用的是128位浮点数——这只是临时方案。RT2、RT3也有待商榷。
    NXCommonRT_GBuffer0,
    NXCommonRT_GBuffer1,
    NXCommonRT_GBuffer2,
    NXCommonRT_GBuffer3,

    NXCommonRT_PostProcessing,

    NXCommonRT_SIZE,
};

class NXTexture
{
public:
    NXTexture() : m_width(-1), m_height(-1), m_arraySize(-1), m_texFormat(DXGI_FORMAT_UNKNOWN), m_mipLevels(-1), m_texFilePath("") {}
    ~NXTexture() {};

    ID3D11Texture2D* GetTex() { return m_pTexture.Get(); }
    ID3D11ShaderResourceView*   GetSRV(UINT index = 0) { return m_pSRVs.empty() ? nullptr : m_pSRVs[index].Get(); }
    ID3D11RenderTargetView*     GetRTV(UINT index = 0) { return m_pRTVs.empty() ? nullptr : m_pRTVs[index].Get(); }
    ID3D11DepthStencilView*     GetDSV(UINT index = 0) { return m_pDSVs.empty() ? nullptr : m_pDSVs[index].Get(); }
    ID3D11UnorderedAccessView*  GetUAV(UINT index = 0) { return m_pUAVs.empty() ? nullptr : m_pUAVs[index].Get(); }

    std::filesystem::path const GetFilePath() { return m_texFilePath; }

    UINT            GetWidth()      { return m_width; }
    UINT            GetHeight()     { return m_height; }
    DXGI_FORMAT     GetFormat()     { return m_texFormat; }

protected:
    std::string m_debugName;
    ComPtr<ID3D11Texture2D> m_pTexture;

    std::filesystem::path m_texFilePath;

    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pSRVs;
    std::vector<ComPtr<ID3D11RenderTargetView>> m_pRTVs;
    std::vector<ComPtr<ID3D11DepthStencilView>> m_pDSVs;
    std::vector<ComPtr<ID3D11UnorderedAccessView>> m_pUAVs;

    DXGI_FORMAT m_texFormat;
    UINT m_width;
    UINT m_height;
    UINT m_arraySize;
    UINT m_mipLevels;
};

class NXTexture2D : public NXTexture
{
public:
    NXTexture2D() : NXTexture() {}
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

    void Create(const std::string& DebugName, const std::wstring& FilePath);

    void AddSRV();
    void AddRTV();
    void AddDSV();
    void AddUAV();
};

class NXTextureCube : public NXTexture
{
public:
    NXTextureCube() : NXTexture() {}
    ~NXTextureCube() {}

    void Create(std::string DebugName,
        const D3D11_SUBRESOURCE_DATA* initData,
        DXGI_FORMAT TexFormat,
        UINT Width,
        UINT Height,
        UINT MipLevels,
        UINT BindFlags,
        D3D11_USAGE Usage,
        UINT CpuAccessFlags,
        UINT SampleCount,
        UINT SampleQuality,
        UINT MiscFlags);

    void Create(const std::string& DebugName, const std::wstring& FilePath, size_t width = 0, size_t height = 0);

    void AddSRV();
    void AddRTV(UINT mipSlice = -1, UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddDSV(UINT mipSlice = -1, UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddUAV(UINT mipSlice = -1, UINT firstArraySlice = 0, UINT arraySize = -1);

    ID3D11ShaderResourceView*   GetSRVPreview2D()     { return m_pSRVPreview2D.Get(); }

private:
    ComPtr<ID3D11ShaderResourceView> m_pSRVPreview2D;
};

class NXTexture2DArray : public NXTexture
{
public:
    NXTexture2DArray() : NXTexture() {}
    ~NXTexture2DArray() {}

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

    void AddSRV(UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddRTV(UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddDSV(UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddUAV(UINT firstArraySlice = 0, UINT arraySize = -1);
};

// by Moso31 2021.12.25
// NX Resource 纹理/缓存 资源管理类。
// 主要职责：
// 1. 创建一般纹理。
// 2. 创建并管理 渲染中多个pass重复使用的CommonRT。
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

    NXTexture2D* CreateTexture2D(const std::string& DebugName,
        const std::wstring& FilePath);

    NXTextureCube* CreateTextureCube(std::string DebugName,
        DXGI_FORMAT TexFormat,
        UINT Width,
        UINT Height,
        UINT MipLevels = 0,
        UINT BindFlags = D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE Usage = D3D11_USAGE_DEFAULT,
        UINT CpuAccessFlags = 0,
        UINT SampleCount = 1,
        UINT SampleQuality = 0,
        UINT MiscFlags = 0);

    NXTextureCube* CreateTextureCube(const std::string& DebugName,
        const std::wstring& FilePath,
        UINT Width = 0,
        UINT Height = 0);

    NXTexture2DArray* CreateTexture2DArray(std::string DebugName,
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

    std::vector<NXTexture*> m_pTextureArray;
};
