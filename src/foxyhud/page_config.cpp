#include <cstdio>
#include <filesystem>

#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using ui::Px;

void AdminPanel::RenderConfigPage(const ImVec2& size) {
    PanelSettings& s = settings_;
    const Palette& c = theme::Colors();
    bool changed = false;

    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);

    if (ui::BeginPanel("##config_left", ImVec2(left_w, size.y))) {
        ui::Section("Menu");
        changed |= ui::LabelKeybind("Menu key", &s.menu_key);
        changed |= ui::ColorEdit("Accent color", s.accent);
        changed |= ui::AccentPresets("##accent_presets", s.accent);
        changed |= ui::SliderFloat("Menu opacity", &s.menu_opacity, 60.0f, 100.0f, "%.0f%%");
        changed |= ui::Checkbox("Animations", &s.animations);
        changed |= ui::Checkbox("Tooltips", &s.tooltips);

        ui::Section("Safety");
        changed |= ui::Checkbox("Confirm before banning", &s.confirm_bans);
        changed |= ui::Checkbox("Confirm before kicking", &s.confirm_kicks);
        changed |= ui::Checkbox("Require a note to ban", &s.require_ban_note);
        changed |= ui::Checkbox("Mask IP addresses", &s.mask_ips);
    }
    ui::EndPanel();

    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##config_right", ImVec2(size.x - left_w - gap, size.y))) {
        char count[32];
        std::snprintf(count, sizeof(count), "%d saved", static_cast<int>(configs_.size()));
        ui::Section("Configs", count);

        for (int i = 0; i < static_cast<int>(configs_.size()); ++i) {
            const std::string& name = configs_[i];
            const char* tag = name == loaded_config_ ? "loaded" : nullptr;
            if (ui::ListRow(name.c_str(), name.c_str(), i == selected_config_, Icon::File, tag)) {
                selected_config_ = i;
                std::snprintf(config_name_, sizeof(config_name_), "%s", name.c_str());
            }
        }
        if (configs_.empty())
            ui::TextDim("No saved configs yet");

        ui::InputText("##config_name", "Config name", config_name_, sizeof(config_name_), -FLT_MIN, Icon::File);

        auto set_status = [&](bool error, const std::string& text) {
            config_status_ = text;
            config_status_error_ = error;
            config_status_time_ = ImGui::GetTime();
        };
        const std::string name = SanitizeConfigName(config_name_);
        const std::string path = config_dir_ + "/" + name + ".cfg";

        const float bw = ui::SplitWidth(3, ImGui::GetContentRegionAvail().x);
        if (ui::Button("Save", ImVec2(bw, Px(24)), ui::ButtonStyle::Accent)) {
            std::string err;
            if (name.empty())
                set_status(true, "Enter a config name first");
            else if (!SaveSettings(s, path, &err))
                set_status(true, err);
            else {
                set_status(false, "Saved \"" + name + "\"");
                loaded_config_ = name;
                std::snprintf(config_name_, sizeof(config_name_), "%s", name.c_str());
                RefreshConfigs();
                Log(AdminLogEntry::Kind::Success, "Saved config \"%s\"", name.c_str());
            }
        }
        ImGui::SameLine();
        if (ui::Button("Load", ImVec2(bw, Px(24)))) {
            std::string err;
            if (name.empty())
                set_status(true, "Pick a config to load");
            else if (!LoadSettings(s, path, &err))
                set_status(true, err);
            else {
                set_status(false, "Loaded \"" + name + "\"");
                loaded_config_ = name;
                changed = true;
                Log(AdminLogEntry::Kind::Success, "Loaded config \"%s\"", name.c_str());
            }
        }
        ImGui::SameLine();
        if (ui::Button("Delete", ImVec2(bw, Px(24)), ui::ButtonStyle::Danger)) {
            std::error_code ec;
            if (!name.empty() && std::filesystem::remove(path, ec)) {
                set_status(false, "Deleted \"" + name + "\"");
                if (loaded_config_ == name)
                    loaded_config_.clear();
                RefreshConfigs();
            } else {
                set_status(true, "Nothing to delete");
            }
        }

        const float hw = ui::SplitWidth(2, ImGui::GetContentRegionAvail().x);
        if (ui::Button("Refresh list", ImVec2(hw, Px(24))))
            RefreshConfigs();
        ImGui::SameLine();
        if (ui::Button("Reset to defaults", ImVec2(hw, Px(24)))) {
            s = PanelSettings{};
            changed = true;
            set_status(false, "Settings reset to defaults");
        }

        // Status line fades out after a few seconds.
        const float age = static_cast<float>(ImGui::GetTime() - config_status_time_);
        if (age < 4.0f) {
            const float a = ImSaturate((4.0f - age) / 0.5f);
            ui::TextColored(theme::WithAlpha(config_status_error_ ? c.danger : c.success, a), "%s",
                            config_status_.c_str());
        }

        ui::Section("About");
        ui::KeyValue("Panel version", "0.1.0");
        ui::KeyValue("Dear ImGui", IMGUI_VERSION);
        ui::KeyValue("Config folder", config_dir_.c_str());
    }
    ui::EndPanel();

    if (changed)
        settings_dirty_ = true;
}

} // namespace foxy
