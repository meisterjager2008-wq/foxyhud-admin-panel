#include <cstdio>

#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using theme::Col;
using ui::Px;

// Top-right bar: "Foxyhud.pw | FPS: 144 | CPU: 12% | GPU: 34% | RAM: 8.1 / 15.9 GB".
// Menu background color with an accent line along the bottom. Shown whenever
// Misc > Windows > Watermark is on, also while the menu is closed.
void AdminPanel::RenderWatermark() {
    if (!settings_.watermark) {
        if (monitor_.IsRunning())
            monitor_.Stop();
        return;
    }
    if (!monitor_.IsRunning())
        monitor_.Start();

    // FPS refreshes twice a second so the number is readable; CPU/GPU/RAM once a second.
    const double now = ImGui::GetTime();
    if (now - fps_updated_at_ >= 0.5) {
        fps_shown_ = ImGui::GetIO().Framerate;
        fps_updated_at_ = now;
    }
    const SystemMonitor::Stats stats = monitor_.Get();

    char fps[16], cpu[16], gpu[16], ram[32];
    std::snprintf(fps, sizeof(fps), "%d", static_cast<int>(fps_shown_ + 0.5f));
    if (stats.cpu_percent >= 0.0f)
        std::snprintf(cpu, sizeof(cpu), "%d%%", static_cast<int>(stats.cpu_percent + 0.5f));
    else
        std::snprintf(cpu, sizeof(cpu), "N/A");
    if (stats.gpu_percent >= 0.0f)
        std::snprintf(gpu, sizeof(gpu), "%d%%", static_cast<int>(stats.gpu_percent + 0.5f));
    else
        std::snprintf(gpu, sizeof(gpu), "N/A");
    if (stats.ram_total_gb > 0.0f)
        std::snprintf(ram, sizeof(ram), "%.1f / %.1f GB", stats.ram_used_gb, stats.ram_total_gb);
    else
        std::snprintf(ram, sizeof(ram), "N/A");

    struct Item {
        const char* label;
        const char* value;
        const char* widest;  // reserves space so the bar doesn't jiggle as numbers change
    };
    const Item items[] = {
        {"FPS: ", fps, "888"},
        {"CPU: ", cpu, "100%"},
        {"GPU: ", gpu, "100%"},
        {"RAM: ", ram, "88.8 / 88.8 GB"},
    };

    const Palette& c = theme::Colors();
    const Fonts& f = theme::GetFonts();
    const float body = theme::FontSize(f.body), bold = theme::FontSize(f.bold);
    auto text_w = [](ImFont* font, float size, const char* text) {
        return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text).x;
    };

    const float pad = Px(11), gap = Px(10), height = Px(28), line = Px(2), margin = Px(10);
    const float name_w = text_w(f.bold, bold, brand_name_.c_str());
    const float brand_w = name_w + text_w(f.bold, bold, brand_suffix_.c_str());
    float item_w[IM_ARRAYSIZE(items)];
    float width = pad + brand_w + pad;
    for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
        const float value_w = ImMax(text_w(f.body, body, items[i].value), text_w(f.body, body, items[i].widest));
        item_w[i] = text_w(f.body, body, items[i].label) + value_w;
        width += gap + 1.0f + gap + item_w[i];
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 mn(IM_ROUND(vp->WorkPos.x + vp->WorkSize.x - margin - width), IM_ROUND(vp->WorkPos.y + margin));
    const ImVec2 mx(mn.x + width, mn.y + height);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    dl->AddRectFilled(mn, mx, Col(c.window_bg, settings_.menu_opacity / 100.0f), Px(4), ImDrawFlags_RoundCornersTop);
    dl->AddRectFilled(ImVec2(mn.x, mx.y - line), mx, Col(c.accent));

    const float content_h = height - line;
    float x = mn.x + pad;
    const float brand_y = mn.y + (content_h - bold) * 0.5f;
    dl->AddText(f.bold, bold, ui::Snap(ImVec2(x, brand_y)), Col(c.text_bright), brand_name_.c_str());
    dl->AddText(f.bold, bold, ui::Snap(ImVec2(x + name_w, brand_y)), Col(c.accent_text), brand_suffix_.c_str());
    x += brand_w;

    const float text_y = mn.y + (content_h - body) * 0.5f;
    for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
        x += gap;
        const float sx = IM_ROUND(x) + 0.5f;
        dl->AddLine(ImVec2(sx, mn.y + Px(7)), ImVec2(sx, mx.y - line - Px(7)), Col(c.widget_border), 1.0f);
        x += 1.0f + gap;
        const float label_w = text_w(f.body, body, items[i].label);
        dl->AddText(f.body, body, ui::Snap(ImVec2(x, text_y)), Col(c.text), items[i].label);
        dl->AddText(f.body, body, ui::Snap(ImVec2(x + label_w, text_y)), Col(c.text_bright), items[i].value);
        x += item_w[i];
    }
}

} // namespace foxy
