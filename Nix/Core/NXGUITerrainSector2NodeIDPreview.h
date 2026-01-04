#pragma once
#include "BaseDefs/DearImGui.h"
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include "NXConstantBuffer.h"

class Renderer;
class NXTexture2D;
class NXComputePassMaterial;
class NXGUITerrainSector2NodeIDPreview
{
	struct CBufferRemapParams
	{
		float remapMin;	
		float remapMax;	
		Vector2 pad0;
		Vector3 invalidColor;
		float pad1;
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

	// 预览的mip等级，实际就是个非常简单的data[i] = i;
	int m_cbMipData[6];
	NXConstantBuffer<int> m_cbMip[6];

	// ImGui 参数
	int m_currentMipLevel = 0;		// 当前查看的 mip 等级 (0-5)
	float m_remapMin = 0.0f;		// remap 范围最小值
	float m_remapMax = 1024.0f;		// remap 范围最大值
	Vector3 m_invalidColor = { 1.0f, 0.5f, 0.5f };	// 无效值的预览颜色
	float m_imageZoomScale = 1.0f;	// 图像缩放系数
};
