#pragma once
#include "NXIntersection.h"
#include "ShaderStructures.h"

class NXPBRMaterial
{
public:
	// 采样概率
	struct SampleProbabilities
	{
		float Diffuse;
		float Specular;
		float Reflect;
		float Refract;
	};

	NXPBRMaterial(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float reflectivity, const float refractivity, const float IOR);
	~NXPBRMaterial() {}

	Vector3 GetF0() const { return m_F0; }
	void CalcSampleProbabilities(float reflectance);

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

	// 当有Primitive选用此材质时，调用此方法记录材质和Primitive的绑定关系。
	void AddPrimitiveReference(NXPrimitive* pPrimitive) { m_pPrimitiveList.push_back(pPrimitive); }

	// 获取正在使用当前材质的所有Primitive。
	const std::list<NXPrimitive*> GetPrimitives() { return m_pPrimitiveList; }

public:
	Vector3 m_albedo;
	Vector3 m_normal;
	float m_metallic;
	float m_roughness;
	float m_ao;

	float m_reflectivity; // 镜面反射比 R
	float m_refractivity; // 镜面折射比 T
	float m_IOR; // 折射率（如果有折射项）

	// 采样概率
	SampleProbabilities m_sampleProbs;

private:
	Vector3 m_F0;

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

	// 记录正在使用当前材质的所有Primivives。
	std::list<NXPrimitive*> m_pPrimitiveList;
};
