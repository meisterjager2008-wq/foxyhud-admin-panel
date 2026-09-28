#include <algorithm>
#include <cstdio>
#include <string>

#include "foxyhud/admin_panel.h"
#include "foxyhud/panel_util.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

// Small always-on-top windows toggled from Misc > Windows. They are click-through
// while the main panel is closed and can be moved/resized while it is open.

namespace foxy {

using theme::Col;
using ui::Px;

namespace {

bool BeginToolWindow(const char* name, const char* title, const ImVec2& default_pos, const ImVec2& default_size,
                     bool interactive, bool auto_resize, float opacity) {
    ImGui::SetNextWindowPos(default_pos, ImGuiCond_FirstUseEver);
    if (!auto_resize)
        ImGui::SetNextWindowSize(default_size, ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse;
    if (auto_resize)
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if (!interactive)
        flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    const Palette& c = theme::Colors();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(10), Px(9)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Px(5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(Px(120), Px(30)));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme::WithAlpha(c.window_bg, opacity));
    ImGui::PushStyleColor(ImGuiCol_Border, c.window_border);
    const bool visible = ImGui::Begin(name, nullptr, flags);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);

    if (visible && title)
        ui::Section(title);
    return visible;
}

// Scrolling region that follows new lines unless the user scrolled up.
void BeginLogRegion(const char* id) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Px(4));
    ImGui::BeginChild(id, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Px(6), Px(4)));
}

void EndLogRegion(bool new_lines) {
    ImGui::PopStyleVar();
    if (new_lines && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - Px(4))
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

} // namespace

void AdminPanel::RenderOverlayWindows() {
    const bool interactive = open_;
    if (settings_.win_player_counter)
        RenderPlayerCounter(interactive);
    if (settings_.win_keybinds)
        RenderKeybindList(interactive);
    if (settings_.win_admin_log)
        RenderAdminLogWindow(interactive);
    if (settings_.win_chat_log)
        RenderChatLogWindow(interactive);
    RenderToasts();
}

void AdminPanel::RenderPlayerCounter(bool interactive) {
    const std::vector<PlayerInfo>& players = backend_.GetPlayers();
    const int reports = static_cast<int>(backend_.GetReports().size());
    int admins = 0, ping_sum = 0;
    for (const PlayerInfo& p : players) {
        admins += p.is_admin ? 1 : 0;
        ping_sum += p.ping_ms;
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos(vp->WorkPos.x + vp->WorkSize.x * 0.5f - Px(150), vp->WorkPos.y + Px(12));
    if (!BeginToolWindow("##foxy_counter", nullptr, pos, ImVec2(0, 0), interactive, true,
                         settings_.menu_opacity / 100.0f)) {
        ImGui::End();
        return;
    }

    const Palette& c = theme::Colors();
    char buf[64];
    bool first = true;
    auto item = [&](const ImVec4& col, const char* text) {
        if (!first) {
            ImGui::SameLine(0.0f, Px(8));
            ui::TextColored(c.text_faint, "\xE2\x80\xA2");
            ImGui::SameLine(0.0f, Px(8));
        }
        first = false;
        ui::TextColored(col, "%s", text);
    };

    // Live dot
    {
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        const float line_h = ImGui::GetTextLineHeight();
        const ImVec2 p = w->DC.CursorPos;
        const float pulse = 0.65f + 0.35f * ImSin(static_cast<float>(ImGui::GetTime()) * 3.0f);
        w->DrawList->AddCircleFilled(ImVec2(p.x + Px(4), p.y + line_h * 0.5f), Px(3.5f), Col(c.success, pulse));
        ImGui::ItemSize(ImVec2(Px(8), line_h));
        ImGui::SameLine(0.0f, Px(8));
    }

    if (settings_.counter_info[0]) {
        std::snprintf(buf, sizeof(buf), "%d/%d players", static_cast<int>(players.size()), settings_.max_players);
        item(c.text_bright, buf);
    }
    if (settings_.counter_info[1]) {
        std::snprintf(buf, sizeof(buf), "%d admin%s", admins, admins == 1 ? "" : "s");
        item(c.accent_text, buf);
    }
    if (settings_.counter_info[2]) {
        std::snprintf(buf, sizeof(buf), "%d report%s", reports, reports == 1 ? "" : "s");
        item(reports > 0 ? c.danger : c.text, buf);
    }
    if (settings_.counter_info[3]) {
        const int avg = players.empty() ? 0 : ping_sum / static_cast<int>(players.size());
        std::snprintf(buf, sizeof(buf), "%d ms avg", avg);
        item(detail::PingColor(avg), buf);
    }
    if (first)
        ui::TextDim("Player counter");
    ImGui::End();
}

void AdminPanel::RenderKeybindList(bool interactive) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos(vp->WorkPos.x + vp->WorkSize.x - Px(226), vp->WorkPos.y + Px(160));
    if (!BeginToolWindow("##foxy_keybinds", "Keybinds", pos, ImVec2(0, 0), interactive, true,
                         settings_.menu_opacity / 100.0f)) {
        ImGui::End();
        return;
    }

    const Palette& c = theme::Colors();
    const float width = Px(190);
    auto row = [&](const char* name, int key, bool active) {
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        const ImVec2 p = w->DC.CursorPos;
        const float line_h = ImGui::GetTextLineHeight();
        ImGui::ItemSize(ImVec2(width, line_h));
        const char* key_name = ui::KeyName(key);
        char right[48];
        std::snprintf(right, sizeof(right), "[%s]", key_name);
        const ImVec2 rs = ImGui::CalcTextSize(right);
        w->DrawList->AddText(ui::Snap(p), Col(active ? c.accent_text : c.text), name);
        w->DrawList->AddText(ui::Snap(ImVec2(p.x + width - rs.x, p.y)), Col(active ? c.accent_text : c.text_dim), right);
    };

    row("Admin menu", settings_.menu_key, open_);
    if (settings_.noclip_key != ImGuiKey_None)
        row("Noclip", settings_.noclip_key, settings_.noclip);
    if (settings_.invisible_key != ImGuiKey_None)
        row("Invisible", settings_.invisible_key, settings_.invisible);
    if (settings_.god_mode_key != ImGuiKey_None)
        row("God mode", settings_.god_mode_key, settings_.god_mode);
    ImGui::End();
}

void AdminPanel::RenderAdminLogWindow(bool interactive) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos(vp->WorkPos.x + Px(16) + Px(400), vp->WorkPos.y + vp->WorkSize.y - Px(236));
    if (!BeginToolWindow("##foxy_admin_log", "Admin log", pos, ImVec2(Px(380), Px(220)), interactive, false,
                         settings_.menu_opacity / 100.0f)) {
        ImGui::End();
        return;
    }

    const Palette& c = theme::Colors();
    BeginLogRegion("##lines");
    ImGui::PushTextWrapPos(0.0f);
    for (const AdminLogEntry& e : log_) {
        ImVec4 col = c.text;
        switch (e.kind) {
        case AdminLogEntry::Kind::Info:    col = c.text; break;
        case AdminLogEntry::Kind::Success: col = c.success; break;
        case AdminLogEntry::Kind::Warning: col = c.warning; break;
        case AdminLogEntry::Kind::Danger:  col = c.danger; break;
        }
        ui::TextColored(c.text_faint, "%s", e.time.c_str());
        ImGui::SameLine();
        ui::TextColored(col, "%s", e.text.c_str());
    }
    ImGui::PopTextWrapPos();
    const bool new_lines = log_.size() != log_seen_;
    log_seen_ = log_.size();
    EndLogRegion(new_lines);
    ImGui::End();
}

void AdminPanel::RenderChatLogWindow(bool interactive) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pos(vp->WorkPos.x + Px(16), vp->WorkPos.y + vp->WorkSize.y - Px(236));
    if (!BeginToolWindow("##foxy_chat_log", "Chat log", pos, ImVec2(Px(390), Px(220)), interactive, false,
                         settings_.menu_opacity / 100.0f)) {
        ImGui::End();
        return;
    }

    const Palette& c = theme::Colors();
    const PanelSettings& s = settings_;
    auto color = [](const float* f) { return ImVec4(f[0], f[1], f[2], f[3]); };
    const std::vector<ChatMessage>& chat = backend_.GetChatLog();
    BeginLogRegion("##lines");
    ImGui::PushTextWrapPos(0.0f);
    for (size_t i = 0; i < chat.size(); ++i) {
        const ChatMessage& m = chat[i];
        ImGui::PushID(static_cast<int>(i));
        ui::TextColored(c.text_faint, "%s", detail::ClockTime(m.timestamp).substr(0, 5).c_str());
        ImGui::SameLine();
        if (m.kind != ChatKind::Player) {
            const float* col = s.chat_system_color;
            if (m.kind == ChatKind::Admin)
                col = s.chat_admin_color;
            else if (m.kind == ChatKind::Announcement)
                col = s.chat_announce_color;
            else if (m.kind == ChatKind::PrivateMessage)
                col = s.chat_pm_color;
            ui::TextColored(color(col), "%s", m.text.c_str());
        } else {
            ui::TextColored(c.accent_text, "%s:", m.player_name.c_str());
            if (interactive && ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                const ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddLine(ImVec2(mn.x, mx.y), mx, Col(c.accent_text), 1.0f);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    selected_player_ = m.player_id;
                    SelectTab(0, 0);
                }
            }
            ImGui::SameLine();
            ui::TextColored(m.flagged ? color(s.chat_filtered_color) : c.text, "%s", m.text.c_str());
        }
        ImGui::PopID();
    }
    ImGui::PopTextWrapPos();
    const bool new_lines = chat.size() != chat_seen_;
    chat_seen_ = chat.size();
    EndLogRegion(new_lines);
    ImGui::End();
}

// Corner pop-ups for new reports (Misc > Overlays > Report toasts).
void AdminPanel::RenderToasts() {
    const std::vector<PlayerReport>& reports = backend_.GetReports();
    uint64_t newest = last_report_id_;
    for (const PlayerReport& r : reports) {
        if (reports_primed_ && r.id > last_report_id_ && settings_.report_toasts)
            toasts_.push_back({"New report", r.target_name + " reported by " + r.reporter_name + " (" + r.reason + ")",
                               ImGui::GetTime()});
        newest = ImMax(newest, r.id);
    }
    last_report_id_ = newest;
    reports_primed_ = true;  // reports that existed before the panel started don't pop up

    const double now = ImGui::GetTime();
    const float life = settings_.toast_seconds;
    toasts_.erase(std::remove_if(toasts_.begin(), toasts_.end(),
                                 [&](const Toast& t) { return now - t.created_at > life; }),
                  toasts_.end());
    if (toasts_.empty())
        return;

    const Palette& c = theme::Colors();
    const Fonts& f = theme::GetFonts();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float w = Px(290), pad = Px(12), stripe = Px(3);
    const float right = vp->WorkPos.x + vp->WorkSize.x - Px(16);
    float y = vp->WorkPos.y + Px(16);

    for (auto it = toasts_.rbegin(); it != toasts_.rend(); ++it) {
        const float age = static_cast<float>(now - it->created_at);
        const float in = settings_.animations ? ImSaturate(age / 0.2f) : 1.0f;
        const float out = settings_.animations ? ImSaturate((life - age) / 0.4f) : 1.0f;
        const float a = in * out;
        const float slide = (1.0f - in) * Px(24);

        const float title_h = theme::FontSize(f.bold);
        const float body_fs = theme::FontSize(f.body);
        const float wrap = w - pad * 2.0f - Px(22);
        const ImVec2 body = f.body->CalcTextSizeA(body_fs, FLT_MAX, wrap, it->text.c_str());
        const float h = pad + title_h + Px(4) + body.y + pad;
        const ImVec2 mn(right - w + slide, y), mx(right + slide, y + h);

        dl->AddRectFilled(mn, mx, Col(c.window_bg, 0.97f * a), Px(5));
        dl->AddRect(mn, mx, Col(c.window_border, a), Px(5));
        dl->AddRectFilled(ImVec2(mn.x, mn.y + Px(6)), ImVec2(mn.x + stripe, mx.y - Px(6)), Col(c.danger, a), Px(2));
        DrawIcon(dl, Icon::Flag, ImVec2(mn.x + pad + Px(7), mn.y + pad + title_h * 0.5f), Px(13), Col(c.danger, a));
        const float tx = mn.x + pad + Px(22);
        dl->AddText(f.bold, title_h, ui::Snap(ImVec2(tx, mn.y + pad)), Col(c.text_bright, a), it->title.c_str());
        dl->AddText(f.body, body_fs, ui::Snap(ImVec2(tx, mn.y + pad + title_h + Px(4))), Col(c.text, a),
                    it->text.c_str(), nullptr, wrap);
        y += h + Px(8);
    }
}

} // namespace foxy
