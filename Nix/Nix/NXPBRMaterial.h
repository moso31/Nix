#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"

enum NXMaterialType
{
	UNKNOWN,
	PBR_STANDARD,
	PBR_TRANSLUCENT
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

class NXMaterial
{
protected:
	explicit NXMaterial() = default;
	NXMaterial(const std::string name, const NXMaterialType type = NXMaterialType::UNKNOWN);

public:
	~NXMaterial() {}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	NXMaterialType GetType() { return m_type; }
	void SetType(NXMaterialType type) { m_type = type; }

	bool IsPBRType();

	ID3D11Buffer* GetConstantBuffer() const { return m_cb.Get(); }

	void Update();

	virtual void Release() = 0;

public:
	NXTexture2D* LoadFromTexFile(const std::wstring TexFilePath, bool GenerateMipMap = false);

	std::vector<NXSubMesh*> GetRefSubMeshes() { return m_pRefSubMeshes; }
	void CleanUpRefSubMeshes();
	void RemoveSubMesh(NXSubMesh* pRemoveSubmesh);
	void AddSubMesh(NXSubMesh* pSubMesh);

protected:
	std::string m_name;
	NXMaterialType m_type;

	std::unique_ptr<CBufferMaterial>	m_cbData;
	ComPtr<ID3D11Buffer>				m_cb;

private:
	// 映射表，记录哪些Submesh使用了这个材质
	std::vector<NXSubMesh*> m_pRefSubMeshes;
	UINT m_RefSubMeshesCleanUpCount;
};

class NXPBRMaterialBase : public NXMaterial
{
protected:
	explicit NXPBRMaterialBase() = default;
	explicit NXPBRMaterialBase(const std::string name, const NXMaterialType type = NXMaterialType::UNKNOWN);
	~NXPBRMaterialBase() {}

public:
	ID3D11ShaderResourceView* GetSRVAlbedo()	const { return m_pTexAlbedo->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVNormal()	const { return m_pTexNormal->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVMetallic()	const { return m_pTexMetallic->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVRoughness() const { return m_pTexRoughness->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVAO()		const { return m_pTexAmbientOcclusion->GetSRV(); }

	void SetTexAlbedo(const std::wstring TexFilePath, bool GenerateMipMap = false);
	void SetTexNormal(const std::wstring TexFilePath, bool GenerateMipMap = false);
	void SetTexMetallic(const std::wstring TexFilePath, bool GenerateMipMap = false);
	void SetTexRoughness(const std::wstring TexFilePath, bool GenerateMipMap = false);
	void SetTexAO(const std::wstring TexFilePath, bool GenerateMipMap = false);

	virtual void Release();

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
	NXPBRMaterialStandard(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao);
	~NXPBRMaterialStandard() {}

	CBufferMaterialStandard* GetCBData() { return static_cast<CBufferMaterialStandard*>(m_cbData.get()); }
	
	Vector3 	GetAlbedo()		{ return GetCBData()->albedo; }
	Vector3 	GetNormal()		{ return GetCBData()->normal; }
	float*		GetMatallic()	{ return &(GetCBData()->metallic); }
	float*		GetRoughness()	{ return &(GetCBData()->roughness); }
	float*		GetAO()			{ return &(GetCBData()->ao); }

	void	SetAlbedo(const Vector3& albedo)		{ GetCBData()->albedo = albedo; }
	void	SetNormal(const Vector3& normal)		{ GetCBData()->normal = normal; }
	void	SetMetallic(const float metallic)		{ GetCBData()->metallic = metallic; }
	void	SetRoughness(const float roughness)		{ GetCBData()->roughness = roughness; }
	void	SetAO(const float ao)					{ GetCBData()->ao = ao; }

private:
	void InitConstantBuffer();

private:
};

class NXPBRMaterialTranslucent : public NXPBRMaterialBase
{
public:
	explicit NXPBRMaterialTranslucent() = default;
	NXPBRMaterialTranslucent(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity);
	~NXPBRMaterialTranslucent() {}

	CBufferMaterialTranslucent* GetCBData() { return static_cast<CBufferMaterialTranslucent*>(m_cbData.get()); }
	
	Vector3 	GetAlbedo()		{ return GetCBData()->albedo; }
	float*		GetOpacity()	{ return &(GetCBData()->opacity); }
	Vector3 	GetNormal()		{ return GetCBData()->normal; }
	float*		GetMatallic()	{ return &(GetCBData()->metallic); }
	float*		GetRoughness()	{ return &(GetCBData()->roughness); }
	float*		GetAO()			{ return &(GetCBData()->ao); }

	void	SetAlbedo(const Vector3& albedo)		{ GetCBData()->albedo = albedo; }
	void	SetAlbedo(const float opacity)			{ GetCBData()->opacity = opacity; }
	void	SetNormal(const Vector3& normal)		{ GetCBData()->normal = normal; }
	void	SetMetallic(const float metallic)		{ GetCBData()->metallic = metallic; }
	void	SetRoughness(const float roughness)		{ GetCBData()->roughness = roughness; }
	void	SetAO(const float ao)					{ GetCBData()->ao = ao; }

private:
	void InitConstantBuffer();
};