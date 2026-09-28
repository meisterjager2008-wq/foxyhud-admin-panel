#include "foxyhud/admin_panel.h"
#include "foxyhud/panel_util.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"

namespace foxy {

using ui::Px;

namespace {
const char* const kSortBy[]  = {"Name", "Ping", "Join time", "Reports"};
const char* const kColumns[] = {"Name", "ID", "Ping", "Playtime"};
} // namespace

// Design only: every control keeps its value in settings_, nothing is sent anywhere.
void AdminPanel::RenderPlayersPage(const ImVec2& size) {
    PanelSettings& s = settings_;
    bool changed = false;

    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);

    if (ui::BeginPanel("##players_left", ImVec2(left_w, size.y))) {
        ui::Section("Player List");
        ui::InputText("##search", "Search name or ID...", player_search_, sizeof(player_search_), -FLT_MIN, Icon::Search);
        changed |= ui::Checkbox("Flagged Only", &s.players_flagged_only);
        changed |= ui::Checkbox("Show Admins", &s.players_show_admins);
        changed |= ui::Combo("Sort By", &s.players_sort, kSortBy, IM_ARRAYSIZE(kSortBy));
        changed |= ui::MultiCombo("Columns", s.players_columns, kColumns, IM_ARRAYSIZE(kColumns));

        ui::Section("Actions");
        const float col2 = (float)(int)(ImGui::GetContentRegionAvail().x * 0.5f);
        changed |= ui::Checkbox("Freeze", &s.act_freeze);
        ui::SameLineAt(col2);
        changed |= ui::Checkbox("Spectate", &s.act_spectate);
        changed |= ui::Checkbox("Mute Chat", &s.act_mute_chat);
        ui::SameLineAt(col2);
        changed |= ui::Checkbox("Mute Voice", &s.act_mute_voice);

        const float bw = ui::SplitWidth(2, ImGui::GetContentRegionAvail().x);
        ui::Button("Go To", ImVec2(bw, Px(24)));
        ImGui::SameLine();
        ui::Button("Bring", ImVec2(bw, Px(24)));

        const float send_w = Px(64);
        ui::InputText("##message", "Private message...", player_message_, sizeof(player_message_),
                      -(send_w + ImGui::GetStyle().ItemSpacing.x), Icon::Chat);
        ImGui::SameLine();
        ui::Button("Send", ImVec2(send_w, ImGui::GetItemRectSize().y), ui::ButtonStyle::Accent);
    }
    ui::EndPanel();

    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##players_right", ImVec2(size.x - left_w - gap, size.y))) {
        ui::Section("Moderation");
        changed |= ui::Combo("Reason", &s.ban_reason, detail::kBanReasons, detail::kBanReasonCount);
        changed |= ui::SliderIndex("Ban Duration", &s.ban_duration, detail::kDurationLabels, detail::kDurationCount);
        changed |= ui::Checkbox("IP Ban", &s.ban_ip);
        ui::InputText("##note", "Note / evidence...", player_note_, sizeof(player_note_), -FLT_MIN);

        const float mw = ui::SplitWidth(3, ImGui::GetContentRegionAvail().x);
        ui::Button("Warn", ImVec2(mw, Px(26)));
        ImGui::SameLine();
        ui::Button("Kick", ImVec2(mw, Px(26)), ui::ButtonStyle::Warning);
        ImGui::SameLine();
        ui::Button("Ban", ImVec2(mw, Px(26)), ui::ButtonStyle::Danger);

        ui::Section("Quick Actions");
        changed |= ui::CheckboxKeybind("Quick Freeze", &s.quick_freeze, &s.quick_freeze_key);
        changed |= ui::CheckboxKeybind("Quick Spectate", &s.quick_spectate, &s.quick_spectate_key);
    }
    ui::EndPanel();

    if (changed)
        settings_dirty_ = true;
}

} // namespace foxy
