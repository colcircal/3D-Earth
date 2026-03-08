#pragma once

/// Pure-math arcball camera logic.
/// No graphics dependencies — testable in isolation.

namespace globe {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

/// Normalize a vector. Returns zero vector if length is ~0.
Vec3 normalize(Vec3 v);

/// Compute the length of a vector.
float length(Vec3 v);

/// Dot product.
float dot(Vec3 a, Vec3 b);

/// Cross product.
Vec3 cross(Vec3 a, Vec3 b);

/// Map a 2D screen point to a point on the unit sphere (arcball projection).
/// @param screen_x, screen_y  Mouse position in pixels.
/// @param viewport_w, viewport_h  Window dimensions.
/// @return Unit-sphere point (may be projected onto sphere surface).
Vec3 map_to_sphere(float screen_x, float screen_y,
                   float viewport_w, float viewport_h);

/// Compute the rotation axis and angle from two arcball sphere points.
/// @param from  Previous sphere point (unit-length).
/// @param to    Current sphere point (unit-length).
/// @param[out] axis   Rotation axis (normalized).
/// @param[out] angle  Rotation angle in radians.
/// @return true if a valid rotation was computed, false otherwise
///         (e.g. identical points, degenerate input).
bool compute_rotation(Vec3 from, Vec3 to, Vec3& axis, float& angle);

/// Convert latitude/longitude (degrees) to 3D Cartesian coordinates.
///   x = R · cos(φ) · sin(θ)
///   y = R · sin(φ)
///   z = R · cos(φ) · cos(θ)
/// @param lat_deg  Latitude in degrees  (-90 to 90).
/// @param lon_deg  Longitude in degrees (-180 to 180).
/// @param radius   Sphere radius.
Vec3 latlon_to_cartesian(float lat_deg, float lon_deg, float radius);

/// Spherical-coordinate camera state.
struct ArcballCamera {
    float latitude  = 0.0f;   // radians, clamped to (-pi/2, pi/2)
    float longitude = 0.0f;   // radians
    float distance  = 5.0f;   // distance from origin

    /// Update camera angles from mouse delta (pixels).
    /// @param dx, dy  Mouse movement delta in pixels.
    /// @param sensitivity  Radians per pixel.
    void rotate(float dx, float dy, float sensitivity = 0.005f);

    /// Get the Cartesian eye position from spherical coordinates.
    Vec3 get_eye_position() const;
};

}  // namespace globe
