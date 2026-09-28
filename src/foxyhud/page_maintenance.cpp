#include <cmath>
#include <cstdio>

#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using theme::Col;
using ui::Px;

// Placeholder for tabs that don't have content yet.
void AdminPanel::RenderMaintenancePage(const ImVec2& size, const char* tab_name) {
    if (ui::BeginPanel("##maintenance", size)) {
        const Palette& c = theme::Colors();
        const Fonts& f = theme::GetFonts();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos(), ws = ImGui::GetWindowSize();
        const float t = static_cast<float>(ImGui::GetTime());
        const float rot = settings_.animations ? t * 0.8f : 0.0f;

        const char* title = "Maintenance";
        char subtitle[128];
        std::snprintf(subtitle, sizeof(subtitle), "The %s tab is being worked on. Check back soon.", tab_name);

        const float title_size = theme::FontSize(f.title);
        const ImVec2 title_sz = f.title->CalcTextSizeA(title_size, FLT_MAX, 0.0f, title);
        const ImVec2 sub_sz = ImGui::CalcTextSize(subtitle);

        // Vertical stack: gears, title, subtitle, progress bar - centered as a block.
        const float gears_h = Px(64), bar_h = Px(4);
        const float block_h = gears_h + Px(18) + title_sz.y + Px(6) + sub_sz.y + Px(20) + bar_h;
        const float cx = wp.x + ws.x * 0.5f;
        float y = IM_ROUND(wp.y + (ws.y - block_h) * 0.5f);

        // Two meshing gears.
        const ImVec2 big(cx - Px(10), y + Px(26));
        const ImVec2 little(big.x + Px(27), big.y + Px(20));
        DrawIcon(dl, Icon::Gear, big, Px(48), Col(c.accent_text), rot, Px(2.4f));
        DrawIcon(dl, Icon::Gear, little, Px(30), Col(c.text_dim), -rot + 0.39f, Px(2.0f));
        y += gears_h + Px(18);

        dl->AddText(f.title, title_size, ui::Snap(ImVec2(cx - title_sz.x * 0.5f, y)), Col(c.text_bright), title);
        y += title_sz.y + Px(6);
        dl->AddText(ui::Snap(ImVec2(cx - sub_sz.x * 0.5f, y)), Col(c.text_dim), subtitle);
        y += sub_sz.y + Px(20);

        // Indeterminate progress bar.
        const float bw = Px(200);
        const ImVec2 bmin(cx - bw * 0.5f, y);
        dl->AddRectFilled(bmin, ImVec2(bmin.x + bw, bmin.y + bar_h), Col(c.widget_bg_hover), bar_h * 0.5f);
        const float seg = bw * 0.3f;
        const float phase = settings_.animations ? std::fmod(t * 0.7f, 1.0f) : 0.35f;
        const float x0 = ImMax(bmin.x, bmin.x - seg + (bw + seg) * phase);
        const float x1 = ImMin(bmin.x + bw, bmin.x + (bw + seg) * phase);
        if (x1 > x0)
            dl->AddRectFilled(ImVec2(x0, bmin.y), ImVec2(x1, bmin.y + bar_h), Col(c.accent), bar_h * 0.5f);
    }
    ui::EndPanel();
}

} // namespace foxy
