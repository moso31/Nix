#pragma once
#include "BaseDefs/DearImGui.h"
#include "Ntr.h"
#include "NXConstantBuffer.h"

class Renderer;
class NXTexture2D;
class NXGUITerrainSector2NodeIDPreview
{
	struct CBufferData
	{

	};

public:
	NXGUITerrainSector2NodeIDPreview(Renderer* pRenderer);
	virtual ~NXGUITerrainSector2NodeIDPreview();

	void Update();
	void Render();

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool GetVisible() const { return m_bVisible; }

private:
	Renderer* m_pRenderer = nullptr;
	bool m_bVisible = false;
	float m_zoomScale = 1.0f;

	Ntr<NXTexture2D> m_pTexture;

	CBufferData m_cbData;
	NXConstantBuffer<CBufferData> m_cb;
};
