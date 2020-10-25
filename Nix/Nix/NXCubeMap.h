#pragma once
#include "Header.h"
#include "DirectXTex.h"

class NXPBREnvironmentLight;

class NXCubeMap
{
public:
	NXCubeMap();
	~NXCubeMap() {}

	bool Init(std::wstring filePath);
	Vector3 BackgroundColorByDirection(const Vector3& v);

	void SetEnvironmentLight(std::shared_ptr<NXPBREnvironmentLight> pEnvironmentLight) { m_pEnvironmentLight = pEnvironmentLight; }
	std::shared_ptr<NXPBREnvironmentLight> GetEnvironmentLight() const;

	void Release();

private:
	std::unique_ptr<ScratchImage> m_image;
	byte* m_faceData[6];
	size_t m_width, m_height;

	std::shared_ptr<NXPBREnvironmentLight> m_pEnvironmentLight;

	ID3D11ShaderResourceView* m_pTextureSRV; // cube texture.
};