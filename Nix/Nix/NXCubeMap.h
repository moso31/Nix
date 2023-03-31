#pragma once
#include "NXTransform.h"
#include "DirectXTex.h"
#include "ShaderStructures.h"

struct ConstantBufferImageData
{
	Vector4 currImgSize; // xy: size zw: sizeInv
	Vector4 nextImgSize; // xy: size zw: sizeInv
};

struct ConstantBufferIrradSH
{
	ConstantBufferIrradSH() {}
	Vector4 irradSH[9];
};

struct ConstantBufferCubeMap
{
	ConstantBufferCubeMap() : intensity(1.0f), irradMode(0.0f) {}
	Vector4 irradSH0123x;
	Vector4 irradSH4567x;
	Vector4 irradSH0123y;
	Vector4 irradSH4567y;
	Vector4 irradSH0123z;
	Vector4 irradSH4567z;
	Vector3 irradSH8xyz;
	float intensity;
	Vector4 irradMode;
};

class NXCubeMap : public NXTransform
{
public:
	NXCubeMap(NXScene* pScene);
	~NXCubeMap() {}

	bool Init(const std::filesystem::path& filePath);
	void Update() override;
	void UpdateViewParams();
	void Render();
	void Release() override;

	NXTextureCube* GenerateCubeMap(NXTexture2D* pTexHDR);
	void GenerateIrradianceSHFromHDRI(NXTexture2D* pTexHDR);
	void GenerateIrradianceSHFromCubeMap();

	void GenerateIrradianceMap();
	void GeneratePreFilterMap();

	ID3D11ShaderResourceView* GetSRVCubeMap();
	ID3D11ShaderResourceView* GetSRVCubeMapPreview2D();
	ID3D11ShaderResourceView* GetSRVIrradianceMap();
	ID3D11ShaderResourceView* GetSRVPreFilterMap();

	ID3D11Buffer* GetConstantBufferParams() { return m_cb.Get(); }

	void SetIntensity(float val) { m_cbData.intensity = val; }
	float* GetIntensity() { return &m_cbData.intensity; }

	void SetIrradMode(int val) { m_cbData.irradMode = Vector4((float)val); };

	void SaveHDRAsDDS(NXTextureCube* pTexture, const std::filesystem::path& filePath);
	void LoadDDS(const std::filesystem::path& filePath);

private:
	void InitVertex();
	void UpdateVBIB();
	void InitConstantBuffer();

private:
	NXScene* m_pScene;

	Matrix m_mxCubeMapProj;
	Matrix m_mxCubeMapView[6];

	std::vector<VertexP>		m_vertices;
	std::vector<UINT>			m_indices;
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexP>		m_verticesCubeBox;
	std::vector<UINT>			m_indicesCubeBox;
	ComPtr<ID3D11Buffer>		m_pVertexBufferCubeBox;
	ComPtr<ID3D11Buffer>		m_pIndexBufferCubeBox;

	NXTextureCube*						m_pTexCubeMap;
	NXTextureCube*						m_pTexIrradianceMap;
	NXTextureCube*						m_pTexPreFilterMap;

	Vector3 m_shIrradianceMap[9];
	Vector3 m_shIrradianceMap_CPU[9];


	ConstantBufferCubeMap	m_cbData;
	ComPtr<ID3D11Buffer>	m_cb;

////////////////////////////////////////////////////////////////////////////
//// Deprecated functions...
////////////////////////////////////////////////////////////////////////////


public:
	void GenerateIrradianceSHFromHDRI_Deprecated(NXTexture2D* pTexHDR);
	ID3D11ShaderResourceView* GetSRVIrradianceSH() { return m_pSRVIrradianceSH.Get(); }

private:
	ComPtr<ID3D11ShaderResourceView>	m_pSRVIrradianceSH;
};