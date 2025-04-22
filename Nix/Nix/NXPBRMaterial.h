#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"
#include "NXConstantBuffer.h"
#include "NXTexture.h"
#include "NXShaderDefinitions.h"
#include "NXHLSLGenerator.h"

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

	void LoadAndCompile(const std::filesystem::path& nslFilePath);

	bool LoadShaderCode();
	// 将 NSL 转换为 HLSL。
	void ConvertNSLToHLSL(NXHLSLGeneratorGBufferStrings& strBlocks);
	// 将 NSL 转换为 HLSL。另外将 GUI 修改后的参数也传了进来，这些 GUI 参数将作为新编译后的 Shader 的默认值。
	void ConvertGUIDataToHLSL(NXHLSLGeneratorGBufferStrings& strBlocks, const std::vector<NXGUICBufferData>& cbDataGUI, const NXGUICBufferSetsData& cbSettingsDataGUI, const std::vector<NXGUITextureData>& texDataGUI, const std::vector<NXGUISamplerData>& samplerDataGUI);
	void CompileShader(const std::string& strGBufferShader, std::string& oErrorMessageVS, std::string& oErrorMessagePS);
	bool Recompile(const std::string& nslParams, const std::vector<std::string>& nslFuncs, const std::vector<std::string>& nslTitles, const std::vector<NXGUICBufferData>& cbDefaultValues, const NXGUICBufferSetsData& cbSettingDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues, std::vector<NXHLSLCodeRegion>& oShaderFuncRegions, std::string& oErrorMessageVS, std::string& oErrorMessagePS);

	// 初始化所有着色器资源，包括 cb, tex, sampler
	void InitShaderResources();

	virtual void Update() override;
	void Render(ID3D12GraphicsCommandList* pCommandList) override;
	void Release() override {}

	void SetNSLParam(const std::string& nslParams) { m_nslParams = nslParams; }

	const std::vector<std::string>& GetNSLFuncs() { return m_nslFuncs; }
	const std::string& GetNSLFunc(UINT index);
	void SetNSLFunc(const std::string& nslFunc, UINT index);
	void SetNSLMainFunc(const std::string& nslFunc);

	void SortShaderCBufferParam();

	UINT GetCBufferElemCount() { return UINT(m_cbInfo.elems.size()); }
	const NXCBufferElem& GetCBufferElem(UINT index) { return m_cbInfo.elems[index]; }

	NXShadingModel GetShadingModel() { return NXShadingModel(m_cbInfo.sets.shadingModel + 1); }
	virtual void SetShadingModel(NXShadingModel shadingModel) { m_cbInfo.sets.shadingModel = (UINT)shadingModel; }
	const NXCBufferSets& GetCBufferSets() { return m_cbInfo.sets; }

	UINT GetTextureCount() { return UINT(m_texInfos.size()); }
	Ntr<NXTexture> GetTexture(UINT index) { return m_texInfos[index].pTexture; }
	const std::string& GetTextureName(UINT index) { return m_texInfos[index].name; }

	void SetTexture(const Ntr<NXTexture>& pTexture, const std::filesystem::path& texFilePath);
	void RemoveTexture(const Ntr<NXTexture>& pTexture);

	UINT GetSamplerCount() { return UINT(m_samplerInfos.size()); }
	const std::string& GetSamplerName(UINT index) { return m_samplerInfos[index].name; }
	NXSamplerFilter GetSamplerFilter(UINT index) { return m_samplerInfos[index].filter; }
	NXSamplerAddressMode GetSamplerAddressMode(UINT index, UINT uvwIndex) { return uvwIndex ? uvwIndex == 1 ? m_samplerInfos[index].addressV : m_samplerInfos[index].addressW : m_samplerInfos[index].addressU; }

	const float* GetCBInfoMemoryData(UINT memoryIndex) { return m_cbInfoMemory.data() + memoryIndex; }
	void SetCBInfoMemoryData(UINT memoryIndex, UINT count, const float* newData);

	NXGUICBufferStyle GetCBGUIStyles(UINT index) { return m_cbInfo.elems[index].style; }
	Vector2 GetCBGUIParams(UINT index) { return m_cbInfo.elems[index].guiParams; }
	NXGUITextureMode GetTextureGUIType(UINT index) { return m_texInfos[index].guiType; }

	void GenerateInfoBackup();
	void RecoverInfosBackup();

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
	// 读取 nsl 文件，获取 nsl shader.
	bool LoadShaderStringFromFile(std::string& shaderContent);
	// 将 nsl shader 拆成 params 和 code 两部分
	void ExtractShaderData(const std::string& shader, std::string& nslParams, std::vector<std::string>& nslFunc);

	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	void ProcessShaderParameters(
		const std::string& nslParams,
		std::string& oHLSLHeadCode,
		const std::vector<NXGUICBufferData>& cbDefaultValues = {},
		const NXGUICBufferSetsData& cbSettingDefaultValues = {},
		const std::vector<NXGUITextureData>& texDefaultValues = {},
		const std::vector<NXGUISamplerData>& samplerDefaultValues = {}
	);

	void ProcessShaderCBufferParam(std::istringstream& in, std::string& outStr, const std::vector<NXGUICBufferData>& cbDefaultValues = {}, const NXGUICBufferSetsData& cbSettingDefaultValues = {});

	// 将 nsl 的主函数 转换成 DX 可以编译的 hlsl 代码，
	void ProcessShaderMainFunc(std::string& oHLSLBodyCode);

	// 将 nsl 的其它函数 转换成 DX 可以编译的 hlsl 代码，
	void ProcessShaderFunctions(const std::vector<std::string>& nslFuncs, std::vector<std::string>& oHLSLFuncCode);

	void UpdateCBData(bool rebuildCB);

private:
	bool m_bIsDirty = true;
	bool m_bNeedRebuildCB = false;
	bool m_bCompileSuccess = false;

private:
	std::string								m_nslParams;
	std::vector<std::string>				m_nslFuncs;

	std::vector<NXMaterialSamplerInfo>		m_samplerInfos;
	std::vector<NXMaterialTextureInfo>		m_texInfos;
	NXMaterialCBufferInfo					m_cbInfo;
	std::vector<float>						m_cbInfoMemory;
	std::vector<int>						m_cbSortedIndex;

	NXConstantBuffer<std::vector<float>>	m_cbData;

	// backup datas
	std::vector<NXMaterialSamplerInfo>		m_samplerInfosBackup;
	std::vector<NXMaterialTextureInfo>		m_texInfosBackup;
	NXMaterialCBufferInfo					m_cbInfoBackup;
	std::vector<float>						m_cbInfoMemoryBackup;
	std::vector<int>						m_cbSortedIndexBackup;
	std::vector<std::string>				m_nslFuncsBackup;

	// SSS profile 的路径
	std::filesystem::path					m_sssProfilePath;
	UINT8									m_sssProfileGBufferIndexInternal;
};
