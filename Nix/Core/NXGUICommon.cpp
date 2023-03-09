#include "NXGUICommon.h"
#include <fstream>
#include "NXPBRMaterial.h"

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

void UpdateMaterialFile(NXMaterial* pMaterial)
{
    std::ofstream ofs(pMaterial->GetFilePath(), std::ios::binary);

    ofs << pMaterial->GetName() << std::endl << pMaterial->GetType(); // 材质名称，材质类型

    if (pMaterial->GetType() == NXMaterialType::PBR_STANDARD)
    {
        NXPBRMaterialStandard* p = (NXPBRMaterialStandard*)pMaterial;

        const Vector3& albedo = p->GetAlbedo();
        const std::string albedoTexPath = p->GetAlbedoTexFilePath();
        ofs << albedoTexPath << std::endl << albedo.x << ' ' << albedo.y << ' ' << albedo.z << std::endl; // albedo

        const Vector3& normal = p->GetNormal();
        const std::string normalTexPath = p->GetNormalTexFilePath();
        ofs << normalTexPath << std::endl << normal.x << ' ' << normal.y << ' ' << normal.z << std::endl; // normal

        const std::string metallicTexPath = p->GetMetallicTexFilePath();
        ofs << metallicTexPath << std::endl << p->GetMetallic() << std::endl; // metallic

        const std::string roughnessTexPath = p->GetRoughnessTexFilePath();
        ofs << roughnessTexPath << std::endl << p->GetRoughness() << std::endl; // roughness

        const std::string aoTexPath = p->GetAOTexFilePath();
        ofs << aoTexPath << std::endl << p->GetAO() << std::endl; // AO
    }

    ofs.close();
}

}
