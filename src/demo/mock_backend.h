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

    const std::vector<foxy::PlayerInfo>&  GetPlayers() override { return players_; }
    const std::vector<foxy::ChatMessage>& GetChatLog() override { return chat_; }

    void Moderate(const foxy::ModerationRequest& req) override;
    void SetFrozen(uint64_t id, bool frozen) override;
    void SetChatMuted(uint64_t id, bool muted) override;
    void SetVoiceMuted(uint64_t id, bool muted) override;
    void SendPrivateMessage(uint64_t id, const std::string& text) override;
    void Broadcast(const std::string& text, bool as_banner) override;

private:
    foxy::PlayerInfo* Find(uint64_t id);
    void AddPlayer(double session_seconds);
    void RemovePlayer(uint64_t id);
    void Say(const foxy::PlayerInfo& p);
    void System(const std::string& text);
    int  Rand(int lo, int hi);

    std::vector<foxy::PlayerInfo>  players_;
    std::vector<foxy::ChatMessage> chat_;
    std::mt19937 rng_{1337};
    float    chat_timer_ = 1.5f;
    float    join_timer_ = 25.0f;
    float    ping_timer_ = 0.0f;
    uint64_t next_id_ = 10471;
    size_t   next_name_ = 0;
};
