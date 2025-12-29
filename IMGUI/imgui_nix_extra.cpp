#include "imgui_nix_extra.h"
#include "imgui_impl_dx12.h"

void ImGui_ImagePointSampling(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Switch to point sampling PSO
    draw_list->AddCallback(ImGui_ImplDX12_SetPointSamplerCallback, nullptr);
    
    // Draw the image
    ImGui::Image(user_texture_id, size, uv0, uv1, tint_col, border_col);
    
    // Restore linear sampling PSO
    draw_list->AddCallback(ImGui_ImplDX12_RestoreLinearSamplerCallback, nullptr);
}

void ImGui_ImageButtonPointSampling(const char* str_id, ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Switch to point sampling PSO
    draw_list->AddCallback(ImGui_ImplDX12_SetPointSamplerCallback, nullptr);
    
    // Draw the image button
    ImGui::ImageButton(str_id, user_texture_id, size, uv0, uv1, bg_col, tint_col);
    
    // Restore linear sampling PSO
    draw_list->AddCallback(ImGui_ImplDX12_RestoreLinearSamplerCallback, nullptr);
}
