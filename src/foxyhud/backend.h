#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace foxy {

struct PanelSettings;

// One connected player, as your server reports it.
struct PlayerInfo {
    uint64_t    id = 0;               // your game's unique player/account id
    std::string name;
    std::string ip;                   // shown masked unless "Mask IP addresses" is off
    std::string country;
    std::string platform;
    int         ping_ms = 0;
    double      session_seconds = 0;  // time since this player joined
    double      playtime_hours = 0;   // lifetime playtime
    int         warnings = 0;
    int         reports = 0;          // open reports against this player
    int         previous_bans = 0;
    bool        is_admin = false;
    bool        chat_muted = false;
    bool        voice_muted = false;
    bool        frozen = false;
    bool        flagged = false;      // e.g. flagged by your anti-exploit or auto-moderation
};

enum class ChatKind { Player, System, Admin, Announcement, PrivateMessage };

struct ChatMessage {
    double      timestamp = 0;        // unix time (seconds)
    uint64_t    player_id = 0;        // sender, 0 for system messages
    std::string player_name;
    std::string text;
    ChatKind    kind = ChatKind::Player;
    bool        flagged = false;      // hit the chat filter
};

// A report one player filed against another.
struct PlayerReport {
    uint64_t    id = 0;
    uint64_t    target_id = 0;
    std::string target_name;
    uint64_t    reporter_id = 0;
    std::string reporter_name;
    std::string reason;
    std::string message;              // what the reporter wrote
    double      timestamp = 0;        // unix time
};

struct BanEntry {
    uint64_t    player_id = 0;
    std::string player_name;
    std::string reason;
    std::string note;
    std::string banned_by;
    double      banned_at = 0;        // unix time
    double      expires_at = 0;       // unix time, 0 = permanent
    bool        ip_ban = false;
};

enum class ModerationAction { Warn, Kick, Ban };

struct ModerationRequest {
    ModerationAction action = ModerationAction::Warn;
    uint64_t    player_id = 0;
    std::string player_name;
    std::string reason;
    std::string note;                 // evidence / details written by the admin
    int         duration_minutes = 0; // bans only, 0 = permanent
    bool        ip_ban = false;
};

// The panel is only UI: every action goes through this interface. Implement it
// in your game and forward each call to your server (RPC / packet / HTTP).
//
// IMPORTANT: the server must check that the sender really is an admin before
// applying anything. Never trust the client for moderation.
class IAdminBackend {
public:
    virtual ~IAdminBackend() = default;

    // Snapshot of connected players and recent chat. Called every frame while
    // the panel is visible, so return cached data.
    virtual const std::vector<PlayerInfo>& GetPlayers() = 0;
    virtual const std::vector<ChatMessage>& GetChatLog() {
        static const std::vector<ChatMessage> empty;
        return empty;
    }
    virtual const std::vector<PlayerReport>& GetReports() {  // open reports, oldest first
        static const std::vector<PlayerReport> empty;
        return empty;
    }
    virtual const std::vector<BanEntry>& GetBans() {
        static const std::vector<BanEntry> empty;
        return empty;
    }

    virtual void Moderate(const ModerationRequest& /*request*/) {}
    virtual void SetFrozen(uint64_t /*player_id*/, bool /*frozen*/) {}
    virtual void SetChatMuted(uint64_t /*player_id*/, bool /*muted*/) {}
    virtual void SetVoiceMuted(uint64_t /*player_id*/, bool /*muted*/) {}
    virtual void Spectate(uint64_t /*player_id*/) {}  // 0 = stop spectating
    virtual void TeleportToPlayer(uint64_t /*player_id*/) {}
    virtual void BringPlayer(uint64_t /*player_id*/) {}
    virtual void SendPrivateMessage(uint64_t /*player_id*/, const std::string& /*text*/) {}
    virtual void Broadcast(const std::string& /*text*/, bool /*as_banner*/) {}
    virtual void ResolveReport(uint64_t /*report_id*/, bool /*action_taken*/) {}
    virtual void Unban(uint64_t /*player_id*/) {}

    // Called once per frame at most, whenever something on the Misc/Config tabs changed.
    virtual void OnSettingsChanged(const PanelSettings& /*settings*/) {}
};

} // namespace foxy
