#pragma once
#include "NXResourceManagerBase.h"

class NXTextureResourceManager : public NXResourceManagerBase
{
public:
	NXTextureResourceManager() {};
	~NXTextureResourceManager() {};

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

    void InitCommonRT(const Vector2& rtSize);
    void ResizeCommonRT(const Vector2& rtSize);
    NXTexture2D* GetCommonRT(NXCommonRTEnum eRT);

    void InitCommonTextures();
    NXTexture2D* GetCommonTextures(NXCommonTexEnum eTex);

    // 移除 m_pTextureArray 中所有 引用计数=0 的纹理。
    void ReleaseUnusedTextures();

    void OnReload() override;
    void Release() override;

    Ntr<NXTexture2D> CreateTexture2D_Internal(const std::string& name, const std::filesystem::path& FilePath, bool bForce = false);
    Ntr<NXTexture2D> CreateTexture2D_Internal(const std::string& name, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize = 1, UINT MipLevels = 0, UINT BindFlags = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0, UINT SampleCount = 1, UINT SampleQuality = 0, UINT MiscFlags = 0);

    Ntr<NXTextureCube> CreateTextureCube_Internal(const std::string& name, const std::wstring& FilePath, UINT Width = 0, UINT Height = 0);
    Ntr<NXTextureCube> CreateTextureCube_Internal(const std::string& name, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT MipLevels = 0, UINT BindFlags = D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0, UINT SampleCount = 1, UINT SampleQuality = 0, UINT MiscFlags = 0);

private:
    std::vector<NXTexture2D*> m_pCommonRT;
    std::vector<NXTexture2D*> m_pCommonTex;
    std::unordered_set<NXTexture*> m_pTextureArray;
    std::vector<Ntr<NXTexture>> m_pTextureArrayInternal;
};
