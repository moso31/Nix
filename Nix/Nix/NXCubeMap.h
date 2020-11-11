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

	ID3D11ShaderResourceView* GetSRVCubeMap() { return m_pSRVCubeMap; } 
	ID3D11ShaderResourceView* GetSRVIrradianceMap() { return m_pSRVIrradianceMap; }
	ID3D11ShaderResourceView* GetSRVPreFilterMap() { return m_pSRVPreFilterMap; }
	ID3D11ShaderResourceView* GetSRVBRDF2DLUT() { return m_pSRVBRDF2DLUT; }

private:
	void InitVertex();
	void InitVertexIndexBuffer() override;

private:
	DXGI_FORMAT m_format;
	std::wstring m_cubeMapFilePath;
	std::unique_ptr<ScratchImage> m_pImage;
	byte* m_faceData[6];
	size_t m_width, m_height;

	NXScene* m_pScene;
	NXPBREnvironmentLight* m_pEnvironmentLight;

	std::vector<VertexP>		m_vertices;
	std::vector<VertexP>		m_verticesCubeBox;
	std::vector<USHORT>			m_indicesCubeBox;
	ComPtr<ID3D11Buffer>		m_pVertexBufferCubeBox;
	ComPtr<ID3D11Buffer>		m_pIndexBufferCubeBox;

	ID3D11ShaderResourceView*	m_pSRVHDRMap;

	ID3D11Texture2D*			m_pTexCubeMap;
	ID3D11ShaderResourceView*	m_pSRVCubeMap;
	ID3D11RenderTargetView*		m_pRTVCubeMaps[6];

	ID3D11Texture2D*			m_pTexIrradianceMap;
	ID3D11ShaderResourceView*	m_pSRVIrradianceMap;
	ID3D11RenderTargetView*		m_pRTVIrradianceMaps[6];

	ID3D11Texture2D*			m_pTexPreFilterMap;
	ID3D11ShaderResourceView*	m_pSRVPreFilterMap;
	ID3D11RenderTargetView*		m_pRTVPreFilterMaps[5][6];

	ID3D11Texture2D*			m_pTexBRDF2DLUT;
	ID3D11ShaderResourceView*	m_pSRVBRDF2DLUT;
	ID3D11RenderTargetView*		m_pRTVBRDF2DLUT;
};