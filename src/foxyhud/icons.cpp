#include "foxyhud/icons.h"

#include <cmath>

namespace foxy {
namespace {

constexpr float kPi = 3.14159265358979f;

struct Pen {
    ImDrawList* dl;
    ImVec2 c;
    float s;
    ImU32 col;
    float t;

    ImVec2 P(float x, float y) const { return ImVec2(c.x + x * s, c.y + y * s); }
    ImVec2 Polar(float r, float a) const { return ImVec2(c.x + std::cos(a) * r * s, c.y + std::sin(a) * r * s); }

    void Line(float x0, float y0, float x1, float y1) const { dl->AddLine(P(x0, y0), P(x1, y1), col, t); }
    void Circle(float x, float y, float r) const { dl->AddCircle(P(x, y), r * s, col, 0, t); }
    void Dot(float x, float y, float r) const { dl->AddCircleFilled(P(x, y), r * s, col); }
    void Arc(float x, float y, float r, float a0, float a1) const {
        dl->PathArcTo(P(x, y), r * s, a0, a1);
        dl->PathStroke(col, 0, t);
    }
};

void DrawPerson(const Pen& p, float x, float y, float k) {
    p.Circle(x, y - 0.20f * k, 0.21f * k);
    p.Arc(x, y + 0.50f * k, 0.36f * k, kPi, 2.0f * kPi);
}

} // namespace

void DrawIcon(ImDrawList* dl, Icon icon, const ImVec2& center, float size, ImU32 col, float rotation, float thickness) {
    const Pen p{dl, center, size, col, thickness > 0.0f ? thickness : std::fmax(1.0f, size * 0.105f)};

    switch (icon) {
    case Icon::None:
        break;

    case Icon::User:
        DrawPerson(p, 0.0f, 0.0f, 1.0f);
        break;

    case Icon::Users:
        p.Circle(-0.12f, -0.16f, 0.19f);
        p.Arc(-0.12f, 0.50f, 0.33f, kPi, 2.0f * kPi);
        p.Circle(0.24f, -0.25f, 0.15f);
        p.Arc(0.24f, 0.42f, 0.26f, 1.45f * kPi, 2.0f * kPi);
        break;

    case Icon::Pencil: {
        const ImVec2 d(0.7071f, -0.7071f), n(0.7071f, 0.7071f);
        const ImVec2 tip = p.P(-0.42f, 0.42f);
        auto at = [&](float along, float side) {
            return ImVec2(tip.x + d.x * along * size + n.x * side * size, tip.y + d.y * along * size + n.y * side * size);
        };
        const float w = 0.14f;
        const ImVec2 pts[] = {tip, at(0.24f, w), at(1.08f, w), at(1.08f, -w), at(0.24f, -w)};
        dl->AddPolyline(pts, 5, col, ImDrawFlags_Closed, p.t);
        dl->AddLine(at(0.24f, w), at(0.24f, -w), col, p.t);
        dl->AddLine(at(0.86f, w), at(0.86f, -w), col, p.t);
        break;
    }

    case Icon::Eye:
        dl->PathLineTo(p.P(-0.5f, 0.0f));
        dl->PathBezierQuadraticCurveTo(p.P(0.0f, -0.62f), p.P(0.5f, 0.0f));
        dl->PathBezierQuadraticCurveTo(p.P(0.0f, 0.62f), p.P(-0.5f, 0.0f));
        dl->PathStroke(col, ImDrawFlags_Closed, p.t);
        p.Circle(0.0f, 0.0f, 0.15f);
        break;

    case Icon::Grid: {
        const float cell = 0.36f, gap = 0.14f, o = -(cell + gap * 0.5f);
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) {
                const float x0 = o + x * (cell + gap), y0 = o + y * (cell + gap);
                dl->AddRect(p.P(x0, y0), p.P(x0 + cell, y0 + cell), col, 0.08f * size, 0, p.t);
            }
        break;
    }

    case Icon::Help:
        p.Circle(0.0f, 0.0f, 0.46f);
        dl->PathArcTo(p.P(0.0f, -0.10f), 0.14f * size, 1.0f * kPi, 2.25f * kPi);
        dl->PathLineTo(p.P(0.0f, 0.06f));
        dl->PathLineTo(p.P(0.0f, 0.11f));
        dl->PathStroke(col, 0, p.t);
        dl->AddCircleFilled(p.P(0.0f, 0.25f), p.t * 0.8f, col);
        break;

    case Icon::Palette:
        p.Circle(0.0f, 0.0f, 0.46f);
        p.Dot(-0.21f, -0.10f, 0.075f);
        p.Dot(0.00f, -0.25f, 0.075f);
        p.Dot(0.21f, -0.10f, 0.075f);
        p.Circle(0.12f, 0.20f, 0.09f);
        break;

    case Icon::Gear: {
        constexpr int kTeeth = 8;
        const float step = 2.0f * kPi / kTeeth, r_out = 0.49f, r_in = 0.35f;
        for (int i = 0; i < kTeeth; ++i) {
            const float a = rotation + i * step;
            dl->PathLineTo(p.Polar(r_in, a - step * 0.26f));
            dl->PathLineTo(p.Polar(r_out, a - step * 0.15f));
            dl->PathLineTo(p.Polar(r_out, a + step * 0.15f));
            dl->PathLineTo(p.Polar(r_in, a + step * 0.26f));
        }
        dl->PathStroke(col, ImDrawFlags_Closed, p.t);
        p.Circle(0.0f, 0.0f, 0.14f);
        break;
    }

    case Icon::Search:
        p.Circle(-0.08f, -0.08f, 0.30f);
        p.Line(0.15f, 0.15f, 0.44f, 0.44f);
        break;

    case Icon::Shield:
        dl->PathLineTo(p.P(0.0f, -0.48f));
        dl->PathLineTo(p.P(0.40f, -0.32f));
        dl->PathLineTo(p.P(0.40f, 0.02f));
        dl->PathBezierQuadraticCurveTo(p.P(0.38f, 0.34f), p.P(0.0f, 0.48f));
        dl->PathBezierQuadraticCurveTo(p.P(-0.38f, 0.34f), p.P(-0.40f, 0.02f));
        dl->PathLineTo(p.P(-0.40f, -0.32f));
        dl->PathStroke(col, ImDrawFlags_Closed, p.t);
        break;

    case Icon::Chat: {
        dl->AddRect(p.P(-0.46f, -0.40f), p.P(0.46f, 0.22f), col, 0.14f * size, 0, p.t);
        const ImVec2 tail[] = {p.P(-0.20f, 0.22f), p.P(-0.28f, 0.46f), p.P(0.04f, 0.22f)};
        dl->AddPolyline(tail, 3, col, 0, p.t);
        break;
    }

    case Icon::Keyboard:
        dl->AddRect(p.P(-0.50f, -0.32f), p.P(0.50f, 0.32f), col, 0.08f * size, 0, p.t);
        for (int i = 0; i < 4; ++i)
            p.Dot(-0.28f + i * 0.186f, -0.10f, 0.05f);
        p.Line(-0.22f, 0.12f, 0.22f, 0.12f);
        break;

    case Icon::File: {
        const ImVec2 outline[] = {p.P(-0.36f, -0.48f), p.P(0.12f, -0.48f), p.P(0.38f, -0.22f), p.P(0.38f, 0.48f), p.P(-0.36f, 0.48f)};
        dl->AddPolyline(outline, 5, col, ImDrawFlags_Closed, p.t);
        const ImVec2 fold[] = {p.P(0.12f, -0.48f), p.P(0.12f, -0.22f), p.P(0.38f, -0.22f)};
        dl->AddPolyline(fold, 3, col, 0, p.t);
        break;
    }

    case Icon::List:
        for (int i = 0; i < 3; ++i) {
            const float y = -0.30f + i * 0.30f;
            p.Dot(-0.40f, y, 0.06f);
            p.Line(-0.20f, y, 0.46f, y);
        }
        break;

    case Icon::Close:
        p.Line(-0.34f, -0.34f, 0.34f, 0.34f);
        p.Line(0.34f, -0.34f, -0.34f, 0.34f);
        break;

    case Icon::ChevronDown: {
        const ImVec2 pts[] = {p.P(-0.32f, -0.14f), p.P(0.0f, 0.16f), p.P(0.32f, -0.14f)};
        dl->AddPolyline(pts, 3, col, 0, p.t * 1.2f);
        break;
    }
    }
}

} // namespace foxy
