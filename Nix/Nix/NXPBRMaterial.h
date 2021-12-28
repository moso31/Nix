#pragma once
#include "ShaderStructures.h"
#include "NXResourceManager.h"

struct ConstantBufferMaterial
{
	ConstantBufferMaterial(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao) :
		albedo(albedo), normal(normal), metallic(metallic), roughness(roughness), ao(ao) {}
	Vector3 albedo;
	float _0;
	Vector3 normal;
	float metallic;
	float roughness;
	float ao;
	Vector2 _1;
};

class NXPBRMaterial : public NXInstance<NXPBRMaterial>
{
public:
	NXPBRMaterial(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao);
	~NXPBRMaterial() {}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	void Update();

	ID3D11Buffer* GetConstantBuffer() const { return m_cb.Get(); }

	Vector3 	GetAlbedo()		{ return m_cbData.albedo; }
	Vector3 	GetNormal()		{ return m_cbData.normal; }
	float*		GetMatallic()	{ return &m_cbData.metallic; }
	float*		GetRoughness()	{ return &m_cbData.roughness; }
	float*		GetAO()			{ return &m_cbData.ao; }

	void	SetAlbedo(const Vector3& albedo)		{ m_cbData.albedo = albedo; }
	void	SetNormal(const Vector3& normal)		{ m_cbData.normal = normal; }
	void	SetMetallic(const float metallic)		{ m_cbData.metallic = metallic; }
	void	SetRoughness(const float roughness)		{ m_cbData.roughness = roughness; }
	void	SetAO(const float ao)					{ m_cbData.ao = ao; }

	ID3D11ShaderResourceView* GetSRVAlbedo()	const { return m_pTexAlbedo->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVNormal()	const { return m_pTexNormal->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVMetallic()	const { return m_pTexMetallic->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVRoughness() const { return m_pTexRoughness->GetSRV(); }
	ID3D11ShaderResourceView* GetSRVAO()		const { return m_pTexAmbientOcclusion->GetSRV(); }

	void SetTexAlbedo(const std::wstring TexFilePath);
	void SetTexNormal(const std::wstring TexFilePath);
	void SetTexMetallic(const std::wstring TexFilePath);
	void SetTexRoughness(const std::wstring TexFilePath);
	void SetTexAO(const std::wstring TexFilePath);

	void Release();

private:
	NXTexture2D* LoadFromTexFile(const std::wstring TexFilePath);
	void InitConstantBuffer();

private:
	std::string m_name;

	NXTexture2D* m_pTexAlbedo;
	NXTexture2D* m_pTexNormal;
	NXTexture2D* m_pTexMetallic;
	NXTexture2D* m_pTexRoughness;
	NXTexture2D* m_pTexAmbientOcclusion;

	ConstantBufferMaterial	m_cbData;
	ComPtr<ID3D11Buffer>	m_cb;
};
