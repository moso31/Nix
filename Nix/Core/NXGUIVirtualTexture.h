#pragma once
#include "NXGUI.h"
#include "BaseDefs/Math.h"
#include <vector>
#include <memory>
#include <unordered_map>

class NXVirtualTexture;
struct NXVTSector;

class NXGUIVirtualTexture
{
public:
    NXGUIVirtualTexture(NXGUI* pOwner);
    ~NXGUIVirtualTexture();
    void Render();

    void SetVisible(bool visible) { m_bShowWindow = visible; }
    bool IsVisible() const { return m_bShowWindow; }

private:
    void Render_Sectors();
    void Render_VirtImageAtlas();
    void Render_Readback();

    void BuildDockLayout(ImGuiID dockspace_id);

    // 绘制
    void DrawWorld(ImDrawList* dl, const ImVec2& regionTL, const ImVec2& regionSize);
    ImU32 GetSectorRectColor(int size, bool isFill);

    // 工具函数
    void IdxToRowCol(int idx, int& row, int& col);
    int RowColToIdx(int row, int col);
    ImVec2 TileMin(int row, int col);
    ImVec2 TileMax(int row, int col);

    ImU32 VTReadbackDecodeData(int val) const;

    // 获取NXVirtualTexture实例
    NXVirtualTexture* GetVirtualTexture() const;

private:
    NXGUI* m_pOwner;

    // 世界 & 网格常量
    static constexpr int WORLD_MIN = -8192;
    static constexpr int WORLD_MAX = 8192;
    static constexpr int SECTOR_SIZE = 64;
    static constexpr int GRID_COUNT = (WORLD_MAX - WORLD_MIN) / SECTOR_SIZE; // 256

    // UI 状态
    float m_zoom = 1.0f;                // 额外缩放
    bool  m_drawGrid = true;            // 是否绘制网格线
    bool  m_showOnlyNear = false;       // 左侧列表只显示相机附近的 Sector
    int   m_selectedIdx = -1;           // 选中的全局线性索引（row*256+col）

    // 视图交互：以“像素”为单位的平移偏移（相对可视区左上角）
    // 可选：后续可扩展右键拖拽；当前用中键
    ImVec2 m_panPix{ 0.0f, 0.0f };
    // Virtual Image Atlas 搜索
    char m_atlasSearchBuf[256] = "";
    bool m_bShowWindow;
    std::vector<std::string> m_strTitle;

    // 缓存数据，避免状态机中间帧数据不一致导致闪烁
    struct CachedData;
    std::unique_ptr<CachedData> m_cachedData;
};
