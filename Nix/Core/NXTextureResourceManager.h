#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "NXResourceManagerBase.h"

class NXTextureResourceManager : public NXResourceManagerBase
{
public:
	NXTextureResourceManager() {};
    virtual ~NXTextureResourceManager() {};

    void InitCommonTextures();
    Ntr<NXTexture2D> GetCommonTextures(NXCommonTexEnum eTex);

    void OnReload() override;
    void Release() override;

    Ntr<NXTexture> CreateTextureAuto(const std::string& name, const std::filesystem::path& filePath, bool bForce = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2D> CreateTexture2D(const std::string& name, const std::filesystem::path& filePath, bool bForce = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2D> CreateRenderTexture(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2D> CreateUAVTexture(const std::string& name, DXGI_FORMAT fmt, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2D> CreateTexture2DSubRegion(const std::string& name, const std::filesystem::path& filePath, const Int2& subRegionXY, const Int2& subRegionSize, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);

    Ntr<NXTextureCube> CreateTextureCube(const std::string& name, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTextureCube> CreateTextureCube(const std::string& name, DXGI_FORMAT texFormat, UINT width, UINT height, UINT mipLevels = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);

    Ntr<NXTexture2DArray> CreateTexture2DArray(const std::string& debugName, const std::wstring& filePath, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2DArray> CreateTexture2DArray(const std::string& debugName, const std::wstring& filePath, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2DArray> CreateRenderTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize = 1, UINT mipLevels = 0, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, bool bAutoMakeViews = true);
    Ntr<NXTexture2DArray> CreateUAVTexture2DArray(const std::string& debugName, DXGI_FORMAT texFormat, UINT width, UINT height, UINT arraySize = 1, UINT mipLevels = 1, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, bool bAutoMakeViews = true);

private:
    std::vector<Ntr<NXTexture2D>> m_pCommonTex;
    std::vector<Ntr<NXTexture>> m_pTextureArrayInternal;
};
