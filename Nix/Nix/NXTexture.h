#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include <future>
#include "SimpleMath.h"
#include "Ntr.h"
#include "NXObject.h"
#include "NXSerializable.h"
#include "NXTextureDefinitions.h"
#include "NXTextureReloadTesk.h"
#include "NXConverter.h"

using namespace Microsoft::WRL;
using namespace SimpleMath;
using namespace DirectX;
using namespace ccmem;

namespace DirectX
{
    class ScratchImage; 
    struct TexMetadata;
}

struct NXTextureReload
{
    bool m_needReload = false;
    bool m_isReloading = false;
    std::filesystem::path m_newTexPath = "";
    Ntr<NXTexture> m_pReloadTex;
};

struct NXTextureUploadChunk
{
    NXTextureUploadChunk() = default;

    // chunk大小
    int chunkBytes; 

    // layout索引（第几个face的mip的slice）
    // 从第几个layout到第几个layout
    int layoutIndexStart;
    int layoutIndexSize;

    // chunk从当前layout第几行开始到第几行结束
    // 仅在layoutIndexSize == 1时有效
    int rowStart; 
    int rowSize;
};

class NXTexture2D;
class NXTexture2DArray;
class NXTextureCube;
class NXTexture : public NXObject, public NXSerializable
{
public:
    NXTexture(NXTextureType type) :
        m_width(-1),
        m_height(-1),
        m_arraySize(-1),
        m_texFormat(DXGI_FORMAT_UNKNOWN),
        m_mipLevels(-1),
        m_texFilePath(""),
        m_type(type),
        m_resourceState(D3D12_RESOURCE_STATE_COPY_DEST),
        m_loadingViews(0),
        m_futureLoadingViews(m_promiseLoadingViews.get_future()),
        m_futureLoadingTexChunks(m_promiseLoadingTexChunks.get_future())
    {}

    virtual ~NXTexture();

    ID3D12Resource* GetTex() const { return m_pTexture.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(uint32_t index = 0);
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32_t index = 0);
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(uint32_t index = 0);
    D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t index = 0);

    // 异步加载纹理资源相关
    void SetTexChunks(int chunks); // 设置纹理加载的chunk数量
    void ProcessLoadingTexChunks(); // 计数--，每加载好一个chunk，调用一次
    void WaitLoadingTexturesFinish();

    // 异步加载Views相关
    void SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, uint32_t otherNum = 0); // 设置View数量
    void ProcessLoadingBuffers(); // 计数--，每加载好一个View，调用一次
    void WaitLoadingViewsFinish(); // 等待所有View都加载完成，渲染传View时调用

    const D3D12_CLEAR_VALUE& GetClearValue() { return m_clearValue; }
    void SetClearValue(float R, float G, float B, float A);
    void SetClearValue(float depth, uint32_t stencilRef);

    const D3D12_RESOURCE_STATES& GetResourceState() { return m_resourceState; }
    void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state);

    const std::filesystem::path& GetFilePath() const { return m_texFilePath; }

    uint32_t        GetWidth() { return m_width; }
    uint32_t        GetHeight() { return m_height; }
    uint32_t        GetArraySize() { return m_arraySize; }
    uint32_t        GetMipLevels() { return m_mipLevels; }
    DXGI_FORMAT     GetFormat() { return m_texFormat; }
    DXGI_FORMAT     GetDSVFormat() { return NXConvert::TypelessToDSVFormat(m_texFormat); }

    void Release();

    // 当出现需要重新加载m_pTexture纹理的事件时（比如纹理属性Apply、Mesh材质变更）会触发这里的逻辑。
    void MarkReload(const std::filesystem::path& newTexPath);
    void ReloadCheck();

    // 序列化和反序列化
	virtual void Serialize() override; 
	virtual void Deserialize() override;

    const NXTextureSerializationData& GetSerializationData() { return m_serializationData; }
    void SetSerializationData(const NXTextureSerializationData& data) { m_serializationData = data; }

protected:    
    // 创建 RT 类型的Texture，调用这个方法
    void CreateRenderTextureInternal(D3D12_RESOURCE_FLAGS flags);

    // 程序化生成 Texture，调用这个方法
    void CreateInternal(const std::shared_ptr<DirectX::ScratchImage>& pImage, D3D12_RESOURCE_FLAGS flags);

    // 从文件创建 Texture，调用这个方法
    void CreatePathTextureInternal(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags);

private:
    bool GetMetadataFromFile(const std::filesystem::path& path, DirectX::TexMetadata& oMetaData);

    void InternalReload(Ntr<NXTexture> pReloadTexture);
    D3D12_RESOURCE_DIMENSION GetResourceDimentionFromType();

    void GenerateUploadChunks(uint32_t layoutSize, uint32_t* numRow, uint64_t* numRowSizeInBytes, uint64_t totalBytes, std::vector<NXTextureUploadChunk>& oChunks);

protected:
    ComPtr<ID3D12Resource> m_pTexture;

    D3D12_RESOURCE_STATES m_resourceState;
    std::filesystem::path m_texFilePath;

    std::atomic<int> m_loadingTexChunks;
    std::promise<void> m_promiseLoadingTexChunks;
    std::future<void> m_futureLoadingTexChunks;

    std::atomic<int> m_loadingViews;
    std::promise<void> m_promiseLoadingViews;
    std::future<void> m_futureLoadingViews;

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_pSRVs;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_pRTVs;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_pDSVs;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_pUAVs;

    NXTextureType m_type;
    DXGI_FORMAT m_texFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_arraySize;
    uint32_t m_mipLevels;

    // 序列化数据
    NXTextureSerializationData m_serializationData;

    D3D12_CLEAR_VALUE m_clearValue;

    NXTextureReload m_reload;
};

class NXTexture2D : public NXTexture
{
public:
    NXTexture2D() : NXTexture(TextureType_2D) {}
    NXTexture2D(const NXTexture2D& other) = delete;
    virtual ~NXTexture2D() {}

    Ntr<NXTexture2D> Create(const std::string& DebugName, const std::filesystem::path& FilePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateRenderTexture(const std::string& debugName, DXGI_FORMAT fmt, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateSolid(const std::string& DebugName, uint32_t TexSize, const Vector4& Color, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateNoise(const std::string& DebugName, uint32_t TexSize, uint32_t Dimension, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index);
    void SetRTV(uint32_t index);
    void SetDSV(uint32_t index);
    void SetUAV(uint32_t index);
};

class NXTextureCube : public NXTexture
{
public:
    NXTextureCube() : 
        NXTexture(TextureType_Cube),
        m_futureLoading2DPreview(m_promiseLoading2DPreview.get_future()) {}
    virtual ~NXTextureCube() {}

    void ProcessLoading2DPreview(); // 计数--，每加载好一个View，调用一次
    void WaitLoading2DPreviewFinish(); // 等待所有View都加载完成，渲染传View时调用

    void Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void Create(const std::string& debugName, const std::wstring& filePath, size_t width = 0, size_t height = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index);
    void SetSRVPreview2D();
    void SetRTV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetDSV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetUAV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreview2D();

private:
    std::promise<void> m_promiseLoading2DPreview;
    std::future<void> m_futureLoading2DPreview;
    D3D12_CPU_DESCRIPTOR_HANDLE m_pSRVPreview2D;
};

class NXTexture2DArray : public NXTexture
{
public:
    NXTexture2DArray() : NXTexture(TextureType_2DArray) {}
    virtual ~NXTexture2DArray() {}

    void Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetRTV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetDSV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetUAV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
};
