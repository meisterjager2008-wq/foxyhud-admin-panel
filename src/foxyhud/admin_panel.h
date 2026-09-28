#pragma once

#include <functional>
#include <string>
#include <vector>

#include "imgui.h"
#include "foxyhud/icons.h"
#include "foxyhud/settings.h"
#include "foxyhud/system_monitor.h"

namespace foxy {

// The menu. Create it after theme::LoadFonts / theme::Apply, then call Render()
// every frame between ImGui::NewFrame() and ImGui::Render().
//
// The controls are design only: each one just stores its value in Settings()
// (which the Config tab can save and load). Keybinds work: the menu key opens
// and closes the menu, and a keybind next to a checkbox toggles that checkbox.
class AdminPanel {
public:
    // Icon buttons shown centered under the header for tabs that have them.
    struct SubTabDef {
        const char* name;  // tooltip
        Icon        icon;
    };

    explicit AdminPanel(std::string config_dir = "configs");

    void Render();

    bool IsOpen() const { return open_; }
    void SetOpen(bool open) { open_ = open; }
    void Toggle() { open_ = !open_; }

    // True while the menu is open and Config > "Block Game Input" is on: your
    // game should ignore mouse/keyboard input and show the cursor.
    bool WantsGameInputBlocked() const { return open_ && settings_.block_input; }

    PanelSettings&       Settings() { return settings_; }
    const PanelSettings& Settings() const { return settings_; }
    void                 ApplySettings();  // push settings_ into the theme / widgets

    // Optional: called (at most once per frame) after any control changed a value.
    // Use it later to wire the menu to your game.
    void SetOnSettingsChanged(std::function<void(const PanelSettings&)> callback) {
        on_settings_changed_ = std::move(callback);
    }

    // Brand shown in the menu header and the watermark: `name` in white,
    // `suffix` in the accent color (default "Foxyhud." + "pw").
    void SetBranding(std::string name, std::string suffix);

private:
    enum class Page { Players, Visuals, Misc, Config, Maintenance };

    struct TabDef {
        const char*      label;
        Icon             icon;
        Page             page;
        const SubTabDef* sub_tabs;
        int              sub_tab_count;
    };

    // admin_panel.cpp
    void  HandleHotkeys();
    void  ClosePanelPopups();
    void  RenderMainWindow();
    void  RenderHeader(const ImVec2& win_pos);
    void  RenderSubTabs(const ImVec2& win_pos, float win_width);
    void  SelectTab(int tab, int sub_tab = -1);
    float MinWindowWidth() const;

    // watermark.cpp
    void RenderWatermark();

    // page_*.cpp
    void RenderPlayersPage(const ImVec2& size);
    void RenderVisualsPage(const ImVec2& size);
    void RenderMiscPage(const ImVec2& size);
    void RenderConfigPage(const ImVec2& size);
    void RenderMaintenancePage(const ImVec2& size, const char* tab_name);

    void RefreshConfigs();
    void SetConfigStatus(bool error, const std::string& text);

    static const TabDef kTabs[];
    static const int    kTabCount;

    PanelSettings settings_;
    std::function<void(const PanelSettings&)> on_settings_changed_;
    std::string brand_name_ = "Foxyhud.";
    std::string brand_suffix_ = "pw";

    bool             open_ = true;
    bool             was_open_ = true;
    float            open_anim_ = 0.0f;
    int              current_tab_ = 0;
    std::vector<int> sub_tab_;  // selected sub tab per tab
    double           tab_changed_at_ = -1.0;
    bool             settings_dirty_ = false;

    // Text fields (not saved in configs)
    char player_search_[64] = {};
    char player_message_[256] = {};
    char player_note_[256] = {};
    char broadcast_[256] = {};

    // Watermark
    SystemMonitor monitor_;
    float         fps_shown_ = 0.0f;
    double        fps_updated_at_ = -100.0;

    // Config tab
    std::string              config_dir_;
    std::vector<std::string> configs_;
    std::string              loaded_config_;
    int                      selected_config_ = -1;
    char                     new_config_name_[64] = {};
    std::string              config_status_;
    bool                     config_status_error_ = false;
    double                   config_status_time_ = -100.0;
};

} // namespace foxy
