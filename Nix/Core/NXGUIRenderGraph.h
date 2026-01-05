#pragma once
#include "Renderer.h"

enum class NXRGGUIResourceViewMode
{
	Virtual,
	Physical
};

struct NXRGGUIResource;

class NXGUIRenderGraph
{
public:
	NXGUIRenderGraph(Renderer* pRenderer);
	virtual ~NXGUIRenderGraph() {}

	void Render();

	void SetVisible(bool visible) { m_bShowWindow = visible; }
	bool IsVisible() const { return m_bShowWindow; }

private:
	// 辅助方法 - 格式化和转换
	static std::string FormatByteSize(size_t bytes);
	static const char* GetDXGIFormatString(int format);
	static constexpr ImVec4 ImU32ToImVec4(ImU32 color);

	// 辅助方法 - 绘制相关
	void ShowResourceTooltip(const NXRGGUIResource& res, NXRenderGraph* pRenderGraph, int segmentIndex = 0);
	void DrawTimeLayerHighlightRect(int timeLayer, int minTimeLayer, int maxTimeLayer, double resourceCount, float bottomPadding);
	void DrawTimeLayerPassNames(int timeLayer, int minTimeLayer, int maxTimeLayer, double resourceCount, NXRenderGraph* pRenderGraph);
	void DrawResourceSeparatorLine(size_t nonImportedCount, int minTimeLayer, int maxTimeLayer);

	// 核心渲染方法
	void RenderResourceView(
		const std::vector<NXRGGUIResource>& displayResources,
		NXRenderGraph* pRenderGraph,
		int minTimeLayer,
		int maxTimeLayer,
		const char* plotLabel,
		bool isPhysicalMode);

private:
	Renderer* m_pRenderer;
	Ntr<NXResource> m_pShowResource;

	bool m_bShowWindow = false;
	NXRGGUIResourceViewMode m_viewMode = NXRGGUIResourceViewMode::Virtual;
	bool m_showImportedResources = true;

	// 静态配置常量
	static constexpr ImU32 COLOR_REGULAR_RESOURCE = IM_COL32(70, 130, 180, 200);
	static constexpr ImU32 COLOR_IMPORTED_RESOURCE = IM_COL32(144, 238, 144, 200);
	static constexpr ImU32 COLOR_HIGHLIGHT_FILL = IM_COL32(255, 255, 0, 100);
	static constexpr ImU32 COLOR_HIGHLIGHT_BORDER = IM_COL32(255, 255, 0, 255);
	static constexpr ImU32 COLOR_COLUMN_HIGHLIGHT = IM_COL32(255, 255, 0, 60);
	static constexpr ImU32 COLOR_TEXT_BACKGROUND = IM_COL32(0, 0, 0, 180);
	static constexpr ImU32 COLOR_TEXT_FOREGROUND = IM_COL32(255, 255, 255, 200);
	static constexpr ImU32 COLOR_SEPARATOR_LINE = IM_COL32(255, 255, 255, 128);
};
