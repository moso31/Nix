#pragma once
#include "NXInstance.h"
#include <filesystem>
#include "NXTextureReloadTesk.h"

enum NXTextureReloadingState
{
    Texture_None, // 正常状态
    Texture_StartReload, // A->Default 状态
    Texture_Reloading,  // Default->B 状态
    Texture_FinishReload,  // B 状态
};;

struct TextureNXInfo
{
    TextureNXInfo() = default;
    TextureNXInfo(const TextureNXInfo& info);
    //TextureNXInfo(const TextureNXInfo&& info) noexcept;

    //TextureNXInfo& operator=(TextureNXInfo&& info);

    int nTexType = 0;
    //int TexFormat = 0;
    //int Width = 0;
    //int Height = 0;
    bool bSRGB = false;
    bool bInvertNormalY = false;
    bool bGenerateMipMap = true;
    bool bCubeMap = false;
};

enum NXTextureType
{
    Default,
    NormalMap
};

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

enum NXCommonTexEnum
{
    NXCommonTex_White,
    NXCommonTex_Normal,
    NXCommonTex_SIZE,
};

class NXTexture2D;
class NXTextureCube;
class NXTexture2DArray;
class NXTexture
{
public:
    NXTexture() : 
        m_nRefCount(0), 
        m_reloadingState(Texture_None),
        m_pReloadingTexture(nullptr),
        m_width(-1),
        m_height(-1),
        m_arraySize(-1),
        m_texFormat(DXGI_FORMAT_UNKNOWN),
        m_mipLevels(-1),
        m_texFilePath(""),
        m_pInfo(nullptr) 
    {}
    virtual ~NXTexture() {};

    virtual NXTexture2D*        Is2D()          { return nullptr; }
    virtual NXTexture2DArray*   Is2DArray()     { return nullptr; }
    virtual NXTextureCube*      IsCubeMap()     { return nullptr; }

    ID3D11Texture2D* GetTex() { return m_pTexture.Get(); }
    ID3D11ShaderResourceView*   GetSRV(UINT index = 0) { return m_pSRVs.empty() ? nullptr : m_pSRVs[index].Get(); }
    ID3D11RenderTargetView*     GetRTV(UINT index = 0) { return m_pRTVs.empty() ? nullptr : m_pRTVs[index].Get(); }
    ID3D11DepthStencilView*     GetDSV(UINT index = 0) { return m_pDSVs.empty() ? nullptr : m_pDSVs[index].Get(); }
    ID3D11UnorderedAccessView*  GetUAV(UINT index = 0) { return m_pUAVs.empty() ? nullptr : m_pUAVs[index].Get(); }

    NXTextureReloadingState GetReloadingState()             { return m_reloadingState; }
    void SetReloadingState(NXTextureReloadingState state)   { m_reloadingState = state; }

    NXTexture* GetReloadingTexture() { return m_pReloadingTexture; }
    void SwapToReloadingTexture();

    std::filesystem::path const GetFilePath() { return m_texFilePath; }
    TextureNXInfo* GetTextureNXInfo() { return m_pInfo; }

    NXTextureReloadTask LoadTextureAsync();
    void LoadTextureSync();

    UINT            GetWidth()      { return m_width; }
    UINT            GetHeight()     { return m_height; }
    UINT            GetArraySize()  { return m_arraySize; }
    UINT            GetMipLevels()  { return m_mipLevels; }
    DXGI_FORMAT     GetFormat()     { return m_texFormat; }

    void AddRef();
    int GetRef() { return m_nRefCount; }
    void RemoveRef();
    void Release();

    // 当出现需要重新加载m_pTexture纹理的事件时（比如纹理属性Apply、Mesh材质变更）会触发这里的逻辑。
    void OnReload();
    void OnReloadFinish();

private:
    void InternalReload(NXTexture* pReloadTexture);

protected:
    std::string m_debugName;
    ComPtr<ID3D11Texture2D> m_pTexture;
    TextureNXInfo* m_pInfo;

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

private:
    // 引用计数
    int m_nRefCount;

    NXTextureReloadingState m_reloadingState;
    NXTexture* m_pReloadingTexture;
};

class NXTexture2D : public NXTexture
{
public:
    NXTexture2D() : NXTexture() {}
    ~NXTexture2D() {}

    NXTexture2D* Is2D() override { return this; }

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

    NXTexture2D* Create(const std::string& DebugName, const std::filesystem::path& FilePath);

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

    NXTextureCube* IsCubeMap() override { return this; }

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

    NXTexture2DArray* Is2DArray() override { return this; }

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

    NXTexture2D* CreateTexture2D(const std::string& DebugName, const std::filesystem::path& FilePath, bool bForce = false);

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

    void InitCommonTextures();
    NXTexture2D* GetCommonTextures(NXCommonTexEnum eTex); 

    // 加载纹理资源元文件（TextureNXInfo）
    // 如果没找到对应的元文件，则将返回一个所有参数都为默认值的元文件。
    TextureNXInfo* LoadTextureInfo(const std::filesystem::path& texFilePath);
    void SaveTextureInfo(const TextureNXInfo* pInfo, const std::filesystem::path& texFilePath);

    // 2023.3.24
    // 当要替换某张纹理时，由于GPU可能正使用此纹理，所以不太可能即时替换。
    // 所以需要在GPU不可能使用此资源的时候（也就是这里的 OnReload），进行替换操作。
    void OnReload();

    // 移除 m_pTextureArray 中所有 引用计数=0 的纹理。
    void ReleaseUnusedTextures();

    void Release();

private:
    std::vector<NXTexture2D*> m_pCommonRT;
    std::vector<NXTexture2D*> m_pCommonTex;

    std::unordered_set<NXTexture*> m_pTextureArray;
};
