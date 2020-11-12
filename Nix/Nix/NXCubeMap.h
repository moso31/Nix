#pragma once
#include "NXPrimitive.h"
#include "DirectXTex.h"

class NXPBREnvironmentLight;

class NXCubeMap : public NXPrimitive
{
public:
	NXCubeMap(NXScene* pScene);
	~NXCubeMap() {}

	bool Init(const std::wstring filePath);
	void Update() override;
	void Render() override;
	void Release() override;

	void GenerateCubeMap(const std::wstring filePath);
	void GenerateIrradianceMap();
	void GeneratePreFilterMap();
	void GenerateBRDF2DLUT();

	Vector3 BackgroundColorByDirection(const Vector3& v);
	void SetEnvironmentLight(NXPBREnvironmentLight* pEnvironmentLight) { m_pEnvironmentLight = pEnvironmentLight; }
	NXPBREnvironmentLight* GetEnvironmentLight() const { return m_pEnvironmentLight; }

	ID3D11ShaderResourceView* GetSRVCubeMap() { return m_pSRVCubeMap.Get(); } 
	ID3D11ShaderResourceView* GetSRVIrradianceMap() { return m_pSRVIrradianceMap.Get(); }
	ID3D11ShaderResourceView* GetSRVPreFilterMap() { return m_pSRVPreFilterMap.Get(); }
	ID3D11ShaderResourceView* GetSRVBRDF2DLUT() { return m_pSRVBRDF2DLUT.Get(); }

private:
	void InitVertex();
	void InitVertexIndexBuffer() override;

private:
	DXGI_FORMAT m_format;
	std::wstring m_cubeMapFilePath;
	std::unique_ptr<ScratchImage> m_pImage;
	std::vector<byte*> m_faceData;
	size_t m_width, m_height;

	NXScene* m_pScene;
	NXPBREnvironmentLight* m_pEnvironmentLight;

	std::vector<VertexP>		m_vertices;
	std::vector<VertexP>		m_verticesCubeBox;
	std::vector<USHORT>			m_indicesCubeBox;
	ComPtr<ID3D11Buffer>		m_pVertexBufferCubeBox;
	ComPtr<ID3D11Buffer>		m_pIndexBufferCubeBox;

	ComPtr<ID3D11ShaderResourceView>	m_pSRVHDRMap;

	ComPtr<ID3D11Texture2D>				m_pTexCubeMap;
	ComPtr<ID3D11ShaderResourceView>	m_pSRVCubeMap;
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
};