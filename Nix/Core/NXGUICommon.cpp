#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXPBRMaterial.h"
#include "NXResourceManager.h"

namespace NXGUICommon
{

void CreateDefaultMaterialFile(const std::filesystem::path& path, const std::string& matName)
{
    // 新建一个StandardPBR材质
    std::ofstream ofs(path, std::ios::binary);
    ofs << matName << std::endl << "Standard\n"; // 材质名称，材质类型
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

    ofs << pMaterial->GetName() << std::endl; // 材质名称，材质类型

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

}