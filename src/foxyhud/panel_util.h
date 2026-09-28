#pragma once

// Internal helpers shared by the panel pages. Not part of the public API.

#include <cstdint>
#include <string>

#include "imgui.h"

namespace foxy::detail {

std::string ClockTime(double unix_seconds);      // "HH:MM:SS", local time
std::string FormatSession(double seconds);       // "1:02:03" / "02:03"
std::string FormatDuration(int minutes);         // "7 days", "Permanent"
std::string MaskIp(const std::string& ip);       // "84.201.•••.•••"
std::string IdString(uint64_t id);

ImVec4 AvatarColor(const std::string& name);
ImVec4 PingColor(int ping_ms);
bool   ContainsNoCase(const std::string& haystack, const char* needle);

extern const char* const kBanReasons[];
extern const int         kBanReasonCount;
extern const char* const kDurationLabels[];
extern const int         kDurationMinutes[];
extern const int         kDurationCount;

} // namespace foxy::detail
