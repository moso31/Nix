#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"
#include "NXConstantBuffer.h"
#include "NXTexture.h"
#include "NXShaderDefinitions.h"
#include "NXCodeProcessHeader.h"

enum class NXShadingModel
{
	None,
	StandardLit,
	Unlit,
	SubSurface,
};

class NXEasyMaterial;
class NXCustomMaterial;
class NXSSSDiffuseProfile;

class NXMaterial : public NXObject, public NXSerializable
{
protected:
	explicit NXMaterial() = default;
	NXMaterial(const std::string& name, const std::filesystem::path& filePath = "");

public:
	virtual ~NXMaterial() {}
	virtual NXCustomMaterial* IsCustomMat() { return nullptr; }
	virtual NXEasyMaterial* IsEasyMat() { return nullptr; }

	const std::filesystem::path& GetFilePath() { return m_filePath; }
	size_t GetFilePathHash() { return std::filesystem::hash_value(m_filePath); }

	virtual NXShadingModel GetShadingModel() = 0;
	virtual void SetShadingModel(NXShadingModel shadingModel) {}

	virtual void Update() = 0;
	virtual void Render(ID3D12GraphicsCommandList* pCommandList) = 0;
	virtual void Release() = 0;

	virtual void Serialize() {}
	virtual void Deserialize() {}

	virtual void UpdatePSORenderStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& oPSODesc);

public:
	std::vector<NXSubMeshBase*> GetRefSubMeshes() { return m_pRefSubMeshes; }
	void RemoveSubMesh(NXSubMeshBase* pRemoveSubmesh);
	void AddSubMesh(NXSubMeshBase* pSubMesh);

protected:
	ComPtr<ID3D12PipelineState> m_pPSO;
	ComPtr<ID3D12RootSignature> m_pRootSig;

	// �����ļ�·��
	std::filesystem::path m_filePath;

private:
	// ӳ�����¼��ЩSubmeshʹ�����������
	std::vector<NXSubMeshBase*> m_pRefSubMeshes;
	UINT m_RefSubMeshesCleanUpCount;
};

// 2023.6.4
// ��ʾ���ڼ��ء�����ʧ�ܵ��м�״̬�Ĺ�������
class NXEasyMaterial : public NXMaterial
{
public:
	NXEasyMaterial(const std::string& name, const std::filesystem::path& filePath);
	NXEasyMaterial* IsEasyMat() override { return this; }

	void Init();
	void Update() override {}
	void Render(ID3D12GraphicsCommandList* pCommandList) override;
	void Release() override {}
	virtual NXShadingModel GetShadingModel() override { return NXShadingModel::Unlit; }

private:
	Ntr<NXTexture2D> m_pTexture;
};

class NXCustomMaterial : public NXMaterial
{
	template <typename NXMatParam>
	using ResourceMap = std::unordered_map<std::string, NXMatParam>;

public:
	explicit NXCustomMaterial() = default;
	NXCustomMaterial(const std::string& name, const std::filesystem::path& path);
	virtual ~NXCustomMaterial() {}
	NXCustomMaterial* IsCustomMat() override { return this; }

	const NXMaterialData& GetMaterialData() { return m_materialDatas; }
	const NXMaterialCode& GetMaterialCode() { return m_codeBlocks; }

	void LoadAndCompile(const std::filesystem::path& nslFilePath);

	bool LoadShaderCode();
	void CompileShader(const std::string& strGBufferShader, std::string& oErrorMessageVS, std::string& oErrorMessagePS);
	bool Recompile(const NXMaterialData& guiData, const NXMaterialCode& code, const NXMaterialData& guiDataBackup, const NXMaterialCode& codeBackup, std::string& oErrorMessageVS, std::string& oErrorMessagePS);

	// ��ʼ��������ɫ����Դ������ cb, tex, sampler
	void InitShaderResources();

	virtual void Update() override;
	void Render(ID3D12GraphicsCommandList* pCommandList) override;
	void Release() override {}

	void SetTexture(const Ntr<NXTexture>& pTexture, const std::filesystem::path& texFilePath);
	void RemoveTexture(const Ntr<NXTexture>& pTexture);

	void SetCBInfoMemoryData();
	virtual NXShadingModel GetShadingModel() override;

	void RequestUpdateCBufferData(bool bNeedRebuildCB);

	// ����ǰ�� param, code, funcs[] �������ϳ� nsl �ļ���
	// һ���ڱ���ʱ���ô˷�����
	void SaveToNSLFile();

	void SetSSSProfile(const std::filesystem::path& path) { m_sssProfilePath = path; }
	std::filesystem::path GetSSSProfilePath() const { return m_sssProfilePath; }
	void SetGBufferIndexInternal(UINT8 index) { m_sssProfileGBufferIndexInternal = index; }

	void Serialize() override;
	void Deserialize() override;

	bool GetCompileSuccess() { return m_bCompileSuccess; }

	// ��ȡ pso RS BS DS ����״̬
	void UpdatePSORenderStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& oPSODesc) override;

private:
	// ��ȡ nsl �ļ�
	bool LoadShaderStringFromFile(std::string& shaderContent);

	void UpdateCBData(bool rebuildCB);

private:
	bool m_bIsDirty = true;
	bool m_bNeedRebuildCB = false;
	bool m_bCompileSuccess = false;

private:
	std::filesystem::path m_nslPath;
	NXConstantBuffer<std::vector<float>>	m_cbData;

	NXMaterialData							m_materialDatas; 
	NXMaterialDataIntermediate				m_matDataIntermediate;
	NXMaterialCode							m_codeBlocks; 

	// SSS profile ��·��
	std::filesystem::path					m_sssProfilePath;
	UINT8									m_sssProfileGBufferIndexInternal;
};
