#pragma once
#include "NXResourceManagerBase.h"

enum NXMaterialType;
class NXMaterialResourceManager : public NXResourceManagerBase
{
public:
	NXMaterialResourceManager() {}
	~NXMaterialResourceManager() {}

    void InitCommonMaterial();

    // ��ȡĬ�ϲ���
    NXMaterial* GetDefaultMaterial() { return m_pDefaultMaterial; }

    // ��ȡ��������
    const std::vector<NXMaterial*>& GetMaterials() { return m_pMaterialArray; }

    // ע��һ���²���newMaterial��
    void RegisterMaterial(NXMaterial* newMaterial);

    // ͨ�������ļ�·�����Ҳ��ʣ����û�ҵ��򷵻�nullptr��
    NXMaterial* FindMaterial(const std::filesystem::path& path);

    // �Ƴ�һ���ɲ���oldMaterial������һ���²���newMaterial��
    // �ڱ�� ���ʱ༭���еĲ������� ʱ����ô˷�����
    // 2023.3.26 ����ֻ��������������ɾ���������ʺ�������Դ�Ĺ��������ⲿʵ�֣���ReTypeMaterial��
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

	// �����������´���ĳ�����ʡ�
	// GUI�и��Ĳ�������ʱ����ʹ�ô��߼���
	void ReTypeMaterial(NXMaterial* pSrcMaterial, NXMaterialType destMaterialType);

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pDefaultMaterial = nullptr;
    std::vector<NXMaterial*> m_pMaterialArray;

	std::vector<NXMaterial*> m_pUnusedMaterials;
};
