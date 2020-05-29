#pragma once
#include "Header.h"
#include "DirectXTex.h"

class NXPBREnvironmentLight;

class NXCubeMap
{
public:
	NXCubeMap();
	~NXCubeMap() {}

	bool Init(wstring filePath);
	Vector3 BackgroundColorByDirection(const Vector3& v);

	void SetEnvironmentLight(shared_ptr<NXPBREnvironmentLight> pEnvironmentLight) { m_pEnvironmentLight = pEnvironmentLight; }
	shared_ptr<NXPBREnvironmentLight> GetEnvironmentLight() const;

	void Release();

private:
	unique_ptr<ScratchImage> m_image;
	byte* m_faceData[6];
	size_t m_width, m_height;

	shared_ptr<NXPBREnvironmentLight> m_pEnvironmentLight;
};