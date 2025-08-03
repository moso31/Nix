#pragma once
#include "BaseDefs/DearImGui.h"
#include "NXVirtualTextureManager.h"

class NXGUIVirtualTexture
{
public:
    NXGUIVirtualTexture();
    void Render();

private:
    void Render_Sectors();
    void Render_VirtImageAtlas();

    void BuildDockLayout(ImGuiID dockspace_id);

    // ����
    void DrawWorld(ImDrawList* dl, const ImVec2& regionTL, const ImVec2& regionSize);

    // ���ߺ���
    void IdxToRowCol(int idx, int& row, int& col);
    int RowColToIdx(int row, int col);
    ImVec2 TileMin(int row, int col);
    ImVec2 TileMax(int row, int col);

private:
    // ���� & ������
    static constexpr int WORLD_MIN = -8192;
    static constexpr int WORLD_MAX = 8192;
    static constexpr int SECTOR_SIZE = 64;
    static constexpr int GRID_COUNT = (WORLD_MAX - WORLD_MIN) / SECTOR_SIZE; // 256

    // UI ״̬
    float m_zoom = 1.0f;                // ��������
    bool  m_drawGrid = true;            // �Ƿ����������
    bool  m_showOnlyNear = false;       // ����б�ֻ��ʾ��������� Sector
    int   m_selectedIdx = -1;           // ѡ�е�ȫ������������row*256+col��

    // ��ͼ�������ԡ����ء�Ϊ��λ��ƽ��ƫ�ƣ���Կ��������Ͻǣ�
    // ��ѡ����������չ�Ҽ���ק����ǰ���м�
    ImVec2 m_panPix{ 0.0f, 0.0f };

    bool m_bShowWindow;
    std::vector<std::string> m_strTitle;
};
