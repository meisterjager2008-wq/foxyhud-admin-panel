#include "foxyhud/panel_util.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "foxyhud/theme.h"
#include "imgui_internal.h"

namespace foxy::detail {

const char* const kBanReasons[] = {
    "Cheating / exploiting", "Harassment", "Hate speech", "Griefing", "Spam / advertising",
    "Impersonation", "Inappropriate name", "Ban evasion", "Other",
};
const int kBanReasonCount = IM_ARRAYSIZE(kBanReasons);

const char* const kDurationLabels[] = {
    "1 hour", "6 hours", "12 hours", "1 day", "3 days", "7 days", "14 days", "30 days", "Permanent",
};
const int kDurationMinutes[] = {60, 360, 720, 1440, 4320, 10080, 20160, 43200, 0};
const int kDurationCount = IM_ARRAYSIZE(kDurationLabels);

std::string ClockTime(double unix_seconds) {
    const std::time_t t = static_cast<std::time_t>(unix_seconds);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    return buf;
}

std::string FormatSession(double seconds) {
    const int total = seconds > 0 ? static_cast<int>(seconds) : 0;
    const int h = total / 3600, m = (total / 60) % 60, s = total % 60;
    char buf[32];
    if (h > 0)
        std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
    else
        std::snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return buf;
}

std::string FormatDuration(int minutes) {
    if (minutes <= 0)
        return "Permanent";
    char buf[32];
    if (minutes % 1440 == 0)
        std::snprintf(buf, sizeof(buf), "%d day%s", minutes / 1440, minutes == 1440 ? "" : "s");
    else if (minutes % 60 == 0)
        std::snprintf(buf, sizeof(buf), "%d hour%s", minutes / 60, minutes == 60 ? "" : "s");
    else
        std::snprintf(buf, sizeof(buf), "%d min", minutes);
    return buf;
}

std::string MaskIp(const std::string& ip) {
    static const char* kDots = "\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2";  // three bullets
    const auto first = ip.find('.');
    const auto second = first == std::string::npos ? std::string::npos : ip.find('.', first + 1);
    if (second != std::string::npos)
        return ip.substr(0, second) + "." + kDots + "." + kDots;
    return ip.substr(0, ip.size() < 4 ? ip.size() : 4) + kDots;
}

std::string IdString(uint64_t id) { return std::to_string(id); }

ImVec4 AvatarColor(const std::string& name) {
    static const ImVec4 kColors[] = {
        ImVec4(0.36f, 0.55f, 0.95f, 1), ImVec4(0.62f, 0.44f, 0.95f, 1), ImVec4(0.90f, 0.42f, 0.62f, 1),
        ImVec4(0.93f, 0.55f, 0.30f, 1), ImVec4(0.36f, 0.78f, 0.52f, 1), ImVec4(0.30f, 0.74f, 0.82f, 1),
        ImVec4(0.86f, 0.74f, 0.32f, 1), ImVec4(0.70f, 0.70f, 0.76f, 1),
    };
    return kColors[ImHashStr(name.c_str()) % IM_ARRAYSIZE(kColors)];
}

ImVec4 PingColor(int ping_ms) {
    const Palette& c = theme::Colors();
    if (ping_ms < 80)
        return c.success;
    if (ping_ms < 150)
        return c.warning;
    return c.danger;
}

bool ContainsNoCase(const std::string& haystack, const char* needle) {
    if (!needle || !*needle)
        return true;
    const size_t n = std::strlen(needle);
    if (n > haystack.size())
        return false;
    for (size_t i = 0; i + n <= haystack.size(); ++i) {
        size_t j = 0;
        while (j < n && std::tolower((unsigned char)haystack[i + j]) == std::tolower((unsigned char)needle[j]))
            ++j;
        if (j == n)
            return true;
    }
    return false;
}

} // namespace foxy::detail
