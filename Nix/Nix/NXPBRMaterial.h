#pragma once
#include "ShaderStructures.h"
#include "NXInstance.h"

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

	ID3D11ShaderResourceView* GetSRVAlbedo()	const { return m_pSRVAlbedo.Get(); }
	ID3D11ShaderResourceView* GetSRVNormal()	const { return m_pSRVNormal.Get(); }
	ID3D11ShaderResourceView* GetSRVMetallic()	const { return m_pSRVMetallic.Get(); }
	ID3D11ShaderResourceView* GetSRVRoughness() const { return m_pSRVRoughness.Get(); }
	ID3D11ShaderResourceView* GetSRVAO()		const { return m_pSRVAmbientOcclusion.Get(); }

	void SetTexAlbedo(const std::wstring TexFilePath);
	void SetTexNormal(const std::wstring TexFilePath);
	void SetTexMetallic(const std::wstring TexFilePath);
	void SetTexRoughness(const std::wstring TexFilePath);
	void SetTexAO(const std::wstring TexFilePath);
	void SetTex(const std::wstring TexFilePath, ComPtr<ID3D11Texture2D>& pTex, ComPtr<ID3D11ShaderResourceView>& pSRV);

private:
	void InitConstantBuffer();

private:
	std::string m_name;

	ComPtr<ID3D11Texture2D> m_pTexAlbedo;
	ComPtr<ID3D11Texture2D> m_pTexNormal;
	ComPtr<ID3D11Texture2D> m_pTexMetallic;
	ComPtr<ID3D11Texture2D> m_pTexRoughness;
	ComPtr<ID3D11Texture2D> m_pTexAmbientOcclusion;
	ComPtr<ID3D11ShaderResourceView> m_pSRVAlbedo;
	ComPtr<ID3D11ShaderResourceView> m_pSRVNormal;
	ComPtr<ID3D11ShaderResourceView> m_pSRVMetallic;
	ComPtr<ID3D11ShaderResourceView> m_pSRVRoughness;
	ComPtr<ID3D11ShaderResourceView> m_pSRVAmbientOcclusion;

	ConstantBufferMaterial	m_cbData;
	ComPtr<ID3D11Buffer>	m_cb;
};
