#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/Math.h"
#include "NXResourceManagerBase.h"
#include "NXConstantBuffer.h"

using namespace DirectX::SimpleMath;

using PathHashValue = size_t;

struct DiffuseProfileData
{
    Vector3 scatterParam; // 1.0 / scatterDistance.xyz
    float maxScatterDist; 

    Vector3 transmit;
    float transmitStrength;
};

struct CBufferDiffuseProfileData
{
    DiffuseProfileData sssProfData[16];
};

class NXSSSDiffuseProfile;
class NXMaterialResourceManager : public NXResourceManagerBase
{
public:
	NXMaterialResourceManager() {}
    virtual ~NXMaterialResourceManager() {}

    void Init();

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
    const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>& GetCBufferDiffuseProfile() { return m_cbDiffuseProfile.GetFrameGPUAddresses(); }

	void OnReload() override;
	void Release() override;

private:
    void ReleaseUnusedMaterials();
    void AdjustSSSProfileMapToGBufferIndex();
    void AdjustDiffuseProfileRenderData(PathHashValue pathHash, UINT index);

private:
    NXMaterial* m_pLoadingMaterial = nullptr;   // ������ʾ ������ ״̬�Ĳ���
    NXMaterial* m_pErrorMaterial = nullptr;     // ������ʾ ���ش��� ״̬�Ĳ���
    std::vector<NXMaterial*> m_pMaterialArray;

	std::vector<NXMaterial*> m_pUnusedMaterials;

    Ntr<NXSSSDiffuseProfile> m_defaultDiffuseProfile;

    // ��¼���г�����ʹ�õ� SSS Profiler
    std::map<PathHashValue, Ntr<NXSSSDiffuseProfile>> m_sssProfilesMap;

    // 2023.11.11
    // �� Nix �У�GBuffer ��ʹ��ĳ��RT������������RT����������ش��룩�� 8bit����¼��ǰ����ʹ�����ĸ� SSSProfile��
    // m_SSSProfileCBufferIndexMap ������ԭʼ�ļ� HashValue �� 8bit ֮�佨��һ��һӳ�䡣
    // �ɴ˾Ϳ���֪�� m_sssProfilesMap ��ÿ�� SSSProfile �� GBufferRT �е� 8bit ��š�
    std::map<PathHashValue, UINT8> m_sssProfileGBufferIndexMap;

    // SSS profile CBuffer
    CBufferDiffuseProfileData m_diffuseProfileData;
    NXConstantBuffer<CBufferDiffuseProfileData> m_cbDiffuseProfile;
};
