#pragma once
#include "NXResourceManagerBase.h"

enum NXMaterialType;
class NXMaterialResourceManager : public NXResourceManagerBase
{
public:
	NXMaterialResourceManager() {}
	~NXMaterialResourceManager() {}

    void InitCommonMaterial();

    // 获取默认材质
    NXMaterial* GetDefaultMaterial() { return m_pDefaultMaterial; }

    // 获取材质数组
    const std::vector<NXMaterial*>& GetMaterials() { return m_pMaterialArray; }

    // 注册一个新材质newMaterial。
    void RegisterMaterial(NXMaterial* newMaterial);

    // 通过材质文件路径查找材质，如果没找到则返回nullptr。
    NXMaterial* FindMaterial(const std::filesystem::path& path);

    // 移除一个旧材质oldMaterial，换上一个新材质newMaterial。
    // 在变更 材质编辑器中的材质类型 时会调用此方法。
    // 2023.3.26 这里只负责材质数组的增删操作。材质和其他资源的关联会在外部实现，见ReTypeMaterial。
    void ReplaceMaterial(NXMaterial* oldMaterial, NXMaterial* newMaterial);

    NXMaterial* LoadFromNmatFile(const std::filesystem::path& matFilePath);

    NXMaterial* LoadStandardPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath);
    NXMaterial* LoadTranslucentPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath);

	NXPBRMaterialStandard* CreatePBRMaterialStandard(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao,
		const std::wstring albedoTexFilePath = g_defaultTex_white_wstr,
		const std::wstring normalTexFilePath = g_defaultTex_normal_wstr,
		const std::wstring metallicTexFilePath = g_defaultTex_white_wstr,
		const std::wstring roughnessTexFilePath = g_defaultTex_white_wstr,
		const std::wstring aoTexFilePath = g_defaultTex_white_wstr,
		const std::string& folderPath = "");
	NXPBRMaterialTranslucent* CreatePBRMaterialTranslucent(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity,
		const std::wstring albedoTexFilePath = g_defaultTex_white_wstr,
		const std::wstring normalTexFilePath = g_defaultTex_normal_wstr,
		const std::wstring metallicTexFilePath = g_defaultTex_white_wstr,
		const std::wstring roughnessTexFilePath = g_defaultTex_white_wstr,
		const std::wstring aoTexFilePath = g_defaultTex_white_wstr,
		const std::string& folderPath = "");
	NXCustomMaterial* CreateCustomMaterial(const std::string& name, const std::string& nslFilePath);

	// 根据类型重新创建某个材质。
	// GUI中更改材质类型时，会使用此逻辑。
	void ReTypeMaterial(NXMaterial* pSrcMaterial, NXMaterialType destMaterialType);

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pDefaultMaterial = nullptr;
    std::vector<NXMaterial*> m_pMaterialArray;

	std::vector<NXMaterial*> m_pUnusedMaterials;
};
