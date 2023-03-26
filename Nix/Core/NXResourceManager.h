#pragma once
#include "NXInstance.h"
#include <filesystem>
#include "NXTextureReloadTesk.h"

enum NXTextureReloadingState
{
    Texture_None, // ����״̬
    Texture_StartReload, // A->Default ״̬
    Texture_Reloading,  // Default->B ״̬
    Texture_FinishReload,  // B ״̬
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

    // ��Ļ�ռ���Ӱ
    NXCommonRT_ShadowTest,

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

    // ��������Ҫ���¼���m_pTexture������¼�ʱ��������������Apply��Mesh���ʱ�����ᴥ��������߼���
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
    // ���ü���
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

    // ����������ԴԪ�ļ���TextureNXInfo��
    // ���û�ҵ���Ӧ��Ԫ�ļ����򽫷���һ�����в�����ΪĬ��ֵ��Ԫ�ļ���
    TextureNXInfo* LoadTextureInfo(const std::filesystem::path& texFilePath);
    void SaveTextureInfo(const TextureNXInfo* pInfo, const std::filesystem::path& texFilePath);

    // 2023.3.24
    // ��Ҫ�滻ĳ������ʱ������GPU������ʹ�ô��������Բ�̫���ܼ�ʱ�滻��
    // ������Ҫ��GPU������ʹ�ô���Դ��ʱ��Ҳ��������� OnReload���������滻������
    void OnReload();

    // �Ƴ� m_pTextureArray ������ ���ü���=0 ������
    void ReleaseUnusedTextures();

    void Release();

private:
    std::vector<NXTexture2D*> m_pCommonRT;
    std::vector<NXTexture2D*> m_pCommonTex;

    std::unordered_set<NXTexture*> m_pTextureArray;
};
