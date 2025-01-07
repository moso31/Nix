#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"

class NXScene;
class NXCamera;
class NXGBufferRenderer
{
private:
	explicit NXGBufferRenderer() = default;
public:
	NXGBufferRenderer(NXScene* pScene);
	virtual ~NXGBufferRenderer();

	void Init();
	void SetCamera(NXCamera* pCamera);
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();

private:
	NXScene* m_pScene;
	NXCamera* m_pCamera;
};
