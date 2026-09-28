#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "imgui.h"
#include "foxyhud/backend.h"
#include "foxyhud/icons.h"
#include "foxyhud/settings.h"

namespace foxy {

struct AdminLogEntry {
    enum class Kind { Info, Success, Warning, Danger };
    std::string time;  // "HH:MM:SS"
    std::string text;
    Kind        kind = Kind::Info;
};

// The admin panel overlay. Create one after ImGui is initialised (and after
// theme::LoadFonts / theme::Apply), then call Render() every frame between
// ImGui::NewFrame() and ImGui::Render().
class AdminPanel {
public:
    explicit AdminPanel(IAdminBackend& backend, std::string config_dir = "configs");

    void Render();

    bool IsOpen() const { return open_; }
    void SetOpen(bool open) { open_ = open; }
    void Toggle() { open_ = !open_; }

    PanelSettings&       Settings() { return settings_; }
    const PanelSettings& Settings() const { return settings_; }
    void                 ApplySettings();  // push settings_ into theme/widgets + notify the backend

    // Brand shown at the top-left, e.g. "FoxyHUD" + ".admin".
    void SetBranding(std::string name, std::string suffix);

    void Log(AdminLogEntry::Kind kind, const char* fmt, ...) IM_FMTARGS(3);
    const std::vector<AdminLogEntry>& AdminLog() const { return log_; }

private:
    enum class Page { Players, Misc, Config, Maintenance };
    struct TabDef {
        const char* label;
        Icon        icon;
        Page        page;
    };

    // admin_panel.cpp
    void HandleHotkeys();
    void ClosePanelPopups();
    void RenderMainWindow();
    void RenderHeader(const ImVec2& win_pos, float win_width);
    float MinWindowWidth() const;
    void RequestModeration(const ModerationRequest& req);
    void ExecuteModeration(const ModerationRequest& req);
    void RenderConfirmModal(const ImVec2& center);

    // page_*.cpp
    void RenderPlayersPage(const ImVec2& size);
    void RenderPlayerList(const std::vector<PlayerInfo>& players);
    void RenderPlayerDetails(const PlayerInfo& p);
    void RenderMiscPage(const ImVec2& size);
    void RenderConfigPage(const ImVec2& size);
    void RenderMaintenancePage(const ImVec2& size, const char* tab_name);

    // overlay_windows.cpp
    void RenderOverlayWindows();
    void RenderPlayerCounter(bool interactive);
    void RenderKeybindList(bool interactive);
    void RenderAdminLogWindow(bool interactive);
    void RenderChatLogWindow(bool interactive);

    void RefreshConfigs();

    static const TabDef kTabs[];
    static const int    kTabCount;

    IAdminBackend& backend_;
    PanelSettings  settings_;
    std::string    brand_name_ = "FoxyHUD";
    std::string    brand_suffix_ = ".admin";

    bool   open_ = true;
    bool   was_open_ = true;
    float  open_anim_ = 0.0f;
    int    current_tab_ = 0;
    double tab_changed_at_ = -1.0;
    bool   settings_dirty_ = false;

    // Players page
    uint64_t selected_player_ = 0;
    uint64_t spectating_ = 0;
    char     search_[64] = {};
    bool     flagged_only_ = false;
    int      reason_index_ = 0;
    int      duration_index_ = 3;
    bool     ip_ban_ = false;
    char     note_[256] = {};
    char     private_message_[256] = {};
    ModerationRequest pending_{};
    bool     open_confirm_ = false;

    // Misc page
    char broadcast_[256] = {};
    bool broadcast_banner_ = false;

    // Config page
    std::string              config_dir_;
    std::vector<std::string> configs_;
    std::string              loaded_config_;
    int                      selected_config_ = -1;
    char                     config_name_[64] = "default";
    std::string              config_status_;
    bool                     config_status_error_ = false;
    double                   config_status_time_ = -100.0;

    // Overlay windows
    std::vector<AdminLogEntry> log_;
    size_t                     log_seen_ = 0;
    size_t                     chat_seen_ = 0;
};

} // namespace foxy
