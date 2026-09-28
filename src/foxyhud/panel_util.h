#pragma once

// Internal helpers shared by the menu pages. Not part of the public API.

#include "imgui.h"

// ImGui renamed ImGuiChildFlags_Border in 1.91.1.
#if IMGUI_VERSION_NUM < 19110
#define ImGuiChildFlags_Borders ImGuiChildFlags_Border
#endif

namespace foxy::detail {

// Items for the Players tab's Reason combo and Ban Duration slider.
extern const char* const kBanReasons[];
extern const int         kBanReasonCount;
extern const char* const kDurationLabels[];
extern const int         kDurationCount;

} // namespace foxy::detail
