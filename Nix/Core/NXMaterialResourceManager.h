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

    Ntr<NXSSSDiffuseProfile> GetOrAddSSSProfile(const std::filesystem::path& sssProfFilePath);

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pLoadingMaterial = nullptr;   // ������ʾ ������ ״̬�Ĳ���
    NXMaterial* m_pErrorMaterial = nullptr;     // ������ʾ ���ش��� ״̬�Ĳ���
    std::vector<NXMaterial*> m_pMaterialArray;

	std::vector<NXMaterial*> m_pUnusedMaterials;

    // ��¼���г�����ʹ�õ� SSS Profiler
    std::map<size_t, Ntr<NXSSSDiffuseProfile>> m_SSSProfilesMap;

    // �� Nix �У�GBuffer ��ʹ��ĳ��RT������������RT����������ش��룩�� 8bit����¼��ǰ����ʹ�����ĸ� SSSProfile��
    // m_SSSProfileCBufferIndexMap ������ԭʼ�ļ� HashValue �� 8bit ֮�佨��һ��һӳ�䡣
    // �ɴ˾Ϳ���֪�� m_SSSProfilesMap ��ÿ�� SSSProfile �� GBufferRT �е� 8bit ��š�
    std::map<size_t, UINT8> m_SSSProfileGBufferIndexMap;
};
