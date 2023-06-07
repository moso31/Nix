#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"
#include "NXTexture.h"
#include "NXShaderDefinitions.h"

class NXEasyMaterial;
class NXCustomMaterial;
class NXMaterial : public NXSerializable
{
protected:
	explicit NXMaterial() = default;
	NXMaterial(const std::string& name, const std::filesystem::path& filePath = "");

public:
	virtual ~NXMaterial() {}
	virtual NXCustomMaterial* IsCustomMat() { return nullptr; }
	virtual NXEasyMaterial* IsEasyMat() { return nullptr; }

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	const std::filesystem::path& GetFilePath() { return m_filePath; }
	size_t GetFilePathHash() { return std::filesystem::hash_value(m_filePath); }

	ID3D11Buffer* GetConstantBuffer() const { return m_cb.Get(); }

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Release() = 0;

	virtual void Serialize() {}
	virtual void Deserialize() {}

public:
	std::vector<NXSubMeshBase*> GetRefSubMeshes() { return m_pRefSubMeshes; }
	void RemoveSubMesh(NXSubMeshBase* pRemoveSubmesh);
	void AddSubMesh(NXSubMeshBase* pSubMesh);

	void SetTex2D(NXTexture2D*& pTex2D, const std::filesystem::path& texFilePath);

protected:
	std::string m_name;
	ComPtr<ID3D11Buffer> m_cb;

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
	struct CBufferData
	{
		Vector2 uvScale;
		Vector2 _0;
	};

public:
	NXEasyMaterial(const std::string& name, const std::filesystem::path& filePath);
	NXEasyMaterial* IsEasyMat() override { return this; }

	void Init();
	void Update() override;
	void Render() override;
	void Release() override {}

private:
	void InitConstantBuffer();

private:
	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;
	ComPtr<ID3D11SamplerState>			m_pSamplerLinearWrap;

	NXTexture2D* m_pTexture;
	CBufferData m_cbData;
};

class NXCustomMaterial : public NXMaterial
{
	template <typename NXMatParam>
	using ResourceMap = std::unordered_map<std::string, NXMatParam>;

public:
	explicit NXCustomMaterial() = default;
	NXCustomMaterial(const std::string& name, const std::filesystem::path& path);
	~NXCustomMaterial() {}
	NXCustomMaterial* IsCustomMat() override { return this; }

	void LoadShaderCode();
	// 将 NSL 转换为 HLSL。
	void ConvertNSLToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody);
	// 将 NSL 转换为 HLSL。另外将 GUI 修改后的参数也传了进来，这些 GUI 参数将作为新编译后的 Shader 的默认值。
	void ConvertGUIDataToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody, const std::vector<NXGUICBufferData>& cbDataGUI, const std::vector<NXGUITextureData>& texDataGUI, const std::vector<NXGUISamplerData>& samplerDataGUI);
	bool CompileShader(const std::string& strHLSLHead, const std::vector<std::string>& strHLSLFunc, const std::string& strHLSLBody, std::string& oErrorMessageVS, std::string& oErrorMessagePS);
	bool Recompile(const std::string& nslParams, const std::vector<std::string>& nslFuncs, const std::string& nslCode, const std::vector<NXGUICBufferData>& cbDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues, std::string& oErrorMessageVS, std::string& oErrorMessagePS);

	// 初始化所有着色器资源，包括 cb, tex, sampler
	void InitShaderResources();

	virtual void Update() override;

	void Render();

	void Release() override {}

	const std::string& GetNSLCode() { return m_nslCode; }
	void SetNSLCode(const std::string& nslCode) { m_nslCode = nslCode; }
	void SetNSLParam(const std::string& nslParams) { m_nslParams = nslParams; }

	const std::vector<std::string>& GetNSLFuncs() { return m_nslFuncs; }

	void SortShaderCBufferParam();

	UINT GetCBufferElemCount() { return UINT(m_cbInfo.elems.size()); }
	const NXCBufferElem& GetCBufferElem(UINT index) { return m_cbInfo.elems[index]; }

	UINT GetTextureCount() { return UINT(m_texInfos.size()); }
	NXTexture2D* GetTexture(UINT index) { return m_texInfos[index].pTexture; }
	const std::string& GetTextureName(UINT index) { return m_texInfos[index].name; }

	UINT GetSamplerCount() { return UINT(m_samplerInfos.size()); }
	const ComPtr<ID3D11SamplerState>& GetSampler(UINT index) { return m_samplerInfos[index].pSampler; }
	const std::string& GetSamplerName(UINT index) { return m_samplerInfos[index].name; }

	const float* GetCBInfoMemoryData(UINT memoryIndex) { return m_cbInfoMemory.data() + memoryIndex; }
	void SetCBInfoMemoryData(UINT memoryIndex, UINT count, const float* newData);

	NXGUICBufferStyle GetCBGUIStyles(UINT index) { return m_cbInfoGUIStyles[index]; }

	void GenerateInfoBackup();
	void RecoverInfosBackup();

	void RequestUpdateCBufferData() { m_bIsDirty = true; }

	void Serialize() override;
	void Deserialize() override;

private:
	// 读取 nsl 文件，获取 nsl shader.
	bool LoadShaderStringFromFile(std::string& shaderContent);
	// 将 nsl shader 拆成 params 和 code 两部分
	void ExtractShaderData(const std::string& shader, std::string& nslParams, std::string& nslCode, std::vector<std::string>& nslFunc);

	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	void ProcessShaderParameters(
		const std::string& nslParams,
		std::string& oHLSLHeadCode,
		const std::vector<NXGUICBufferData>& cbDefaultValues = {},
		const std::vector<NXGUITextureData>& texDefaultValues = {},
		const std::vector<NXGUISamplerData>& samplerDefaultValues = {}
	);

	void ProcessShaderCBufferParam(std::istringstream& in, std::ostringstream& out, const std::vector<NXGUICBufferData>& cbDefaultValues = {});

	// 将 nsl funcs 转换成 DX 可以编译的 hlsl 代码，
	void ProcessShaderFunctions(const std::vector<std::string>& nslFuncs, std::vector<std::string>& oHLSLFuncCode);

	// 将 nsl code 转换成 DX 可以编译的 hlsl 代码，
	void ProcessShaderCode(const std::string& nslCode, std::string& oHLSLBodyCode);

	void UpdateCBData();

private:
	std::string							m_nslParams;
	std::string							m_nslCode;
	std::vector<std::string>			m_nslFuncs;

	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	std::vector<NXMaterialSamplerInfo>	m_samplerInfos;
	std::vector<NXMaterialTextureInfo>	m_texInfos;
	NXMaterialCBufferInfo				m_cbInfo;
	std::vector<float>					m_cbInfoMemory;
	std::vector<int>					m_cbSortedIndex;

	std::vector<NXGUICBufferStyle>		m_cbInfoGUIStyles;

	std::vector<float>					m_cbData;
	bool m_bIsDirty;

	// backup datas
	std::vector<NXMaterialSamplerInfo>	m_samplerInfosBackup;
	std::vector<NXMaterialTextureInfo>	m_texInfosBackup;
	NXMaterialCBufferInfo				m_cbInfoBackup;
	std::vector<float>					m_cbInfoMemoryBackup;
	std::vector<int>					m_cbSortedIndexBackup;
	std::vector<NXGUICBufferStyle>		m_cbInfoGUIStylesBackup;
	std::string							m_nslCodeBackup;
	std::vector<std::string>			m_nslFuncsBackup;
};
