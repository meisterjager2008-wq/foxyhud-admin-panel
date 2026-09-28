#pragma once

// Internal helpers shared by the panel pages. Not part of the public API.

#include <cstdint>
#include <string>

#include "imgui.h"

// ImGui renamed ImGuiChildFlags_Border in 1.91.1.
#if IMGUI_VERSION_NUM < 19110
#define ImGuiChildFlags_Borders ImGuiChildFlags_Border
#endif

namespace foxy::detail {

std::string ClockTime(double unix_seconds);      // "HH:MM:SS", local time
std::string FormatSession(double seconds);       // "1:02:03" / "02:03"
std::string FormatDuration(int minutes);         // "7 days", "Permanent"
std::string MaskIp(const std::string& ip);       // "84.201.•••.•••"
std::string IdString(uint64_t id);
std::string TimeAgo(double unix_seconds);        // "just now", "5m ago", "3h ago", "2d ago"
std::string TimeLeft(double expires_at);         // "2d 4h left", "Permanent", "Expired"
std::string DateTime(double unix_seconds);       // "Sep 28, 21:14"

ImVec4 AvatarColor(const std::string& name);
ImVec4 PingColor(int ping_ms);
bool   ContainsNoCase(const std::string& haystack, const char* needle);

extern const char* const kBanReasons[];
extern const int         kBanReasonCount;
extern const char* const kDurationLabels[];
extern const int         kDurationMinutes[];
extern const int         kDurationCount;

} // namespace foxy::detail
