#include "mock_backend.h"

#include <algorithm>
#include <ctime>

using foxy::ChatMessage;
using foxy::ModerationAction;
using foxy::PlayerInfo;

namespace {

const char* const kNames[] = {
    "NovaFox", "pixel_pirate", "Kestrel", "moonwalker22", "LunaByte", "grumpycat", "Sir_Lagalot",
    "ZeroCool", "tinyturtle", "Vex", "CaptainCrunch", "echo.wav", "blizzard", "nightowl",
    "Rook", "sparkplug", "Waffles", "Juniper", "ghost_in_shell", "Maple", "Quokka", "Hyperion",
};
const char* const kCountries[] = {
    "Germany", "United States", "Brazil", "Poland", "Sweden", "Canada", "France", "Japan", "United Kingdom", "Netherlands",
};
const char* const kPlatforms[] = {"Windows", "Windows", "Windows", "Linux", "macOS", "Steam Deck"};
const char* const kChat[] = {
    "anyone want to team up?", "gg", "how do I get to the north island?", "lol", "brb", "that was insane",
    "who built the tower near spawn?", "can an admin help me pls", "nice build!", "wait what just happened",
    "my game froze for a sec", "hi everyone", "someone keeps pushing me off the bridge", "where is the shop?",
};
const char* const kFlaggedChat[] = {
    "you're all trash ****", "FREE COINS >> [link removed]", "*** ***** noob", "report me idc lol",
};

double Now() { return static_cast<double>(std::time(nullptr)); }

} // namespace

MockBackend::MockBackend() {
    for (int i = 0; i < 14; ++i)
        AddPlayer(Rand(60, 3 * 3600));

    // A few interesting cases to look at.
    players_[2].is_admin = true;                        // Kestrel
    players_[5].reports = 3;                            // grumpycat
    players_[5].warnings = 2;
    players_[5].flagged = true;
    players_[6].ping_ms = 212;                          // Sir_Lagalot
    players_[7].flagged = true;                         // ZeroCool
    players_[7].warnings = 1;
    players_[7].previous_bans = 1;
    players_[9].chat_muted = true;                      // Vex
    players_[11].reports = 1;                           // echo.wav

    chat_.clear();
    for (int i = 0; i < 8; ++i)
        Say(players_[Rand(0, static_cast<int>(players_.size()) - 1)]);
    ChatMessage bad;
    bad.timestamp = Now();
    bad.player_id = players_[5].id;
    bad.player_name = players_[5].name;
    bad.text = kFlaggedChat[0];
    bad.flagged = true;
    chat_.push_back(bad);
}

int MockBackend::Rand(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng_); }

PlayerInfo* MockBackend::Find(uint64_t id) {
    for (PlayerInfo& p : players_)
        if (p.id == id)
            return &p;
    return nullptr;
}

void MockBackend::AddPlayer(double session_seconds) {
    PlayerInfo p;
    p.id = next_id_++;
    p.name = kNames[next_name_++ % (sizeof(kNames) / sizeof(kNames[0]))];
    p.ip = std::to_string(Rand(23, 213)) + "." + std::to_string(Rand(1, 254)) + "." + std::to_string(Rand(1, 254)) +
           "." + std::to_string(Rand(1, 254));
    p.country = kCountries[Rand(0, static_cast<int>(sizeof(kCountries) / sizeof(kCountries[0])) - 1)];
    p.platform = kPlatforms[Rand(0, static_cast<int>(sizeof(kPlatforms) / sizeof(kPlatforms[0])) - 1)];
    p.ping_ms = Rand(18, 120);
    p.session_seconds = session_seconds;
    p.playtime_hours = Rand(1, 4000) / 10.0;
    players_.push_back(p);
}

void MockBackend::RemovePlayer(uint64_t id) {
    players_.erase(std::remove_if(players_.begin(), players_.end(), [&](const PlayerInfo& p) { return p.id == id; }),
                   players_.end());
}

void MockBackend::Say(const PlayerInfo& p) {
    ChatMessage m;
    m.timestamp = Now();
    m.player_id = p.id;
    m.player_name = p.name;
    m.flagged = Rand(0, 100) < 10;
    m.text = m.flagged ? kFlaggedChat[Rand(0, 3)] : kChat[Rand(0, static_cast<int>(sizeof(kChat) / sizeof(kChat[0])) - 1)];
    chat_.push_back(m);
    if (chat_.size() > 200)
        chat_.erase(chat_.begin(), chat_.begin() + 50);
}

void MockBackend::System(const std::string& text) {
    ChatMessage m;
    m.timestamp = Now();
    m.text = text;
    chat_.push_back(m);
}

void MockBackend::Update(float dt) {
    for (PlayerInfo& p : players_)
        p.session_seconds += dt;

    ping_timer_ -= dt;
    if (ping_timer_ <= 0.0f) {
        ping_timer_ = 1.0f;
        for (PlayerInfo& p : players_)
            p.ping_ms = std::max(8, p.ping_ms + Rand(-6, 6));
    }

    chat_timer_ -= dt;
    if (chat_timer_ <= 0.0f && !players_.empty()) {
        chat_timer_ = Rand(15, 40) / 10.0f;
        PlayerInfo& p = players_[Rand(0, static_cast<int>(players_.size()) - 1)];
        if (!p.chat_muted) {
            Say(p);
            if (chat_.back().flagged && Rand(0, 1) == 0) {
                p.reports++;
                p.flagged = true;
            }
        }
    }

    join_timer_ -= dt;
    if (join_timer_ <= 0.0f) {
        join_timer_ = static_cast<float>(Rand(25, 45));
        if (players_.size() < 18 || Rand(0, 1) == 0) {
            AddPlayer(0.0);
            System(players_.back().name + " joined the game");
        } else if (!players_.empty()) {
            const PlayerInfo& p = players_[Rand(0, static_cast<int>(players_.size()) - 1)];
            System(p.name + " left the game");
            RemovePlayer(p.id);
        }
    }
}

void MockBackend::Moderate(const foxy::ModerationRequest& req) {
    PlayerInfo* p = Find(req.player_id);
    if (!p)
        return;
    switch (req.action) {
    case ModerationAction::Warn:
        p->warnings++;
        System(p->name + " was warned (" + req.reason + ")");
        break;
    case ModerationAction::Kick:
        System(p->name + " was kicked (" + req.reason + ")");
        RemovePlayer(req.player_id);
        break;
    case ModerationAction::Ban:
        System(p->name + " was banned (" + req.reason + ")");
        RemovePlayer(req.player_id);
        break;
    }
}

void MockBackend::SetFrozen(uint64_t id, bool frozen) {
    if (PlayerInfo* p = Find(id))
        p->frozen = frozen;
}

void MockBackend::SetChatMuted(uint64_t id, bool muted) {
    if (PlayerInfo* p = Find(id))
        p->chat_muted = muted;
}

void MockBackend::SetVoiceMuted(uint64_t id, bool muted) {
    if (PlayerInfo* p = Find(id))
        p->voice_muted = muted;
}

void MockBackend::SendPrivateMessage(uint64_t id, const std::string& text) {
    if (PlayerInfo* p = Find(id))
        System("[PM to " + p->name + "] " + text);
}

void MockBackend::Broadcast(const std::string& text, bool as_banner) {
    System(std::string(as_banner ? "[ANNOUNCEMENT] " : "[ADMIN] ") + text);
}
