#pragma once
#include "NXResourceManagerBase.h"

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

	void OnReload() override;
	void Release() override;

private:
    NXMaterial* m_pDefaultMaterial = nullptr;
    std::vector<NXMaterial*> m_pMaterialArray;
};
