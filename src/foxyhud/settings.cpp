#include "foxyhud/settings.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <sstream>
#include <type_traits>

namespace foxy {
namespace {

// Visits every persisted field as (key, reference). Works for const and
// non-const settings so the same table drives saving and loading.
template <typename S, typename F>
void VisitFields(S& s, F&& f) {
    f("menu_key", s.menu_key);
    f("accent", s.accent);
    f("menu_opacity", s.menu_opacity);
    f("animations", s.animations);
    f("tooltips", s.tooltips);

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
    for (float& c : s.accent)
        clamp(c, 0.0f, 1.0f);
    clamp(s.menu_opacity, 60.0f, 100.0f);
    clamp(s.max_players, 2, 128);
    clamp(s.afk_minutes, 1, 60);
    clamp(s.filter_action, 0, 3);
    clamp(s.slow_mode_seconds, 1.0f, 30.0f);
    clamp(s.alert_sound, 0, 3);
    clamp(s.alert_volume, 0.0f, 1.0f);
    for (int* key : {&s.menu_key, &s.noclip_key, &s.invisible_key, &s.god_mode_key})
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
