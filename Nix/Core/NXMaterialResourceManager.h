#pragma once
#include "NXResourceManagerBase.h"

class NXSSSDiffuseProfile;
class NXMaterialResourceManager : public NXResourceManagerBase
{
public:
	NXMaterialResourceManager() {}
	~NXMaterialResourceManager() {}

    void InitCommonMaterial();

    // 表示 加载中 状态的过渡材质
    NXMaterial* GetLoadingMaterial() { return m_pLoadingMaterial; }

    // 表示 加载错误 状态的过渡材质
    NXMaterial* GetErrorMaterial() { return m_pErrorMaterial; }

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

    NXMaterial* LoadFromNSLFile(const std::filesystem::path& matFilePath);

	NXCustomMaterial* CreateCustomMaterial(const std::string& name, const std::filesystem::path& nslFilePath);

    Ntr<NXSSSDiffuseProfile> GetOrAddSSSProfile(const std::filesystem::path& sssProfFilePath);

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pLoadingMaterial = nullptr;   // 用于显示 加载中 状态的材质
    NXMaterial* m_pErrorMaterial = nullptr;     // 用于显示 加载错误 状态的材质
    std::vector<NXMaterial*> m_pMaterialArray;

	std::vector<NXMaterial*> m_pUnusedMaterials;

    // 记录所有场景中使用的 SSS Profiler
    std::map<size_t, Ntr<NXSSSDiffuseProfile>> m_SSSProfilesMap;

    // 在 Nix 中，GBuffer 将使用某张RT（具体是哪张RT，见最新相关代码）的 8bit，记录当前像素使用了哪个 SSSProfile。
    // m_SSSProfileCBufferIndexMap 负责在原始文件 HashValue 和 8bit 之间建立一对一映射。
    // 由此就可以知道 m_SSSProfilesMap 的每个 SSSProfile 在 GBufferRT 中的 8bit 编号。
    std::map<size_t, UINT8> m_SSSProfileGBufferIndexMap;
};
