#pragma once
#include "NXResourceManagerBase.h"

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

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pDefaultMaterial = nullptr;
    std::vector<NXMaterial*> m_pMaterialArray;
};
