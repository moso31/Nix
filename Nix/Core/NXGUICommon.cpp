#include "NXGUICommon.h"
#include <fstream>
#include "NXConverter.h"
#include "NXPBRMaterial.h"
#include "NXResourceManager.h"

NXTextureInfoData::NXTextureInfoData() :
    eTexType(NXTextureType::Default),
    bSRGB(false),
    bInvertNormalY(false),
    bGenerateMipMap(true),
    bCubeMap(false)
{
}

namespace NXGUICommon
{

void CreateDefaultMaterialFile(const std::filesystem::path& path, const std::string& matName)
{
    // �½�һ��StandardPBR����
    std::ofstream ofs(path, std::ios::binary);
    ofs << matName << std::endl << "Standard\n"; // �������ƣ���������
    ofs << "?\n" << 1.0f << ' ' << 1.0f << ' ' << 1.0f << ' ' << std::endl; // albedo
    ofs << "?\n" << 1.0f << ' ' << 1.0f << ' ' << 1.0f << ' ' << std::endl; // normal
    ofs << "?\n" << 1.0f << std::endl; // metallic
    ofs << "?\n" << 1.0f << std::endl; // roughness
    ofs << "?\n" << 1.0f << std::endl; // AO
    ofs.close();
}

void SaveMaterialFile(NXMaterial* pMaterial)
{
    std::ofstream ofs(pMaterial->GetFilePath(), std::ios::binary);

    ofs << pMaterial->GetName() << std::endl; // �������ƣ���������

    if (pMaterial->GetType() == NXMaterialType::PBR_STANDARD)
    {
        ofs << "Standard" << std::endl;
        NXPBRMaterialStandard* p = (NXPBRMaterialStandard*)pMaterial;

        const Vector3& albedo = p->GetAlbedo();
        const std::string albedoTexPath = p->GetAlbedoTexFilePath();
        ofs << albedoTexPath << std::endl << albedo.x << ' ' << albedo.y << ' ' << albedo.z << std::endl; // albedo

        const Vector3& normal = p->GetNormal();
        const std::string normalTexPath = p->GetNormalTexFilePath();
        ofs << normalTexPath << std::endl << normal.x << ' ' << normal.y << ' ' << normal.z << std::endl; // normal

        const std::string metallicTexPath = p->GetMetallicTexFilePath();
        float metallic = *p->GetMetallic();
        ofs << metallicTexPath << std::endl << metallic << std::endl; // metallic

        const std::string roughnessTexPath = p->GetRoughnessTexFilePath();
        float roughness = *p->GetRoughness();
        ofs << roughnessTexPath << std::endl << roughness << std::endl; // roughness

        const std::string aoTexPath = p->GetAOTexFilePath();
        float ao = *p->GetAO();
        ofs << aoTexPath << std::endl << ao << std::endl; // AO
    }

    if (pMaterial->GetType() == NXMaterialType::PBR_TRANSLUCENT)
    {
        ofs << "Translucent" << std::endl;
        NXPBRMaterialTranslucent* p = (NXPBRMaterialTranslucent*)pMaterial;

        const Vector3& albedo = p->GetAlbedo();
        float opacity = *p->GetOpacity();
        const std::string albedoTexPath = p->GetAlbedoTexFilePath();
        ofs << albedoTexPath << std::endl << albedo.x << ' ' << albedo.y << ' ' << albedo.z << ' ' << opacity << std::endl; // albedo & opacity

        const Vector3& normal = p->GetNormal();
        const std::string normalTexPath = p->GetNormalTexFilePath();
        ofs << normalTexPath << std::endl << normal.x << ' ' << normal.y << ' ' << normal.z << std::endl; // normal

        const std::string metallicTexPath = p->GetMetallicTexFilePath();
        float metallic = *p->GetMetallic();
        ofs << metallicTexPath << std::endl << metallic << std::endl; // metallic

        const std::string roughnessTexPath = p->GetRoughnessTexFilePath();
        float roughness = *p->GetRoughness();
        ofs << roughnessTexPath << std::endl << roughness << std::endl; // roughness

        const std::string aoTexPath = p->GetAOTexFilePath();
        float ao = *p->GetAO();
        ofs << aoTexPath << std::endl << ao << std::endl; // AO
    }

    ofs.close();
}

NXTextureInfoData LoadTextureInfoFile(const std::filesystem::path& path)
{
    NXTextureInfoData result;

    std::string strTexInfoPath = path.string() + ".nxInfo";
    std::ifstream ifs(strTexInfoPath, std::ios::binary);

    // nxInfo ·�����û�򿪣��ͷ���һ������ֵ����Ĭ��ֵ�� InfoData
    if (!ifs.is_open())
        return result;

    std::string strIgnore;

    size_t nHashFile;
    ifs >> nHashFile;
    std::getline(ifs, strIgnore);

    // �����ȡ·�����ļ��ڵ�·��Hash�Բ��ϣ�Ҳ����Ĭ�� InfoData
    size_t nHashPath = std::filesystem::hash_value(path);
    if (nHashFile != nHashPath)
        return result;

    ifs.close();

    return result;
}

void SaveTextureInfoFile(NXTexture* pTexture, const NXTextureInfoData& info)
{
    if (!pTexture)
        return;

    auto path = pTexture->GetFilePath();
    if (path.empty())
        return;

    std::string strPath = path.string().c_str();
    std::string strPathInfo = strPath + ".nxInfo";

    std::ofstream ofs(strPathInfo, std::ios::binary);

    // �ļ���ʽ��
    // �����ļ�·���Ĺ�ϣ
    // (int)TexFormat, Width, Height, Arraysize, Miplevel
    // (int)TextureType, (int)IsSRGB, (int)IsInvertNormalY, (int)IsGenerateCubeMap, (int)IsCubeMap

    size_t pathHashValue = std::filesystem::hash_value(pTexture->GetFilePath());
    ofs << pathHashValue << std::endl;
    ofs << (int)pTexture->GetFormat() << ' ' << pTexture->GetWidth() << ' ' << pTexture->GetHeight() << ' ' << pTexture->GetArraySize() << ' ' << pTexture->GetMipLevels() << std::endl;
    ofs << (int)info.eTexType << ' ' << (int)info.bSRGB << ' ' << (int)info.bInvertNormalY << ' ' << (int)info.bGenerateMipMap << ' ' << (int)info.bCubeMap << std::endl;

    ofs.close();
}

}