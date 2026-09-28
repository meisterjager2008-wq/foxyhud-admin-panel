#pragma once

#include <string>
#include <vector>

#include "imgui.h"

namespace foxy {

// The value of every control in the menu. Saved to / loaded from the config
// files in the Config tab. Keybinds are ImGuiKey values.
struct PanelSettings {
    // Config > Settings
    int   menu_key              = ImGuiKey_Insert;
    bool  accent_override       = false;  // use `accent` instead of the default blue
    float accent[3]             = {0.0f, 0.361f, 0.816f};
    float menu_opacity          = 97.0f;  // percent
    bool  animations            = true;
    bool  tooltips              = true;
    bool  block_input           = true;   // see AdminPanel::WantsGameInputBlocked()
    bool  confirm_bans          = true;
    bool  confirm_kicks         = false;
    bool  require_ban_note      = false;
    bool  mask_ips              = true;

    // Misc > Server
    bool  lock_server           = false;
    bool  whitelist_only        = false;
    int   max_players           = 32;
    bool  afk_kick              = true;
    int   afk_minutes           = 15;
    bool  announce_joins        = true;
    bool  announce_bans         = true;

    // Misc > Chat
    bool  chat_filter           = true;
    int   filter_action         = 0;      // index into kFilterActions (page_misc.cpp)
    bool  slow_mode             = false;
    float slow_mode_seconds     = 3.0f;
    bool  mute_all              = false;
    bool  broadcast_banner      = false;
    bool  log_events[5]         = {true, true, true, true, true};  // joins, leaves, chat, admin actions, reports

    // Misc > Admin
    bool  admin_tag             = true;
    bool  noclip                = false;
    int   noclip_key            = ImGuiKey_V;
    bool  invisible             = false;
    int   invisible_key         = ImGuiKey_None;
    bool  god_mode              = false;
    int   god_mode_key          = ImGuiKey_None;
    bool  overhead_ids          = true;

    // Misc > Windows
    bool  watermark             = true;   // top-right bar: brand | FPS | CPU | GPU | RAM
    bool  win_admin_log         = false;
    bool  win_chat_log          = true;
    bool  win_keybinds          = true;
    bool  win_player_counter    = true;
    bool  counter_info[4]       = {true, true, true, false};  // players, admins, reports, avg ping
    bool  report_alerts         = true;
    int   alert_sound           = 1;
    float alert_volume          = 0.5f;

    // Players
    bool  players_flagged_only  = false;
    bool  players_show_admins   = true;
    int   players_sort          = 0;
    bool  players_columns[4]    = {true, true, true, false};  // name, id, ping, playtime
    bool  act_freeze            = false;
    bool  act_spectate          = false;
    bool  act_mute_chat         = false;
    bool  act_mute_voice        = false;
    bool  quick_freeze          = false;
    int   quick_freeze_key      = ImGuiKey_None;
    bool  quick_spectate        = false;
    int   quick_spectate_key    = ImGuiKey_None;
    int   ban_reason            = 0;
    int   ban_duration          = 3;
    bool  ban_ip                = false;

    // Visuals > Players
    bool  name_tags             = true;
    float name_tag_color[4]     = {1.0f, 1.0f, 1.0f, 1.0f};
    bool  name_tag_info[4]      = {true, true, false, false};  // name, id, ping, distance
    bool  highlight_reported    = true;
    float reported_color[4]     = {0.87f, 0.25f, 0.27f, 1.0f};
    float flagged_color[4]      = {0.93f, 0.55f, 0.20f, 1.0f};
    int   highlight_style       = 0;      // outline, box, glow
    bool  highlight_frozen      = true;
    float frozen_color[4]       = {0.25f, 0.75f, 0.85f, 1.0f};
    bool  highlight_admins      = false;
    float admin_color[4]        = {0.18f, 0.45f, 0.85f, 1.0f};
    int   overlay_distance      = 250;    // meters

    // Visuals > Players > Map
    bool  map_blips             = true;
    float blip_color[4]         = {0.90f, 0.90f, 0.92f, 1.0f};
    bool  report_markers        = true;
    float marker_color[4]       = {0.87f, 0.25f, 0.27f, 1.0f};

    // Visuals > World
    bool  world_ambient         = false;
    float ambient_color[4]      = {0.55f, 0.35f, 0.95f, 1.0f};
    bool  world_color_on        = true;
    float world_color[4]        = {0.45f, 0.45f, 0.48f, 1.0f};
    bool  sky_color_on          = false;
    float sky_color[4]          = {0.50f, 0.52f, 0.80f, 1.0f};
    bool  fog_on                = false;
    float fog_color[4]          = {0.70f, 0.72f, 0.78f, 1.0f};
    float fog_density           = 0.25f;
    bool  safe_zones            = true;
    float safe_zone_color[4]    = {0.30f, 0.75f, 0.45f, 0.60f};
    bool  build_areas           = false;
    float build_area_color[4]   = {0.95f, 0.85f, 0.25f, 0.60f};
    bool  spawn_protection      = true;
    float spawn_color[4]        = {0.25f, 0.75f, 0.85f, 0.60f};
    int   zone_style            = 0;      // outline, filled, both
    float zone_opacity          = 0.60f;

    // Visuals > Chat
    float chat_admin_color[4]   = {0.34f, 0.60f, 0.98f, 1.0f};
    float chat_announce_color[4]= {0.95f, 0.78f, 0.30f, 1.0f};
    float chat_pm_color[4]      = {0.72f, 0.55f, 0.98f, 1.0f};
    float chat_system_color[4]  = {0.93f, 0.62f, 0.25f, 1.0f};
    float chat_filtered_color[4]= {0.87f, 0.25f, 0.27f, 1.0f};

    bool  chat_timestamps       = true;
    bool  chat_show_filtered    = false;
    bool  chat_fade             = true;
    int   chat_position         = 0;      // bottom left, top left, bottom right
    float chat_scale            = 1.0f;

    // Visuals > Chat > Alerts
    bool  report_toasts         = true;
    float toast_seconds         = 6.0f;
    bool  report_flash          = false;
    float flash_color[4]        = {0.87f, 0.25f, 0.27f, 0.35f};
};

bool SaveSettings(const PanelSettings& s, const std::string& path, std::string* error = nullptr);
bool LoadSettings(PanelSettings& s, const std::string& path, std::string* error = nullptr);  // also clamps

// Clamps every value into the range the UI allows (config files can be edited by hand).
void ClampSettings(PanelSettings& s);

// Config files (*.cfg) inside `dir`, names without extension, sorted.
std::vector<std::string> ListConfigs(const std::string& dir);

// Keeps letters, digits, space, '-' and '_' so a name can't escape the config dir.
std::string SanitizeConfigName(const std::string& name);

// Opens `dir` (created if missing) in the system file manager.
bool OpenFolder(const std::string& dir);

} // namespace foxy
