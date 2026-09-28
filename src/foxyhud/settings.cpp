#include "foxyhud/settings.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <sstream>
#include <thread>
#include <type_traits>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

namespace foxy {
namespace {

// Visits every persisted field as (key, reference). Works for const and
// non-const settings so the same table drives saving and loading.
template <typename S, typename F>
void VisitFields(S& s, F&& f) {
    f("menu_key", s.menu_key);
    f("accent_override", s.accent_override);
    f("accent", s.accent);
    f("menu_opacity", s.menu_opacity);
    f("animations", s.animations);
    f("tooltips", s.tooltips);
    f("block_input", s.block_input);

    f("confirm_bans", s.confirm_bans);
    f("confirm_kicks", s.confirm_kicks);
    f("require_ban_note", s.require_ban_note);
    f("mask_ips", s.mask_ips);

    f("lock_server", s.lock_server);
    f("whitelist_only", s.whitelist_only);
    f("max_players", s.max_players);
    f("afk_kick", s.afk_kick);
    f("afk_minutes", s.afk_minutes);
    f("announce_joins", s.announce_joins);
    f("announce_bans", s.announce_bans);

    f("chat_filter", s.chat_filter);
    f("filter_action", s.filter_action);
    f("slow_mode", s.slow_mode);
    f("slow_mode_seconds", s.slow_mode_seconds);
    f("mute_all", s.mute_all);
    f("broadcast_banner", s.broadcast_banner);
    f("log_events", s.log_events);

    f("admin_tag", s.admin_tag);
    f("noclip", s.noclip);
    f("noclip_key", s.noclip_key);
    f("invisible", s.invisible);
    f("invisible_key", s.invisible_key);
    f("god_mode", s.god_mode);
    f("god_mode_key", s.god_mode_key);
    f("overhead_ids", s.overhead_ids);

    f("win_admin_log", s.win_admin_log);
    f("win_chat_log", s.win_chat_log);
    f("win_keybinds", s.win_keybinds);
    f("win_player_counter", s.win_player_counter);
    f("counter_info", s.counter_info);
    f("report_alerts", s.report_alerts);
    f("alert_sound", s.alert_sound);
    f("alert_volume", s.alert_volume);

    f("players_flagged_only", s.players_flagged_only);
    f("players_show_admins", s.players_show_admins);
    f("players_sort", s.players_sort);
    f("players_columns", s.players_columns);
    f("act_freeze", s.act_freeze);
    f("act_spectate", s.act_spectate);
    f("act_mute_chat", s.act_mute_chat);
    f("act_mute_voice", s.act_mute_voice);
    f("quick_freeze", s.quick_freeze);
    f("quick_freeze_key", s.quick_freeze_key);
    f("quick_spectate", s.quick_spectate);
    f("quick_spectate_key", s.quick_spectate_key);
    f("ban_reason", s.ban_reason);
    f("ban_duration", s.ban_duration);
    f("ban_ip", s.ban_ip);

    f("name_tags", s.name_tags);
    f("name_tag_color", s.name_tag_color);
    f("name_tag_info", s.name_tag_info);
    f("highlight_reported", s.highlight_reported);
    f("reported_color", s.reported_color);
    f("flagged_color", s.flagged_color);
    f("highlight_style", s.highlight_style);
    f("highlight_frozen", s.highlight_frozen);
    f("frozen_color", s.frozen_color);
    f("highlight_admins", s.highlight_admins);
    f("admin_color", s.admin_color);
    f("overlay_distance", s.overlay_distance);
    f("map_blips", s.map_blips);
    f("blip_color", s.blip_color);
    f("report_markers", s.report_markers);
    f("marker_color", s.marker_color);
    f("world_ambient", s.world_ambient);
    f("ambient_color", s.ambient_color);
    f("world_color_on", s.world_color_on);
    f("world_color", s.world_color);
    f("sky_color_on", s.sky_color_on);
    f("sky_color", s.sky_color);
    f("fog_on", s.fog_on);
    f("fog_color", s.fog_color);
    f("fog_density", s.fog_density);
    f("safe_zones", s.safe_zones);
    f("safe_zone_color", s.safe_zone_color);
    f("build_areas", s.build_areas);
    f("build_area_color", s.build_area_color);
    f("spawn_protection", s.spawn_protection);
    f("spawn_color", s.spawn_color);
    f("zone_style", s.zone_style);
    f("zone_opacity", s.zone_opacity);
    f("chat_timestamps", s.chat_timestamps);
    f("chat_show_filtered", s.chat_show_filtered);
    f("chat_fade", s.chat_fade);
    f("chat_position", s.chat_position);
    f("chat_scale", s.chat_scale);
    f("chat_admin_color", s.chat_admin_color);
    f("chat_announce_color", s.chat_announce_color);
    f("chat_pm_color", s.chat_pm_color);
    f("chat_system_color", s.chat_system_color);
    f("chat_filtered_color", s.chat_filtered_color);
    f("report_toasts", s.report_toasts);
    f("toast_seconds", s.toast_seconds);
    f("report_flash", s.report_flash);
    f("flash_color", s.flash_color);
}

std::string ToString(bool v) { return v ? "1" : "0"; }
std::string ToString(int v) { return std::to_string(v); }
std::string ToString(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.4f", v);
    return buf;
}
template <typename T, size_t N>
std::string ToString(const T (&arr)[N]) {
    std::string out;
    for (size_t i = 0; i < N; ++i) {
        if (i)
            out += ',';
        out += ToString(arr[i]);
    }
    return out;
}

void FromString(const std::string& s, bool& v) { v = std::atoi(s.c_str()) != 0; }
void FromString(const std::string& s, int& v) { v = std::atoi(s.c_str()); }
void FromString(const std::string& s, float& v) { v = static_cast<float>(std::atof(s.c_str())); }
template <typename T, size_t N>
void FromString(const std::string& s, T (&arr)[N]) {
    std::stringstream ss(s);
    std::string item;
    for (size_t i = 0; i < N && std::getline(ss, item, ','); ++i)
        FromString(item, arr[i]);
}

std::string Trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

} // namespace

bool SaveSettings(const PanelSettings& s, const std::string& path, std::string* error) {
    std::error_code ec;
    const std::filesystem::path p(path);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path(), ec);

    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        if (error)
            *error = "Could not write " + path;
        return false;
    }
    out << "# FoxyHUD admin panel config\n";
    VisitFields(s, [&](const char* key, const auto& value) { out << key << '=' << ToString(value) << '\n'; });
    return static_cast<bool>(out);
}

bool LoadSettings(PanelSettings& s, const std::string& path, std::string* error) {
    std::ifstream in(path);
    if (!in) {
        if (error)
            *error = "Could not open " + path;
        return false;
    }
    std::map<std::string, std::string> values;
    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        values[Trim(line.substr(0, eq))] = Trim(line.substr(eq + 1));
    }
    VisitFields(s, [&](const char* key, auto& value) {
        const auto it = values.find(key);
        if (it != values.end())
            FromString(it->second, value);
    });
    ClampSettings(s);
    return true;
}

void ClampSettings(PanelSettings& s) {
    auto clamp = [](auto& v, auto lo, auto hi) { v = v < lo ? lo : (v > hi ? hi : v); };
    auto valid_key = [](int key) {
        return key == ImGuiKey_None || (key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END);
    };
    for (float* col : {s.accent, s.name_tag_color, s.reported_color, s.flagged_color, s.frozen_color, s.admin_color,
                       s.blip_color, s.marker_color, s.chat_admin_color, s.chat_announce_color, s.chat_pm_color,
                       s.chat_system_color, s.chat_filtered_color, s.flash_color, s.ambient_color, s.world_color,
                       s.sky_color, s.fog_color, s.safe_zone_color, s.build_area_color, s.spawn_color})
        for (int i = 0; i < (col == s.accent ? 3 : 4); ++i)
            clamp(col[i], 0.0f, 1.0f);
    clamp(s.highlight_style, 0, 2);
    clamp(s.players_sort, 0, 3);
    clamp(s.ban_reason, 0, 8);
    clamp(s.ban_duration, 0, 8);
    clamp(s.zone_style, 0, 2);
    clamp(s.chat_position, 0, 2);
    clamp(s.fog_density, 0.0f, 1.0f);
    clamp(s.zone_opacity, 0.0f, 1.0f);
    clamp(s.chat_scale, 0.75f, 1.5f);
    clamp(s.overlay_distance, 10, 1000);
    clamp(s.toast_seconds, 2.0f, 20.0f);
    clamp(s.menu_opacity, 60.0f, 100.0f);
    clamp(s.max_players, 2, 128);
    clamp(s.afk_minutes, 1, 60);
    clamp(s.filter_action, 0, 3);
    clamp(s.slow_mode_seconds, 1.0f, 30.0f);
    clamp(s.alert_sound, 0, 3);
    clamp(s.alert_volume, 0.0f, 1.0f);
    for (int* key : {&s.menu_key, &s.noclip_key, &s.invisible_key, &s.god_mode_key, &s.quick_freeze_key,
                     &s.quick_spectate_key})
        if (!valid_key(*key))
            *key = ImGuiKey_None;
    if (s.menu_key == ImGuiKey_None)
        s.menu_key = ImGuiKey_Insert;  // never lock the admin out of their own menu
}

std::vector<std::string> ListConfigs(const std::string& dir) {
    std::vector<std::string> names;
    std::error_code ec;
    for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".cfg")
            names.push_back(it->path().stem().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool OpenFolder(const std::string& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path abs = std::filesystem::absolute(dir, ec);
    if (ec)
        return false;
#if defined(_WIN32)
    const std::wstring wpath = abs.wstring();
    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#else
#if defined(__APPLE__)
    const char* tool = "open";
#else
    const char* tool = "xdg-open";
#endif
    // Spawned directly (no shell), and reaped on a detached thread so the frame never blocks.
    std::string path = abs.string();
    char* argv[] = {const_cast<char*>(tool), path.data(), nullptr};
    pid_t pid = 0;
    if (posix_spawnp(&pid, tool, nullptr, nullptr, argv, environ) != 0)
        return false;
    std::thread([pid] {
        int status = 0;
        waitpid(pid, &status, 0);
    }).detach();
    return true;
#endif
}

std::string SanitizeConfigName(const std::string& name) {
    std::string out;
    for (char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
                        c == '_' || c == ' ';
        if (ok)
            out += c;
    }
    return Trim(out);
}

} // namespace foxy
