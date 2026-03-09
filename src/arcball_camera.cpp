#include "arcball_camera.h"

#include <cmath>
#include <algorithm>

namespace globe {

// ── Vec3 utilities ────────────────────────────────────────────────

float length(Vec3 v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3 normalize(Vec3 v) {
    float len = length(v);
    if (len < 1e-8f) return {0.0f, 0.0f, 0.0f};
    return {v.x / len, v.y / len, v.z / len};
}

float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// ── Arcball sphere mapping ────────────────────────────────────────

Vec3 map_to_sphere(float screen_x, float screen_y,
                   float viewport_w, float viewport_h) {
    // Guard: invalid viewport → return safe default (front of sphere)
    if (viewport_w <= 0.0f || viewport_h <= 0.0f ||
        std::isnan(viewport_w) || std::isnan(viewport_h)) {
        return {0.0f, 0.0f, 1.0f};
    }

    // Guard: NaN input coordinates
    if (std::isnan(screen_x) || std::isnan(screen_y)) {
        return {0.0f, 0.0f, 1.0f};
    }

    // Normalize to [-1, 1] range using the smaller dimension as radius
    float radius = std::min(viewport_w, viewport_h) / 2.0f;
    float x = (screen_x - viewport_w / 2.0f) / radius;
    float y = (viewport_h / 2.0f - screen_y) / radius;   // flip Y (screen→math)

    float dist_sq = x * x + y * y;

    Vec3 result;
    if (dist_sq <= 1.0f) {
        // Inside the sphere — compute z from sphere equation: x²+y²+z²=1
        result = {x, y, std::sqrt(1.0f - dist_sq)};
    } else {
        // Outside the sphere — project onto the equator (z=0) and normalize
        float dist = std::sqrt(dist_sq);
        result = {x / dist, y / dist, 0.0f};
    }

    return result;
}

// ── Rotation from two sphere points ───────────────────────────────

bool compute_rotation(Vec3 from, Vec3 to, Vec3& axis, float& angle) {
    // Reject NaN inputs
    if (std::isnan(from.x) || std::isnan(from.y) || std::isnan(from.z) ||
        std::isnan(to.x)   || std::isnan(to.y)   || std::isnan(to.z)) {
        return false;
    }

    // Reject zero-length vectors
    if (length(from) < 1e-8f || length(to) < 1e-8f) {
        return false;
    }

    // Rotation axis = cross product of the two points
    Vec3 raw_axis = cross(from, to);
    float axis_len = length(raw_axis);

    if (axis_len < 1e-8f) {
        // Identical (or antipodal) points — no meaningful rotation
        return false;
    }

    axis = normalize(raw_axis);

    // Angle = arcsin(|cross|) for unit vectors, but use atan2 for stability
    float d = dot(from, to);
    d = std::clamp(d, -1.0f, 1.0f);
    angle = std::acos(d);

    return angle > 1e-8f;
}

// ── Lat/Lon to Cartesian ──────────────────────────────────────────

Vec3 latlon_to_cartesian(float lat_deg, float lon_deg, float radius) {
    constexpr float DEG2RAD = 3.14159265358979323846f / 180.0f;

    float lat_rad = lat_deg * DEG2RAD;
    float lon_rad = lon_deg * DEG2RAD;

    float cos_lat = std::cos(lat_rad);
    float sin_lat = std::sin(lat_rad);
    float cos_lon = std::cos(lon_rad);
    float sin_lon = std::sin(lon_rad);

    return {
        radius * cos_lat * sin_lon,  // x = R·cos(φ)·sin(θ)
        radius * sin_lat,            // y = R·sin(φ)
        radius * cos_lat * cos_lon   // z = R·cos(φ)·cos(θ)
    };
}

// ── ArcballCamera ─────────────────────────────────────────────────

/// θ ← θ − Δx · S  (negated so sphere follows drag direction)
/// φ ← φ + Δy · S  (negated so sphere follows drag direction)
/// Clamp: −π/2 < φ < π/2
void ArcballCamera::rotate(float dx, float dy, float sensitivity) {
    constexpr float PI_HALF  = 3.14159265358979323846f / 2.0f;
    constexpr float PI_LIMIT = PI_HALF - 0.001f;  // strict < π/2
    constexpr float TWO_PI   = 3.14159265358979323846f * 2.0f;

    // Guard: reject NaN / Inf deltas or sensitivity
    if (std::isnan(dx) || std::isnan(dy) || std::isnan(sensitivity) ||
        std::isinf(dx) || std::isinf(dy) || std::isinf(sensitivity)) {
        return;
    }

    // Dynamic sensitivity: S_mult = min(1.0, R / R_default)
    float s_mult = (initial_distance > 1e-8f)
                       ? std::min(1.0f, distance / initial_distance)
                       : 1.0f;
    float effective_sens = sensitivity * s_mult;

    // θ ← θ − Δx · S · S_mult
    longitude -= dx * effective_sens;

    // φ ← φ + Δy · S · S_mult
    latitude += dy * effective_sens;

    // Gimbal-lock prevention: −π/2 < φ < π/2
    latitude = std::clamp(latitude, -PI_LIMIT, PI_LIMIT);

    // Keep longitude in a finite range to prevent float overflow
    longitude = std::fmod(longitude, TWO_PI);
}

// ── Zoom ─────────────────────────────────────────────────────────

void ArcballCamera::zoom(float delta, float sensitivity) {
    // Guard: reject NaN / Inf
    if (std::isnan(delta) || std::isnan(sensitivity) ||
        std::isinf(delta) || std::isinf(sensitivity)) {
        return;
    }

    // Positive delta = zoom in = decrease distance
    distance -= delta * sensitivity;

    // Clamp to [0.5 × initial, 2.0 × initial]
    float min_dist = initial_distance * 0.5f;
    float max_dist = initial_distance * 2.0f;
    distance = std::clamp(distance, min_dist, max_dist);
}

/// Spherical → Cartesian (θ measured from +Z axis):
///   x = R · cos(φ) · sin(θ)
///   y = R · sin(φ)
///   z = R · cos(φ) · cos(θ)
/// At θ=0, φ=0 the camera sits on the +Z axis looking at the origin.
Vec3 ArcballCamera::get_eye_position() const {
    float cos_lat = std::cos(latitude);
    float sin_lat = std::sin(latitude);
    float cos_lon = std::cos(longitude);
    float sin_lon = std::sin(longitude);

    return {
        distance * cos_lat * sin_lon,  // x = R·cos(φ)·sin(θ)
        distance * sin_lat,            // y = R·sin(φ)
        distance * cos_lat * cos_lon   // z = R·cos(φ)·cos(θ)
    };
}

}  // namespace globe
