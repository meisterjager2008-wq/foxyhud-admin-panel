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
    // Icon buttons shown centered under the header for tabs that have them.
    struct SubTabDef {
        const char* name;  // tooltip
        Icon        icon;
    };

    explicit AdminPanel(IAdminBackend& backend, std::string config_dir = "configs");

    void Render();

    bool IsOpen() const { return open_; }
    void SetOpen(bool open) { open_ = open; }
    void Toggle() { open_ = !open_; }

    // True while the panel is open and Config > "Block game input" is on: your
    // game should ignore mouse/keyboard input and show the cursor.
    bool WantsGameInputBlocked() const { return open_ && settings_.block_input; }

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
        const char*      label;
        Icon             icon;
        Page             page;
        const SubTabDef* sub_tabs;
        int              sub_tab_count;
    };

    struct Toast {
        std::string title;
        std::string text;
        double      created_at;
    };

    // admin_panel.cpp
    void HandleHotkeys();
    void ClosePanelPopups();
    void RenderMainWindow();
    void RenderHeader(const ImVec2& win_pos, float win_width);
    void RenderSubTabs(const ImVec2& win_pos, float win_width);
    void SelectTab(int tab, int sub_tab = -1);
    float MinWindowWidth() const;
    void RequestModeration(const ModerationRequest& req);
    void ExecuteModeration(const ModerationRequest& req);
    void RenderConfirmModal(const ImVec2& center);

    // page_players.cpp
    void RenderPlayersPage(const ImVec2& size);
    void RenderOnlinePage(const ImVec2& size);
    void RenderPlayerList(const std::vector<PlayerInfo>& players);
    void RenderPlayerDetails(const PlayerInfo& p);
    void RenderRecentChat(uint64_t player_id);
    void RenderReportsPage(const ImVec2& size);
    void RenderBanListPage(const ImVec2& size);

    // page_misc.cpp
    void RenderMiscPage(const ImVec2& size);
    void RenderMiscGeneral(const ImVec2& size);
    void RenderMiscOverlays(const ImVec2& size);

    // page_config.cpp / page_maintenance.cpp
    void RenderConfigPage(const ImVec2& size);
    void RenderMaintenancePage(const ImVec2& size, const char* tab_name);

    // overlay_windows.cpp
    void RenderOverlayWindows();
    void RenderPlayerCounter(bool interactive);
    void RenderKeybindList(bool interactive);
    void RenderAdminLogWindow(bool interactive);
    void RenderChatLogWindow(bool interactive);
    void RenderToasts();

    void RefreshConfigs();
    void SetConfigStatus(bool error, const std::string& text);

    static const TabDef kTabs[];
    static const int    kTabCount;

    IAdminBackend& backend_;
    PanelSettings  settings_;
    std::string    brand_name_ = "FoxyHUD";
    std::string    brand_suffix_ = ".admin";

    bool             open_ = true;
    bool             was_open_ = true;
    float            open_anim_ = 0.0f;
    int              current_tab_ = 0;
    std::vector<int> sub_tab_;  // selected sub tab per tab
    double           tab_changed_at_ = -1.0;
    bool             settings_dirty_ = false;

    // Players > Online
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

    // Players > Reports / Ban list
    uint64_t selected_report_ = 0;
    uint64_t selected_ban_ = 0;
    char     ban_search_[64] = {};

    // Misc
    char broadcast_[256] = {};
    bool broadcast_banner_ = false;

    // Config
    std::string              config_dir_;
    std::vector<std::string> configs_;
    std::string              loaded_config_;
    int                      selected_config_ = -1;
    char                     new_config_name_[64] = {};
    std::string              config_status_;
    bool                     config_status_error_ = false;
    double                   config_status_time_ = -100.0;

    // Overlay windows
    std::vector<AdminLogEntry> log_;
    size_t                     log_seen_ = 0;
    size_t                     chat_seen_ = 0;
    std::vector<Toast>         toasts_;
    uint64_t                   last_report_id_ = 0;
    bool                       reports_primed_ = false;
};

} // namespace foxy
