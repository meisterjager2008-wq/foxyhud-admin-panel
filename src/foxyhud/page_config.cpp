#include <cstdio>
#include <filesystem>

#include "foxyhud/admin_panel.h"
#include "foxyhud/panel_util.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using ui::Px;

namespace {
constexpr const char* kBuildDate = __DATE__ ", " __TIME__;
}

void AdminPanel::RenderConfigPage(const ImVec2& size) {
    PanelSettings& s = settings_;
    const Palette& c = theme::Colors();
    bool changed = false;

    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);
    const bool has_selection = selected_config_ >= 0 && selected_config_ < static_cast<int>(configs_.size());
    const std::string selected_name = has_selection ? configs_[selected_config_] : std::string();
    auto path_of = [&](const std::string& name) { return config_dir_ + "/" + name + ".cfg"; };

    auto load = [&](const std::string& name) {
        std::string err;
        if (!LoadSettings(s, path_of(name), &err)) {
            SetConfigStatus(true, err);
            return;
        }
        loaded_config_ = name;
        changed = true;
        SetConfigStatus(false, "Loaded " + name + ".cfg");
        Log(AdminLogEntry::Kind::Success, "Loaded config %s.cfg", name.c_str());
    };

    // --- Configurations -------------------------------------------------------------
    if (ui::BeginPanel("##config_left", ImVec2(left_w, size.y))) {
        ui::Section("Configurations");

        const float input_h = ImGui::GetFrameHeight();
        const float list_h = ImGui::GetContentRegionAvail().y - input_h - ImGui::GetStyle().ItemSpacing.y;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, c.window_bg);
        ImGui::PushStyleColor(ImGuiCol_Border, c.panel_border);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Px(3));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(4), Px(4)));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Px(4));
        ImGui::BeginChild("##config_list", ImVec2(0, ImMax(list_h, Px(60))),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, Px(1)));
        for (int i = 0; i < static_cast<int>(configs_.size()); ++i) {
            const std::string file = configs_[i] + ".cfg";
            if (ui::SelectRow(file.c_str(), i == selected_config_))
                selected_config_ = i;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                load(configs_[i]);
        }
        ImGui::PopStyleVar();
        if (configs_.empty())
            ui::CenteredMessage("No configs yet - create one below");
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);

        const float create_w = Px(64);
        bool create = ui::InputText("##new_config", "New config name...", new_config_name_, sizeof(new_config_name_),
                                    -(create_w + ImGui::GetStyle().ItemSpacing.x), Icon::None,
                                    ImGuiInputTextFlags_EnterReturnsTrue, false);
        ImGui::SameLine();
        create |= ui::Button("Create", ImVec2(create_w, ImGui::GetItemRectSize().y), ui::ButtonStyle::Ghost);
        if (create) {
            const std::string name = SanitizeConfigName(new_config_name_);
            std::string err;
            std::error_code ec;
            if (name.empty()) {
                SetConfigStatus(true, "Type a name for the new config");
            } else if (std::filesystem::exists(path_of(name), ec)) {
                SetConfigStatus(true, name + ".cfg already exists");
            } else if (!SaveSettings(s, path_of(name), &err)) {
                SetConfigStatus(true, err);
            } else {
                loaded_config_ = name;
                new_config_name_[0] = '\0';
                configs_ = ListConfigs(config_dir_);
                selected_config_ = -1;
                for (int i = 0; i < static_cast<int>(configs_.size()); ++i)
                    if (configs_[i] == name)
                        selected_config_ = i;
                SetConfigStatus(false, "Created " + name + ".cfg");
                Log(AdminLogEntry::Kind::Success, "Created config %s.cfg", name.c_str());
            }
        }
    }
    ui::EndPanel();

    // --- Actions + Settings --------------------------------------------------------------
    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##config_right", ImVec2(size.x - left_w - gap, size.y))) {
        ui::Section("Actions");

        // "Selected: x.cfg", replaced for a few seconds by the result of the last action.
        const float age = static_cast<float>(ImGui::GetTime() - config_status_time_);
        if (age < 3.5f)
            ui::TextColored(theme::WithAlpha(config_status_error_ ? c.danger : c.success, ImSaturate((3.5f - age) / 0.4f)),
                            "%s", config_status_.c_str());
        else
            ui::TextColored(c.accent_text, "Selected: %s", has_selection ? (selected_name + ".cfg").c_str() : "none");

        const ImVec2 bsize(-FLT_MIN, Px(22));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Px(8), Px(3)));
        ImGui::BeginDisabled(!has_selection);
        if (ui::Button("Load", bsize, ui::ButtonStyle::Flat))
            load(selected_name);
        if (ui::Button("Save", bsize, ui::ButtonStyle::Flat)) {
            std::string err;
            if (!SaveSettings(s, path_of(selected_name), &err)) {
                SetConfigStatus(true, err);
            } else {
                loaded_config_ = selected_name;
                SetConfigStatus(false, "Saved " + selected_name + ".cfg");
                Log(AdminLogEntry::Kind::Success, "Saved config %s.cfg", selected_name.c_str());
            }
        }
        if (ui::ConfirmButton("Delete", "Click again to delete", bsize, ui::ButtonStyle::Flat, ui::ButtonStyle::Danger)) {
            std::error_code ec;
            if (std::filesystem::remove(path_of(selected_name), ec)) {
                if (loaded_config_ == selected_name)
                    loaded_config_.clear();
                selected_config_ = -1;
                RefreshConfigs();
                SetConfigStatus(false, "Deleted " + selected_name + ".cfg");
                Log(AdminLogEntry::Kind::Warning, "Deleted config %s.cfg", selected_name.c_str());
            } else {
                SetConfigStatus(true, "Could not delete " + selected_name + ".cfg");
            }
        }
        ImGui::EndDisabled();
        ui::Spacing(3);
        if (ui::Button("Refresh", bsize, ui::ButtonStyle::Flat)) {
            RefreshConfigs();
            SetConfigStatus(false, "Found " + std::to_string(configs_.size()) + " config(s)");
        }
        if (ui::Button("Open Folder", bsize, ui::ButtonStyle::Flat)) {
            if (!OpenFolder(config_dir_))
                SetConfigStatus(true, "Could not open " + config_dir_);
        }
        ImGui::PopStyleVar();
        ui::Spacing(2);

        ui::Section("Settings");
        changed |= ui::LabelKeybind("Menu Key", &s.menu_key);
        changed |= ui::CheckboxColor("Accent Override", &s.accent_override, s.accent, false);
        ui::Tooltip("Use your own accent color instead of the default blue");
        changed |= ui::Checkbox("Animations", &s.animations);
        changed |= ui::Checkbox("Tooltips", &s.tooltips);
        changed |= ui::Checkbox("Block Game Input", &s.block_input);
        ui::Tooltip("Tell the game to ignore mouse and keyboard while the panel is open");
        changed |= ui::Checkbox("Confirm Bans", &s.confirm_bans);
        changed |= ui::Checkbox("Confirm Kicks", &s.confirm_kicks);
        changed |= ui::Checkbox("Require Ban Note", &s.require_ban_note);
        ui::Tooltip("The Ban button stays disabled until a note / evidence is written");
        changed |= ui::Checkbox("Mask IP Addresses", &s.mask_ips);
        changed |= ui::SliderFloat("Menu Opacity", &s.menu_opacity, 60.0f, 100.0f, "%.0f%%");

        theme::PushFont(theme::GetFonts().bold);
        ui::TextColored(c.text_bright, "Last Build Date:");
        theme::PopFont();
        ImGui::SameLine(0.0f, Px(6));
        ui::TextColored(c.text, "%s", kBuildDate);
    }
    ui::EndPanel();

    if (changed)
        settings_dirty_ = true;
}

} // namespace foxy
