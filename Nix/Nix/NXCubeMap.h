#pragma once
#include "Header.h"
#include "DirectXTex.h"

class NXCubeMap
{
public:
	NXCubeMap();
	~NXCubeMap() {}

	bool Init(wstring filePath);
	Vector3 BackgroundColorByDirection(const Vector3& v);

	void Release();

private:
	unique_ptr<ScratchImage> m_image;
	byte* m_faceData[6];
	size_t m_width, m_height;
};