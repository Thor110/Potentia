// Sieve hallway — camera, projection and near-plane clipping.
#pragma once

#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace hallway {

struct Vec3
{
    float x = 0, y = 0, z = 0;
    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline Vec3 normalize(Vec3 v)
{
    const float l = std::sqrt(dot(v, v));
    return l > 0 ? v * (1.0f / l) : v;
}

struct Point2
{
    float x, y;
};

class Camera
{
public:
    Vec3 pos{0, 1.6f, 1.0f};
    float yaw = 0;   // radians; 0 looks down +z (along the hallway)
    float pitch = 0; // radians; positive looks up
    float fov_y = 1.2f;
    float near_z = 0.05f;

    void update(int width, int height)
    {
        w_ = float(width);
        h_ = float(height);
        fwd_ = {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)};
        right_ = normalize(cross(Vec3{0, 1, 0}, fwd_));
        up_ = cross(fwd_, right_);
        focal_ = (h_ * 0.5f) / std::tan(fov_y * 0.5f);
    }

    Vec3 forward() const { return fwd_; }
    Vec3 to_camera(Vec3 p) const
    {
        const Vec3 r = p - pos;
        return {dot(r, right_), dot(r, up_), dot(r, fwd_)};
    }
    Point2 project_camera(Vec3 c) const { return {w_ * 0.5f + c.x / c.z * focal_, h_ * 0.5f - c.y / c.z * focal_}; }

    // Projects a world segment, clipped to the near plane. Also returns the depth of its midpoint.
    std::optional<std::pair<std::pair<Point2, Point2>, float>> project_segment(Vec3 a, Vec3 b) const
    {
        Vec3 ca = to_camera(a), cb = to_camera(b);
        if (ca.z < near_z && cb.z < near_z) return std::nullopt;
        if (ca.z < near_z) ca = ca + (cb - ca) * ((near_z - ca.z) / (cb.z - ca.z));
        else if (cb.z < near_z) cb = cb + (ca - cb) * ((near_z - cb.z) / (ca.z - cb.z));
        return std::make_pair(std::make_pair(project_camera(ca), project_camera(cb)), (ca.z + cb.z) * 0.5f);
    }

    // Conservative view-frustum test for an axis-aligned box: false only when all eight corners
    // lie outside one of the near, left, right, top or bottom planes (so nothing of it can show).
    bool box_visible(Vec3 lo, Vec3 hi) const
    {
        const float tx = (w_ * 0.5f) / focal_ * 1.05f, ty = (h_ * 0.5f) / focal_ * 1.05f;
        int near_out = 0, left = 0, right = 0, below = 0, above = 0;
        for (int i = 0; i < 8; ++i)
        {
            const Vec3 c = to_camera({i & 1 ? hi.x : lo.x, i & 2 ? hi.y : lo.y, i & 4 ? hi.z : lo.z});
            if (c.z < near_z) ++near_out;
            if (c.x < -c.z * tx) ++left;
            if (c.x > c.z * tx) ++right;
            if (c.y < -c.z * ty) ++below;
            if (c.y > c.z * ty) ++above;
        }
        return near_out < 8 && left < 8 && right < 8 && below < 8 && above < 8;
    }

    // Projects a convex polygon, clipped to the near plane (Sutherland-Hodgman).
    std::vector<Point2> project_polygon(const std::vector<Vec3>& world) const
    {
        std::vector<Vec3> in;
        for (const Vec3& p : world) in.push_back(to_camera(p));
        std::vector<Vec3> out;
        for (size_t i = 0; i < in.size(); ++i)
        {
            const Vec3 a = in[i], b = in[(i + 1) % in.size()];
            const bool ia = a.z >= near_z, ib = b.z >= near_z;
            if (ia) out.push_back(a);
            if (ia != ib) out.push_back(a + (b - a) * ((near_z - a.z) / (b.z - a.z)));
        }
        std::vector<Point2> screen;
        for (const Vec3& c : out) screen.push_back(project_camera(c));
        return clip_to_view(screen);
    }

private:
    // Clips a convex screen polygon to a rectangle a little larger than the window, so a surface
    // right beside the camera never becomes a triangle millions of pixels wide.
    std::vector<Point2> clip_to_view(std::vector<Point2> poly) const
    {
        const float lo_x = -w_, hi_x = 2 * w_, lo_y = -h_, hi_y = 2 * h_;
        auto clip = [&](auto inside, auto intersect) {
            std::vector<Point2> out;
            for (size_t i = 0; i < poly.size(); ++i)
            {
                const Point2 a = poly[i], b = poly[(i + 1) % poly.size()];
                if (inside(a)) out.push_back(a);
                if (inside(a) != inside(b)) out.push_back(intersect(a, b));
            }
            poly = std::move(out);
        };
        auto at_x = [](Point2 a, Point2 b, float x) { return Point2{x, a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x)}; };
        auto at_y = [](Point2 a, Point2 b, float y) { return Point2{a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y), y}; };
        clip([&](Point2 p) { return p.x >= lo_x; }, [&](Point2 a, Point2 b) { return at_x(a, b, lo_x); });
        clip([&](Point2 p) { return p.x <= hi_x; }, [&](Point2 a, Point2 b) { return at_x(a, b, hi_x); });
        clip([&](Point2 p) { return p.y >= lo_y; }, [&](Point2 a, Point2 b) { return at_y(a, b, lo_y); });
        clip([&](Point2 p) { return p.y <= hi_y; }, [&](Point2 a, Point2 b) { return at_y(a, b, hi_y); });
        return poly;
    }

    float w_ = 1, h_ = 1, focal_ = 1;
    Vec3 fwd_{0, 0, 1}, right_{1, 0, 0}, up_{0, 1, 0};
};

} // namespace hallway
