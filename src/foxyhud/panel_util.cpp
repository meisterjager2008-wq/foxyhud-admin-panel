#include "foxyhud/panel_util.h"

namespace foxy::detail {

const char* const kBanReasons[] = {
    "Cheating / exploiting", "Harassment", "Hate speech", "Griefing", "Spam / advertising",
    "Impersonation", "Inappropriate name", "Ban evasion", "Other",
};
const int kBanReasonCount = IM_ARRAYSIZE(kBanReasons);

const char* const kDurationLabels[] = {
    "1 hour", "6 hours", "12 hours", "1 day", "3 days", "7 days", "14 days", "30 days", "Permanent",
};
const int kDurationCount = IM_ARRAYSIZE(kDurationLabels);

} // namespace foxy::detail
