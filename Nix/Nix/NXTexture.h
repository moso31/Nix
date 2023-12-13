#pragma once
#include "BaseDefs/DX12.h"
#include "SimpleMath.h"
#include "Ntr.h"
#include "NXObject.h"
#include "NXSerializable.h"
#include "NXTextureDefinitions.h"
#include "NXTextureReloadTesk.h"

using namespace Microsoft::WRL;
using namespace SimpleMath;

enum NXTextureReloadingState
{
    Texture_None, // 正常状态
    Texture_StartReload, // A->Default 状态
    Texture_Reloading,  // Default->B 状态
    Texture_FinishReload,  // B 状态
};

class NXTexture2D;
class NXTexture2DArray;
class NXTextureCube;
class NXTexture : public NXObject, public NXSerializable
{
public:
    NXTexture() :
        m_reloadingState(Texture_None),
        m_width(-1),
        m_height(-1),
        m_arraySize(-1),
        m_texFormat(DXGI_FORMAT_UNKNOWN),
        m_mipLevels(-1),
        m_texFilePath("")
    {}

    virtual ~NXTexture();

    ID3D11Texture2D* GetTex() const { return m_pTexture.Get(); }
    ID3D11ShaderResourceView* GetSRV(UINT index = 0) const { return m_pSRVs.empty() ? nullptr : m_pSRVs[index].Get(); }
    ID3D11RenderTargetView* GetRTV(UINT index = 0) const { return m_pRTVs.empty() ? nullptr : m_pRTVs[index].Get(); }
    ID3D11DepthStencilView* GetDSV(UINT index = 0) const { return m_pDSVs.empty() ? nullptr : m_pDSVs[index].Get(); }
    ID3D11UnorderedAccessView* GetUAV(UINT index = 0) const { return m_pUAVs.empty() ? nullptr : m_pUAVs[index].Get(); }

    NXTextureReloadingState GetReloadingState() { return m_reloadingState; }
    void SetReloadingState(NXTextureReloadingState state) { m_reloadingState = state; }

    Ntr<NXTexture> GetReloadingTexture() { return m_pReloadingTexture; }
    void SwapToReloadingTexture();

    std::filesystem::path const GetFilePath() { return m_texFilePath; }

    NXTextureReloadTask LoadTextureAsync();
    void LoadTextureSync();

    UINT            GetWidth() { return m_width; }
    UINT            GetHeight() { return m_height; }
    UINT            GetArraySize() { return m_arraySize; }
    UINT            GetMipLevels() { return m_mipLevels; }
    DXGI_FORMAT     GetFormat() { return m_texFormat; }

    void Release();

    // 当出现需要重新加载m_pTexture纹理的事件时（比如纹理属性Apply、Mesh材质变更）会触发这里的逻辑。
    void MarkReload();
    void OnReloadFinish();

    // 序列化和反序列化
	virtual void Serialize() override; 
	virtual void Deserialize() override;

    const NXTextureSerializationData& GetSerializationData() { return m_serializationData; }
    void SetSerializationData(const NXTextureSerializationData& data) { m_serializationData = data; }

private:
    void InternalReload(Ntr<NXTexture> pReloadTexture);

protected:
    ComPtr<ID3D12Resource> m_pTexture;

    // TODO: 临时资源，后续考虑优化
    ComPtr<ID3D12Resource> m_pTextureUploadBuffer; 

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

    // 序列化数据
    NXTextureSerializationData m_serializationData;

private:
    NXTextureReloadingState m_reloadingState;
    Ntr<NXTexture> m_pReloadingTexture;
};

class NXTexture2D : public NXTexture
{
public:
    NXTexture2D() {}
    NXTexture2D(const NXTexture2D& other) = delete;
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

    Ntr<NXTexture2D> Create(const std::string& DebugName, const std::filesystem::path& FilePath);
    Ntr<NXTexture2D> CreateSolid(const std::string& DebugName, UINT TexSize, const Vector4& Color);
    Ntr<NXTexture2D> CreateNoise(const std::string& DebugName, UINT TexSize, UINT Dimension);

    void AddSRV();
    void AddRTV();
    void AddDSV();
    void AddUAV();

private:
    void CreateInternal(const std::string& debugName, const std::unique_ptr<DirectX::ScratchImage>& pImage);
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

    ID3D11ShaderResourceView* GetSRVPreview2D() { return m_pSRVPreview2D.Get(); }

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
