#include "foxyhud/admin_panel.h"

#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using theme::Col;
using ui::Px;

// Sub tabs: the icon buttons centered under the header, in page order.
const AdminPanel::SubTabDef kLegitbotSubTabs[] = {
    {"General", Icon::Crosshair},
    {"Advanced", Icon::Target},
};
const AdminPanel::SubTabDef kVisualsSubTabs[] = {
    {"Players", Icon::EyeFrame},
    {"World", Icon::Globe},
    {"Chat", Icon::Chat},
};

// Header tabs, left to right. Tabs mapped to Page::Maintenance show the
// "Maintenance" placeholder until they get real content.
const AdminPanel::TabDef AdminPanel::kTabs[] = {
    {"Ragebot",   Icon::Gauge,     Page::Maintenance, nullptr, 0},
    {"Legitbot",  Icon::Crosshair, Page::Maintenance, kLegitbotSubTabs, IM_ARRAYSIZE(kLegitbotSubTabs)},
    {"Visuals",   Icon::EyeFrame,  Page::Visuals,     kVisualsSubTabs, IM_ARRAYSIZE(kVisualsSubTabs)},
    {"Misc",      Icon::Cog,       Page::Misc,        nullptr, 0},
    {"Helper",    Icon::Help,      Page::Maintenance, nullptr, 0},
    {"Players",   Icon::EyeFrame,  Page::Players,     nullptr, 0},
    {"Inventory", Icon::Palette,   Page::Maintenance, nullptr, 0},
    {"Config",    Icon::Gear,      Page::Config,      nullptr, 0},
};
const int AdminPanel::kTabCount = IM_ARRAYSIZE(AdminPanel::kTabs);

namespace {
constexpr const char* kMainWindowName = "##foxyhud_admin_panel";
constexpr float kHeaderCenterY  = 24.0f;  // design px from the window top
constexpr float kContentTop     = 50.0f;  // tabs without sub tabs
constexpr float kSubTabCenterY  = 60.0f;
constexpr float kSubContentTop  = 78.0f;  // tabs with sub tabs
constexpr float kMargin         = 12.0f;
constexpr float kTabHeight      = 26.0f;
constexpr float kTabGap         = 4.0f;
constexpr float kSubTabSize     = 24.0f;
constexpr float kSubTabGap      = 6.0f;
} // namespace

AdminPanel::AdminPanel(std::string config_dir) : sub_tab_(kTabCount, 0), config_dir_(std::move(config_dir)) {
    IM_ASSERT(theme::GetFonts().body && "Call foxy::theme::LoadFonts() before creating the AdminPanel");
    for (int i = 0; i < kTabCount; ++i)  // open on Misc, like the reference
        if (kTabs[i].page == Page::Misc)
            current_tab_ = i;
    RefreshConfigs();
    settings_dirty_ = true;  // first frame pushes the settings into the theme
}

void AdminPanel::SetBranding(std::string name, std::string suffix) {
    brand_name_ = std::move(name);
    brand_suffix_ = std::move(suffix);
}

void AdminPanel::ApplySettings() {
    theme::SetAccent(settings_.accent_override
                         ? ImVec4(settings_.accent[0], settings_.accent[1], settings_.accent[2], 1.0f)
                         : theme::DefaultAccent());
    ui::SetAnimationsEnabled(settings_.animations);
    ui::SetTooltipsEnabled(settings_.tooltips);
    if (on_settings_changed_)
        on_settings_changed_(settings_);
}

void AdminPanel::Render() {
    HandleHotkeys();
    if (was_open_ && !open_)
        ClosePanelPopups();
    was_open_ = open_;

    const float target = open_ ? 1.0f : 0.0f;
    if (settings_.animations)
        open_anim_ += (target - open_anim_) * ImMin(1.0f, ImGui::GetIO().DeltaTime * 14.0f);
    if (!settings_.animations || ImFabs(target - open_anim_) < 0.01f)
        open_anim_ = target;

    if (open_anim_ > 0.0f)
        RenderMainWindow();

    if (settings_dirty_) {
        settings_dirty_ = false;
        ApplySettings();
    }
}

// The menu key toggles the menu; a keybind next to a checkbox toggles that checkbox.
void AdminPanel::HandleHotkeys() {
    if (ui::IsCapturingKeybind() || ImGui::GetIO().WantTextInput)
        return;

    auto pressed = [](int key) { return key != ImGuiKey_None && ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false); };
    if (pressed(settings_.menu_key))
        Toggle();

    PanelSettings& s = settings_;
    const std::pair<bool*, int> hotkeys[] = {
        {&s.noclip, s.noclip_key},
        {&s.invisible, s.invisible_key},
        {&s.god_mode, s.god_mode_key},
        {&s.quick_freeze, s.quick_freeze_key},
        {&s.quick_spectate, s.quick_spectate_key},
    };
    for (const auto& [value, key] : hotkeys) {
        if (pressed(key)) {
            *value = !*value;
            settings_dirty_ = true;
        }
    }
}

// Closes dropdowns / color pickers that belong to the menu, so nothing stays
// open (or blocks input) after the menu is toggled off.
void AdminPanel::ClosePanelPopups() {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* main_window = ImGui::FindWindowByName(kMainWindowName);
    if (!main_window)
        return;
    for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
        for (ImGuiWindow* w = g.OpenPopupStack[i].Window; w; w = w->ParentWindow) {
            if (w == main_window) {
                ImGui::ClosePopupToLevel(i, false);
                return;
            }
        }
    }
}

float AdminPanel::MinWindowWidth() const {
    const Fonts& f = theme::GetFonts();
    const std::string brand = brand_name_ + brand_suffix_;
    float w = Px(16) + f.brand->CalcTextSizeA(theme::FontSize(f.brand), FLT_MAX, 0.0f, brand.c_str()).x + Px(22);
    for (int i = 0; i < kTabCount; ++i) {
        const float text = f.body->CalcTextSizeA(theme::FontSize(f.body), FLT_MAX, 0.0f, kTabs[i].label).x;
        w += Px(8 + 12 + 6) + text + Px(8) + Px(kTabGap);
    }
    return w + Px(kMargin);
}

void AdminPanel::RenderHeader(const ImVec2& pos) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Palette& c = theme::Colors();
    const Fonts& f = theme::GetFonts();

    // Brand: "Name" in white + ".suffix" in the accent colour.
    const float bs = theme::FontSize(f.brand);
    const ImVec2 bp(pos.x + Px(16), IM_ROUND(pos.y + Px(kHeaderCenterY) - bs * 0.5f));
    const float name_w = f.brand->CalcTextSizeA(bs, FLT_MAX, 0.0f, brand_name_.c_str()).x;
    const float suffix_w = f.brand->CalcTextSizeA(bs, FLT_MAX, 0.0f, brand_suffix_.c_str()).x;
    dl->AddText(f.brand, bs, bp, Col(c.text_bright), brand_name_.c_str());
    dl->AddText(f.brand, bs, ui::Snap(ImVec2(bp.x + name_w, bp.y)), Col(c.accent_text), brand_suffix_.c_str());

    // Tabs
    const float x = bp.x + name_w + suffix_w + Px(22);
    ImGui::SetCursorScreenPos(ImVec2(x, IM_ROUND(pos.y + Px(kHeaderCenterY) - Px(kTabHeight) * 0.5f)));
    for (int i = 0; i < kTabCount; ++i) {
        if (i > 0)
            ImGui::SameLine(0.0f, Px(kTabGap));
        ImGui::PushID(i);
        if (ui::Tab(kTabs[i].label, kTabs[i].icon, i == current_tab_) && i != current_tab_)
            SelectTab(i);
        ImGui::PopID();
    }
}

void AdminPanel::RenderSubTabs(const ImVec2& pos, float width) {
    const TabDef& tab = kTabs[current_tab_];
    const float btn = Px(kSubTabSize), gap = Px(kSubTabGap);
    const float total = tab.sub_tab_count * btn + (tab.sub_tab_count - 1) * gap;
    ImGui::SetCursorScreenPos(ImVec2(IM_ROUND(pos.x + (width - total) * 0.5f), IM_ROUND(pos.y + Px(kSubTabCenterY) - btn * 0.5f)));
    for (int i = 0; i < tab.sub_tab_count; ++i) {
        if (i > 0)
            ImGui::SameLine(0.0f, gap);
        ImGui::PushID(1000 + i);
        if (ui::IconTab("##sub", tab.sub_tabs[i].icon, i == sub_tab_[current_tab_]) && i != sub_tab_[current_tab_])
            SelectTab(current_tab_, i);
        ui::Tooltip(tab.sub_tabs[i].name);
        ImGui::PopID();
    }
}

void AdminPanel::SelectTab(int tab, int sub_tab) {
    if (tab < 0 || tab >= kTabCount)
        return;
    if (tab == current_tab_ && (sub_tab < 0 || sub_tab == sub_tab_[tab]))
        return;
    current_tab_ = tab;
    if (sub_tab >= 0 && sub_tab < ImMax(1, kTabs[tab].sub_tab_count))
        sub_tab_[tab] = sub_tab;
    tab_changed_at_ = ImGui::GetTime();
}

void AdminPanel::RenderMainWindow() {
    const Palette& c = theme::Colors();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float min_w = MinWindowWidth();

    ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(ImMax(Px(800), min_w), Px(540)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(min_w, Px(440)), ImVec2(FLT_MAX, FLT_MAX));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse;
    if (!open_)
        flags |= ImGuiWindowFlags_NoInputs;  // fading out

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, open_anim_);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Px(6));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme::WithAlpha(c.window_bg, settings_.menu_opacity / 100.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, c.window_border);
    const bool visible = ImGui::Begin(kMainWindowName, nullptr, flags);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    if (visible) {
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        RenderHeader(pos);

        const TabDef& tab = kTabs[current_tab_];
        float content_top = Px(kContentTop);
        if (tab.sub_tab_count > 0) {
            RenderSubTabs(pos, size.x);
            content_top = Px(kSubContentTop);
        }
        const ImVec2 content_size(size.x - Px(kMargin) * 2.0f, size.y - content_top - Px(kMargin));

        // Small fade + slide when switching tabs.
        float t = 1.0f;
        if (settings_.animations && tab_changed_at_ >= 0.0)
            t = ImSaturate(static_cast<float>(ImGui::GetTime() - tab_changed_at_) / 0.22f);
        const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

        ImGui::SetCursorScreenPos(ImVec2(pos.x + Px(kMargin), pos.y + content_top + (1.0f - ease) * Px(8)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ease);
        ImGui::BeginGroup();
        ImGui::PushID(current_tab_ * 100 + sub_tab_[current_tab_]);
        switch (tab.page) {
        case Page::Players:     RenderPlayersPage(content_size); break;
        case Page::Visuals:     RenderVisualsPage(content_size); break;
        case Page::Misc:        RenderMiscPage(content_size); break;
        case Page::Config:      RenderConfigPage(content_size); break;
        case Page::Maintenance: RenderMaintenancePage(content_size, tab.label); break;
        }
        ImGui::PopID();
        ImGui::EndGroup();
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleVar();  // Alpha
}

void AdminPanel::RefreshConfigs() {
    const std::string previous =
        selected_config_ >= 0 && selected_config_ < static_cast<int>(configs_.size()) ? configs_[selected_config_] : loaded_config_;
    configs_ = ListConfigs(config_dir_);
    selected_config_ = -1;
    for (int i = 0; i < static_cast<int>(configs_.size()); ++i)
        if (configs_[i] == previous)
            selected_config_ = i;
}

void AdminPanel::SetConfigStatus(bool error, const std::string& text) {
    config_status_ = text;
    config_status_error_ = error;
    config_status_time_ = ImGui::GetTime();
}

} // namespace foxy
