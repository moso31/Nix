#include "NXPBRTRenderer.h"
#include "NXScene.h"

void NXPBRTRenderer::DrawPhotonMapping(const shared_ptr<NXScene>& pScene, const NXPBRTCamera& cameraInfo, const XMINT2& pixel)
{
	float fAspectRatio = (float)cameraInfo.iImgSize.x / (float)cameraInfo.iImgSize.y;
	Matrix mxProj = XMMatrixPerspectiveFovLH(cameraInfo.fFovAngleY * DEGTORAD, fAspectRatio, 0.1f, 1000.0f);
}
