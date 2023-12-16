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

    Ntr<NXTexture2D> CreateTexture2D(const std::string& name, const std::filesystem::path& FilePath, bool bForce = false);
    Ntr<NXTexture2D> CreateTexture2D(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height);
    Ntr<NXTexture2D> CreateTexture2D(const std::string& name, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize = 1, UINT MipLevels = 0, UINT BindFlags = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0, UINT SampleCount = 1, UINT SampleQuality = 0, UINT MiscFlags = 0);

    Ntr<NXTextureCube> CreateTextureCube(const std::string& name, const std::wstring& FilePath, UINT Width = 0, UINT Height = 0);
    Ntr<NXTextureCube> CreateTextureCube(const std::string& name, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT MipLevels = 0, UINT BindFlags = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0, UINT SampleCount = 1, UINT SampleQuality = 0, UINT MiscFlags = 0);

    Ntr<NXTexture2DArray> CreateTexture2DArray(std::string DebugName, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize = 1, UINT MipLevels = 0, UINT BindFlags = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0, UINT SampleCount = 1, UINT SampleQuality = 0, UINT MiscFlags = 0);

private:
    std::vector<Ntr<NXTexture2D>> m_pCommonTex;
    std::vector<Ntr<NXTexture2D>> m_pCommonRT; 

	ComPtr<ID3D12DescriptorHeap> m_pSRVHeapTex;
	ComPtr<ID3D12DescriptorHeap> m_pUAVHeapTex;
	ComPtr<ID3D12DescriptorHeap> m_pSRVHeapRT;
	ComPtr<ID3D12DescriptorHeap> m_pUAVHeapRT;
	ComPtr<ID3D12DescriptorHeap> m_pRTVHeapRT;
	ComPtr<ID3D12DescriptorHeap> m_pDSVHeapRT;
};
