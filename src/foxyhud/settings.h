#pragma once

#include <string>
#include <vector>

#include "imgui.h"

namespace foxy {

// Everything the Misc and Config tabs edit. Saved to / loaded from the config
// files in the Config tab. Keybinds are ImGuiKey values.
struct PanelSettings {
    // Config > Menu
    int   menu_key           = ImGuiKey_Insert;
    float accent[3]          = {0.0f, 0.361f, 0.816f};
    float menu_opacity       = 97.0f;  // percent
    bool  animations         = true;
    bool  tooltips           = true;

    // Config > Safety
    bool  confirm_bans       = true;
    bool  confirm_kicks      = false;
    bool  require_ban_note   = false;
    bool  mask_ips           = true;

    // Misc > Server
    bool  lock_server        = false;
    bool  whitelist_only     = false;
    int   max_players        = 32;
    bool  afk_kick           = true;
    int   afk_minutes        = 15;
    bool  announce_joins     = true;
    bool  announce_bans      = true;

    // Misc > Chat
    bool  chat_filter        = true;
    int   filter_action      = 0;      // index into kFilterActions (page_misc.cpp)
    bool  slow_mode          = false;
    float slow_mode_seconds  = 3.0f;
    bool  mute_all           = false;
    bool  log_events[5]      = {true, true, true, true, true};  // joins, leaves, chat, admin actions, reports

    // Misc > Admin
    bool  admin_tag          = true;
    bool  noclip             = false;
    int   noclip_key         = ImGuiKey_V;
    bool  invisible          = false;
    int   invisible_key      = ImGuiKey_None;
    bool  god_mode           = false;
    int   god_mode_key       = ImGuiKey_None;
    bool  overhead_ids       = true;

    // Misc > Windows
    bool  win_admin_log      = false;
    bool  win_chat_log       = true;
    bool  win_keybinds       = true;
    bool  win_player_counter = true;
    bool  counter_info[4]    = {true, true, true, false};  // players, admins, reports, avg ping
    bool  report_alerts      = true;
    int   alert_sound        = 1;
    float alert_volume       = 0.5f;
};

bool SaveSettings(const PanelSettings& s, const std::string& path, std::string* error = nullptr);
bool LoadSettings(PanelSettings& s, const std::string& path, std::string* error = nullptr);  // also clamps

// Clamps every value into the range the UI allows (config files can be edited by hand).
void ClampSettings(PanelSettings& s);

// Config files (*.cfg) inside `dir`, names without extension, sorted.
std::vector<std::string> ListConfigs(const std::string& dir);

// Keeps letters, digits, space, '-' and '_' so a name can't escape the config dir.
std::string SanitizeConfigName(const std::string& name);

} // namespace foxy
