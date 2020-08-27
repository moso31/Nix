#pragma once
#include "NXInstance.h"

enum NXRayTraceRenderMode
{
	DirectLighting,
	PathTracing,
	PhotonMapping,
	IrradianceCache,
	SPPM,
};

class NXScene;
class NXRayTracer : public NXInstance<NXRayTracer>
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void RenderImage(const std::shared_ptr<NXScene>& pScene, NXRayTraceRenderMode rayTraceMode);

private:
	NXRayTraceRenderMode m_rayTraceMode;
};
