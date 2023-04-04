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

    void InitCommonRT();
    NXTexture2D* GetCommonRT(NXCommonRTEnum eRT);

    void InitCommonTextures();
    NXTexture2D* GetCommonTextures(NXCommonTexEnum eTex);

    // ����������ԴԪ�ļ���TextureNXInfo��
    // ���û�ҵ���Ӧ��Ԫ�ļ����򽫷���һ�����в�����ΪĬ��ֵ��Ԫ�ļ���
    TextureNXInfo* LoadTextureInfo(const std::filesystem::path& texFilePath);
    void SaveTextureInfo(const TextureNXInfo* pInfo, const std::filesystem::path& texFilePath);

    // �Ƴ� m_pTextureArray ������ ���ü���=0 ��������
    void ReleaseUnusedTextures();

    void OnReload() override;
    void Release() override;

private:
    std::vector<NXTexture2D*> m_pCommonRT;
    std::vector<NXTexture2D*> m_pCommonTex;
    std::unordered_set<NXTexture*> m_pTextureArray;
};