#pragma once
#include "BaseDefs/DearImGui.h"
#include "Ntr.h"
#include "NXConstantBuffer.h"

class Renderer;
class NXTexture2D;
class NXComputePassMaterial;
class NXGUITerrainSector2NodeIDPreview
{
	// 与 TerrainSector2NodePreview.fx 中的 cbRemapParams 对应
	struct CBufferRemapParams
	{
		float remapMin = 0.0f;		// remap 范围的最小值 (0-1024)
		float remapMax = 1024.0f;	// remap 范围的最大值 (0-1024)
		int padding[2];
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

	// R16_UNORM 预览纹理 (6个mip等级，每个mip都有SRV和UAV)
	Ntr<NXTexture2D> m_pTexture;

	// Compute Pass 材质
	NXComputePassMaterial* m_pPassMat = nullptr;

	// Remap 参数 Constant Buffer
	CBufferRemapParams m_cbRemapData;
	NXConstantBuffer<CBufferRemapParams> m_cbRemap;

	int m_cbMipData[6];
	NXConstantBuffer<int> m_cbMip[6];

	// ImGui 参数
	int m_currentMipLevel = 0;		// 当前查看的 mip 等级 (0-5)
	float m_remapMin = 0.0f;		// remap 范围最小值
	float m_remapMax = 1024.0f;		// remap 范围最大值
};
