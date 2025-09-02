#pragma once
#include "NXResource.h"
#include <future>
#include "NXTextureReloadTesk.h"
#include "NXConverter.h"
#include "NXTextureLoader.h"

using namespace DirectX;

namespace DirectX
{
    class ScratchImage; 
    struct TexMetadata;
}

class NXTexture;
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
class NXTexture : public NXResource
{
public:
    NXTexture(NXResourceType type);
    virtual ~NXTexture();

    ID3D12Resource* GetTex() const { return m_pTexture.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(uint32_t index = 0);
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(uint32_t index = 0);
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(uint32_t index = 0);
    D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t index = 0);

    ID3D12Resource* GetD3DResource() const override { return m_pTexture.Get(); }

    // 异步加载纹理资源相关
    void SetTexChunks(int chunks); // 设置纹理加载的chunk数量
    void ProcessLoadingTexChunks(); // 计数--，每加载好一个chunk，调用一次
    void WaitLoadingTexturesFinish();

    // 异步加载Views相关
    void SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, uint32_t otherNum = 0); // 设置View数量
    void ProcessLoadingViews(); // 计数--，每加载好一个View，调用一次
    void WaitLoadingViewsFinish(); // 等待所有View都加载完成，渲染传View时调用

    void ProcessLoading2DPreview(); // 计数--，每加载好一个View，调用一次
    void WaitLoading2DPreviewFinish(); // 等待所有View都加载完成，渲染传View时调用

    virtual uint32_t GetSRVPreviewCount() { return (uint32_t)m_pSRVPreviews.size(); }
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreview(uint32_t index);

    virtual void SetSRVPreviews() { m_loading2DPreviews = 0; }
    virtual void SetSRVPreview(uint32_t idx) {}

    const D3D12_CLEAR_VALUE& GetClearValue() { return m_clearValue; }
    void SetClearValue(float R, float G, float B, float A);
    void SetClearValue(float depth, uint32_t stencilRef);

    void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state) override;

    const std::filesystem::path& GetFilePath() const { return m_texFilePath; }

    uint32_t        GetWidth()      const override { return m_width; }
    uint32_t        GetHeight()     const override { return m_height; }
    uint32_t        GetArraySize()  const override { return m_arraySize; }
    uint32_t        GetMipLevels()  const override { return m_mipLevels; }
    DXGI_FORMAT     GetFormat()     const { return m_texFormat; }
    DXGI_FORMAT     GetDSVFormat()  const { return NXConvert::TypelessToDSVFormat(m_texFormat); }

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
    void CreatePathTextureInternal(const std::filesystem::path& filePath, Int2 subRegionXY, Int2 subRegionSize, D3D12_RESOURCE_FLAGS flags);

    void AfterTexLoaded(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags, const NXTextureLoaderTaskResult& result);
    void AfterTexMemoryAllocated(const NXTextureLoaderTaskResult& result, const PlacedBufferAllocTaskResult& taskResult, std::vector<NXTextureUploadChunk>&& chunks);

private:
    void InternalReload(Ntr<NXTexture> pReloadTexture);
    D3D12_RESOURCE_DIMENSION GetResourceDimentionFromType();

    void GenerateUploadChunks(uint32_t layoutSize, uint32_t* numRow, uint64_t* numRowSizeInBytes, uint64_t totalBytes, std::vector<NXTextureUploadChunk>& oChunks);

    // 向下取整对齐，alignment必须是2的幂
    int AlignDownForPow2Only(int value, int alignment) { return value & ~(alignment - 1); }

    // 计算mip数量
    uint32_t CalcMipCount(int width, int height);

    // 计算指定layout的subRegion的偏移；
    // oSrcRow：行/块行的起始行数
    // oSrcBytes: 每行/块行的起始偏移量
    void ComputeSubRegionOffsets(const std::shared_ptr<ScratchImage>& pImage, int layoutIndex, uint32_t& oSrcRow, uint32_t& oSrcBytes);

    // 分块传输接口：传输一个Layout的一部分数据（将较大的layout打散，多次处理）
    void CopyPartOfLayoutToChunk(const NXTextureUploadChunk& texChunk, const std::shared_ptr<ScratchImage>& pImage);

    // 分块传输接口：传输多个Layout的数据（同时处理多个小的layout）
    void CopyMultiLayoutsToChunk(const NXTextureUploadChunk& texChunk, const std::shared_ptr<ScratchImage>& pImage);

protected:
    ComPtr<ID3D12Resource> m_pTexture;

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

    std::atomic<int> m_loading2DPreviews;
    std::promise<void> m_promiseLoading2DPreview;
    std::future<void> m_futureLoading2DPreview;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_pSRVPreviews;

    // 基本信息；如果是subRegion，那么wh就是subRegion的wh。
    // 目前还没有实际需求需要明确区分 源文件和实际创建的wh
    DXGI_FORMAT m_texFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_arraySize;
    uint32_t m_mipLevels;

    // 是否只加载贴图文件的一部分区域
    bool m_useSubRegion;
    Int2 m_subRegionXY;
    Int2 m_subRegionSize;

    // 序列化数据
    NXTextureSerializationData m_serializationData;

    D3D12_CLEAR_VALUE m_clearValue;

    NXTextureReload m_reload;
};

class NXTexture2D : public NXTexture
{
public:
    NXTexture2D() : NXTexture(NXResourceType::Tex2D) {}
    NXTexture2D(const NXTexture2D& other) = delete;
    virtual ~NXTexture2D() {}

    // 注意：Create是异步的！其他下面几个都是同步的
    Ntr<NXTexture2D> Create(const std::string& debugName, const std::filesystem::path& FilePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    // 只加载贴图文件的一部分区域，这个也异步
    Ntr<NXTexture2D> CreateSub(const std::string& debugName, const std::filesystem::path& filePath, Int2 subRegionXY, Int2 subRegionSize, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE); 

    Ntr<NXTexture2D> CreateRenderTexture(const std::string& debugName, DXGI_FORMAT fmt, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateSolid(const std::string& debugName, uint32_t TexSize, const Vector4& Color, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateNoise(const std::string& debugName, uint32_t TexSize, uint32_t Dimension, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateHeightRaw(const std::string& debugName, const std::filesystem::path& rawPath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateByData(const std::string& debugName, const std::shared_ptr<ScratchImage>& pImage, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index);
    void SetRTV(uint32_t index);
    void SetDSV(uint32_t index);
    void SetUAV(uint32_t index);
};

class NXTextureCube : public NXTexture
{
public:
    NXTextureCube() : NXTexture(NXResourceType::TexCube) {}
    virtual ~NXTextureCube() {}

    void Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void Create(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index);
    void SetRTV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetDSV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetUAV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);

    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreview(uint32_t index) override;
    virtual void SetSRVPreviews() override;
    virtual void SetSRVPreview(uint32_t idx) override;
};

class NXTexture2DArray : public NXTexture
{
public:
    NXTexture2DArray() : NXTexture(NXResourceType::Tex2DArray) {}
    virtual ~NXTexture2DArray() {}

    void Create(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void Create(const std::string& debugName, const std::wstring& filePath, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void CreateRT(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetRTV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetDSV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetUAV(uint32_t index, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);

    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreview(uint32_t index) override;
    virtual void SetSRVPreviews() override;
    virtual void SetSRVPreview(uint32_t idx) override;
};
