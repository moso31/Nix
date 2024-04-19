#pragma once
#include "BaseDefs/DX12.h"
#include "ShaderStructures.h"

class NXScene;
class NXGBufferRenderer
{
private:
	explicit NXGBufferRenderer() = default;
public:
	NXGBufferRenderer(NXScene* pScene);
	~NXGBufferRenderer();

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();

private:
	NXScene* m_pScene;
};
