#include "NXRayTraceWay.h"
#include "NXRayTracePassDirect.h"

void NXRayTraceWay::Render()
{
	for (int i = 0; i < m_passes.size(); i++)
	{
		m_passes[i]->Render();
	}
}

void NXRayTraceWayDirect::LoadPasses(const std::shared_ptr<NXScene>& pScene, XMINT2 imageSize)
{
	std::shared_ptr<NXRayTracePassDirect> pass = std::make_shared<NXRayTracePassDirect>();
	XMINT2 tileSize(16, 16);
	int eachPixelSamples = 4;
	pass->SetOutFilePath("D:\\nix_directLighting.bmp");
	pass->Load(pScene, imageSize, tileSize, eachPixelSamples);
	m_passes.push_back(pass);
}