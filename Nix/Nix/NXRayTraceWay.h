#pragma once
#include "Header.h"
#include "NXRayTracePass.h"

class NXRayTraceWay
{
public:
	NXRayTraceWay() {}
	~NXRayTraceWay() {}

	void Render();

protected:
	std::vector<std::shared_ptr<NXRayTracePass>> m_passes;
};

class NXRayTraceWayDirect : public NXRayTraceWay
{
public:
	NXRayTraceWayDirect() {}
	~NXRayTraceWayDirect() {}

	void LoadPasses(const std::shared_ptr<NXScene>& pScene, XMINT2 imageSize);

private:
};