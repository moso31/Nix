#pragma once
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <fstream>
#include <iostream>
#include <cctype>
#include <filesystem>
#include "imgui.h"

class NXGUICodeEditor
{
    class ThreadPool
    {
    public:
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ThreadPool(int threadCount = 4)
        {
            m_threads.resize(threadCount);
            for (auto& thread : m_threads)
            {
                thread = std::thread([this]() {
                    while (!m_bShutdown)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(m_mutex);
                            m_condition.wait(lock, [this]() { return m_bShutdown || !m_tasks.empty(); });
                            task = std::move(m_tasks.front());
                            m_tasks.pop();
                        }
                        if (task) task();
                    }
                    });
            }
        }

        void Shutdown()
        {
            m_bShutdown = true;
            m_condition.notify_all();
            for (auto& thread : m_threads)
            {
                if (thread.joinable())
                    thread.join();
            }
        }

        void AddTaskFunc(std::function<void()> func)
        {
            if (m_bShutdown) return;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_tasks.push(func);
            }
            m_condition.notify_one();
        }

        void ClearTaskFunc()
        {
            if (m_bShutdown) return;
            {
				std::unique_lock<std::mutex> lock(m_mutex);
				std::queue<std::function<void()>> empty;
				std::swap(m_tasks, empty);
			}
        }

        ~ThreadPool() { Shutdown(); }

    private:
        std::vector<std::thread> m_threads;
        std::queue<std::function<void()>> m_tasks;

        std::condition_variable m_condition;
        std::mutex m_mutex;

        std::atomic_bool m_bShutdown = false;
    };

    struct Coordinate
    {
        Coordinate() : row(0), col(0) {}
        Coordinate(int r, int c) : row(r), col(c) {}

        bool operator==(const Coordinate& rhs) const
        {
            return row == rhs.row && col == rhs.col;
        }

        bool operator!=(const Coordinate& rhs) const
        {
            return !(*this == rhs);
        }

        bool operator<(const Coordinate& rhs) const
        {
            return row < rhs.row || (row == rhs.row && col < rhs.col);
        }

        bool operator>(const Coordinate& rhs) const
        {
            return row > rhs.row || (row == rhs.row && col > rhs.col);
        }

        bool operator<=(const Coordinate& rhs) const
        {
            return *this < rhs || *this == rhs;
        }

        bool operator>=(const Coordinate& rhs) const
        {
            return *this > rhs || *this == rhs;
        }

        int row;
        int col;
    };

    // 记录单条所选文本信息 from L to R
    struct SelectionInfo
    {
        SelectionInfo() {}

        // 自动对 A B 排序
        SelectionInfo(const Coordinate& A, const Coordinate& B) : L(A < B ? A : B), R(A < B ? B : A), flickerAtFront(A > B) {}

        bool operator==(const SelectionInfo& rhs) const
        {
            return L == rhs.L && R == rhs.R;
        }

        bool operator==(const Coordinate& rhs) const
        {
            return L == rhs && R == rhs;
        }

        // 检测另一个 SelectionInfo 是否是当前 SelectionInfo 的子集
        bool Include(const SelectionInfo& selection) const
        {
            return L <= selection.L && R >= selection.R;
        }

        // 检测 Coordinate 是否在当前 SelectionInfo 内
        bool Include(const Coordinate& X) const
        {
            return L <= X && R >= X;
        }

        Coordinate L;
        Coordinate R;
        bool flickerAtFront = false;
    };

    struct SignedCoordinate
    {
        SignedCoordinate(Coordinate value, bool isLeft, bool flickerAtFront) : value(value), isLeft(isLeft), flickerAtFront(flickerAtFront) {}

        bool operator<(const SignedCoordinate& rhs) const
        {
            return value < rhs.value || (value == rhs.value && isLeft < rhs.isLeft);
        }

        Coordinate value;
        int isLeft;
        bool flickerAtFront;
    };

    struct TextFormat
    {
        TextFormat(ImU32 color, int index) : color(color), length(index) {}

        ImU32 color = 0xffffffff;
        int length = INT_MAX;
    };

    // std::string 扩展类，用于在渲染时，根据颜色绘制字符
    struct TextString : public std::string
    {
        TextString() = default;
        TextString(const std::string& str) : std::string(str) {};

        // 渲染时遍历formatArray，使用 color[i] 颜色，持续 length[i] 个字符
        std::vector<TextFormat> formatArray;
    };

    struct TextKeyword
    {
        std::string string;
        int startIndex;
        int tokenColorIndex = -1;
    };

    // 语法高亮关键词
    static std::vector<std::vector<std::string>> const s_hlsl_tokens;

    // 语法高亮关键词颜色
    static std::vector<ImU32> s_hlsl_token_color;

public:
    NXGUICodeEditor(ImFont* pFont);
    ~NXGUICodeEditor() {}

    void Load(const std::filesystem::path& filePath);
    void Load(const std::string& text);
    void Render();

    void AddSelection(const Coordinate& A, const Coordinate& B);
    void RemoveSelection(const SelectionInfo& removeSelection);
    void ClearSelection();

public:
    void Enter(const std::vector<std::vector<std::string>>& strArray);
    void Backspace(bool IsDelete, bool bCtrl);
    void Escape();
    void Copy();
    void Paste();
    void SelectAll();

    void HighLightSyntax(int lineIndex);
    void SetLineUpdateTime(int lineIndex, double manualTime = FLT_MIN);

private:
    void Render_MainLayer();
    void Render_DebugLayer();

    void RenderSelections();
    void RenderTexts();
    void RenderLineNumber();
    void CalcLineNumberRectWidth();

    void RenderSelection(const SelectionInfo& selection);
    void SelectionsOverlayCheckForMouseEvent(bool bIsDoubleClick);
    void SelectionsOverlayCheckForKeyEvent(bool bFlickerAtFront);
    void ScrollCheckForKeyEvent();
    int CalcSelectionLength(const SelectionInfo& selection);

private:
    void Render_OnMouseInputs();

    void RenderTexts_OnMouseInputs();
    void RenderTexts_OnKeyInputs();

    void MoveUp(bool bShift, bool bPageUp, bool bCtrlHome);
    void MoveDown(bool bShift, bool bPageDown, bool bCtrlEnd);
    void MoveLeft(bool bShift, bool bCtrl, bool bHome, int size);
    void MoveRight(bool bShift, bool bCtrl, bool bEnd, int size);

    void MoveLeft(SelectionInfo& selection, bool bShift, bool bCtrl, bool bHome, int size);
    void MoveRight(SelectionInfo& selection, bool bShift, bool bCtrl, bool bEnd, int size);

    bool IsVariableChar(const char& ch);

    // 从当前行中提取出可能是关键词的字符
    std::vector<TextKeyword> ExtractKeywords(const TextString& text);

private:
    std::vector<TextString> m_lines = { TextString("") };

private:
    // 记录行号文本能达到的最大宽度
    float m_lineNumberWidth = 0.0f;

    // 行号矩形两侧留出 4px 的空白
    float m_lineNumberPaddingX = 4.0f;
    float m_lineNumberWidthWithPaddingX;

    // 记录最大行号，用于计算行号文本的宽度
    size_t m_maxLineNumber = 0;

    // 文本的起始像素位置
    float m_lineTextStartX;

    // 记录最大行字符数
    size_t m_maxLineCharCount = 0;

    // 单个字符的大小
    float m_charWidth;
    float m_charHeight;

    // 闪烁计时
    double m_flickerDt = 0.0f;
    bool m_bResetFlickerDt = false;

    // 记录选中信息
    std::vector<SelectionInfo> m_selections;
    // 另一组选中信息，用于在选区变化时进行去重。
    std::vector<SignedCoordinate> m_overlaySelectCheck;

    bool m_bIsSelecting = false;

    Coordinate m_activeSelectionDown;
    Coordinate m_activeSelectionMove;

    bool m_bNeedFocusOnText = true;

    // 使用的字体（最好为此TextEditor单独设置一个字体）
    ImFont* m_pFont;

    // 记录每行的更新时间，避免异步覆盖
    std::vector<double> m_lineUpdateTime;

    // 2023.7.18 使用线程池优化高亮逻辑
    // 当进行较多行的复制操作时，异步处理高亮
    NXGUICodeEditor::ThreadPool m_threadPool;
};
