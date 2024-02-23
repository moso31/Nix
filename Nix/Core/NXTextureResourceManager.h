#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "NXResourceManagerBase.h"

class NXTextureResourceManager : public NXResourceManagerBase
{
public:
	NXTextureResourceManager() {};
	~NXTextureResourceManager() {};

    void InitCommonRT(const Vector2& rtSize);
    void ResizeCommonRT(const Vector2& rtSize);
    Ntr<NXTexture2D> GetCommonRT(NXCommonRTEnum eRT);

    void InitCommonTextures();
    Ntr<NXTexture2D> GetCommonTextures(NXCommonTexEnum eTex);

    void OnReload() override;
    void Release() override;

    Ntr<NXTexture2D> CreateTexture2D(const std::string& name, const std::filesystem::path& FilePath, bool bForce = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTexture2D> CreateTexture2D(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    Ntr<NXTextureCube> CreateTextureCube(const std::string& name, const std::wstring& filePath, UINT width = 0, UINT height = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    Ntr<NXTextureCube> CreateTextureCube(const std::string& name, DXGI_FORMAT texFormat, UINT width, UINT height, UINT mipLevels = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    Ntr<NXTexture2DArray> CreateTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize = 1, UINT mipLevels = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

private:
    std::vector<Ntr<NXTexture2D>> m_pCommonTex;
    std::vector<Ntr<NXTexture2D>> m_pCommonRT;

    std::vector<Ntr<NXTexture>> m_pTextureArrayInternal;

	ComPtr<ID3D12DescriptorHeap> m_pSRVHeapTex;
	ComPtr<ID3D12DescriptorHeap> m_pUAVHeapTex;
	ComPtr<ID3D12DescriptorHeap> m_pSRVHeapRT;
	ComPtr<ID3D12DescriptorHeap> m_pUAVHeapRT;
	ComPtr<ID3D12DescriptorHeap> m_pRTVHeapRT;
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeapRT;
};
