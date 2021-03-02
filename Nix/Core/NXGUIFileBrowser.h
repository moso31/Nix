#pragma once
#pragma once
#include "imfilebrowser.h"

enum NXGUIFlagsFileBrowser
{
    NXGUIFlagsFileBrowser_SelectDirectory = 1 << 0, // select directory instead of regular file
    NXGUIFlagsFileBrowser_EnterNewFilename = 1 << 1, // allow user to enter new filename when selecting regular file
    NXGUIFlagsFileBrowser_NoModal = 1 << 2, // file browsing window is modal by default. specify this to use a popup window
    NXGUIFlagsFileBrowser_NoTitleBar = 1 << 3, // hide window title bar
    NXGUIFlagsFileBrowser_NoStatusBar = 1 << 4, // hide status bar at the bottom of browsing window
    NXGUIFlagsFileBrowser_CloseOnEsc = 1 << 5, // close file browser when pressing 'ESC'
    NXGUIFlagsFileBrowser_CreateNewDir = 1 << 6, // allow user to create new directory
    NXGUIFlagsFileBrowser_MultipleSelection = 1 << 7, // allow user to select multiple files. this will hide ImGuiFileBrowserFlags_EnterNewFilename
};

class NXGUIFileBrowser : public ImGui::FileBrowser
{
public:
    // pwd is set to current working directory by default
    explicit NXGUIFileBrowser(NXGUIFlagsFileBrowser flags = (NXGUIFlagsFileBrowser)0);

    NXGUIFileBrowser FileBrowser(const NXGUIFileBrowser& copyFrom);

    // set the window size (in pixels)
    // default is (700, 450)
    void SetWindowSize(int width, int height) noexcept;

    // set the window title text
    void SetTitle(std::string title);

    // open the browsing window
    void Open();

    // close the browsing window
    void Close();

    // the browsing window is opened or not
    bool IsOpened() const noexcept;

    // display the browsing window if opened
    void Display();

    // returns true when there is a selected filename and the "ok" button was clicked
    bool HasSelected() const noexcept;

    // set current browsing directory
    bool SetPwd(const std::filesystem::path& pwd =
        std::filesystem::current_path());

    // get current browsing directory
    const std::filesystem::path& GetPwd() const noexcept;

    // returns selected filename. make sense only when HasSelected returns true
    // when ImGuiFileBrowserFlags_MultipleSelection is enabled, only one of
    // selected filename will be returned
    std::filesystem::path GetSelected() const;

    // returns all selected filenames.
    // when ImGuiFileBrowserFlags_MultipleSelection is enabled, use this
    // instead of GetSelected
    std::vector<std::filesystem::path> GetMultiSelected() const;

    // set selected filename to empty
    void ClearSelected();

    // (optional) set file type filters. eg. { ".h", ".cpp", ".hpp" }
    // ".*" matches any file types
    void SetTypeFilters(const std::vector<std::string>& typeFilters);

    // set currently applied type filter
    // default value is 0 (the first type filter)
    void SetCurrentTypeFilterIndex(int index);

    void SetOnDialogOK(const std::function<void()>& func);
    void SetOnDialogCancel(const std::function<void()>& func);
    void SetOnItemDoubleClicked(const std::function<void()>& func);

private:
};

inline NXGUIFileBrowser::NXGUIFileBrowser(NXGUIFlagsFileBrowser flags)
{
    ImGui::FileBrowser((ImGuiFileBrowserFlags)flags);
}

inline NXGUIFileBrowser NXGUIFileBrowser::FileBrowser(const NXGUIFileBrowser& copyFrom)
{
	ImGui::FileBrowser((ImGui::FileBrowser)copyFrom);

    m_onOK = copyFrom.m_onOK;
    m_onCancel = copyFrom.m_onCancel;
}

inline void NXGUIFileBrowser::SetWindowSize(int width, int height) noexcept
{
    ImGui::FileBrowser::SetWindowSize(width, height);
}

inline void NXGUIFileBrowser::SetTitle(std::string title)
{
    ImGui::FileBrowser::SetTitle(title);
}

inline void NXGUIFileBrowser::Open()
{
    ImGui::FileBrowser::Open();
}

inline void NXGUIFileBrowser::Close()
{
    ImGui::FileBrowser::Close();
}

inline bool NXGUIFileBrowser::IsOpened() const noexcept
{
    return ImGui::FileBrowser::IsOpened();
}

inline void NXGUIFileBrowser::Display()
{
    ImGui::FileBrowser::Display();
}

inline bool NXGUIFileBrowser::HasSelected() const noexcept
{
    return ImGui::FileBrowser::HasSelected();
}

inline bool NXGUIFileBrowser::SetPwd(const std::filesystem::path& pwd)
{
    return ImGui::FileBrowser::SetPwd(pwd);
}

inline const std::filesystem::path& NXGUIFileBrowser::GetPwd() const noexcept
{
    return ImGui::FileBrowser::GetPwd();
}

inline std::filesystem::path NXGUIFileBrowser::GetSelected() const
{
    return ImGui::FileBrowser::GetSelected();
}

inline std::vector<std::filesystem::path> NXGUIFileBrowser::GetMultiSelected() const
{
    return ImGui::FileBrowser::GetMultiSelected();
}

inline void NXGUIFileBrowser::ClearSelected()
{
    ImGui::FileBrowser::ClearSelected();
}

inline void NXGUIFileBrowser::SetTypeFilters(const std::vector<std::string>& typeFilters)
{
    ImGui::FileBrowser::SetTypeFilters(typeFilters);
}

inline void NXGUIFileBrowser::SetCurrentTypeFilterIndex(int index)
{
    ImGui::FileBrowser::SetCurrentTypeFilterIndex(index);
}

inline void NXGUIFileBrowser::SetOnDialogOK(const std::function<void()>& func)
{
    m_onOK = func;
}

inline void NXGUIFileBrowser::SetOnDialogCancel(const std::function<void()>& func)
{
    m_onCancel = func;
}
