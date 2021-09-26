#pragma once
#include "ShaderStructures.h"
#include "NXInstance.h"

class NXPBRMaterial : public NXInstance
{
public:
	NXPBRMaterial() {}
	NXPBRMaterial(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao);
	~NXPBRMaterial() {}

	// DirectX
	ConstantBufferMaterial GetConstantBuffer();

	ID3D11ShaderResourceView* GetSRVAlbedo() const { return m_pSRVAlbedo.Get(); }
	ID3D11ShaderResourceView* GetSRVNormal() const { return m_pSRVNormal.Get(); }
	ID3D11ShaderResourceView* GetSRVMetallic() const { return m_pSRVMetallic.Get(); }
	ID3D11ShaderResourceView* GetSRVRoughness() const { return m_pSRVRoughness.Get(); }
	ID3D11ShaderResourceView* GetSRVAO() const { return m_pSRVAmbientOcclusion.Get(); }

	void SetTexAlbedo(const std::wstring TexFilePath);
	void SetTexNormal(const std::wstring TexFilePath);
	void SetTexMetallic(const std::wstring TexFilePath);
	void SetTexRoughness(const std::wstring TexFilePath);
	void SetTexAO(const std::wstring TexFilePath);
	void SetTex(const std::wstring TexFilePath, ComPtr<ID3D11Texture2D>& pTex, ComPtr<ID3D11ShaderResourceView>& pSRV);

	void Release();

public:
	Vector3 m_albedo;
	Vector3 m_normal;
	float m_metallic;
	float m_roughness;
	float m_ao;

private:
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
};
