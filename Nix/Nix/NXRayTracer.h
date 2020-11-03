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

	void RenderImage(NXScene* pScene, NXRayTraceRenderMode rayTraceMode, bool bCenterRayTest = false);

private:
	NXRayTraceRenderMode m_rayTraceMode;
};
