#include "NXRayTracer.h"
#include "NXCamera.h"
#include "NXScene.h"

void NXRayTracer::MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo)
{
	// ����ImageInfo�����Ϊÿ�������������ߡ�
}

void NXRayTracer::Release()
{
}
