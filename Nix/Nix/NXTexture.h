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
using namespace DirectX;

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
        m_type(type)
    {}

    virtual ~NXTexture();

    ID3D12Resource* GetTex() const { return m_pTexture.Get(); }
    size_t GetSRV(UINT index = 0) const { m_pSRVs[index]; }
    size_t GetRTV(UINT index = 0) const { m_pRTVs[index]; }
    size_t GetDSV(UINT index = 0) const { m_pDSVs[index]; }
    size_t GetUAV(UINT index = 0) const { m_pUAVs[index]; }

    NXTextureReloadingState GetReloadingState() { return m_reloadingState; }
    void SetReloadingState(NXTextureReloadingState state) { m_reloadingState = state; }

    Ntr<NXTexture> GetReloadingTexture() { return m_pReloadingTexture; }
    void SwapToReloadingTexture();

    const std::filesystem::path& const GetFilePath() { return m_texFilePath; }

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

protected:
    // 创建纹理，空（RT用）
    // m_pTexture 独立存储在NXTexture成员上
    void CreateInternal();

    // 创建纹理，从文件读取
    // m_pTexture 存储在全局分配器g_pTextureAllocator，并作为大PlaceResource的一部分被统一管理。
    void CreateInternal(const std::unique_ptr<DirectX::ScratchImage>& pImage);

private:
    void InternalReload(Ntr<NXTexture> pReloadTexture);
    D3D12_RESOURCE_DIMENSION GetResourceDimentionFromType();

protected:
    ComPtr<ID3D12Resource> m_pTexture;
    ComPtr<ID3D12Resource> m_pTextureUpload; 

    std::filesystem::path m_texFilePath;

    std::vector<UINT64> m_pSRVs;
    std::vector<UINT64> m_pRTVs;
    std::vector<UINT64> m_pDSVs;
    std::vector<UINT64> m_pUAVs;

    NXTextureType m_type;
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
    NXTexture2D() : NXTexture(TextureType_2D) {}
    NXTexture2D(const NXTexture2D& other) = delete;
    ~NXTexture2D() {}

    Ntr<NXTexture2D> Create(const std::string& DebugName, const std::filesystem::path& FilePath);
    Ntr<NXTexture2D> CreateRT(const std::string& debugName, DXGI_FORMAT fmt, UINT width, UINT height);
    Ntr<NXTexture2D> CreateSolid(const std::string& DebugName, UINT TexSize, const Vector4& Color);
    Ntr<NXTexture2D> CreateNoise(const std::string& DebugName, UINT TexSize, UINT Dimension);

    void AddSRV();
    void AddRTV();
    void AddDSV();
    void AddUAV();
};

class NXTextureCube : public NXTexture
{
public:
    NXTextureCube() : NXTexture(TextureType_Cube) {}
    ~NXTextureCube() {}

    void Create(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT mipLevels);
    void Create(const std::string& debugName, const std::wstring& filePath, size_t width = 0, size_t height = 0);

    void AddSRV();
    void AddRTV(UINT mipSlice = -1, UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddDSV(UINT mipSlice = -1, UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddUAV(UINT mipSlice = -1, UINT firstArraySlice = 0, UINT arraySize = -1);

    size_t GetSRVPreview2D() { return m_pSRVPreview2D; }

private:
    size_t m_pSRVPreview2D = 0;
};

class NXTexture2DArray : public NXTexture
{
public:
    NXTexture2DArray() : NXTexture(TextureType_2DArray) {}
    ~NXTexture2DArray() {}

    void Create(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize, UINT mipLevels);

    void AddSRV(UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddRTV(UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddDSV(UINT firstArraySlice = 0, UINT arraySize = -1);
    void AddUAV(UINT firstArraySlice = 0, UINT arraySize = -1);
};
