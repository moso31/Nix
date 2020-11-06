#pragma once
#include "NXIntersection.h"
#include "ShaderStructures.h"

class NXPBRMaterial
{
public:
	// ��������
	struct SampleProbabilities
	{
		float Diffuse;
		float Specular;
		float Reflect;
		float Refract;
	};

	NXPBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR);
	~NXPBRMaterial() {}

	Vector3 GetF0() const { return m_F0; }
	void CalcSampleProbabilities(float reflectance);

	// DirectX
	ConstantBufferMaterial GetConstantBuffer();

	ID3D11ShaderResourceView* GetSRVAlbedo() const { return m_pSRVAlbedo; }
	ID3D11ShaderResourceView* GetSRVNormal() const { return m_pSRVNormal; }
	ID3D11ShaderResourceView* GetSRVMetallic() const { return m_pSRVMetallic; }
	ID3D11ShaderResourceView* GetSRVRoughness() const { return m_pSRVRoughness; }
	ID3D11ShaderResourceView* GetSRVAO() const { return m_pSRVAmbientOcclusion; }

	void SetTexAlbedo(const std::wstring& TexFilePath);
	void SetTexNormal(const std::wstring& TexFilePath);
	void SetTexMetallic(const std::wstring& TexFilePath);
	void SetTexRoughness(const std::wstring& TexFilePath);
	void SetTexAO(const std::wstring& TexFilePath);
	void SetTex(const std::wstring& TexFilePath, ID3D11Texture2D*& pTex, ID3D11ShaderResourceView*& pSRV);

	void Release();

public:
	Vector3 m_albedo;
	float m_metallic;
	float m_roughness;

	float m_reflectivity; // ���淴��� R
	float m_refractivity; // ��������� T
	float m_IOR; // �����ʣ�����������

	// ��������
	SampleProbabilities m_sampleProbs;

private:
	Vector3 m_F0;

private:
	ID3D11Texture2D* m_pTexAlbedo;
	ID3D11Texture2D* m_pTexNormal;
	ID3D11Texture2D* m_pTexMetallic;
	ID3D11Texture2D* m_pTexRoughness;
	ID3D11Texture2D* m_pTexAmbientOcclusion;
	ID3D11ShaderResourceView* m_pSRVAlbedo;
	ID3D11ShaderResourceView* m_pSRVNormal;
	ID3D11ShaderResourceView* m_pSRVMetallic;
	ID3D11ShaderResourceView* m_pSRVRoughness;
	ID3D11ShaderResourceView* m_pSRVAmbientOcclusion;
};
