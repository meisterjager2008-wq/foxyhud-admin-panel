#pragma once

// Bits shared by the demo programs (not needed in your game).

#include <cstdio>

#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui.h"

namespace demo {

// Background color behind the menu.
inline constexpr float kClearColor[3] = {0.055f, 0.058f, 0.068f};

// ImGui context + the menu's fonts and theme (the DirectX demos do this
// through foxy::dxN::Init instead).
inline void InitImGui(float scale) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = "foxyhud_demo.ini";
    foxy::theme::LoadFonts(scale);
    foxy::theme::Apply(scale);
}

// The menu, plus a small hint while it is closed so the window isn't just empty.
inline void DrawFrame(foxy::AdminPanel& panel) {
    panel.Render();
    if (panel.IsOpen())
        return;
    char hint[64];
    std::snprintf(hint, sizeof(hint), "Press %s to open the menu", foxy::ui::KeyName(panel.Settings().menu_key));
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 ts = ImGui::CalcTextSize(hint);
    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(vp->Pos.x + (vp->Size.x - ts.x) * 0.5f, vp->Pos.y + vp->Size.y - ts.y - 16.0f),
        IM_COL32(255, 255, 255, 80), hint);
}

} // namespace demo
