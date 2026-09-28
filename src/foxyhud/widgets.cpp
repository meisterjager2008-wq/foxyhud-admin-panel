#include "foxyhud/widgets.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

#include "foxyhud/theme.h"
#include "imgui_internal.h"

#if IMGUI_VERSION_NUM < 19110
#define ImGuiChildFlags_Borders ImGuiChildFlags_Border
#endif

namespace foxy::ui {
namespace {

using theme::Col;
using theme::Lerp;
using theme::WithAlpha;

std::unordered_map<ImGuiID, float> g_anim;
bool    g_animations = true;
bool    g_tooltips = true;
ImGuiID g_keybind_waiting = 0;
int     g_keybind_start_frame = -1;
int     g_keybind_seen_frame = -1;

const Palette& C() { return theme::Colors(); }

ImGuiWindow* Win() { return ImGui::GetCurrentWindow(); }

float ContentX0() { return ImGui::GetCurrentWindowRead()->WorkRect.Min.x; }
float ContentWidth() { return ImGui::GetCurrentWindowRead()->WorkRect.GetWidth(); }

bool HasLabel(const char* label) { return ImGui::FindRenderedTextEnd(label) != label; }

void DrawLabel(ImDrawList* dl, const ImVec2& pos, ImU32 col, const char* label) {
    dl->AddText(Snap(pos), col, label, ImGui::FindRenderedTextEnd(label));
}

// Text clipped with "..." to fit `max_width`.
void DrawTextEllipsis(ImDrawList* dl, const ImVec2& pos, ImU32 col, const char* text, float max_width) {
    const char* end = text + std::strlen(text);
    if (ImGui::CalcTextSize(text, end).x <= max_width) {
        dl->AddText(Snap(pos), col, text, end);
        return;
    }
    const float dots = ImGui::CalcTextSize("...").x;
    while (end > text && ImGui::CalcTextSize(text, end).x + dots > max_width)
        --end;
    std::string clipped(text, end);
    clipped += "...";
    dl->AddText(Snap(pos), col, clipped.c_str());
}

void ShowTooltip(const char* text) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(8), Px(5)));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Px(4));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C().popup_bg);
    ImGui::PushStyleColor(ImGuiCol_Border, C().widget_border);
    ImGui::PushStyleColor(ImGuiCol_Text, C().text);
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(Px(260));
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}

// Colours for tinted buttons (accent / danger / warning): translucent fill,
// stronger outline, bright text. Matches the selected tab look.
struct TintColors {
    ImU32 bg, border, text;
};

TintColors Tinted(const ImVec4& tint, float hover, float held) {
    const float fill = 0.14f + 0.10f * hover - 0.05f * held;
    return {Col(tint, fill), Col(tint, 0.45f + 0.25f * hover), Col(Lerp(tint, ImVec4(1, 1, 1, 1), 0.25f + 0.25f * hover))};
}

void DrawChevron(ImDrawList* dl, const ImVec2& center, float size, ImU32 col, float flip) {
    // flip: 0 = pointing down, 1 = pointing up
    const float h = size * 0.30f * (1.0f - 2.0f * flip);
    const ImVec2 pts[] = {ImVec2(center.x - size * 0.5f, center.y - h), ImVec2(center.x, center.y + h),
                          ImVec2(center.x + size * 0.5f, center.y - h)};
    dl->AddPolyline(pts, 3, col, 0, ImMax(1.0f, Px(1.5f)));
}

// Draws the label line used above sliders / combos. Returns its height.
float LabelAbove(ImDrawList* dl, const ImVec2& pos, const char* label) {
    if (!HasLabel(label))
        return 0.0f;
    DrawLabel(dl, pos, Col(C().text), label);
    return ImGui::GetTextLineHeight() + Px(5);
}

// Combo frame (label + box + preview + chevron). Returns true when clicked.
bool ComboFrame(const char* label, const char* preview, ImGuiID* out_id, ImRect* out_frame) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImGuiID popup_id = ImHashStr("##dropdown", 0, id);
    const float w = ItemWidth();
    const float frame_h = Px(20);
    const float label_h = HasLabel(label) ? ImGui::GetTextLineHeight() + Px(5) : 0.0f;
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect total(pos, ImVec2(pos.x + w, pos.y + label_h + frame_h));
    const ImRect frame(ImVec2(pos.x, pos.y + label_h), total.Max);
    ImGui::ItemSize(total);
    if (!ImGui::ItemAdd(total, id, &frame))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(frame, id, &hovered, &held);
    const bool open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    const float th = Animate(id, 0, hovered ? 1.0f : 0.0f);
    const float to = Animate(id, 1, open ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    LabelAbove(dl, pos, label);
    const float r = Px(3);
    dl->AddRectFilled(frame.Min, frame.Max, Col(Lerp(C().widget_bg, C().widget_bg_hover, th)), r);
    dl->AddRect(frame.Min, frame.Max, Col(Lerp(C().widget_border, WithAlpha(C().accent, 0.8f), to)), r);

    const float text_y = frame.Min.y + (frame_h - ImGui::GetTextLineHeight()) * 0.5f;
    DrawTextEllipsis(dl, ImVec2(frame.Min.x + Px(8), text_y), Col(C().text), preview, w - Px(30));
    DrawChevron(dl, ImVec2(frame.Max.x - Px(11), frame.GetCenter().y), Px(6), Col(C().accent_text), to);

    *out_id = id;
    *out_frame = frame;
    return pressed;
}

bool SliderBar(const char* label, float* frac, const char* value_text, int steps) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const float w = ItemWidth();
    const float bar_h = Px(12);
    const float label_h = HasLabel(label) ? ImGui::GetTextLineHeight() + Px(5) : 0.0f;
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect total(pos, ImVec2(pos.x + w, pos.y + label_h + bar_h));
    const ImRect bar(ImVec2(pos.x, pos.y + label_h), total.Max);
    ImGui::ItemSize(total);
    if (!ImGui::ItemAdd(total, id, &bar))
        return false;

    bool hovered, held;
    ImGui::ButtonBehavior(bar, id, &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    bool changed = false;
    if (held) {
        float f = ImSaturate((ImGui::GetIO().MousePos.x - bar.Min.x) / bar.GetWidth());
        if (steps > 0)
            f = ImFloor(f * steps + 0.5f) / steps;
        if (f != *frac) {
            *frac = f;
            changed = true;
            ImGui::MarkItemEdited(id);
        }
    }

    const float th = Animate(id, 0, (hovered || held) ? 1.0f : 0.0f);
    const float shown = Animate(id, 1, *frac, 18.0f);

    ImDrawList* dl = window->DrawList;
    LabelAbove(dl, pos, label);

    const float r = Px(3);
    dl->AddRectFilled(bar.Min, bar.Max, Col(Lerp(C().widget_bg, C().widget_bg_hover, th * 0.6f)), r);

    const float fill_w = bar.GetWidth() * shown;
    if (fill_w >= 1.0f) {
        const ImVec2 fill_max(bar.Min.x + ImMax(fill_w, r * 2.0f), bar.Max.y);
        const int vtx_begin = dl->VtxBuffer.Size;
        dl->AddRectFilled(bar.Min, fill_max, IM_COL32_WHITE, r);
        const int vtx_end = dl->VtxBuffer.Size;
        const ImVec4 hi = Lerp(C().accent, C().accent_text, 0.35f * th);
        ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, vtx_begin, vtx_end, bar.Min, ImVec2(fill_max.x, bar.Min.y),
                                                      Col(C().accent_dark), Col(hi));
    }

    const ImVec2 ts = ImGui::CalcTextSize(value_text);
    dl->AddText(Snap(ImVec2(bar.GetCenter().x - ts.x * 0.5f, bar.GetCenter().y - ts.y * 0.5f)), Col(C().text_bright), value_text);
    return changed;
}

bool KeybindImpl(const char* str_id, int* key, float line_h) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(str_id);
    const bool waiting = g_keybind_waiting == id;
    const char* text = waiting ? "..." : KeyName(*key);

    const float h = Px(17);
    const float w = ImMax(Px(48), ImGui::CalcTextSize(text).x + Px(16));
    const ImVec2 pos = window->DC.CursorPos;
    const float dy = line_h > 0.0f ? (h - line_h) * 0.5f : 0.0f;
    const ImRect bb(ImVec2(pos.x, pos.y - dy), ImVec2(pos.x + w, pos.y - dy + h));
    ImGui::ItemSize(ImVec2(w, line_h > 0.0f ? line_h : h));
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed && !waiting) {
        g_keybind_waiting = id;
        g_keybind_start_frame = ImGui::GetFrameCount();
    }

    bool changed = false;
    if (g_keybind_waiting == id) {
        g_keybind_seen_frame = ImGui::GetFrameCount();
        if (ImGui::GetFrameCount() > g_keybind_start_frame) {
            for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
                if (k == ImGuiKey_MouseLeft || k == ImGuiKey_MouseWheelX || k == ImGuiKey_MouseWheelY)
                    continue;
                if (k >= ImGuiKey_ReservedForModCtrl && k <= ImGuiKey_ReservedForModSuper)
                    continue;
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) {
                    *key = (k == ImGuiKey_Escape || k == ImGuiKey_Backspace) ? ImGuiKey_None : k;
                    g_keybind_waiting = 0;
                    changed = true;
                    ImGui::MarkItemEdited(id);
                    break;
                }
            }
            if (!changed && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hovered)
                g_keybind_waiting = 0;
        }
    }

    const float th = Animate(id, 0, hovered ? 1.0f : 0.0f);
    const float tw = Animate(id, 1, g_keybind_waiting == id ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    const float r = Px(3);
    dl->AddRectFilled(bb.Min, bb.Max, Col(Lerp(C().widget_bg, C().widget_bg_hover, th)), r);
    dl->AddRect(bb.Min, bb.Max, Col(Lerp(C().widget_border, C().accent, tw)), r);

    const ImVec2 ts = ImGui::CalcTextSize(text);
    const ImVec4 tc = Lerp(Lerp(C().text, C().text_bright, th), C().accent_text, tw);
    dl->AddText(Snap(ImVec2(bb.GetCenter().x - ts.x * 0.5f, bb.GetCenter().y - ts.y * 0.5f)), Col(tc), text);
    return changed;
}

} // namespace

// ---------------------------------------------------------------------------

float Px(float v) { return v * theme::Scale(); }

ImVec2 Snap(const ImVec2& p) { return ImVec2(ImFloor(p.x + 0.5f), ImFloor(p.y + 0.5f)); }

void SetAnimationsEnabled(bool enabled) { g_animations = enabled; }
void SetTooltipsEnabled(bool enabled) { g_tooltips = enabled; }

bool IsCapturingKeybind() {
    // A keybind that stopped being drawn (tab switched, window closed) releases the capture.
    if (g_keybind_waiting != 0 && g_keybind_seen_frame < ImGui::GetFrameCount() - 1)
        g_keybind_waiting = 0;
    return g_keybind_waiting != 0;
}

float Animate(ImGuiID id, int channel, float target, float speed) {
    const ImGuiID key = ImHashData(&channel, sizeof(channel), id);
    auto it = g_anim.find(key);
    if (it == g_anim.end()) {
        g_anim.emplace(key, target);
        return target;
    }
    float& v = it->second;
    if (!g_animations) {
        v = target;
        return v;
    }
    v += (target - v) * ImMin(1.0f, ImGui::GetIO().DeltaTime * speed);
    if (ImFabs(target - v) < 0.002f)
        v = target;
    return v;
}

// --- Layout ------------------------------------------------------------------

bool BeginPanel(const char* id, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, C().panel_bg);
    ImGui::PushStyleColor(ImGuiCol_Border, C().panel_border);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, WithAlpha(C().widget_bg, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Px(4));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(10), Px(10)));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Px(4));
    const bool visible = ImGui::BeginChild(id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(3);
    return visible;
}

void EndPanel() { ImGui::EndChild(); }

void Section(const char* title, const char* right_text) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return;

    const ImVec2 pos = window->DC.CursorPos;
    const float w = ContentWidth();
    const float line_h = ImGui::GetTextLineHeight();
    ImDrawList* dl = window->DrawList;

    theme::PushFont(theme::GetFonts().bold);
    dl->AddText(Snap(pos), Col(C().text_bright), title);
    theme::PopFont();

    if (right_text) {
        const ImVec2 ts = ImGui::CalcTextSize(right_text);
        dl->AddText(Snap(ImVec2(pos.x + w - ts.x, pos.y)), Col(C().text_dim), right_text);
    }

    const float y = IM_ROUND(pos.y + line_h + Px(4)) + 0.5f;
    dl->AddLine(ImVec2(pos.x, y), ImVec2(pos.x + w, y), Col(C().separator), 1.0f);

    ImGui::ItemSize(ImVec2(w, line_h + Px(3)));
    ImGui::ItemAdd(ImRect(pos, ImVec2(pos.x + w, pos.y + line_h)), 0);
}

float ItemWidth() {
    const float w = ContentWidth();
    return w < Px(240) ? w : (float)(int)(w * 0.72f);
}

float ItemRight() { return ContentX0() + ItemWidth(); }

float SplitWidth(int count, float total) {
    if (total <= 0.0f)
        total = ItemWidth();
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    return (float)(int)((total - spacing * (count - 1)) / count);
}

void SameLineAt(float x) {
    ImGui::SameLine();
    Win()->DC.CursorPos.x = ContentX0() + x;
}

void Spacing(float px) { Win()->DC.CursorPos.y += Px(px); }

// --- Controls ------------------------------------------------------------------

bool Checkbox(const char* label, bool* v) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    const float box = Px(15);
    const float gap = Px(6);
    const float h = ImMax(box, label_size.y);
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect bb(pos, ImVec2(pos.x + box + (label_size.x > 0.0f ? gap + label_size.x : 0.0f), pos.y + h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) {
        *v = !*v;
        ImGui::MarkItemEdited(id);
    }

    const float t = Animate(id, 0, *v ? 1.0f : 0.0f, 16.0f);
    const float th = Animate(id, 1, hovered ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    const ImRect b(ImVec2(pos.x, pos.y + (h - box) * 0.5f), ImVec2(pos.x + box, pos.y + (h - box) * 0.5f + box));
    const float r = Px(3);
    dl->AddRectFilled(b.Min, b.Max, Col(Lerp(C().widget_bg, C().widget_bg_hover, th)), r);
    dl->AddRect(b.Min, b.Max, Col(Lerp(C().widget_border, C().accent, 0.35f * th)), r);
    if (t > 0.001f) {
        dl->AddRectFilled(b.Min, b.Max, Col(C().accent, t), r);
        const float pad = box * 0.2f;
        ImGui::RenderCheckMark(dl, ImVec2(b.Min.x + pad, b.Min.y + pad), Col(ImVec4(1, 1, 1, 1), t), box - pad * 2.0f);
    }

    if (label_size.x > 0.0f) {
        const ImVec4 lc = Lerp(C().text, C().accent_text, ImMax(t, th * 0.45f));
        DrawLabel(dl, ImVec2(b.Max.x + gap, pos.y + (h - label_size.y) * 0.5f), Col(lc), label);
    }
    return pressed;
}

const char* KeyName(int key) {
    switch (key) {
    case ImGuiKey_None:        return "None";
    case ImGuiKey_LeftShift:   return "LShift";
    case ImGuiKey_RightShift:  return "RShift";
    case ImGuiKey_LeftCtrl:    return "LCtrl";
    case ImGuiKey_RightCtrl:   return "RCtrl";
    case ImGuiKey_LeftAlt:     return "LAlt";
    case ImGuiKey_RightAlt:    return "RAlt";
    case ImGuiKey_MouseRight:  return "Mouse2";
    case ImGuiKey_MouseMiddle: return "Mouse3";
    case ImGuiKey_MouseX1:     return "Mouse4";
    case ImGuiKey_MouseX2:     return "Mouse5";
    default:                   return ImGui::GetKeyName(static_cast<ImGuiKey>(key));
    }
}

bool Keybind(const char* str_id, int* key) { return KeybindImpl(str_id, key, 0.0f); }

static bool RightAlignedKeybind(const char* label, int* key) {
    const char* text = KeyName(*key);
    const float w = ImMax(Px(48), ImGui::CalcTextSize(text).x + Px(16));
    ImGui::SameLine();
    Win()->DC.CursorPos.x = ImMax(Win()->DC.CursorPos.x, ItemRight() - w);
    ImGui::PushID(label);
    const bool changed = KeybindImpl("##key", key, ImGui::GetTextLineHeight());
    ImGui::PopID();
    return changed;
}

bool CheckboxKeybind(const char* label, bool* v, int* key) {
    bool changed = Checkbox(label, v);
    changed |= RightAlignedKeybind(label, key);
    return changed;
}

bool LabelKeybind(const char* label, int* key) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;
    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    ImGui::ItemSize(ts);
    ImGui::ItemAdd(ImRect(pos, ImVec2(pos.x + ts.x, pos.y + ts.y)), 0);
    DrawLabel(window->DrawList, pos, Col(C().text), label);
    return RightAlignedKeybind(label, key);
}

bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
    float frac = (v_max > v_min) ? ImSaturate((*v - v_min) / (v_max - v_min)) : 0.0f;
    char buf[64];
    std::snprintf(buf, sizeof(buf), format, *v);
    if (!SliderBar(label, &frac, buf, 0))
        return false;
    *v = v_min + frac * (v_max - v_min);
    return true;
}

bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format) {
    const int range = v_max - v_min;
    float frac = range > 0 ? ImSaturate(float(*v - v_min) / range) : 0.0f;
    char buf[64];
    std::snprintf(buf, sizeof(buf), format, *v);
    if (!SliderBar(label, &frac, buf, range <= 1000 ? range : 0))
        return false;
    const int nv = v_min + (int)(frac * range + 0.5f);
    if (nv == *v)
        return false;
    *v = nv;
    return true;
}

bool SliderIndex(const char* label, int* index, const char* const labels[], int count) {
    if (count <= 0)
        return false;
    *index = ImClamp(*index, 0, count - 1);
    float frac = count > 1 ? float(*index) / (count - 1) : 0.0f;
    if (!SliderBar(label, &frac, labels[*index], count - 1))
        return false;
    const int ni = (int)(frac * (count - 1) + 0.5f);
    if (ni == *index)
        return false;
    *index = ni;
    return true;
}

bool Combo(const char* label, int* current, const char* const items[], int count) {
    ImGuiID id = 0;
    ImRect frame;
    const char* preview = (*current >= 0 && *current < count) ? items[*current] : "";
    const bool pressed = ComboFrame(label, preview, &id, &frame);
    if (id == 0)
        return false;

    const ImGuiID popup_id = ImHashStr("##dropdown", 0, id);
    if (pressed && !ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None))
        ImGui::OpenPopupEx(popup_id);

    bool changed = false;
    if (BeginDropdown(popup_id, frame.Min, frame.Max)) {
        for (int i = 0; i < count; ++i) {
            ImGui::PushID(i);
            if (DropdownItem(items[i], i == *current)) {
                changed = *current != i;
                *current = i;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        EndDropdown();
    }
    return changed;
}

bool MultiCombo(const char* label, bool* selected, const char* const items[], int count) {
    std::string preview;
    for (int i = 0; i < count; ++i) {
        if (!selected[i])
            continue;
        if (!preview.empty())
            preview += ", ";
        preview += items[i];
    }
    if (preview.empty())
        preview = "None";

    ImGuiID id = 0;
    ImRect frame;
    const bool pressed = ComboFrame(label, preview.c_str(), &id, &frame);
    if (id == 0)
        return false;

    const ImGuiID popup_id = ImHashStr("##dropdown", 0, id);
    if (pressed && !ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None))
        ImGui::OpenPopupEx(popup_id);

    bool changed = false;
    if (BeginDropdown(popup_id, frame.Min, frame.Max)) {
        for (int i = 0; i < count; ++i) {
            ImGui::PushID(i);
            if (DropdownItem(items[i], selected[i], true)) {
                selected[i] = !selected[i];
                changed = true;
            }
            ImGui::PopID();
        }
        EndDropdown();
    }
    return changed;
}

bool InputText(const char* id, const char* hint, char* buf, size_t buf_size, float width, Icon icon,
               ImGuiInputTextFlags flags) {
    float w = width;
    if (w == 0.0f)
        w = ItemWidth();
    else if (w < 0.0f)
        w = ImMax(1.0f, ImGui::GetContentRegionAvail().x + w);

    const float icon_w = icon != Icon::None ? Px(18) : 0.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Px(8), Px(3)));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Px(3));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, C().widget_bg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, C().widget_bg_hover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, C().widget_bg_hover);
    ImGui::PushStyleColor(ImGuiCol_Border, C().widget_border);
    ImGui::PushStyleColor(ImGuiCol_Text, C().text_bright);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, C().text_faint);

    if (icon_w > 0.0f)  // leave room on the left for the icon
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Px(8) + icon_w, Px(3)));
    ImGui::SetNextItemWidth(w);
    const bool changed = ImGui::InputTextWithHint(id, hint, buf, buf_size, flags);
    if (icon_w > 0.0f)
        ImGui::PopStyleVar();

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(3);

    const ImGuiID item_id = ImGui::GetItemID();
    const ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
    const float ta = Animate(item_id, 0, ImGui::IsItemActive() ? 1.0f : 0.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (ta > 0.001f)
        dl->AddRect(mn, mx, Col(C().accent, 0.85f * ta), Px(3));
    if (icon_w > 0.0f)
        DrawIcon(dl, icon, ImVec2(mn.x + Px(8) + icon_w * 0.35f, (mn.y + mx.y) * 0.5f), Px(11),
                 Col(Lerp(C().text_dim, C().accent_text, ta)));
    return changed;
}

bool Button(const char* label, const ImVec2& size_arg, ButtonStyle style) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    theme::PushFont(theme::GetFonts().bold);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    const ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + Px(22), Px(22));
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) {
        theme::PopFont();
        return false;
    }

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float th = Animate(id, 0, hovered ? 1.0f : 0.0f);
    const float tp = Animate(id, 1, held ? 1.0f : 0.0f, 22.0f);

    TintColors cols{};
    switch (style) {
    case ButtonStyle::Default:
        cols.bg = Col(Lerp(Lerp(C().widget_bg, C().widget_bg_hover, th), C().widget_bg, tp));
        cols.border = Col(Lerp(C().widget_border, C().text_faint, th * 0.6f));
        cols.text = Col(Lerp(C().text, C().text_bright, th));
        break;
    case ButtonStyle::Accent:  cols = Tinted(C().accent, th, tp); break;
    case ButtonStyle::Danger:  cols = Tinted(C().danger, th, tp); break;
    case ButtonStyle::Warning: cols = Tinted(C().warning, th, tp); break;
    }

    ImDrawList* dl = window->DrawList;
    const float r = Px(3);
    dl->AddRectFilled(bb.Min, bb.Max, cols.bg, r);
    dl->AddRect(bb.Min, bb.Max, cols.border, r);
    DrawLabel(dl, ImVec2(bb.GetCenter().x - label_size.x * 0.5f, bb.GetCenter().y - label_size.y * 0.5f), cols.text, label);
    theme::PopFont();
    return pressed;
}

bool ColorEdit(const char* label, float rgb[3]) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImGuiID popup_id = ImHashStr("##picker", 0, id);
    const float line_h = ImGui::GetTextLineHeight();
    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 swatch_size(Px(30), Px(14));
    const ImRect swatch(ImVec2(ItemRight() - swatch_size.x, pos.y + (line_h - swatch_size.y) * 0.5f),
                        ImVec2(ItemRight(), pos.y + (line_h + swatch_size.y) * 0.5f));
    const ImRect bb(pos, ImVec2(swatch.Max.x, pos.y + line_h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id, &swatch))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(swatch, id, &hovered, &held);
    if (pressed)
        ImGui::OpenPopupEx(popup_id);

    ImDrawList* dl = window->DrawList;
    DrawLabel(dl, pos, Col(C().text), label);
    const float r = Px(3);
    dl->AddRectFilled(swatch.Min, swatch.Max, Col(ImVec4(rgb[0], rgb[1], rgb[2], 1.0f)), r);
    dl->AddRect(swatch.Min, swatch.Max, Col(hovered ? C().text_dim : C().widget_border), r);

    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(swatch.Max.x, swatch.Max.y + Px(4)), ImGuiCond_Appearing, ImVec2(1.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(8), Px(8)));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Px(4));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C().popup_bg);
    ImGui::PushStyleColor(ImGuiCol_Border, C().widget_border);
    const bool open = ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    if (open) {
        ImGui::SetNextItemWidth(Px(170));
        changed = ImGui::ColorPicker3("##picker", rgb,
                                      ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
                                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
        TextDim("#%02X%02X%02X", (int)(rgb[0] * 255 + 0.5f), (int)(rgb[1] * 255 + 0.5f), (int)(rgb[2] * 255 + 0.5f));
        ImGui::EndPopup();
    }
    return changed;
}

bool AccentPresets(const char* id, float rgb[3]) {
    static const ImVec4 kPresets[] = {
        ImVec4(0.000f, 0.361f, 0.816f, 1), ImVec4(0.486f, 0.302f, 1.000f, 1), ImVec4(0.859f, 0.243f, 0.549f, 1),
        ImVec4(0.839f, 0.212f, 0.251f, 1), ImVec4(0.910f, 0.502f, 0.125f, 1), ImVec4(0.180f, 0.667f, 0.376f, 1),
        ImVec4(0.086f, 0.627f, 0.667f, 1),
    };
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    ImGui::PushID(id);
    const float d = Px(14), gap = Px(8);
    const ImVec2 start = window->DC.CursorPos;
    bool changed = false;
    for (int i = 0; i < IM_ARRAYSIZE(kPresets); ++i) {
        const ImVec4& p = kPresets[i];
        const ImVec2 mn(start.x + i * (d + gap), start.y);
        const ImRect bb(mn, ImVec2(mn.x + d, mn.y + d));
        const ImGuiID pid = window->GetID(i);
        if (i > 0)
            ImGui::SameLine(0.0f, gap);
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, pid))
            continue;
        bool hovered, held;
        if (ImGui::ButtonBehavior(bb, pid, &hovered, &held)) {
            rgb[0] = p.x;
            rgb[1] = p.y;
            rgb[2] = p.z;
            changed = true;
        }
        const bool active = ImFabs(rgb[0] - p.x) + ImFabs(rgb[1] - p.y) + ImFabs(rgb[2] - p.z) < 0.02f;
        const float th = Animate(pid, 0, hovered ? 1.0f : 0.0f);
        window->DrawList->AddCircleFilled(bb.GetCenter(), d * 0.5f - Px(1) * (1.0f - th), Col(p));
        if (active)
            window->DrawList->AddCircle(bb.GetCenter(), d * 0.5f + Px(2.5f), Col(C().text_bright, 0.9f), 0, Px(1.5f));
    }
    ImGui::PopID();
    return changed;
}

bool Tab(const char* label, Icon icon, bool selected) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    const float icon_sz = Px(12), pad = Px(8), gap = Px(6), h = Px(26);
    const float w = pad + icon_sz + gap + ts.x + pad;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float ts_sel = Animate(id, 0, selected ? 1.0f : 0.0f);
    const float th = Animate(id, 1, hovered ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    const float r = Px(4);
    if (ts_sel > 0.001f) {
        dl->AddRectFilled(bb.Min, bb.Max, Col(C().accent, 0.16f * ts_sel), r);
        dl->AddRect(bb.Min, bb.Max, Col(C().accent, 0.42f * ts_sel), r);
    }
    if (th > 0.001f && !selected)
        dl->AddRectFilled(bb.Min, bb.Max, Col(C().widget_bg_hover, 0.6f * th), r);

    const ImVec4 col = Lerp(Lerp(C().text_dim, C().text_bright, th * 0.8f), C().accent_text, ts_sel);
    DrawIcon(dl, icon, ImVec2(bb.Min.x + pad + icon_sz * 0.5f, bb.GetCenter().y), icon_sz, Col(col));
    DrawLabel(dl, ImVec2(bb.Min.x + pad + icon_sz + gap, bb.GetCenter().y - ts.y * 0.5f), Col(col), label);
    return pressed;
}

bool ListRow(const char* id_str, const char* text, bool selected, Icon icon, const char* right_text) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(id_str);
    const float h = Px(26);
    const float w = ContentWidth();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float ts = Animate(id, 0, selected ? 1.0f : 0.0f);
    const float th = Animate(id, 1, hovered ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    const float r = Px(3);
    dl->AddRectFilled(bb.Min, bb.Max, Col(C().widget_bg_hover, 0.7f * th * (1.0f - ts)), r);
    if (ts > 0.001f) {
        dl->AddRectFilled(bb.Min, bb.Max, Col(C().accent, 0.14f * ts), r);
        dl->AddRect(bb.Min, bb.Max, Col(C().accent, 0.40f * ts), r);
    }

    const ImVec4 col = Lerp(Lerp(C().text, C().text_bright, th), C().accent_text, ts);
    float x = bb.Min.x + Px(8);
    if (icon != Icon::None) {
        DrawIcon(dl, icon, ImVec2(x + Px(6), bb.GetCenter().y), Px(12), Col(Lerp(C().text_dim, C().accent_text, ts)));
        x += Px(20);
    }
    const float line_h = ImGui::GetTextLineHeight();
    dl->AddText(Snap(ImVec2(x, bb.GetCenter().y - line_h * 0.5f)), Col(col), text);
    if (right_text) {
        const ImVec2 rs = ImGui::CalcTextSize(right_text);
        dl->AddText(Snap(ImVec2(bb.Max.x - Px(8) - rs.x, bb.GetCenter().y - rs.y * 0.5f)), Col(C().text_dim), right_text);
    }
    return pressed;
}

// --- Text ------------------------------------------------------------------------

void TextDim(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, C().text_dim);
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

void TextColored(const ImVec4& col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

bool KeyValue(const char* key, const char* value, const ImVec4* value_col) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(key);
    const float line_h = ImGui::GetTextLineHeight();
    const float w = ContentWidth();
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + line_h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float th = Animate(id, 0, hovered ? 1.0f : 0.0f);

    ImDrawList* dl = window->DrawList;
    if (th > 0.001f)
        dl->AddRectFilled(ImVec2(bb.Min.x - Px(4), bb.Min.y - Px(2)), ImVec2(bb.Max.x + Px(4), bb.Max.y + Px(2)),
                          Col(C().widget_bg_hover, 0.6f * th), Px(3));
    DrawLabel(dl, pos, Col(C().text_dim), key);
    const ImVec2 vs = ImGui::CalcTextSize(value);
    dl->AddText(Snap(ImVec2(bb.Max.x - vs.x, pos.y)), Col(value_col ? *value_col : C().text_bright), value);
    return pressed;
}

float BadgeWidth(const char* text) {
    ImFont* f = theme::GetFonts().caption;
    return f->CalcTextSizeA(theme::FontSize(f), FLT_MAX, 0.0f, text).x + Px(12);
}

float DrawBadge(ImDrawList* dl, const ImVec2& pos, const char* text, const ImVec4& col) {
    ImFont* f = theme::GetFonts().caption;
    const float fs = theme::FontSize(f);
    const ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0.0f, text);
    const float h = Px(16), w = ts.x + Px(12);
    const ImVec2 mx(pos.x + w, pos.y + h);
    dl->AddRectFilled(pos, mx, Col(col, 0.14f), h * 0.5f);
    dl->AddRect(pos, mx, Col(col, 0.40f), h * 0.5f);
    dl->AddText(f, fs, Snap(ImVec2(pos.x + Px(6), pos.y + (h - ts.y) * 0.5f)), Col(Lerp(col, ImVec4(1, 1, 1, 1), 0.2f)), text);
    return w;
}

void Badge(const char* text, const ImVec4& col) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return;
    const float line_h = ImGui::GetTextLineHeight();
    const float h = Px(16), w = BadgeWidth(text);
    const ImVec2 pos = window->DC.CursorPos;
    ImGui::ItemSize(ImVec2(w, line_h));
    const ImRect bb(ImVec2(pos.x, pos.y + (line_h - h) * 0.5f), ImVec2(pos.x + w, pos.y + (line_h + h) * 0.5f));
    if (!ImGui::ItemAdd(bb, 0))
        return;
    DrawBadge(window->DrawList, bb.Min, text, col);
}

void Tooltip(const char* text) {
    if (g_tooltips && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled))
        ShowTooltip(text);
}

void CenteredMessage(const char* text) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 ts = ImGui::CalcTextSize(text);
    const ImVec2 cur = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cur.x + (avail.x - ts.x) * 0.5f, cur.y + (avail.y - ts.y) * 0.5f));
    TextDim("%s", text);
}

// --- Popups ----------------------------------------------------------------------

bool BeginModal(const char* name, const ImVec2& center) {
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(18), Px(16)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, Px(6));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Px(6));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C().panel_bg);
    ImGui::PushStyleColor(ImGuiCol_Border, C().window_border);
    const bool open = ImGui::BeginPopupModal(name, nullptr,
                                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
    return open;
}

void EndModal() { ImGui::EndPopup(); }

bool BeginDropdown(ImGuiID popup_id, const ImVec2& anchor_min, const ImVec2& anchor_max, float max_height) {
    if (!ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None))
        return false;

    const float w = anchor_max.x - anchor_min.x;
    if (max_height <= 0.0f)
        max_height = Px(260);
    ImGui::SetNextWindowPos(ImVec2(anchor_min.x, anchor_max.y + Px(3)));
    ImGui::SetNextWindowSizeConstraints(ImVec2(w, 0.0f), ImVec2(w, max_height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Px(4), Px(4)));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, Px(4));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Px(4));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C().popup_bg);
    ImGui::PushStyleColor(ImGuiCol_Border, C().widget_border);
    const bool open = ImGui::BeginPopupEx(popup_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar |
                                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                                        ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
    if (open)
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, Px(1)));
    return open;
}

void EndDropdown() {
    ImGui::PopStyleVar();
    ImGui::EndPopup();
}

bool DropdownItem(const char* label, bool selected, bool show_check) {
    ImGuiWindow* window = Win();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 ts = ImGui::CalcTextSize(label, nullptr, true);
    const float h = Px(22);
    const float w = ImMax(ContentWidth(), ts.x + Px(34));
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + w, pos.y + h));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float th = Animate(id, 0, hovered ? 1.0f : 0.0f, 20.0f);

    ImDrawList* dl = window->DrawList;
    if (th > 0.001f)
        dl->AddRectFilled(bb.Min, bb.Max, Col(C().widget_bg_hover, th), Px(3));

    const ImVec4 col = selected ? C().accent_text : Lerp(C().text, C().text_bright, th);
    DrawLabel(dl, ImVec2(bb.Min.x + Px(8), bb.GetCenter().y - ts.y * 0.5f), Col(col), label);
    if (show_check) {
        const float box = Px(12);
        const ImRect cb(ImVec2(bb.Max.x - Px(8) - box, bb.GetCenter().y - box * 0.5f),
                        ImVec2(bb.Max.x - Px(8), bb.GetCenter().y + box * 0.5f));
        dl->AddRectFilled(cb.Min, cb.Max, Col(selected ? C().accent : C().widget_bg), Px(3));
        if (!selected)
            dl->AddRect(cb.Min, cb.Max, Col(C().widget_border), Px(3));
        else
            ImGui::RenderCheckMark(dl, ImVec2(cb.Min.x + box * 0.2f, cb.Min.y + box * 0.2f), IM_COL32_WHITE, box * 0.6f);
    }
    return pressed;
}

} // namespace foxy::ui
