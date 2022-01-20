#pragma once
#include "NXTransform.h"
#include "DirectXTex.h"
#include "ShaderStructures.h"

struct ConstantBufferCubeMap
{
	ConstantBufferCubeMap() : intensity(1.0f) {}
	float intensity;
	Vector3 _0;
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
	void GenerateIrradianceMap();
	void GeneratePreFilterMap();
	void GenerateBRDF2DLUT();

	Vector3 BackgroundColorByDirection(const Vector3& v);

	ID3D11ShaderResourceView* GetSRVCubeMap() { return m_pSRVCubeMap.Get(); }
	ID3D11ShaderResourceView* GetSRVCubeMapPreview2D() { return m_pSRVCubeMapPreview2D.Get(); }
	ID3D11ShaderResourceView* GetSRVIrradianceMap() { return m_pSRVIrradianceMap.Get(); }
	ID3D11ShaderResourceView* GetSRVPreFilterMap() { return m_pSRVPreFilterMap.Get(); }
	ID3D11ShaderResourceView* GetSRVBRDF2DLUT() { return m_pSRVBRDF2DLUT.Get(); }

	ID3D11Buffer* GetConstantBufferParams() { return m_cb.Get(); }

	float* GetIntensity() { return &m_cbData.intensity; }

private:
	void InitVertex();
	void InitVertexIndexBuffer();
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

	ComPtr<ID3D11Texture2D>				m_pTexIrradianceMap;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVIrradianceMap;
	ComPtr<ID3D11RenderTargetView>		m_pRTVIrradianceMaps[6];

	ComPtr<ID3D11Texture2D>				m_pTexPreFilterMap;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVPreFilterMap;
	ComPtr<ID3D11RenderTargetView>		m_pRTVPreFilterMaps[5][6];

	ComPtr<ID3D11Texture2D>				m_pTexBRDF2DLUT;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVBRDF2DLUT;
	ComPtr<ID3D11RenderTargetView>		m_pRTVBRDF2DLUT;

	ConstantBufferCubeMap	m_cbData;
	ComPtr<ID3D11Buffer>	m_cb;
};