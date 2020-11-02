#pragma once
#include "NXPrimitive.h"
#include "DirectXTex.h"

class NXPBREnvironmentLight;

class NXCubeMap : public NXPrimitive
{
public:
	NXCubeMap(const std::shared_ptr<NXScene>& pScene);
	~NXCubeMap() {}

	bool Init(std::wstring filePath);
	void Update() override;
	void Render() override;
	void Release() override;

	void GenerateIrradianceMap();
	void GeneratePreFilterMap();

	Vector3 BackgroundColorByDirection(const Vector3& v);
	void SetEnvironmentLight(std::shared_ptr<NXPBREnvironmentLight> pEnvironmentLight) { m_pEnvironmentLight = pEnvironmentLight; }
	std::shared_ptr<NXPBREnvironmentLight> GetEnvironmentLight() const;

	ID3D11ShaderResourceView* GetIrradianceMapSRV() { return m_pIrradianceMapSRV; }
	ID3D11ShaderResourceView* GetPreFilterMapSRV() { return m_pPreFilterMapSRV; }

private:
	void InitVertex();
	void InitVertexIndexBuffer() override;

private:
	std::unique_ptr<ScratchImage> m_image;
	byte* m_faceData[6];
	size_t m_width, m_height;

	std::shared_ptr<NXScene>	m_pScene;
	std::shared_ptr<NXPBREnvironmentLight> m_pEnvironmentLight;

	std::vector<VertexP>		m_vertices;

	ID3D11Texture2D*			m_pIrradianceMap;
	ID3D11ShaderResourceView*	m_pIrradianceMapSRV;
	ID3D11RenderTargetView*		m_pIrradianceMapRTVs[6];
	ID3D11Texture2D*			m_pPreFilterMap;
	ID3D11ShaderResourceView*	m_pPreFilterMapSRV;
	ID3D11RenderTargetView*		m_pPreFilterMapRTVs[6];
};