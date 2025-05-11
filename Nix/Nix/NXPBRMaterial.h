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

	// 材质文件路径
	std::filesystem::path m_filePath;

private:
	// 映射表，记录哪些Submesh使用了这个材质
	std::vector<NXSubMeshBase*> m_pRefSubMeshes;
	UINT m_RefSubMeshesCleanUpCount;
};

// 2023.6.4
// 显示正在加载、加载失败等中间状态的过渡纹理
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

	// 初始化所有着色器资源，包括 cb, tex, sampler
	void InitShaderResources();

	virtual void Update() override;
	void Render(ID3D12GraphicsCommandList* pCommandList) override;
	void Release() override {}

	void SetTexture(const Ntr<NXTexture>& pTexture, const std::filesystem::path& texFilePath);
	void RemoveTexture(const Ntr<NXTexture>& pTexture);

	void SetCBInfoMemoryData();
	virtual NXShadingModel GetShadingModel() override;

	void RequestUpdateCBufferData(bool bNeedRebuildCB);

	// 将当前的 param, code, funcs[] 重新整合成 nsl 文件，
	// 一般在保存时调用此方法。
	void SaveToNSLFile();

	void SetSSSProfile(const std::filesystem::path& path) { m_sssProfilePath = path; }
	std::filesystem::path GetSSSProfilePath() const { return m_sssProfilePath; }
	void SetGBufferIndexInternal(UINT8 index) { m_sssProfileGBufferIndexInternal = index; }

	void Serialize() override;
	void Deserialize() override;

	bool GetCompileSuccess() { return m_bCompileSuccess; }

	// 获取 pso RS BS DS 三大状态
	void UpdatePSORenderStates(D3D12_GRAPHICS_PIPELINE_STATE_DESC& oPSODesc) override;

private:
	// 读取 nsl 文件
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

	// SSS profile 的路径
	std::filesystem::path					m_sssProfilePath;
	UINT8									m_sssProfileGBufferIndexInternal;
};
