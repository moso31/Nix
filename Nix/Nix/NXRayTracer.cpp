#include "NXRayTracer.h"
#include "NXCamera.h"
#include "NXScene.h"

void NXRayTracer::MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo)
{
	// 根据ImageInfo的情况为每个像素生成射线。
}

void NXRayTracer::Release()
{
}
