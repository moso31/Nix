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

    // chunk��С
    int chunkBytes; 

    // layout�������ڼ���face��mip��slice��
    // �ӵڼ���layout���ڼ���layout
    int layoutIndexStart;
    int layoutIndexSize;

    // chunk�ӵ�ǰlayout�ڼ��п�ʼ���ڼ��н���
    // ����layoutIndexSize == 1ʱ��Ч
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

    // �첽����������Դ���
    void SetTexChunks(int chunks); // ����������ص�chunk����
    void ProcessLoadingTexChunks(); // ����--��ÿ���غ�һ��chunk������һ��
    void WaitLoadingTexturesFinish();

    // �첽����Views���
    void SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, uint32_t otherNum = 0); // ����View����
    void ProcessLoadingViews(); // ����--��ÿ���غ�һ��View������һ��
    void WaitLoadingViewsFinish(); // �ȴ�����View��������ɣ���Ⱦ��Viewʱ����

    void ProcessLoading2DPreview(); // ����--��ÿ���غ�һ��View������һ��
    void WaitLoading2DPreviewFinish(); // �ȴ�����View��������ɣ���Ⱦ��Viewʱ����

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

    // ��������Ҫ���¼���m_pTexture������¼�ʱ��������������Apply��Mesh���ʱ�����ᴥ��������߼���
    void MarkReload(const std::filesystem::path& newTexPath);
    void ReloadCheck();

    // ���л��ͷ����л�
	virtual void Serialize() override; 
	virtual void Deserialize() override;

    const NXTextureSerializationData& GetSerializationData() { return m_serializationData; }
    void SetSerializationData(const NXTextureSerializationData& data) { m_serializationData = data; }

protected:    
    // ���� RT ���͵�Texture�������������
    void CreateRenderTextureInternal(D3D12_RESOURCE_FLAGS flags);

    // �������� Texture�������������
    void CreateInternal(const std::shared_ptr<DirectX::ScratchImage>& pImage, D3D12_RESOURCE_FLAGS flags);

    // ���ļ����� Texture�������������
    void CreatePathTextureInternal(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags);
    void CreatePathTextureInternal(const std::filesystem::path& filePath, Int2 subRegionXY, Int2 subRegionSize, D3D12_RESOURCE_FLAGS flags);

    void AfterTexLoaded(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags, const NXTextureLoaderTaskResult& result);
    void AfterTexMemoryAllocated(const NXTextureLoaderTaskResult& result, const PlacedBufferAllocTaskResult& taskResult, std::vector<NXTextureUploadChunk>&& chunks);

private:
    void InternalReload(Ntr<NXTexture> pReloadTexture);
    D3D12_RESOURCE_DIMENSION GetResourceDimentionFromType();

    void GenerateUploadChunks(uint32_t layoutSize, uint32_t* numRow, uint64_t* numRowSizeInBytes, uint64_t totalBytes, std::vector<NXTextureUploadChunk>& oChunks);

    // ����ȡ�����룬alignment������2����
    int AlignDownForPow2Only(int value, int alignment) { return value & ~(alignment - 1); }

    // ����mip����
    uint32_t CalcMipCount(int width, int height);

    // ����ָ��layout��subRegion��ƫ�ƣ�
    // oSrcRow����/���е���ʼ����
    // oSrcBytes: ÿ��/���е���ʼƫ����
    void ComputeSubRegionOffsets(const std::shared_ptr<ScratchImage>& pImage, int layoutIndex, uint32_t& oSrcRow, uint32_t& oSrcBytes);

    // �ֿ鴫��ӿڣ�����һ��Layout��һ�������ݣ����ϴ��layout��ɢ����δ���
    void CopyPartOfLayoutToChunk(const NXTextureUploadChunk& texChunk, const std::shared_ptr<ScratchImage>& pImage);

    // �ֿ鴫��ӿڣ�������Layout�����ݣ�ͬʱ������С��layout��
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

    // ������Ϣ�������subRegion����ôwh����subRegion��wh��
    // Ŀǰ��û��ʵ��������Ҫ��ȷ���� Դ�ļ���ʵ�ʴ�����wh
    DXGI_FORMAT m_texFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_arraySize;
    uint32_t m_mipLevels;

    // �Ƿ�ֻ������ͼ�ļ���һ��������
    bool m_useSubRegion;
    Int2 m_subRegionXY;
    Int2 m_subRegionSize;

    // ���л�����
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

    // ע�⣺Create���첽�ģ��������漸������ͬ����
    Ntr<NXTexture2D> Create(const std::string& debugName, const std::filesystem::path& FilePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    // ֻ������ͼ�ļ���һ�����������Ҳ�첽
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
