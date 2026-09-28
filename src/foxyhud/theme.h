#pragma once

#include "imgui.h"

namespace foxy {

// Every colour the panel draws with. The accent-derived entries are rebuilt by
// theme::SetAccent(), everything else is fixed.
struct Palette {
    ImVec4 window_bg;        // main window + header
    ImVec4 window_border;
    ImVec4 panel_bg;         // the bordered boxes that hold sections
    ImVec4 panel_border;
    ImVec4 separator;        // line under section titles
    ImVec4 widget_bg;        // checkbox / slider track / combo / input frame
    ImVec4 widget_bg_hover;
    ImVec4 widget_border;
    ImVec4 popup_bg;
    ImVec4 scrollbar_grab;

    ImVec4 text;             // regular labels
    ImVec4 text_bright;      // titles, values
    ImVec4 text_dim;         // inactive tabs, secondary info
    ImVec4 text_faint;       // hints, placeholders

    ImVec4 accent;           // fills (checked boxes, slider fill)
    ImVec4 accent_text;      // accent coloured text (checked labels, active tab)
    ImVec4 accent_dark;      // gradient start

    ImVec4 danger;
    ImVec4 warning;
    ImVec4 success;
    ImVec4 info;
};

struct Fonts {
    ImFont* body    = nullptr;  // Montserrat Medium   - labels, values
    ImFont* bold    = nullptr;  // Montserrat SemiBold - section titles, buttons
    ImFont* caption = nullptr;  // Montserrat Medium   - badges, secondary lines
    ImFont* brand   = nullptr;  // Montserrat Bold     - brand name in the header
    ImFont* title   = nullptr;  // Montserrat Bold     - big "Maintenance" text
};

namespace theme {

Palette& Colors();
Fonts&   GetFonts();

// UI scale chosen at init time (1.0 = 100%). All widget metrics go through Px().
float Scale();

// Adds the embedded fonts to ImGui::GetIO().Fonts and makes `body` the default.
// Call once after ImGui::CreateContext(), before the first NewFrame().
void LoadFonts(float scale = 1.0f);

// Writes the palette into ImGuiStyle so stock ImGui widgets match as well.
void Apply(float scale = 1.0f);

void   SetAccent(const ImVec4& accent);
ImVec4 DefaultAccent();

// Colour helpers. Col() honours ImGuiStyle::Alpha so fades and BeginDisabled() work.
ImU32  Col(const ImVec4& c, float alpha_mul = 1.0f);
ImVec4 Lerp(const ImVec4& a, const ImVec4& b, float t);
ImVec4 WithAlpha(const ImVec4& c, float a);

// Font helpers that work on ImGui versions before and after the 1.92 font rework.
void  PushFont(ImFont* font);
void  PopFont();
float FontSize(const ImFont* font);

} // namespace theme
} // namespace foxy
