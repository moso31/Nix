#pragma once
#include "imgui.h"

// Helper function to render an ImGui::Image with point sampling (nearest neighbor)
// This is useful for displaying textures without filtering, e.g. pixel art or debug views
//
// Usage:
//   ImGui_ImagePointSampling(myTextureID, ImVec2(256, 256));
//
// Note: This function uses ImDrawList callbacks to switch PSO, so it works within
// the normal ImGui rendering flow without requiring any special setup.
void ImGui_ImagePointSampling(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

// Helper function to render an ImGui::ImageButton with point sampling
void ImGui_ImageButtonPointSampling(const char* str_id, ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
