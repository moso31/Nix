#pragma once
#include "NXResourceManagerBase.h"

class NXSSSDiffuseProfile;
class NXMaterialResourceManager : public NXResourceManagerBase
{
public:
	NXMaterialResourceManager() {}
	~NXMaterialResourceManager() {}

    void InitCommonMaterial();

    // ��ʾ ������ ״̬�Ĺ��ɲ���
    NXMaterial* GetLoadingMaterial() { return m_pLoadingMaterial; }

    // ��ʾ ���ش��� ״̬�Ĺ��ɲ���
    NXMaterial* GetErrorMaterial() { return m_pErrorMaterial; }

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

    NXMaterial* LoadFromNSLFile(const std::filesystem::path& matFilePath);

	NXCustomMaterial* CreateCustomMaterial(const std::string& name, const std::filesystem::path& nslFilePath);

    void CreateSSSProfile(const std::filesystem::path& sssProfFilePath);
    Ntr<NXSSSDiffuseProfile> GetSSSProfile(const std::filesystem::path& sssProfFilePath, bool tryCreate = false);

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pLoadingMaterial = nullptr;   // ������ʾ ������ ״̬�Ĳ���
    NXMaterial* m_pErrorMaterial = nullptr;     // ������ʾ ���ش��� ״̬�Ĳ���
    std::vector<NXMaterial*> m_pMaterialArray;

	std::vector<NXMaterial*> m_pUnusedMaterials;

    // ��¼���г�����ʹ�õ� SSS Profiler
    std::vector<Ntr<NXSSSDiffuseProfile>> m_pSSSProfileArray;
};
