#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"
#include "NXTexture.h"
#include "NXShaderDefinitions.h"

enum class NXShadingModel
{
	None,
	StandardLit,
	Unlit,
	SubSurface,
};

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

	virtual void SetShadingModel(NXShadingModel shadingModel) {}

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Release() = 0;

	virtual void Serialize() {}
	virtual void Deserialize() {}

public:
	std::vector<NXSubMeshBase*> GetRefSubMeshes() { return m_pRefSubMeshes; }
	void RemoveSubMesh(NXSubMeshBase* pRemoveSubmesh);
	void AddSubMesh(NXSubMeshBase* pSubMesh);

protected:
	std::string m_name;
	ComPtr<ID3D11Buffer> m_cb;

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
	// �� NSL ת��Ϊ HLSL��
	void ConvertNSLToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody);
	// �� NSL ת��Ϊ HLSL�����⽫ GUI �޸ĺ�Ĳ���Ҳ���˽�������Щ GUI ��������Ϊ�±����� Shader ��Ĭ��ֵ��
	void ConvertGUIDataToHLSL(std::string& oHLSLHead, std::vector<std::string>& oHLSLFuncs, std::string& oHLSLBody, const std::vector<NXGUICBufferData>& cbDataGUI, const NXGUICBufferSetsData& cbSettingsDataGUI, const std::vector<NXGUITextureData>& texDataGUI, const std::vector<NXGUISamplerData>& samplerDataGUI);
	void CompileShader(const std::string& strGBufferShader, std::string& oErrorMessageVS, std::string& oErrorMessagePS);
	bool Recompile(const std::string& nslParams, const std::vector<std::string>& nslFuncs, const std::vector<std::string>& nslTitles, const std::vector<NXGUICBufferData>& cbDefaultValues, const NXGUICBufferSetsData& cbSettingDefaultValues, const std::vector<NXGUITextureData>& texDefaultValues, const std::vector<NXGUISamplerData>& samplerDefaultValues, std::vector<NXHLSLCodeRegion>& oShaderFuncRegions, std::string& oErrorMessageVS, std::string& oErrorMessagePS);

	// ��ʼ��������ɫ����Դ������ cb, tex, sampler
	void InitShaderResources();

	virtual void Update() override;
	void Render();
	void Release() override {}

	void SetNSLParam(const std::string& nslParams) { m_nslParams = nslParams; }

	const std::vector<std::string>& GetNSLFuncs() { return m_nslFuncs; }
	const std::string& GetNSLFunc(UINT index);
	void SetNSLFunc(const std::string& nslFunc, UINT index);
	void SetNSLMainFunc(const std::string& nslFunc);

	void SortShaderCBufferParam();

	UINT GetCBufferElemCount() { return UINT(m_cbInfo.elems.size()); }
	const NXCBufferElem& GetCBufferElem(UINT index) { return m_cbInfo.elems[index]; }

	const NXShadingModel& GetShadingModel() { return NXShadingModel(m_cbInfo.sets.shadingModel + 1); }
	virtual void SetShadingModel(NXShadingModel shadingModel) { m_cbInfo.sets.shadingModel = (UINT)shadingModel; }
	const NXCBufferSets& GetCBufferSets() { return m_cbInfo.sets; }

	UINT GetTextureCount() { return UINT(m_texInfos.size()); }
	NXTexture* GetTexture(UINT index) { return m_texInfos[index].pTexture; }
	const std::string& GetTextureName(UINT index) { return m_texInfos[index].name; }

	void SetTexture(NXTexture* pTexture, const std::filesystem::path& texFilePath);
	void RemoveTexture(NXTexture* pTexture);

	UINT GetSamplerCount() { return UINT(m_samplerInfos.size()); }
	const std::string& GetSamplerName(UINT index) { return m_samplerInfos[index].name; }
	NXSamplerFilter GetSamplerFilter(UINT index) { return m_samplerInfos[index].filter; }
	NXSamplerAddressMode GetSamplerAddressMode(UINT index, UINT uvwIndex) { return uvwIndex ? uvwIndex == 1 ? m_samplerInfos[index].addressV : m_samplerInfos[index].addressW : m_samplerInfos[index].addressU; }

	const float* GetCBInfoMemoryData(UINT memoryIndex) { return m_cbInfoMemory.data() + memoryIndex; }
	void SetCBInfoMemoryData(UINT memoryIndex, UINT count, const float* newData);

	NXGUICBufferStyle GetCBGUIStyles(UINT index) { return m_cbInfo.elems[index].style; }
	Vector2 GetCBGUIParams(UINT index) { return m_cbInfo.elems[index].guiParams; }
	NXGUITextureType GetTextureGUIType(UINT index) { return m_texInfos[index].guiType; }

	void GenerateInfoBackup();
	void RecoverInfosBackup();

	void RequestUpdateCBufferData() { m_bIsDirty = true; }

	// ����ǰ�� param, code, funcs[] �������ϳ� nsl �ļ���
	// һ���ڱ���ʱ���ô˷�����
	void SaveToNSLFile();

	void Serialize() override;
	void Deserialize() override;

	bool GetCompileSuccess() { return m_bCompileSuccess; }

private:
	// ��ȡ nsl �ļ�����ȡ nsl shader.
	bool LoadShaderStringFromFile(std::string& shaderContent);
	// �� nsl shader ��� params �� code ������
	void ExtractShaderData(const std::string& shader, std::string& nslParams, std::vector<std::string>& nslFunc);

	// �� nsl params ת���� DX ���Ա���� hlsl ���룬
	// ͬʱ������зּ𣬽� cb ���浽 m_cbInfo�������浽 m_texInfoMap�����������浽 m_ssInfoMap
	void ProcessShaderParameters(
		const std::string& nslParams,
		std::string& oHLSLHeadCode,
		const std::vector<NXGUICBufferData>& cbDefaultValues = {},
		const NXGUICBufferSetsData& cbSettingDefaultValues = {},
		const std::vector<NXGUITextureData>& texDefaultValues = {},
		const std::vector<NXGUISamplerData>& samplerDefaultValues = {}
	);

	void ProcessShaderCBufferParam(std::istringstream& in, std::ostringstream& out, const std::vector<NXGUICBufferData>& cbDefaultValues = {}, const NXGUICBufferSetsData& cbSettingDefaultValues = {});

	// �� nsl �������� ת���� DX ���Ա���� hlsl ���룬
	void ProcessShaderMainFunc(std::string& oHLSLBodyCode);

	// �� nsl ���������� ת���� DX ���Ա���� hlsl ���룬
	void ProcessShaderFunctions(const std::vector<std::string>& nslFuncs, std::vector<std::string>& oHLSLFuncCode);

	void UpdateCBData();

private:
	bool m_bIsDirty;
	bool m_bCompileSuccess = false;

private:
	std::string							m_nslParams;
	std::vector<std::string>			m_nslFuncs;

	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	std::vector<NXMaterialSamplerInfo>	m_samplerInfos;
	std::vector<NXMaterialTextureInfo>	m_texInfos;
	NXMaterialCBufferInfo				m_cbInfo;
	std::vector<float>					m_cbInfoMemory;
	std::vector<int>					m_cbSortedIndex;

	std::vector<float>					m_cbData;

	// backup datas
	std::vector<NXMaterialSamplerInfo>	m_samplerInfosBackup;
	std::vector<NXMaterialTextureInfo>	m_texInfosBackup;
	NXMaterialCBufferInfo				m_cbInfoBackup;
	std::vector<float>					m_cbInfoMemoryBackup;
	std::vector<int>					m_cbSortedIndexBackup;
	std::vector<std::string>			m_nslFuncsBackup;
};
