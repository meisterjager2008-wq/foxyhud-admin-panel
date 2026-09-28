#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"

namespace foxy {

using ui::Px;

namespace {
const char* const kFilterActions[] = {"Censor Message", "Warn Player", "Mute For 5 Minutes", "Kick Player"};
const char* const kLogEvents[]     = {"Joins", "Leaves", "Chat", "Admin Actions", "Reports"};
const char* const kCounterInfo[]   = {"Players", "Admins", "Reports", "Avg. Ping"};
const char* const kAlertSounds[]   = {"None", "Ping", "Chime", "Alarm"};
} // namespace

// Design only: every control keeps its value in settings_, nothing is sent anywhere.
void AdminPanel::RenderMiscPage(const ImVec2& size) {
    PanelSettings& s = settings_;
    bool changed = false;

    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);

    if (ui::BeginPanel("##misc_left", ImVec2(left_w, size.y))) {
        ui::Section("Server");
        changed |= ui::Checkbox("Lock Server", &s.lock_server);
        changed |= ui::Checkbox("Whitelist Only", &s.whitelist_only);
        changed |= ui::SliderInt("Max Players", &s.max_players, 2, 128, "%d players");
        changed |= ui::Checkbox("Kick AFK Players", &s.afk_kick);
        if (s.afk_kick)
            changed |= ui::SliderInt("AFK Timeout", &s.afk_minutes, 1, 60, "%d min");
        changed |= ui::Checkbox("Announce Joins & Leaves", &s.announce_joins);
        changed |= ui::Checkbox("Announce Bans", &s.announce_bans);

        ui::Section("Chat");
        changed |= ui::Checkbox("Chat Filter", &s.chat_filter);
        if (s.chat_filter)
            changed |= ui::Combo("##filter_action", &s.filter_action, kFilterActions, IM_ARRAYSIZE(kFilterActions));
        changed |= ui::Checkbox("Slow Mode", &s.slow_mode);
        if (s.slow_mode)
            changed |= ui::SliderFloat("Message Cooldown", &s.slow_mode_seconds, 1.0f, 30.0f, "%.0f sec");
        changed |= ui::Checkbox("Mute All Players", &s.mute_all);
        changed |= ui::MultiCombo("Log Events", s.log_events, kLogEvents, IM_ARRAYSIZE(kLogEvents));

        ui::Section("Broadcast");
        const float send_w = Px(64);
        ui::InputText("##broadcast", "Message to everyone...", broadcast_, sizeof(broadcast_),
                      ui::ItemWidth() - send_w - ImGui::GetStyle().ItemSpacing.x, Icon::Chat);
        ImGui::SameLine();
        ui::Button("Send", ImVec2(send_w, ImGui::GetItemRectSize().y), ui::ButtonStyle::Accent);
        changed |= ui::Checkbox("Show As Banner", &s.broadcast_banner);
    }
    ui::EndPanel();

    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##misc_right", ImVec2(size.x - left_w - gap, size.y))) {
        ui::Section("Admin");
        changed |= ui::Checkbox("Admin Tag", &s.admin_tag);
        changed |= ui::CheckboxKeybind("Noclip", &s.noclip, &s.noclip_key);
        changed |= ui::CheckboxKeybind("Invisible", &s.invisible, &s.invisible_key);
        changed |= ui::CheckboxKeybind("God Mode", &s.god_mode, &s.god_mode_key);
        changed |= ui::Checkbox("Player IDs Above Heads", &s.overhead_ids);

        ui::Section("Windows");
        changed |= ui::Checkbox("Admin Log", &s.win_admin_log);
        changed |= ui::Checkbox("Chat Log", &s.win_chat_log);
        changed |= ui::Checkbox("Keybind List", &s.win_keybinds);
        changed |= ui::Checkbox("Player Counter", &s.win_player_counter);
        if (s.win_player_counter)
            changed |= ui::MultiCombo("##counter_info", s.counter_info, kCounterInfo, IM_ARRAYSIZE(kCounterInfo));
        changed |= ui::Checkbox("Report Alerts", &s.report_alerts);
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
