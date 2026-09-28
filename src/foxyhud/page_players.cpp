#include <cstdio>

#include "foxyhud/admin_panel.h"
#include "foxyhud/panel_util.h"
#include "foxyhud/theme.h"
#include "foxyhud/widgets.h"
#include "imgui_internal.h"

namespace foxy {

using theme::Col;
using theme::Lerp;
using ui::Px;

namespace {

struct BadgeDef {
    const char* text;
    ImVec4      col;
};

// Status badges for a player, most important first.
int CollectBadges(const PlayerInfo& p, BadgeDef out[4], char* reports_buf, size_t reports_buf_size) {
    const Palette& c = theme::Colors();
    int n = 0;
    if (p.reports > 0) {
        std::snprintf(reports_buf, reports_buf_size, "%d report%s", p.reports, p.reports == 1 ? "" : "s");
        out[n++] = {reports_buf, c.danger};
    } else if (p.flagged) {
        out[n++] = {"FLAGGED", c.danger};
    }
    if (p.is_admin)
        out[n++] = {"ADMIN", c.accent_text};
    if (p.frozen)
        out[n++] = {"FROZEN", c.info};
    if (p.chat_muted || p.voice_muted)
        out[n++] = {"MUTED", c.warning};
    return n;
}

void DrawAvatar(ImDrawList* dl, const ImVec2& center, float radius, const PlayerInfo& p) {
    const ImVec4 col = detail::AvatarColor(p.name);
    dl->AddCircleFilled(center, radius, Col(col, 0.20f));
    dl->AddCircle(center, radius, Col(col, 0.45f), 0, 1.0f);

    char initial[2] = {p.name.empty() ? '?' : p.name[0], 0};
    if (initial[0] >= 'a' && initial[0] <= 'z')
        initial[0] = static_cast<char>(initial[0] - 'a' + 'A');
    ImFont* f = theme::GetFonts().bold;
    const float fs = theme::FontSize(f) * (radius > Px(15) ? 1.25f : 1.0f);
    const ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.0f, initial);
    dl->AddText(f, fs, ui::Snap(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f)),
                Col(Lerp(col, ImVec4(1, 1, 1, 1), 0.35f)), initial);
}

bool PlayerRow(const PlayerInfo& p, bool selected) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const std::string id_str = detail::IdString(p.id);
    const ImGuiID id = window->GetID(id_str.c_str());
    const float h = Px(40);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + window->WorkRect.GetWidth(), pos.y + h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float ts = ui::Animate(id, 0, selected ? 1.0f : 0.0f);
    const float th = ui::Animate(id, 1, hovered ? 1.0f : 0.0f);

    const Palette& c = theme::Colors();
    ImDrawList* dl = window->DrawList;
    const float r = Px(4);
    dl->AddRectFilled(bb.Min, bb.Max, Col(c.widget_bg_hover, 0.7f * th * (1.0f - ts)), r);
    if (ts > 0.001f) {
        dl->AddRectFilled(bb.Min, bb.Max, Col(c.accent, 0.13f * ts), r);
        dl->AddRect(bb.Min, bb.Max, Col(c.accent, 0.40f * ts), r);
    }

    // Avatar with a ping dot.
    const float ar = Px(13);
    const ImVec2 ac(bb.Min.x + Px(8) + ar, bb.GetCenter().y);
    DrawAvatar(dl, ac, ar, p);
    const ImVec2 dot(ac.x + ar * 0.72f, ac.y + ar * 0.72f);
    dl->AddCircleFilled(dot, Px(4.5f), Col(c.panel_bg));
    dl->AddCircleFilled(dot, Px(3.0f), Col(detail::PingColor(p.ping_ms)));

    // Badges, right aligned.
    BadgeDef badges[4];
    char reports[24];
    const int badge_count = ImMin(CollectBadges(p, badges, reports, sizeof(reports)), 2);
    float bx = bb.Max.x - Px(8);
    for (int i = 0; i < badge_count; ++i) {
        bx -= ui::BadgeWidth(badges[i].text);
        ui::DrawBadge(dl, ImVec2(bx, bb.GetCenter().y - Px(8)), badges[i].text, badges[i].col);
        bx -= Px(4);
    }

    // Name + details line, clipped before the badges.
    const float tx = ac.x + ar + Px(10);
    dl->PushClipRect(ImVec2(tx, bb.Min.y), ImVec2(bx - Px(4), bb.Max.y), true);
    const ImVec4 name_col = Lerp(Lerp(c.text, c.text_bright, 0.6f + 0.4f * th), c.accent_text, ts);
    dl->AddText(ui::Snap(ImVec2(tx, bb.Min.y + Px(5))), Col(name_col), p.name.c_str());

    char sub[96];
    std::snprintf(sub, sizeof(sub), "#%s  \xE2\x80\xA2  %d ms  \xE2\x80\xA2  %s", id_str.c_str(), p.ping_ms,
                  detail::FormatSession(p.session_seconds).c_str());
    ImFont* sf = theme::GetFonts().caption;
    dl->AddText(sf, theme::FontSize(sf), ui::Snap(ImVec2(tx, bb.Min.y + Px(22))), Col(c.text_dim), sub);
    dl->PopClipRect();
    return pressed;
}

} // namespace

void AdminPanel::RenderPlayersPage(const ImVec2& size) {
    const std::vector<PlayerInfo>& players = backend_.GetPlayers();

    // Work on a copy of the selected player: actions below may change the
    // backend's list (a kick removes the player) while we are still drawing.
    PlayerInfo selected;
    bool has_selection = false;
    for (const PlayerInfo& p : players) {
        if (p.id == selected_player_) {
            selected = p;
            has_selection = true;
            break;
        }
    }

    const float gap = Px(10);
    const float left_w = (float)(int)((size.x - gap) * 0.5f);

    if (ui::BeginPanel("##player_list", ImVec2(left_w, size.y)))
        RenderPlayerList(players);
    ui::EndPanel();

    ImGui::SameLine(0.0f, gap);
    if (ui::BeginPanel("##player_details", ImVec2(size.x - left_w - gap, size.y))) {
        if (has_selection) {
            RenderPlayerDetails(selected);
        } else {
            ui::Section("Player");
            ui::CenteredMessage("Select a player from the list");
        }
    }
    ui::EndPanel();
}

void AdminPanel::RenderPlayerList(const std::vector<PlayerInfo>& players) {
    char count[32];
    std::snprintf(count, sizeof(count), "%d / %d online", static_cast<int>(players.size()), settings_.max_players);
    ui::Section("Players", count);

    ui::InputText("##search", "Search name or ID...", search_, sizeof(search_), -FLT_MIN, Icon::Search);
    ui::Checkbox("Flagged / reported only", &flagged_only_);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Px(4));
    ImGui::BeginChild("##rows", ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, Px(3)));

    int shown = 0;
    for (const PlayerInfo& p : players) {
        if (flagged_only_ && !p.flagged && p.reports == 0)
            continue;
        if (search_[0] && !detail::ContainsNoCase(p.name, search_) &&
            !detail::ContainsNoCase(detail::IdString(p.id), search_))
            continue;
        ++shown;
        if (PlayerRow(p, p.id == selected_player_))
            selected_player_ = p.id;
    }

    ImGui::PopStyleVar();
    if (shown == 0)
        ui::CenteredMessage(players.empty() ? "Nobody is online" : "No players match the filter");
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void AdminPanel::RenderPlayerDetails(const PlayerInfo& p) {
    const Palette& c = theme::Colors();
    const std::string id_str = detail::IdString(p.id);
    ImGui::PushID(id_str.c_str());

    // --- Header card: avatar, name, id, badges ---------------------------------
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* dl = window->DrawList;
        const ImVec2 pos = window->DC.CursorPos;
        const float w = window->WorkRect.GetWidth(), h = Px(42);
        const float ar = Px(20);
        DrawAvatar(dl, ImVec2(pos.x + ar, pos.y + h * 0.5f), ar, p);

        const float tx = pos.x + ar * 2.0f + Px(12);
        theme::PushFont(theme::GetFonts().bold);
        dl->AddText(ui::Snap(ImVec2(tx, pos.y + Px(4))), Col(c.text_bright), p.name.c_str());
        const float name_w = ImGui::CalcTextSize(p.name.c_str()).x;
        theme::PopFont();

        char sub[96];
        std::snprintf(sub, sizeof(sub), "ID %s  \xE2\x80\xA2  %s  \xE2\x80\xA2  online %s", id_str.c_str(),
                      p.platform.c_str(), detail::FormatSession(p.session_seconds).c_str());
        ImFont* sf = theme::GetFonts().caption;
        dl->AddText(sf, theme::FontSize(sf), ui::Snap(ImVec2(tx, pos.y + Px(23))), Col(c.text_dim), sub);

        BadgeDef badges[4];
        char reports[24];
        const int n = CollectBadges(p, badges, reports, sizeof(reports));
        float bx = tx + name_w + Px(8);
        for (int i = 0; i < n; ++i) {
            if (bx + ui::BadgeWidth(badges[i].text) > pos.x + w)
                break;
            bx += ui::DrawBadge(dl, ImVec2(bx, pos.y + Px(3)), badges[i].text, badges[i].col) + Px(4);
        }
        ImGui::ItemSize(ImVec2(w, h));
        ImGui::ItemAdd(ImRect(pos, ImVec2(pos.x + w, pos.y + h)), 0);
    }
    ui::Spacing(2);

    // --- Information -------------------------------------------------------------
    ui::Section("Information");
    if (ui::KeyValue("User ID", id_str.c_str())) {
        ImGui::SetClipboardText(id_str.c_str());
        Log(AdminLogEntry::Kind::Info, "Copied user ID of %s", p.name.c_str());
    }
    ui::Tooltip("Click to copy");
    const std::string ip = settings_.mask_ips ? detail::MaskIp(p.ip) : p.ip;
    if (ui::KeyValue("IP address", ip.c_str())) {
        ImGui::SetClipboardText(p.ip.c_str());
        Log(AdminLogEntry::Kind::Info, "Copied IP address of %s", p.name.c_str());
    }
    ui::Tooltip("Click to copy the full address");
    ui::KeyValue("Country", p.country.c_str());

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d ms", p.ping_ms);
    const ImVec4 ping_col = detail::PingColor(p.ping_ms);
    ui::KeyValue("Ping", buf, &ping_col);
    std::snprintf(buf, sizeof(buf), "%.1f h", p.playtime_hours);
    ui::KeyValue("Total playtime", buf);

    std::snprintf(buf, sizeof(buf), "%d", p.warnings);
    ui::KeyValue("Warnings", buf, p.warnings > 0 ? &c.warning : nullptr);
    std::snprintf(buf, sizeof(buf), "%d", p.reports);
    ui::KeyValue("Open reports", buf, p.reports > 0 ? &c.danger : nullptr);
    std::snprintf(buf, sizeof(buf), "%d", p.previous_bans);
    ui::KeyValue("Previous bans", buf, p.previous_bans > 0 ? &c.danger : nullptr);

    // --- Actions -----------------------------------------------------------------
    ui::Section("Actions");
    const float col2 = (float)(int)(ImGui::GetContentRegionAvail().x * 0.5f);

    bool frozen = p.frozen;
    if (ui::Checkbox("Freeze", &frozen)) {
        backend_.SetFrozen(p.id, frozen);
        Log(AdminLogEntry::Kind::Info, "%s %s", frozen ? "Froze" : "Unfroze", p.name.c_str());
    }
    ui::Tooltip("Stops the player from moving or interacting");
    ui::SameLineAt(col2);
    bool spectate = spectating_ == p.id;
    if (ui::Checkbox("Spectate", &spectate)) {
        spectating_ = spectate ? p.id : 0;
        backend_.Spectate(spectating_);
        Log(AdminLogEntry::Kind::Info, "%s %s", spectate ? "Spectating" : "Stopped spectating", p.name.c_str());
    }

    bool chat_muted = p.chat_muted;
    if (ui::Checkbox("Mute chat", &chat_muted)) {
        backend_.SetChatMuted(p.id, chat_muted);
        Log(AdminLogEntry::Kind::Info, "%s chat of %s", chat_muted ? "Muted" : "Unmuted", p.name.c_str());
    }
    ui::SameLineAt(col2);
    bool voice_muted = p.voice_muted;
    if (ui::Checkbox("Mute voice", &voice_muted)) {
        backend_.SetVoiceMuted(p.id, voice_muted);
        Log(AdminLogEntry::Kind::Info, "%s voice of %s", voice_muted ? "Muted" : "Unmuted", p.name.c_str());
    }

    const float bw = ui::SplitWidth(2, ImGui::GetContentRegionAvail().x);
    if (ui::Button("Go to player", ImVec2(bw, Px(24)))) {
        backend_.TeleportToPlayer(p.id);
        Log(AdminLogEntry::Kind::Info, "Teleported to %s", p.name.c_str());
    }
    ImGui::SameLine();
    if (ui::Button("Bring player", ImVec2(bw, Px(24)))) {
        backend_.BringPlayer(p.id);
        Log(AdminLogEntry::Kind::Info, "Brought %s to you", p.name.c_str());
    }

    const float send_w = Px(64);
    bool send = ui::InputText("##pm", "Private message...", private_message_, sizeof(private_message_),
                              -(send_w + ImGui::GetStyle().ItemSpacing.x), Icon::Chat,
                              ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    send |= ui::Button("Send", ImVec2(send_w, ImGui::GetItemRectSize().y), ui::ButtonStyle::Accent);
    if (send && private_message_[0]) {
        backend_.SendPrivateMessage(p.id, private_message_);
        Log(AdminLogEntry::Kind::Info, "Messaged %s: %s", p.name.c_str(), private_message_);
        private_message_[0] = '\0';
    }

    // --- Moderation --------------------------------------------------------------
    ui::Section("Moderation");
    ui::Combo("Reason", &reason_index_, detail::kBanReasons, detail::kBanReasonCount);
    ui::SliderIndex("Ban duration", &duration_index_, detail::kDurationLabels, detail::kDurationCount);
    ui::Checkbox("IP ban", &ip_ban_);
    ui::Tooltip("Also blocks new accounts coming from the same IP address");
    ui::InputText("##note", "Note / evidence (clip link, chat quote...)", note_, sizeof(note_), -FLT_MIN);

    ModerationRequest req;
    req.player_id = p.id;
    req.player_name = p.name;
    req.reason = detail::kBanReasons[reason_index_];
    req.note = note_;
    req.duration_minutes = detail::kDurationMinutes[duration_index_];
    req.ip_ban = ip_ban_;

    const bool note_missing = settings_.require_ban_note && note_[0] == '\0';
    const float mw = ui::SplitWidth(3, ImGui::GetContentRegionAvail().x);
    if (ui::Button("Warn", ImVec2(mw, Px(26)))) {
        req.action = ModerationAction::Warn;
        RequestModeration(req);
    }
    ImGui::SameLine();
    if (ui::Button("Kick", ImVec2(mw, Px(26)), ui::ButtonStyle::Warning)) {
        req.action = ModerationAction::Kick;
        RequestModeration(req);
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(note_missing);
    if (ui::Button("Ban", ImVec2(mw, Px(26)), ui::ButtonStyle::Danger)) {
        req.action = ModerationAction::Ban;
        RequestModeration(req);
    }
    ImGui::EndDisabled();
    if (note_missing)
        ui::Tooltip("Write a note first (Config > Safety > Require a note to ban)");

    // --- Recent chat -------------------------------------------------------------
    ui::Section("Recent chat");
    const std::vector<ChatMessage>& chat = backend_.GetChatLog();
    const ChatMessage* recent[6];
    int n = 0;
    for (auto it = chat.rbegin(); it != chat.rend() && n < IM_ARRAYSIZE(recent); ++it)
        if (it->player_id == p.id)
            recent[n++] = &*it;
    if (n == 0)
        ui::TextDim("No messages this session");
    ImGui::PushTextWrapPos(0.0f);
    for (int i = n - 1; i >= 0; --i) {
        const ChatMessage& m = *recent[i];
        ui::TextColored(c.text_faint, "%s", detail::ClockTime(m.timestamp).substr(0, 5).c_str());
        ImGui::SameLine();
        ui::TextColored(m.flagged ? c.danger : c.text, "%s", m.text.c_str());
    }
    ImGui::PopTextWrapPos();

    ImGui::PopID();
}

} // namespace foxy
