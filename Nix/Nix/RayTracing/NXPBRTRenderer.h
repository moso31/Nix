#pragma once
#include "Header.h"
#include "NXInstance.h"

struct NXPBRTCamera
{
	Vector3 fPosition;
	Vector3 fDirection;
	Vector3 fUp;
	float fFovAngleY;

	// 最终生成的图片size
	XMINT2 iImgSize;
};

class NXPBRTRenderer : public NXInstance<NXPBRTRenderer>
{
public:
	NXPBRTRenderer() {}
	~NXPBRTRenderer() {}

	void DrawPhotonMapping(const shared_ptr<NXScene>& pScene, const NXPBRTCamera& cameraInfo, const XMINT2& imageInfo);
private:
};

