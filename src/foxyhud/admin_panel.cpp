#include "foxyhud/admin_panel.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>

#include "foxyhud/panel_util.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using theme::Col;
using ui::Px;

// Header tabs, left to right. Tabs mapped to Page::Maintenance show the
// "Maintenance" placeholder until they get real content.
const AdminPanel::TabDef AdminPanel::kTabs[] = {
    {"Players",  Icon::User,    Page::Players},
    {"Creator",  Icon::Pencil,  Page::Maintenance},
    {"Visitors", Icon::Eye,     Page::Maintenance},
    {"Misc",     Icon::Grid,    Page::Misc},
    {"Helper",   Icon::Help,    Page::Maintenance},
    {"Players",  Icon::Users,   Page::Maintenance},
    {"Painting", Icon::Palette, Page::Maintenance},
    {"Config",   Icon::Gear,    Page::Config},
};
const int AdminPanel::kTabCount = IM_ARRAYSIZE(AdminPanel::kTabs);

namespace {
constexpr const char* kMainWindowName = "##foxyhud_admin_panel";
constexpr float kHeaderCenterY = 24.0f;  // design px from the window top
constexpr float kContentTop    = 50.0f;
constexpr float kMargin        = 12.0f;
constexpr float kTabHeight     = 26.0f;
constexpr float kTabGap        = 4.0f;
} // namespace

AdminPanel::AdminPanel(IAdminBackend& backend, std::string config_dir)
    : backend_(backend), config_dir_(std::move(config_dir)) {
    IM_ASSERT(theme::GetFonts().body && "Call foxy::theme::LoadFonts() before creating the AdminPanel");
    RefreshConfigs();
    settings_dirty_ = true;  // first frame pushes settings to the theme and the backend
    Log(AdminLogEntry::Kind::Info, "Admin panel ready");
}

void AdminPanel::SetBranding(std::string name, std::string suffix) {
    brand_name_ = std::move(name);
    brand_suffix_ = std::move(suffix);
}

void AdminPanel::ApplySettings() {
    theme::SetAccent(ImVec4(settings_.accent[0], settings_.accent[1], settings_.accent[2], 1.0f));
    ui::SetAnimationsEnabled(settings_.animations);
    ui::SetTooltipsEnabled(settings_.tooltips);
    backend_.OnSettingsChanged(settings_);
}

void AdminPanel::Log(AdminLogEntry::Kind kind, const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_.push_back({detail::ClockTime(static_cast<double>(std::time(nullptr))), buf, kind});
    if (log_.size() > 500) {
        log_.erase(log_.begin(), log_.begin() + 100);
        log_seen_ = 0;
    }
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

    RenderOverlayWindows();
    if (open_anim_ > 0.0f)
        RenderMainWindow();

    if (settings_dirty_) {
        settings_dirty_ = false;
        ApplySettings();
    }
}

void AdminPanel::HandleHotkeys() {
    if (ui::IsCapturingKeybind() || ImGui::GetIO().WantTextInput)
        return;

    auto pressed = [](int key) { return key != ImGuiKey_None && ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false); };
    if (pressed(settings_.menu_key))
        Toggle();

    struct Hotkey {
        bool*       value;
        int         key;
        const char* name;
    };
    const Hotkey hotkeys[] = {
        {&settings_.noclip, settings_.noclip_key, "Noclip"},
        {&settings_.invisible, settings_.invisible_key, "Invisible"},
        {&settings_.god_mode, settings_.god_mode_key, "God mode"},
    };
    for (const Hotkey& h : hotkeys) {
        if (!pressed(h.key))
            continue;
        *h.value = !*h.value;
        settings_dirty_ = true;
        Log(AdminLogEntry::Kind::Info, "%s %s", h.name, *h.value ? "enabled" : "disabled");
    }
}

// Closes dropdowns / the confirm modal that belong to the panel, so a hidden
// modal can't keep blocking input after the menu is toggled off.
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

void AdminPanel::RenderHeader(const ImVec2& pos, float /*width*/) {
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
        if (ui::Tab(kTabs[i].label, kTabs[i].icon, i == current_tab_) && i != current_tab_) {
            current_tab_ = i;
            tab_changed_at_ = ImGui::GetTime();
        }
        ImGui::PopID();
    }
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
        RenderHeader(pos, size.x);

        const ImVec2 content_size(size.x - Px(kMargin) * 2.0f, size.y - Px(kContentTop) - Px(kMargin));

        // Small fade + slide when switching tabs.
        float t = 1.0f;
        if (settings_.animations && tab_changed_at_ >= 0.0)
            t = ImSaturate(static_cast<float>(ImGui::GetTime() - tab_changed_at_) / 0.22f);
        const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

        ImGui::SetCursorScreenPos(ImVec2(pos.x + Px(kMargin), pos.y + Px(kContentTop) + (1.0f - ease) * Px(8)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ease);
        ImGui::BeginGroup();
        const TabDef& tab = kTabs[current_tab_];
        ImGui::PushID(current_tab_);
        switch (tab.page) {
        case Page::Players:     RenderPlayersPage(content_size); break;
        case Page::Misc:        RenderMiscPage(content_size); break;
        case Page::Config:      RenderConfigPage(content_size); break;
        case Page::Maintenance: RenderMaintenancePage(content_size, tab.label); break;
        }
        ImGui::PopID();
        ImGui::EndGroup();
        ImGui::PopStyleVar();

        if (open_confirm_) {
            ImGui::OpenPopup("##foxy_confirm");
            open_confirm_ = false;
        }
        RenderConfirmModal(ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f));
    }
    ImGui::End();
    ImGui::PopStyleVar();  // Alpha
}

void AdminPanel::RequestModeration(const ModerationRequest& req) {
    const bool confirm = (req.action == ModerationAction::Ban && settings_.confirm_bans) ||
                         (req.action == ModerationAction::Kick && settings_.confirm_kicks);
    if (confirm) {
        pending_ = req;
        open_confirm_ = true;
    } else {
        ExecuteModeration(req);
    }
}

void AdminPanel::ExecuteModeration(const ModerationRequest& req) {
    backend_.Moderate(req);
    switch (req.action) {
    case ModerationAction::Warn:
        Log(AdminLogEntry::Kind::Warning, "Warned %s (%s)", req.player_name.c_str(), req.reason.c_str());
        break;
    case ModerationAction::Kick:
        Log(AdminLogEntry::Kind::Warning, "Kicked %s (%s)", req.player_name.c_str(), req.reason.c_str());
        note_[0] = '\0';
        break;
    case ModerationAction::Ban:
        Log(AdminLogEntry::Kind::Danger, "Banned %s - %s (%s)%s", req.player_name.c_str(),
            detail::FormatDuration(req.duration_minutes).c_str(), req.reason.c_str(), req.ip_ban ? " + IP ban" : "");
        note_[0] = '\0';
        break;
    }
}

void AdminPanel::RenderConfirmModal(const ImVec2& center) {
    if (!ui::BeginModal("##foxy_confirm", center))
        return;

    const Palette& c = theme::Colors();
    const bool ban = pending_.action == ModerationAction::Ban;

    ImGui::Dummy(ImVec2(Px(320), 0.0f));
    ImGui::SetCursorPosY(ImGui::GetStyle().WindowPadding.y);
    theme::PushFont(theme::GetFonts().bold);
    ui::TextColored(ban ? c.danger : c.warning, "%s", ban ? "Ban player?" : "Kick player?");
    theme::PopFont();
    ui::TextDim("%s %s (#%s)", ban ? "You are about to ban" : "You are about to kick", pending_.player_name.c_str(),
                detail::IdString(pending_.player_id).c_str());
    ui::Spacing(4);

    ui::KeyValue("Reason", pending_.reason.c_str());
    if (ban) {
        ui::KeyValue("Duration", detail::FormatDuration(pending_.duration_minutes).c_str());
        ui::KeyValue("IP ban", pending_.ip_ban ? "Yes" : "No");
    }
    if (!pending_.note.empty()) {
        ui::TextDim("Note");
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + Px(320));
        ImGui::TextUnformatted(pending_.note.c_str());
        ImGui::PopTextWrapPos();
    }
    ui::Spacing(6);

    const float bw = ui::SplitWidth(2, ImGui::GetContentRegionAvail().x);
    if (ui::Button("Cancel", ImVec2(bw, Px(26))) || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        ImGui::CloseCurrentPopup();
    ImGui::SameLine();
    if (ui::Button(ban ? "Confirm ban" : "Confirm kick", ImVec2(bw, Px(26)),
                   ban ? ui::ButtonStyle::Danger : ui::ButtonStyle::Warning)) {
        ExecuteModeration(pending_);
        ImGui::CloseCurrentPopup();
    }
    ui::EndModal();
}

void AdminPanel::RefreshConfigs() {
    configs_ = ListConfigs(config_dir_);
    selected_config_ = -1;
    for (int i = 0; i < static_cast<int>(configs_.size()); ++i)
        if (configs_[i] == config_name_)
            selected_config_ = i;
}

} // namespace foxy
