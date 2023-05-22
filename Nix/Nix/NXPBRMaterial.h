#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"
#include "NXTexture.h"
#include "NXShaderDefinitions.h"

enum NXMaterialType
{
	UNKNOWN,
	PBR_STANDARD,
	PBR_TRANSLUCENT,
	PBR_SUBSURFACE,
	CUSTOM,
};

struct CBufferMaterial 
{
};

struct CBufferMaterialStandard : public CBufferMaterial
{
	CBufferMaterialStandard(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao) :
		albedo(albedo), normal(normal), metallic(metallic), roughness(roughness), ao(ao) {}
	Vector3 albedo;
	float _0;
	Vector3 normal;
	float metallic;
	float roughness;
	float ao;
	Vector2 _1;
};

struct CBufferMaterialTranslucent : public CBufferMaterial
{
	CBufferMaterialTranslucent(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity) :
		albedo(albedo), normal(normal), metallic(metallic), roughness(roughness), ao(ao), opacity(opacity) {}
	Vector3 albedo;
	float opacity;
	Vector3 normal;
	float metallic;
	float roughness;
	float ao;
	Vector2 _1;
};

struct CBufferMaterialSubsurface : public CBufferMaterial
{
	CBufferMaterialSubsurface(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, const Vector3& SubsurfaceColor) :
		albedo(albedo), normal(normal), metallic(metallic), roughness(roughness), ao(ao), opacity(opacity), SubsurfaceColor(SubsurfaceColor) {}
	Vector3 albedo;
	float opacity;
	Vector3 normal;
	float metallic;
	float roughness;
	float ao;
	Vector2 _1;
	Vector3 SubsurfaceColor;
	float _2;
};

class NXMaterial
{
protected:
	explicit NXMaterial() = default;
	NXMaterial(const std::string& name, const NXMaterialType type = NXMaterialType::UNKNOWN, const std::string& filePath = "");

public:
	virtual ~NXMaterial() {}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	NXMaterialType GetType() { return m_type; }
	void SetType(NXMaterialType type) { m_type = type; }

	std::string GetFilePath() { return m_filePath; }
	size_t GetFilePathHash() { return std::filesystem::hash_value(m_filePath); }

	ID3D11Buffer* GetConstantBuffer() const { return m_cb.Get(); }

	virtual void Update();

	virtual NXPBRMaterialBase*			IsPBRMat()			{ return nullptr; }
	virtual NXPBRMaterialStandard*		IsStandardMat()		{ return nullptr; }
	virtual NXPBRMaterialTranslucent*	IsTranslucentMat()	{ return nullptr; }
	virtual NXPBRMaterialSubsurface*	IsSubsurfaceMat()	{ return nullptr; }
	virtual NXCustomMaterial*			IsCustomMat()		{ return nullptr; }

	virtual void Release() = 0;
	virtual void ReloadTextures() = 0;

public:
	std::vector<NXSubMeshBase*> GetRefSubMeshes() { return m_pRefSubMeshes; }
	void RemoveSubMesh(NXSubMeshBase* pRemoveSubmesh);
	void AddSubMesh(NXSubMeshBase* pSubMesh);

	void SetTex2D(NXTexture2D*& pTex2D, const std::wstring& texFilePath);

protected:
	std::string m_name;
	NXMaterialType m_type;

	std::unique_ptr<CBufferMaterial>	m_cbData;
	ComPtr<ID3D11Buffer>				m_cb;

private:
	// 映射表，记录哪些Submesh使用了这个材质
	std::vector<NXSubMeshBase*> m_pRefSubMeshes;
	UINT m_RefSubMeshesCleanUpCount;

	// 材质文件路径、哈希
	std::string m_filePath;
	size_t m_pathHash;
};

class NXPBRMaterialBase : public NXMaterial
{
protected:
	explicit NXPBRMaterialBase() = default;
	explicit NXPBRMaterialBase(const std::string& name, const NXMaterialType type = NXMaterialType::UNKNOWN, const std::string& filePath = "");
	~NXPBRMaterialBase() {}

public:
	ID3D11ShaderResourceView* GetSRVAlbedo()	const { return m_pTexAlbedo->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVNormal()	const { return m_pTexNormal->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVMetallic()	const { return m_pTexMetallic->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVRoughness() const { return m_pTexRoughness->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVAO()		const { return m_pTexAmbientOcclusion->GetSRV(); }

	void SetTexAlbedo(const std::wstring& TexFilePath);
	void SetTexNormal(const std::wstring& TexFilePath);
	void SetTexMetallic(const std::wstring& TexFilePath);
	void SetTexRoughness(const std::wstring& TexFilePath);
	void SetTexAO(const std::wstring& TexFilePath);

	std::string GetAlbedoTexFilePath()		{ return m_pTexAlbedo->GetFilePath().string(); }
	std::string GetNormalTexFilePath()		{ return m_pTexNormal->GetFilePath().string(); }
	std::string GetMetallicTexFilePath()	{ return m_pTexMetallic->GetFilePath().string(); }
	std::string GetRoughnessTexFilePath()	{ return m_pTexRoughness->GetFilePath().string(); }
	std::string GetAOTexFilePath()			{ return m_pTexAmbientOcclusion->GetFilePath().string(); }

	virtual NXPBRMaterialBase* IsPBRMat() override { return this; }

	virtual void Release() override;
	virtual void ReloadTextures() override;

private:
	NXTexture2D* m_pTexAlbedo;
	NXTexture2D* m_pTexNormal;
	NXTexture2D* m_pTexMetallic;
	NXTexture2D* m_pTexRoughness;
	NXTexture2D* m_pTexAmbientOcclusion;
};

class NXPBRMaterialStandard : public NXPBRMaterialBase
{
public:
	explicit NXPBRMaterialStandard() = default;
	NXPBRMaterialStandard(const std::string& name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const std::string& filePath);
	~NXPBRMaterialStandard() {}

	CBufferMaterialStandard* GetCBData() { return static_cast<CBufferMaterialStandard*>(m_cbData.get()); }
	
	Vector3 	GetAlbedo()		{ return GetCBData()->albedo; }
	Vector3 	GetNormal()		{ return GetCBData()->normal; }
	float		GetMetallic()	{ return GetCBData()->metallic; }
	float		GetRoughness()	{ return GetCBData()->roughness; }
	float		GetAO()			{ return GetCBData()->ao; }

	void	SetAlbedo(const Vector3& albedo)		{ GetCBData()->albedo = albedo; }
	void	SetNormal(const Vector3& normal)		{ GetCBData()->normal = normal; }
	void	SetMetallic(const float metallic)		{ GetCBData()->metallic = metallic; }
	void	SetRoughness(const float roughness)		{ GetCBData()->roughness = roughness; }
	void	SetAO(const float ao)					{ GetCBData()->ao = ao; }

	virtual NXPBRMaterialStandard* IsStandardMat() override { return this; }

private:
	void InitConstantBuffer();

private:
};

class NXPBRMaterialTranslucent : public NXPBRMaterialBase
{
public:
	explicit NXPBRMaterialTranslucent() = default;
	NXPBRMaterialTranslucent(const std::string& name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, const std::string& filePath);
	~NXPBRMaterialTranslucent() {}

	CBufferMaterialTranslucent* GetCBData() { return static_cast<CBufferMaterialTranslucent*>(m_cbData.get()); }
	
	Vector3 	GetAlbedo()		{ return GetCBData()->albedo; }
	float		GetOpacity()	{ return GetCBData()->opacity; }
	Vector3 	GetNormal()		{ return GetCBData()->normal; }
	float		GetMetallic()	{ return GetCBData()->metallic; }
	float		GetRoughness()	{ return GetCBData()->roughness; }
	float		GetAO()			{ return GetCBData()->ao; }

	void	SetAlbedo(const Vector3& albedo)		{ GetCBData()->albedo = albedo; }
	void	SetOpacity(const float opacity)			{ GetCBData()->opacity = opacity; }
	void	SetColor(const Vector4& color)			{ GetCBData()->albedo = Vector3(color); GetCBData()->opacity = color.w; }
	void	SetNormal(const Vector3& normal)		{ GetCBData()->normal = normal; }
	void	SetMetallic(const float metallic)		{ GetCBData()->metallic = metallic; }
	void	SetRoughness(const float roughness)		{ GetCBData()->roughness = roughness; }
	void	SetAO(const float ao)					{ GetCBData()->ao = ao; }

	virtual NXPBRMaterialTranslucent* IsTranslucentMat() override { return this; }

private:
	void InitConstantBuffer();
};

class NXPBRMaterialSubsurface : public NXPBRMaterialBase
{
public:
	explicit NXPBRMaterialSubsurface() = default;
	NXPBRMaterialSubsurface(const std::string& name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity, const Vector3& Subsurface, const std::string& filePath);
	~NXPBRMaterialSubsurface() {}
	CBufferMaterialSubsurface* GetCBData() { return static_cast<CBufferMaterialSubsurface*>(m_cbData.get()); }
	
	Vector3 	GetAlbedo()		{ return GetCBData()->albedo; }
	float		GetOpacity()	{ return GetCBData()->opacity; }
	Vector3 	GetNormal()		{ return GetCBData()->normal; }
	float		GetMetallic()	{ return GetCBData()->metallic; }
	float		GetRoughness()	{ return GetCBData()->roughness; }
	float		GetAO()			{ return GetCBData()->ao; }
	Vector3		GetSubsurface()	{ return GetCBData()->SubsurfaceColor; }

	void	SetAlbedo(const Vector3& albedo)				{ GetCBData()->albedo = albedo; }
	void	SetOpacity(const float opacity)					{ GetCBData()->opacity = opacity; }
	void	SetColor(const Vector4& color)					{ GetCBData()->albedo = Vector3(color); GetCBData()->opacity = color.w; }
	void	SetNormal(const Vector3& normal)				{ GetCBData()->normal = normal; }
	void	SetMetallic(const float metallic)				{ GetCBData()->metallic = metallic; }
	void	SetRoughness(const float roughness)				{ GetCBData()->roughness = roughness; }
	void	SetAO(const float ao)							{ GetCBData()->ao = ao; }
	void	SetSubsurface(const Vector3& SubsurfaceColor)	{ GetCBData()->SubsurfaceColor = SubsurfaceColor; }

	virtual NXPBRMaterialSubsurface* IsSubsurfaceMat() override { return this; }

private:
	void InitConstantBuffer();
};

class NXCustomMaterial : public NXMaterial
{
	template <typename NXMatParam>
	using ResourceMap = std::unordered_map<std::string, NXMatParam>;

public:
	explicit NXCustomMaterial() = default;
	NXCustomMaterial(const std::string& name);
	~NXCustomMaterial() {}

	void SetShaderFilePath(const std::filesystem::path& path);
	void LoadShaderCode();
	void CompileShader();

	// 初始化所有着色器资源，包括 cb, tex, sampler
	void InitShaderResources();
	void UpdateCBData();

	virtual void Update() override;

	void Render();

	virtual NXCustomMaterial* IsCustomMat() override { return this; }

	virtual void Release() override;
	virtual void ReloadTextures() override;

	const std::string& GetNSLCode() { return m_nslCode; }
	void SetNSLCode(const std::string& nslCode) { m_nslCode = nslCode; }
	void SetNSLParam(const std::string& nslParams) { m_nslParams = nslParams; }

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

private:
	// 读取 nsl 文件，获取 nsl shader.
	bool LoadShaderStringFromFile(std::string& shaderContent);
	// 将 nsl shader 拆成 params 和 code 两部分
	void ExtractShaderData(const std::string& shader, std::string& nslParams, std::string& nslCode);

	// 将 nsl params 转换成 DX 可以编译的 hlsl 代码，
	// 同时对其进行分拣，将 cb 储存到 m_cbInfo，纹理储存到 m_texInfoMap，采样器储存到 m_ssInfoMap
	void ProcessShaderParameters(const std::string& nslParams, std::string& oHLSLHeadCode);
	void ProcessShaderCBufferParam(std::istringstream& in, std::ostringstream& out);

	// 将 nsl code 转换成 DX 可以编译的 hlsl 代码，
	void ProcessShaderCode(const std::string& nslCode, std::string& oHLSLBodyCode);

private:
	std::filesystem::path		m_nslFilePath;
	std::string					m_nslParams;
	std::string					m_nslCode;

	ComPtr<ID3D11VertexShader>			m_pVertexShader;
	ComPtr<ID3D11PixelShader>			m_pPixelShader;
	ComPtr<ID3D11InputLayout>			m_pInputLayout;

	std::vector<NXMaterialSamplerInfo>	m_samplerInfos;
	std::vector<NXMaterialTextureInfo>	m_texInfos;
	NXMaterialCBufferInfo				m_cbInfo;
	std::vector<float>					m_cbInfoMemory;
	std::vector<int>					m_cbSortedIndex;

	std::vector<float>					m_cbufferData;
};
