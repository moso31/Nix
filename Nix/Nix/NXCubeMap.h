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
	ConstantBufferCubeMap() : intensity(1.0f) {}
	Vector4 irradSH0123x;
	Vector4 irradSH4567x;
	Vector4 irradSH0123y;
	Vector4 irradSH4567y;
	Vector4 irradSH0123z;
	Vector4 irradSH4567z;
	Vector3 irradSH8xyz;
	float intensity;
};

class NXCubeMap : public NXTransform
{
public:
	NXCubeMap(NXScene* pScene);
	~NXCubeMap() {}

	bool Init(const std::wstring filePath);
	void Update() override;
	void UpdateViewParams();
	void Render();
	void Release() override;

	void GenerateCubeMap(const std::wstring filePath);
	void GenerateIrradianceSH(size_t imgWidth, size_t imgHeight);
	void GenerateIrradianceMap();
	void GeneratePreFilterMap();
	void GenerateBRDF2DLUT();

	Vector3 BackgroundColorByDirection(const Vector3& v);

	ID3D11ShaderResourceView* GetSRVCubeMap() { return m_pSRVCubeMap.Get(); }
	ID3D11ShaderResourceView* GetSRVCubeMapPreview2D() { return m_pSRVCubeMapPreview2D.Get(); }
	ID3D11ShaderResourceView* GetSRVIrradianceMap();
	ID3D11ShaderResourceView* GetSRVPreFilterMap() { return m_pSRVPreFilterMap.Get(); }
	ID3D11ShaderResourceView* GetSRVBRDF2DLUT() { return m_pSRVBRDF2DLUT.Get(); }

	ID3D11ShaderResourceView* GetSRVIrradianceSH() { return m_pSRVIrradianceSH.Get(); }

	ID3D11Buffer* GetConstantBufferParams() { return m_cb.Get(); }

	void SetIntensity(float val) { m_cbData.intensity = val; }
	float* GetIntensity() { return &m_cbData.intensity; }

private:
	void InitVertex();
	void UpdateVBIB();
	void InitConstantBuffer();

private:
	DXGI_FORMAT m_format;
	std::wstring m_cubeMapFilePath;
	std::unique_ptr<ScratchImage> m_pImage;
	std::vector<byte*> m_faceData;
	size_t m_width, m_height;

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

	ComPtr<ID3D11ShaderResourceView>	m_pSRVHDRMap;

	ComPtr<ID3D11Texture2D>				m_pTexCubeMap;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVCubeMap;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVCubeMapPreview2D;
	ComPtr<ID3D11RenderTargetView>		m_pRTVCubeMaps[6];

	NXTextureCube*						m_pTexIrradianceMap;

	ComPtr<ID3D11ShaderResourceView>	m_pSRVIrradianceSH;
	Vector3 m_shIrradianceMap[9];

	//NXTextureCube*						m_pTexPreFilterMap;
	ComPtr<ID3D11Texture2D>				m_pTexPreFilterMap;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVPreFilterMap;
	ComPtr<ID3D11RenderTargetView>		m_pRTVPreFilterMaps[5][6];

	ComPtr<ID3D11Texture2D>				m_pTexBRDF2DLUT;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVBRDF2DLUT;
	ComPtr<ID3D11RenderTargetView>		m_pRTVBRDF2DLUT;

	ConstantBufferCubeMap	m_cbData;
	ComPtr<ID3D11Buffer>	m_cb;
};