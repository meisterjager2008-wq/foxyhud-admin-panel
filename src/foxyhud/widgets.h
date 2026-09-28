#pragma once

#include <cstddef>

#include "imgui.h"
#include "foxyhud/icons.h"

// Custom widgets that reproduce the look of the reference design. They behave
// like their ImGui counterparts (IDs, return values, IsItemXXX queries) but draw
// everything themselves.
namespace foxy::ui {

float  Px(float v);            // scale a design pixel value by theme::Scale()
ImVec2 Snap(const ImVec2& p);  // round to whole pixels so text stays crisp

void SetAnimationsEnabled(bool enabled);
void SetTooltipsEnabled(bool enabled);
bool IsCapturingKeybind();  // true while a Keybind() waits for a key press

// --- Layout -----------------------------------------------------------------

// Bordered, scrollable box that holds sections (the "Miscellaneous" boxes).
bool BeginPanel(const char* id, const ImVec2& size);
void EndPanel();

// Section title with a divider line under it. `right_text` is drawn dimmed on
// the right side of the title row.
void Section(const char* title, const char* right_text = nullptr);

// Standard width for sliders, combos and inputs inside the current panel, and
// the x coordinate (screen space) where that column ends.
float ItemWidth();
float ItemRight();

// Width of each of `count` items sharing `total` width (0 = ItemWidth()).
float SplitWidth(int count, float total = 0.0f);

// Continue on the same line at `x` pixels from the panel's content start.
void SameLineAt(float x);

void Spacing(float px);

// --- Controls ---------------------------------------------------------------

bool Checkbox(const char* label, bool* v);
bool Keybind(const char* str_id, int* key);                        // key is an ImGuiKey
bool CheckboxKeybind(const char* label, bool* v, int* key);        // checkbox + right aligned keybind
bool LabelKeybind(const char* label, int* key);                    // plain label + right aligned keybind
const char* KeyName(int key);

bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.2f");
bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
bool SliderIndex(const char* label, int* index, const char* const labels[], int count);

bool Combo(const char* label, int* current, const char* const items[], int count);
bool MultiCombo(const char* label, bool* selected, const char* const items[], int count);

// width: 0 = ItemWidth(), > 0 = pixels, < 0 = available width + width
// (so -FLT_MIN fills the line, -50 leaves 50px on the right).
bool InputText(const char* id, const char* hint, char* buf, size_t buf_size, float width = 0.0f,
               Icon icon = Icon::None, ImGuiInputTextFlags flags = 0);

enum class ButtonStyle { Default, Accent, Danger, Warning };
bool Button(const char* label, const ImVec2& size = ImVec2(0, 0), ButtonStyle style = ButtonStyle::Default);

bool ColorEdit(const char* label, float rgb[3]);
bool AccentPresets(const char* id, float rgb[3]);

// Header navigation tab (icon + label, accent outline when selected).
bool Tab(const char* label, Icon icon, bool selected);

// Clickable row used for lists (configs, etc). Returns true when clicked.
bool ListRow(const char* id, const char* text, bool selected, Icon icon = Icon::None, const char* right_text = nullptr);

// --- Text -------------------------------------------------------------------

void TextDim(const char* fmt, ...) IM_FMTARGS(1);
void TextColored(const ImVec4& col, const char* fmt, ...) IM_FMTARGS(2);

// "Key ........ value" row spanning the panel width. Returns true when clicked.
bool KeyValue(const char* key, const char* value, const ImVec4* value_col = nullptr);

// Rounded pill. DrawBadge renders at `pos` and returns its width, Badge() is the
// layout-aware version.
float BadgeWidth(const char* text);
float DrawBadge(ImDrawList* dl, const ImVec2& pos, const char* text, const ImVec4& col);
void  Badge(const char* text, const ImVec4& col);

void Tooltip(const char* text);  // for the previous item, when tooltips are enabled

// Centered text inside the remaining space of the current window.
void CenteredMessage(const char* text);

// --- Popups -----------------------------------------------------------------

bool BeginModal(const char* name, const ImVec2& center);
void EndModal();

// Themed dropdown popup anchored under `anchor` (used by Combo/MultiCombo, and
// reusable for custom menus).
bool BeginDropdown(ImGuiID popup_id, const ImVec2& anchor_min, const ImVec2& anchor_max, float max_height = 0.0f);
void EndDropdown();
bool DropdownItem(const char* label, bool selected, bool show_check = false);

// Frame-rate independent smoothing toward `target`, keyed by `id` (+ `channel`).
float Animate(ImGuiID id, int channel, float target, float speed = 14.0f);

} // namespace foxy::ui
