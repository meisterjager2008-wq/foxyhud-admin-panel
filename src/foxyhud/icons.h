#pragma once

#include "imgui.h"

namespace foxy {

// Small line icons drawn with ImDrawList, so no icon font is needed and they
// stay sharp at any scale.
enum class Icon {
    None,
    User,       // single person
    Users,      // group of people
    Pencil,
    Eye,
    Grid,
    Help,
    Palette,
    Gear,
    Search,
    Shield,
    Chat,
    Keyboard,
    File,
    List,
    Close,
    ChevronDown,
    Flag,
    Ban,        // circle with a slash
    Sliders,
    EyeFrame,   // eye inside corner brackets
    Gauge,      // circle with a dial
    Crosshair,  // four ticks around a dot
    Target,     // concentric circles
    Globe,
    Cog,        // small 6-tooth gear
};

// `size` is the edge of the square the icon fits in, `rotation` is in radians
// (only used by Gear), `thickness` 0 = derived from size.
void DrawIcon(ImDrawList* dl, Icon icon, const ImVec2& center, float size, ImU32 col, float rotation = 0.0f,
              float thickness = 0.0f);

} // namespace foxy
