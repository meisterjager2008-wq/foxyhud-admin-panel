#include "foxyhud/theme.h"

#include "foxyhud/fonts/montserrat_bold.h"
#include "foxyhud/fonts/montserrat_medium.h"
#include "foxyhud/fonts/montserrat_semibold.h"

namespace foxy::theme {
namespace {

constexpr ImVec4 Rgb(int r, int g, int b, float a = 1.0f) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

Palette MakeDefaultPalette() {
    Palette p{};
    p.window_bg       = Rgb(17, 17, 18);
    p.window_border   = Rgb(40, 40, 43);
    p.panel_bg        = Rgb(21, 21, 22);
    p.panel_border    = Rgb(33, 33, 36);
    p.separator       = Rgb(38, 38, 41);
    p.widget_bg       = Rgb(27, 27, 29);
    p.widget_bg_hover = Rgb(34, 34, 37);
    p.widget_border   = Rgb(46, 46, 50);
    p.popup_bg        = Rgb(24, 24, 26);
    p.scrollbar_grab  = Rgb(46, 46, 50);

    p.text            = Rgb(200, 200, 203);
    p.text_bright     = Rgb(238, 238, 240);
    p.text_dim        = Rgb(135, 135, 140);
    p.text_faint      = Rgb(92, 92, 97);

    p.danger          = Rgb(222, 64, 70);
    p.warning         = Rgb(232, 154, 44);
    p.success         = Rgb(62, 186, 108);
    p.info            = Rgb(56, 170, 222);
    return p;
}

Palette g_palette = MakeDefaultPalette();
Fonts   g_fonts;
float   g_scale = 1.0f;

// Default accent: the deep royal blue from the reference design.
constexpr ImVec4 kDefaultAccent = Rgb(0, 92, 208);

struct AccentInit {
    AccentInit() { SetAccent(kDefaultAccent); }
} g_accent_init;

} // namespace

Palette& Colors() { return g_palette; }
ImVec4   DefaultAccent() { return kDefaultAccent; }
Fonts&   GetFonts() { return g_fonts; }
float    Scale() { return g_scale; }

ImVec4 Lerp(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

ImVec4 WithAlpha(const ImVec4& c, float a) { return ImVec4(c.x, c.y, c.z, a); }

ImU32 Col(const ImVec4& c, float alpha_mul) {
    return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, c.w * alpha_mul));
}

void SetAccent(const ImVec4& accent) {
    g_palette.accent      = WithAlpha(accent, 1.0f);
    g_palette.accent_text = Lerp(g_palette.accent, ImVec4(1, 1, 1, 1), 0.16f);
    g_palette.accent_dark = Lerp(g_palette.accent, ImVec4(0, 0, 0, 1), 0.30f);

    if (ImGui::GetCurrentContext()) {
        ImVec4* c = ImGui::GetStyle().Colors;
        c[ImGuiCol_CheckMark]            = g_palette.accent;
        c[ImGuiCol_SliderGrab]           = g_palette.accent;
        c[ImGuiCol_SliderGrabActive]     = g_palette.accent_text;
        c[ImGuiCol_Header]               = WithAlpha(g_palette.accent, 0.20f);
        c[ImGuiCol_HeaderHovered]        = WithAlpha(g_palette.accent, 0.30f);
        c[ImGuiCol_HeaderActive]         = WithAlpha(g_palette.accent, 0.40f);
        c[ImGuiCol_TextSelectedBg]       = WithAlpha(g_palette.accent, 0.40f);
        c[ImGuiCol_ResizeGripHovered]    = WithAlpha(g_palette.accent, 0.50f);
        c[ImGuiCol_ResizeGripActive]     = WithAlpha(g_palette.accent, 0.80f);
        c[ImGuiCol_SeparatorHovered]     = WithAlpha(g_palette.accent, 0.60f);
        c[ImGuiCol_SeparatorActive]      = g_palette.accent;
        c[ImGuiCol_ScrollbarGrabActive]  = WithAlpha(g_palette.accent, 0.80f);
        c[ImGuiCol_DragDropTarget]       = g_palette.accent;
    }
}

void LoadFonts(float scale) {
    g_scale = scale;
    ImGuiIO& io = ImGui::GetIO();

    // Latin-1 plus the few punctuation glyphs the panel uses (bullet, dashes, ellipsis).
    static const ImWchar kRanges[] = {
        0x0020, 0x00FF, 0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D,
        0x2022, 0x2022, 0x2026, 0x2026, 0,
    };

    ImFontConfig cfg;
    cfg.OversampleH = 3;
    cfg.OversampleV = 1;
    cfg.PixelSnapH  = true;

    g_fonts.body    = io.Fonts->AddFontFromMemoryCompressedBase85TTF(montserrat_medium_compressed_data_base85,   13.0f * scale, &cfg, kRanges);
    g_fonts.bold    = io.Fonts->AddFontFromMemoryCompressedBase85TTF(montserrat_semibold_compressed_data_base85, 13.0f * scale, &cfg, kRanges);
    g_fonts.caption = io.Fonts->AddFontFromMemoryCompressedBase85TTF(montserrat_medium_compressed_data_base85,   11.0f * scale, &cfg, kRanges);
    g_fonts.brand   = io.Fonts->AddFontFromMemoryCompressedBase85TTF(montserrat_bold_compressed_data_base85,     19.0f * scale, &cfg, kRanges);
    g_fonts.title   = io.Fonts->AddFontFromMemoryCompressedBase85TTF(montserrat_bold_compressed_data_base85,     30.0f * scale, &cfg, kRanges);
    io.FontDefault = g_fonts.body;
}

void Apply(float scale) {
    g_scale = scale;
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle();

    s.WindowPadding     = ImVec2(10, 10);
    s.FramePadding      = ImVec2(7, 3);
    s.CellPadding       = ImVec2(6, 3);
    s.ItemSpacing       = ImVec2(8, 8);
    s.ItemInnerSpacing  = ImVec2(6, 4);
    s.IndentSpacing     = 16;
    s.ScrollbarSize     = 5;
    s.GrabMinSize       = 8;

    s.WindowBorderSize  = 1;
    s.ChildBorderSize   = 1;
    s.PopupBorderSize   = 1;
    s.FrameBorderSize   = 0;

    s.WindowRounding    = 6;
    s.ChildRounding     = 4;
    s.FrameRounding     = 3;
    s.PopupRounding     = 4;
    s.ScrollbarRounding = 4;
    s.GrabRounding      = 3;
    s.TabRounding       = 4;

    s.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    s.WindowMenuButtonPosition = ImGuiDir_None;
    s.DisabledAlpha     = 0.45f;

    const Palette& p = g_palette;
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = p.text;
    c[ImGuiCol_TextDisabled]         = p.text_faint;
    c[ImGuiCol_WindowBg]             = p.window_bg;
    c[ImGuiCol_ChildBg]              = p.panel_bg;
    c[ImGuiCol_PopupBg]              = p.popup_bg;
    c[ImGuiCol_Border]               = p.panel_border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = p.widget_bg;
    c[ImGuiCol_FrameBgHovered]       = p.widget_bg_hover;
    c[ImGuiCol_FrameBgActive]        = p.widget_bg_hover;
    c[ImGuiCol_TitleBg]              = p.window_bg;
    c[ImGuiCol_TitleBgActive]        = p.window_bg;
    c[ImGuiCol_TitleBgCollapsed]     = p.window_bg;
    c[ImGuiCol_MenuBarBg]            = p.window_bg;
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]        = p.scrollbar_grab;
    c[ImGuiCol_ScrollbarGrabHovered] = Lerp(p.scrollbar_grab, p.text_dim, 0.3f);
    c[ImGuiCol_Button]               = p.widget_bg;
    c[ImGuiCol_ButtonHovered]        = p.widget_bg_hover;
    c[ImGuiCol_ButtonActive]         = p.widget_border;
    c[ImGuiCol_Separator]            = p.separator;
    c[ImGuiCol_ResizeGrip]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_Tab]                  = p.widget_bg;
    c[ImGuiCol_TableHeaderBg]        = p.widget_bg;
    c[ImGuiCol_TableBorderStrong]    = p.panel_border;
    c[ImGuiCol_TableBorderLight]     = p.separator;
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0, 0, 0, 0.55f);
    SetAccent(p.accent);

    s.ScaleAllSizes(scale);
}

void PushFont(ImFont* font) {
#if IMGUI_VERSION_NUM >= 19200
    ImGui::PushFont(font, font ? font->LegacySize : 0.0f);
#else
    ImGui::PushFont(font);
#endif
}

void PopFont() { ImGui::PopFont(); }

float FontSize(const ImFont* font) {
#if IMGUI_VERSION_NUM >= 19200
    return font->LegacySize;
#else
    return font->FontSize;
#endif
}

} // namespace foxy::theme
