#pragma once

#include <random>

#include "foxyhud/backend.h"

// Fake server for the demo: a handful of players whose ping and session time
// change, random chat, people joining and leaving. Replace with your own
// IAdminBackend implementation that talks to your game server.
class MockBackend final : public foxy::IAdminBackend {
public:
    MockBackend();

    void Update(float dt);

    const std::vector<foxy::PlayerInfo>&   GetPlayers() override { return players_; }
    const std::vector<foxy::ChatMessage>&  GetChatLog() override { return chat_; }
    const std::vector<foxy::PlayerReport>& GetReports() override { return reports_; }
    const std::vector<foxy::BanEntry>&     GetBans() override { return bans_; }

    void Moderate(const foxy::ModerationRequest& req) override;
    void SetFrozen(uint64_t id, bool frozen) override;
    void SetChatMuted(uint64_t id, bool muted) override;
    void SetVoiceMuted(uint64_t id, bool muted) override;
    void SendPrivateMessage(uint64_t id, const std::string& text) override;
    void Broadcast(const std::string& text, bool as_banner) override;
    void ResolveReport(uint64_t report_id, bool action_taken) override;
    void Unban(uint64_t player_id) override;

private:
    foxy::PlayerInfo* Find(uint64_t id);
    void AddPlayer(double session_seconds);
    void RemovePlayer(uint64_t id);
    void Say(const foxy::PlayerInfo& p);
    void System(const std::string& text, foxy::ChatKind kind = foxy::ChatKind::System);
    void AddReport(uint64_t target_id, const std::string& target_name, const foxy::PlayerInfo& reporter,
                   const std::string& reason, const std::string& message, double age_seconds = 0.0);
    void RecountReports();
    int  Rand(int lo, int hi);

    std::vector<foxy::PlayerInfo>   players_;
    std::vector<foxy::ChatMessage>  chat_;
    std::vector<foxy::PlayerReport> reports_;
    std::vector<foxy::BanEntry>     bans_;
    std::mt19937 rng_{1337};
    float    chat_timer_ = 1.5f;
    float    join_timer_ = 25.0f;
    float    ping_timer_ = 0.0f;
    float    report_timer_ = 45.0f;
    uint64_t next_id_ = 10471;
    uint64_t next_report_id_ = 1;
    size_t   next_name_ = 0;
};
