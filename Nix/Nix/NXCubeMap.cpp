#include "NXCubeMap.h"
#include "NXLight.h"

NXCubeMap::NXCubeMap()
{
}

bool NXCubeMap::Init(wstring filePath)
{
	m_image.reset(); 
	m_image = make_unique<ScratchImage>();

	TexMetadata metadata;
	unique_ptr<ScratchImage> dcImage = make_unique<ScratchImage>();
	HRESULT hr; 
	hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &metadata, *m_image);
	if (FAILED(hr))
	{
		dcImage.reset();
		return false;
	}

	if (IsCompressed(metadata.format))
	{
		auto img = m_image->GetImage(0, 0, 0);
		size_t nimg = m_image->GetImageCount();

		hr = Decompress(img, nimg, metadata, DXGI_FORMAT_UNKNOWN /* picks good default */, *dcImage);
		if (FAILED(hr))
		{
			dcImage.reset();
			return false;
		}
	}
	if (dcImage) m_image.swap(dcImage);
	dcImage.reset();

	if (!m_image) return false;

	m_width = metadata.width;
	m_height = metadata.height;

	for (int item = 0; item < 6; item++)
	{
		auto faceImage = m_image->GetImage(0, item, 0);
		m_faceData[item] = faceImage->pixels;
	}

	return true;
}

Vector3 NXCubeMap::BackgroundColorByDirection(const Vector3& v)
{
	assert(v.LengthSquared() != 0);

	Vector3 vAbs = Vector3::Abs(v);
	int dim = vAbs.MaxDimension();
	float scale = 1.0f / vAbs[dim];
	Vector3 touchCube = v * scale;	// 将v延长到能摸到单位Cube。此时dim轴即被选中的CubeMap的面，剩下的两个轴即=该面的uv。
	touchCube = (touchCube + Vector3(1.0f)) * 0.5f; //将坐标从([-1, 1]^3)映射到([0, 1]^3)

	assert(dim >= 0 && dim < 3);
	Vector2 uvHit;

	int faceId = dim * 2 + !(v[dim] > 0);	
	switch (faceId)
	{
	case 0:		uvHit = Vector2(1.0f - touchCube.z, 1.0f - touchCube.y);	break;		// +X
	case 1:		uvHit = Vector2(touchCube.z, 1.0f - touchCube.y);	break;		// -X
	case 2:		uvHit = Vector2(touchCube.x, touchCube.z);	break;		// +Y
	case 3:		uvHit = Vector2(touchCube.z, touchCube.x);	break;		// -Y
	case 4:		uvHit = Vector2(touchCube.x, 1.0f - touchCube.y);	break;		// +Z
	case 5:		uvHit = Vector2(1.0f - touchCube.x, 1.0f - touchCube.y);	break;		// -Z
	}

	int offset = ((int)floorf(uvHit.y * (float)m_height) * (int)m_width + (int)floorf(uvHit.x * (float)m_width)) * 4;
	byte* c = m_faceData[faceId] + offset;

	float fInv = 1.0f / 255.0f;
	float r = *(c + 0) * fInv;
	float g = *(c + 1) * fInv;
	float b = *(c + 2) * fInv;
	return Vector3(r, g, b);
}

shared_ptr<NXPBREnvironmentLight> NXCubeMap::GetEnvironmentLight() const 
{ 
	return m_pEnvironmentLight; 
}


void NXCubeMap::Release()
{
	if (m_image) m_image.reset();
}
