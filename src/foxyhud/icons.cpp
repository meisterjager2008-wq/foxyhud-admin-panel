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

void DrawGear(const Pen& p, int teeth, float r_out, float r_in, float hole, float rotation) {
    const float step = 2.0f * kPi / teeth;
    for (int i = 0; i < teeth; ++i) {
        const float a = rotation + i * step;
        p.dl->PathLineTo(p.Polar(r_in, a - step * 0.26f));
        p.dl->PathLineTo(p.Polar(r_out, a - step * 0.15f));
        p.dl->PathLineTo(p.Polar(r_out, a + step * 0.15f));
        p.dl->PathLineTo(p.Polar(r_in, a + step * 0.26f));
    }
    p.dl->PathStroke(p.col, ImDrawFlags_Closed, p.t);
    p.Circle(0.0f, 0.0f, hole);
}

// Smooth curve (Catmull-Rom) through `pts`, appended to the current path.
void SmoothPath(const Pen& p, const ImVec2* pts, int n, bool skip_first) {
    constexpr int kSteps = 6;
    for (int i = 0; i < n - 1; ++i) {
        const ImVec2& p0 = pts[i > 0 ? i - 1 : i];
        const ImVec2& p1 = pts[i];
        const ImVec2& p2 = pts[i + 1];
        const ImVec2& p3 = pts[i + 2 < n ? i + 2 : i + 1];
        for (int k = (i == 0 && !skip_first) ? 0 : 1; k <= kSteps; ++k) {
            const float t = static_cast<float>(k) / kSteps, t2 = t * t, t3 = t2 * t;
            const float x = 0.5f * (2 * p1.x + (p2.x - p0.x) * t + (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 +
                                    (3 * p1.x - p0.x - 3 * p2.x + p3.x) * t3);
            const float y = 0.5f * (2 * p1.y + (p2.y - p0.y) * t + (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 +
                                    (3 * p1.y - p0.y - 3 * p2.y + p3.y) * t3);
            p.dl->PathLineTo(p.P(x, y));
        }
    }
}

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

    case Icon::Gear:
        DrawGear(p, 8, 0.49f, 0.35f, 0.14f, rotation);
        break;

    case Icon::Cog:
        DrawGear(p, 6, 0.48f, 0.32f, 0.16f, rotation + kPi / 6.0f);
        break;

    case Icon::Gauge:
        p.Circle(0.0f, 0.0f, 0.46f);
        p.Arc(0.0f, 0.06f, 0.26f, 1.15f * kPi, 1.85f * kPi);
        p.Line(0.0f, 0.06f, 0.16f, -0.12f);
        p.Dot(0.0f, 0.06f, 0.07f);
        break;

    case Icon::Crosshair:
        p.Line(0.0f, -0.48f, 0.0f, -0.20f);
        p.Line(0.0f, 0.20f, 0.0f, 0.48f);
        p.Line(-0.48f, 0.0f, -0.20f, 0.0f);
        p.Line(0.20f, 0.0f, 0.48f, 0.0f);
        p.Dot(0.0f, 0.0f, 0.07f);
        break;

    case Icon::Target:
        p.Circle(0.0f, 0.0f, 0.45f);
        p.Circle(0.0f, 0.0f, 0.24f);
        p.Dot(0.0f, 0.0f, 0.07f);
        break;

    case Icon::Karambit: {
        // Traced from a karambit silhouette turned upside down: finger ring at the
        // top left, handle to the right, blade curving down to the tip.
        static const ImVec2 kBack[] = {  // ring -> tip, along the back of the blade
            {-0.283f, -0.408f}, {-0.099f, -0.395f}, {0.033f, -0.342f}, {0.112f, -0.289f}, {0.164f, -0.237f},
            {0.243f, -0.197f}, {0.322f, -0.158f}, {0.375f, -0.092f}, {0.414f, -0.026f}, {0.454f, 0.066f},
            {0.480f, 0.158f}, {0.493f, 0.263f}, {0.475f, 0.355f}, {0.449f, 0.421f},
        };
        static const ImVec2 kEdge[] = {  // tip -> ring, along the cutting edge and finger notch
            {0.449f, 0.421f}, {0.428f, 0.368f}, {0.414f, 0.289f}, {0.401f, 0.237f}, {0.388f, 0.184f},
            {0.362f, 0.132f}, {0.322f, 0.105f}, {0.283f, 0.053f}, {0.204f, 0.000f}, {0.151f, -0.026f},
            {0.112f, -0.053f}, {0.099f, -0.132f}, {0.086f, -0.158f}, {0.007f, -0.184f}, {-0.059f, -0.211f},
            {-0.178f, -0.229f}, {-0.270f, -0.224f},
        };
        SmoothPath(p, kBack, IM_ARRAYSIZE(kBack), false);
        SmoothPath(p, kEdge, IM_ARRAYSIZE(kEdge), true);
        // The blade is slim; at tab size (~12 px) outline it too so it stays readable.
        const float boost = size < 24.0f ? (24.0f - size) / 24.0f : 0.0f;
        if (boost > 0.0f)
            dl->AddPolyline(dl->_Path.Data, dl->_Path.Size, col, ImDrawFlags_Closed, 1.3f * boost);
        dl->PathFillConcave(col);
        const float ring_r = 0.084f * size + 0.35f * boost, ring_t = 0.064f * size + 0.9f * boost;
        dl->AddCircle(p.P(-0.360f, -0.312f), ring_r, col, 0, ring_t);  // finger ring
        break;
    }

    case Icon::Globe:
        p.Circle(0.0f, 0.0f, 0.46f);
        dl->AddEllipse(center, ImVec2(0.20f * size, 0.46f * size), col, 0.0f, 0, p.t);
        p.Line(-0.46f, 0.0f, 0.46f, 0.0f);
        p.Line(-0.38f, -0.24f, 0.38f, -0.24f);
        p.Line(-0.38f, 0.24f, 0.38f, 0.24f);
        break;

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

    case Icon::Flag: {
        p.Line(-0.36f, -0.48f, -0.36f, 0.48f);
        const ImVec2 cloth[] = {p.P(-0.36f, -0.42f), p.P(0.42f, -0.42f), p.P(0.22f, -0.18f), p.P(0.42f, 0.06f),
                                p.P(-0.36f, 0.06f)};
        dl->AddPolyline(cloth, 5, col, 0, p.t);
        break;
    }

    case Icon::Ban:
        p.Circle(0.0f, 0.0f, 0.44f);
        p.Line(-0.31f, -0.31f, 0.31f, 0.31f);
        break;

    case Icon::Sliders: {
        const float knobs[] = {0.18f, -0.22f, 0.08f};
        for (int i = 0; i < 3; ++i) {
            const float y = -0.32f + i * 0.32f;
            p.Line(-0.48f, y, 0.48f, y);
            p.Dot(knobs[i], y, 0.12f);
        }
        break;
    }

    case Icon::EyeFrame: {
        const float e = 0.48f, l = 0.17f;
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sy = -1; sy <= 1; sy += 2) {
                const ImVec2 corner[] = {p.P(sx * e, sy * (e - l)), p.P(sx * e, sy * e), p.P(sx * (e - l), sy * e)};
                dl->AddPolyline(corner, 3, col, 0, p.t);
            }
        dl->PathLineTo(p.P(-0.30f, 0.0f));
        dl->PathBezierQuadraticCurveTo(p.P(0.0f, -0.36f), p.P(0.30f, 0.0f));
        dl->PathBezierQuadraticCurveTo(p.P(0.0f, 0.36f), p.P(-0.30f, 0.0f));
        dl->PathStroke(col, ImDrawFlags_Closed, p.t);
        p.Dot(0.0f, 0.0f, 0.08f);
        break;
    }
    }
}

} // namespace foxy
