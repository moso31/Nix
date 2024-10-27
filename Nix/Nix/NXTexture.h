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

enum NXTextureReloadingState
{
    Texture_None, // 正常状态
    Texture_StartReload, // A->Default 状态
    Texture_Reloading,  // Default->B 状态
    Texture_FinishReload,  // B 状态
};

enum NXTextureType
{
    TextureType_None,
    TextureType_1D,
    TextureType_2D,
    TextureType_Cube,
    TextureType_2DArray,
    TextureType_3D,
};

namespace DirectX
{
    class ScratchImage;
}

class NXTexture2D;
class NXTexture2DArray;
class NXTextureCube;
class NXTexture : public NXObject, public NXSerializable
{
public:
    NXTexture(NXTextureType type) :
        m_reloadingState(Texture_None),
        m_width(-1),
        m_height(-1),
        m_arraySize(-1),
        m_texFormat(DXGI_FORMAT_UNKNOWN),
        m_mipLevels(-1),
        m_texFilePath(""),
        m_type(type),
        m_resourceState(D3D12_RESOURCE_STATE_COPY_DEST),
        m_loadingViews(0),
        m_futureLoadingViews(m_promiseLoadingViews.get_future())
    {}

    virtual ~NXTexture();

    static void Init();

    ID3D12Resource* GetTex() const { return m_pTexture.Get(); }
    const ShaderVisibleDescriptorTaskResult& GetSRV(uint32_t index = 0);
    const NonVisibleDescriptorTaskResult& GetRTV(uint32_t index = 0);
    const NonVisibleDescriptorTaskResult& GetDSV(uint32_t index = 0);
    const ShaderVisibleDescriptorTaskResult& GetUAV(uint32_t index = 0);
    const size_t GetSRVs() const { return m_pSRVs.size(); }
    const size_t GetRTVs() const { return m_pRTVs.size(); }
    const size_t GetDSVs() const { return m_pDSVs.size(); }
    const size_t GetUAVs() const { return m_pUAVs.size(); }
    const size_t* GetSRVArray() { return m_pSRVs.data(); }
    const size_t* GetRTVArray() { return m_pRTVs.data(); }
    const size_t* GetDSVArray() { return m_pDSVs.data(); }
    const size_t* GetUAVArray() { return m_pUAVs.data(); }

    // 异步加载Views相关
    void SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, bool bAutoSubmitViews = true); // 设置View数量
    void SubmitLoadingViews(int asyncLoadingViewsCount); // 设置View数量（异步计数）
    void ProcessLoadingBuffers(); // 计数--，每加载好一个View，调用一次
    void WaitLoadingViewsFinish(); // 等待所有View都加载完成，渲染传View时调用

    const D3D12_CLEAR_VALUE& GetClearValue() { return m_clearValue; }
    void SetClearValue(float R, float G, float B, float A);
    void SetClearValue(float depth, uint32_t stencilRef);

    const D3D12_RESOURCE_STATES& GetResourceState() { return m_resourceState; }
    const void SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state);

    NXTextureReloadingState GetReloadingState() { return m_reloadingState; }
    void SetReloadingState(NXTextureReloadingState state) { m_reloadingState = state; }

    Ntr<NXTexture> GetReloadingTexture() { return m_pReloadingTexture; }
    void SwapToReloadingTexture();

    const std::filesystem::path& GetFilePath() const { return m_texFilePath; }

    NXTextureReloadTask LoadTextureAsync();
    void LoadTextureSync();

    uint32_t            GetWidth() { return m_width; }
    uint32_t            GetHeight() { return m_height; }
    uint32_t            GetArraySize() { return m_arraySize; }
    uint32_t            GetMipLevels() { return m_mipLevels; }
    DXGI_FORMAT     GetFormat() { return m_texFormat; }
    DXGI_FORMAT     GetDSVFormat() { return NXConvert::TypelessToDSVFormat(m_texFormat); }

    void Release();

    // 当出现需要重新加载m_pTexture纹理的事件时（比如纹理属性Apply、Mesh材质变更）会触发这里的逻辑。
    void MarkReload();
    void OnReloadFinish();

    // 序列化和反序列化
	virtual void Serialize() override; 
	virtual void Deserialize() override;

    const NXTextureSerializationData& GetSerializationData() { return m_serializationData; }
    void SetSerializationData(const NXTextureSerializationData& data) { m_serializationData = data; }

protected:    
    // 创建 RT 类型的Texture2D，调用这个方法
    // 创建 RT 类型的Texture2DArray，调用这个方法
    // 创建 RT 类型的Texture2DCube，调用这个方法
    void CreateRenderTextureInternal(D3D12_RESOURCE_FLAGS flags);

    // Texture2D Solid/Noise纹理生成，
    // CubeMap 从文件创建
    // 调用这个方法
    void CreateInternal(std::unique_ptr<DirectX::ScratchImage>&& pImage, D3D12_RESOURCE_FLAGS flags);

    // Texture2D 从文件创建，调用这个方法
    void CreatePathTextureInternal(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags);

private:
    bool GetMetadataFromFile(const std::filesystem::path& path, TexMetadata& oMetaData);

    void InternalReload(Ntr<NXTexture> pReloadTexture);
    D3D12_RESOURCE_DIMENSION GetResourceDimentionFromType();

    static ComPtr<ID3D12CommandAllocator> s_pCmdAllocator;
    static ComPtr<ID3D12GraphicsCommandList> s_pCmdList;

protected:
    ComPtr<ID3D12Resource> m_pTexture;

    D3D12_RESOURCE_STATES m_resourceState;
    std::filesystem::path m_texFilePath;

    std::promise<void> m_promiseLoadingTextures;
    std::future<void> m_futureLoadingTextures;

    std::atomic<int> m_loadingViews;
    std::promise<void> m_promiseLoadingViews;
    std::future<void> m_futureLoadingViews;

    std::vector<ShaderVisibleDescriptorTaskResult> m_pSRVs;
    std::vector<NonVisibleDescriptorTaskResult> m_pRTVs;
    std::vector<NonVisibleDescriptorTaskResult> m_pDSVs;
    std::vector<ShaderVisibleDescriptorTaskResult> m_pUAVs;

    NXTextureType m_type;
    DXGI_FORMAT m_texFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_arraySize;
    uint32_t m_mipLevels;

    // 序列化数据
    NXTextureSerializationData m_serializationData;

    D3D12_CLEAR_VALUE m_clearValue;

private:
    NXTextureReloadingState m_reloadingState;
    Ntr<NXTexture> m_pReloadingTexture;
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
    NXTextureCube() : NXTexture(TextureType_Cube) {}
    virtual ~NXTextureCube() {}

    void Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    void Create(const std::string& debugName, const std::wstring& filePath, size_t width = 0, size_t height = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    void SetSRV(uint32_t index);
    void SetSRVPreview2D();
    void SetRTV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetDSV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);
    void SetUAV(uint32_t index, uint32_t mipSlice = -1, uint32_t firstArraySlice = 0, uint32_t arraySize = -1);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreview2D() { return { m_pSRVPreview2D }; }

private:
    ShaderVisibleDescriptorTaskResult m_pSRVPreview2D;
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
