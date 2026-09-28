#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"

namespace foxy {

using ui::Px;

namespace {

const char* const kNameTagInfo[]    = {"Name", "ID", "Ping", "Distance"};
const char* const kHighlightStyle[] = {"Outline", "Box", "Glow"};
const char* const kZoneStyle[]      = {"Outline", "Filled", "Outline + Filled"};
const char* const kChatPosition[]   = {"Bottom Left", "Top Left", "Bottom Right"};

// Two equal columns filling `size`; `left` / `right` draw the panel contents.
template <typename Left, typename Right>
void TwoPanels(const char* id, const ImVec2& size, Left&& left, Right&& right) {
    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);
    ImGui::PushID(id);
    if (ui::BeginPanel("##left", ImVec2(left_w, size.y)))
        left();
    ui::EndPanel();
    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##right", ImVec2(size.x - left_w - gap, size.y)))
        right();
    ui::EndPanel();
    ImGui::PopID();
}

} // namespace

// Design only: every control keeps its value in settings_, nothing is drawn in a game.
void AdminPanel::RenderVisualsPage(const ImVec2& size) {
    PanelSettings& s = settings_;
    bool changed = false;

    switch (sub_tab_[current_tab_]) {
    default:  // Players
        TwoPanels("players", size,
            [&] {
                ui::Section("Name Tags");
                changed |= ui::CheckboxColor("Name Tags", &s.name_tags, s.name_tag_color);
                changed |= ui::MultiCombo("##name_tag_info", s.name_tag_info, kNameTagInfo, IM_ARRAYSIZE(kNameTagInfo));
                changed |= ui::SliderInt("Max Distance", &s.overlay_distance, 10, 1000, "%d m");

                ui::Section("Highlights");
                float* const reported_cols[] = {s.reported_color, s.flagged_color};
                changed |= ui::CheckboxColors("Highlight Reported", &s.highlight_reported, reported_cols, 2);
                changed |= ui::Combo("##highlight_style", &s.highlight_style, kHighlightStyle, IM_ARRAYSIZE(kHighlightStyle));
                changed |= ui::CheckboxColor("Highlight Frozen", &s.highlight_frozen, s.frozen_color);
                changed |= ui::CheckboxColor("Highlight Admins", &s.highlight_admins, s.admin_color);
            },
            [&] {
                ui::Section("Map");
                changed |= ui::CheckboxColor("Player Blips", &s.map_blips, s.blip_color);
                changed |= ui::CheckboxColor("Report Markers", &s.report_markers, s.marker_color);

                ui::Section("Alerts");
                changed |= ui::Checkbox("Report Toasts", &s.report_toasts);
                if (s.report_toasts)
                    changed |= ui::SliderFloat("Toast Duration", &s.toast_seconds, 2.0f, 20.0f, "%.0f sec");
                changed |= ui::CheckboxColor("Flash Screen On Report", &s.report_flash, s.flash_color);
            });
        break;

    case 1:  // World
        TwoPanels("world", size,
            [&] {
                ui::Section("World");
                changed |= ui::CheckboxColor("Ambient Tint", &s.world_ambient, s.ambient_color);
                changed |= ui::CheckboxColor("World Color", &s.world_color_on, s.world_color);
                changed |= ui::CheckboxColor("Sky Color", &s.sky_color_on, s.sky_color);
                changed |= ui::CheckboxColor("Override Fog", &s.fog_on, s.fog_color);
                if (s.fog_on)
                    changed |= ui::SliderFloat("Fog Density", &s.fog_density, 0.0f, 1.0f, "%.2f");
            },
            [&] {
                ui::Section("Zones");
                changed |= ui::CheckboxColor("Safe Zones", &s.safe_zones, s.safe_zone_color);
                changed |= ui::CheckboxColor("Build Areas", &s.build_areas, s.build_area_color);
                changed |= ui::CheckboxColor("Spawn Protection", &s.spawn_protection, s.spawn_color);
                changed |= ui::Combo("##zone_style", &s.zone_style, kZoneStyle, IM_ARRAYSIZE(kZoneStyle));
                changed |= ui::SliderFloat("Zone Opacity", &s.zone_opacity, 0.0f, 1.0f, "%.2f");
            });
        break;

    case 2:  // Chat
        TwoPanels("chat", size,
            [&] {
                ui::Section("Chat Colors");
                changed |= ui::LabelColor("Admin Messages", s.chat_admin_color);
                changed |= ui::LabelColor("Announcements", s.chat_announce_color);
                changed |= ui::LabelColor("Private Messages", s.chat_pm_color);
                changed |= ui::LabelColor("System Messages", s.chat_system_color);
                changed |= ui::LabelColor("Filtered Messages", s.chat_filtered_color);
            },
            [&] {
                ui::Section("Chat Box");
                changed |= ui::Checkbox("Timestamps", &s.chat_timestamps);
                changed |= ui::Checkbox("Show Filtered Messages", &s.chat_show_filtered);
                changed |= ui::Checkbox("Fade Old Messages", &s.chat_fade);
                changed |= ui::Combo("Position", &s.chat_position, kChatPosition, IM_ARRAYSIZE(kChatPosition));
                changed |= ui::SliderFloat("Scale", &s.chat_scale, 0.75f, 1.5f, "%.2fx");
            });
        break;
    }

    if (changed)
        settings_dirty_ = true;
}

} // namespace foxy
