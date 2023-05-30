#pragma once
#include "Header.h"
#include "NXSerializable.h"
#include "NXTextureReloadTesk.h"

enum class NXTextureType
{
    Default,        // 默认颜色纹理，开启sRGB
    LinearColor,    // 线性颜色纹理
    NormalMap,      // 法线贴图,
    Count
};

extern const char* g_NXTextureType[];

enum NXTextureReloadingState
{
    Texture_None, // 正常状态
    Texture_StartReload, // A->Default 状态
    Texture_Reloading,  // Default->B 状态
    Texture_FinishReload,  // B 状态
};

struct TextureNXInfo
{
    TextureNXInfo() = default;
    TextureNXInfo(const TextureNXInfo& info);

    NXTextureType eType = NXTextureType::Default;
    bool bInvertNormalY = false;
    bool bGenerateMipMap = true;
    bool bCubeMap = false;
};

class NXTexture : public NXSerializable
{
    NXSERIALIZABLE_DERIVED();

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

    virtual NXTexture2D* Is2D() { return nullptr; }
    virtual NXTexture2DArray* Is2DArray() { return nullptr; }
    virtual NXTextureCube* IsCubeMap() { return nullptr; }

    ID3D11Texture2D* GetTex() { return m_pTexture.Get(); }
    ID3D11ShaderResourceView* GetSRV(UINT index = 0) { return m_pSRVs.empty() ? nullptr : m_pSRVs[index].Get(); }
    ID3D11RenderTargetView* GetRTV(UINT index = 0) { return m_pRTVs.empty() ? nullptr : m_pRTVs[index].Get(); }
    ID3D11DepthStencilView* GetDSV(UINT index = 0) { return m_pDSVs.empty() ? nullptr : m_pDSVs[index].Get(); }
    ID3D11UnorderedAccessView* GetUAV(UINT index = 0) { return m_pUAVs.empty() ? nullptr : m_pUAVs[index].Get(); }

    NXTextureReloadingState GetReloadingState() { return m_reloadingState; }
    void SetReloadingState(NXTextureReloadingState state) { m_reloadingState = state; }

    NXTexture* GetReloadingTexture() { return m_pReloadingTexture; }
    void SwapToReloadingTexture();

    std::filesystem::path const GetFilePath() { return m_texFilePath; }
    TextureNXInfo* GetTextureNXInfo() { return m_pInfo; }

    NXTextureReloadTask LoadTextureAsync();
    void LoadTextureSync();

    UINT            GetWidth() { return m_width; }
    UINT            GetHeight() { return m_height; }
    UINT            GetArraySize() { return m_arraySize; }
    UINT            GetMipLevels() { return m_mipLevels; }
    DXGI_FORMAT     GetFormat() { return m_texFormat; }

    void AddRef();
    int GetRef() { return m_nRefCount; }
    void RemoveRef();
    void Release();

    // 当出现需要重新加载m_pTexture纹理的事件时（比如纹理属性Apply、Mesh材质变更）会触发这里的逻辑。
    void MarkReload();
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

    ID3D11ShaderResourceView* GetSRVPreview2D() { return m_pSRVPreview2D.Get(); }

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