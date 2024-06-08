#pragma once
#include "NXRendererPass.h"

class NXTexture2D;
class NXDepthRenderer : public NXRendererPass
{
public:
	NXDepthRenderer() {}
	~NXDepthRenderer() {}

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();
};
