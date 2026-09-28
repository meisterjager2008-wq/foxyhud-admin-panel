#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"

namespace foxy {

using ui::Px;

namespace {
const char* const kFilterActions[] = {"Censor message", "Warn player", "Mute for 5 minutes", "Kick player"};
const char* const kLogEvents[]     = {"Joins", "Leaves", "Chat", "Admin actions", "Reports"};
const char* const kCounterInfo[]   = {"Players", "Admins", "Reports", "Avg. ping"};
const char* const kAlertSounds[]   = {"None", "Ping", "Chime", "Alarm"};
} // namespace

void AdminPanel::RenderMiscPage(const ImVec2& size) {
    PanelSettings& s = settings_;
    bool changed = false;

    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);

    if (ui::BeginPanel("##misc_left", ImVec2(left_w, size.y))) {
        ui::Section("Server");
        changed |= ui::Checkbox("Lock server", &s.lock_server);
        ui::Tooltip("Nobody new can join until you unlock the server");
        changed |= ui::Checkbox("Whitelist only", &s.whitelist_only);
        changed |= ui::SliderInt("Max players", &s.max_players, 2, 128, "%d players");
        changed |= ui::Checkbox("Kick AFK players", &s.afk_kick);
        if (s.afk_kick)
            changed |= ui::SliderInt("AFK timeout", &s.afk_minutes, 1, 60, "%d min");
        changed |= ui::Checkbox("Announce joins & leaves", &s.announce_joins);
        changed |= ui::Checkbox("Announce bans", &s.announce_bans);

        ui::Section("Chat");
        changed |= ui::Checkbox("Chat filter", &s.chat_filter);
        if (s.chat_filter)
            changed |= ui::Combo("##filter_action", &s.filter_action, kFilterActions, IM_ARRAYSIZE(kFilterActions));
        changed |= ui::Checkbox("Slow mode", &s.slow_mode);
        if (s.slow_mode)
            changed |= ui::SliderFloat("Message cooldown", &s.slow_mode_seconds, 1.0f, 30.0f, "%.0f sec");
        if (ui::Checkbox("Mute all players", &s.mute_all)) {
            changed = true;
            Log(s.mute_all ? AdminLogEntry::Kind::Warning : AdminLogEntry::Kind::Info, "%s global chat",
                s.mute_all ? "Muted" : "Unmuted");
        }
        changed |= ui::MultiCombo("Log events", s.log_events, kLogEvents, IM_ARRAYSIZE(kLogEvents));

        ui::Section("Broadcast");
        const float send_w = Px(64);
        bool send = ui::InputText("##broadcast", "Message to everyone...", broadcast_, sizeof(broadcast_),
                                  ui::ItemWidth() - send_w - ImGui::GetStyle().ItemSpacing.x, Icon::Chat,
                                  ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        send |= ui::Button("Send", ImVec2(send_w, ImGui::GetItemRectSize().y), ui::ButtonStyle::Accent);
        ui::Checkbox("Show as banner", &broadcast_banner_);
        if (send && broadcast_[0]) {
            backend_.Broadcast(broadcast_, broadcast_banner_);
            Log(AdminLogEntry::Kind::Success, "Broadcast: %s", broadcast_);
            broadcast_[0] = '\0';
        }
    }
    ui::EndPanel();

    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##misc_right", ImVec2(size.x - left_w - gap, size.y))) {
        ui::Section("Admin");
        changed |= ui::Checkbox("Admin tag", &s.admin_tag);
        ui::Tooltip("Shows your [ADMIN] tag in chat and above your character");
        changed |= ui::CheckboxKeybind("Noclip", &s.noclip, &s.noclip_key);
        changed |= ui::CheckboxKeybind("Invisible", &s.invisible, &s.invisible_key);
        changed |= ui::CheckboxKeybind("God mode", &s.god_mode, &s.god_mode_key);
        changed |= ui::Checkbox("Player IDs above heads", &s.overhead_ids);

        ui::Section("Windows");
        changed |= ui::Checkbox("Admin log", &s.win_admin_log);
        changed |= ui::Checkbox("Chat log", &s.win_chat_log);
        changed |= ui::Checkbox("Keybind list", &s.win_keybinds);
        changed |= ui::Checkbox("Player counter", &s.win_player_counter);
        if (s.win_player_counter)
            changed |= ui::MultiCombo("##counter_info", s.counter_info, kCounterInfo, IM_ARRAYSIZE(kCounterInfo));
        changed |= ui::Checkbox("Report alerts", &s.report_alerts);
        if (s.report_alerts) {
            changed |= ui::Combo("##alert_sound", &s.alert_sound, kAlertSounds, IM_ARRAYSIZE(kAlertSounds));
            changed |= ui::SliderFloat("Volume", &s.alert_volume, 0.0f, 1.0f, "%.2f");
        }
    }
    ui::EndPanel();

    if (changed)
        settings_dirty_ = true;
}

} // namespace foxy
