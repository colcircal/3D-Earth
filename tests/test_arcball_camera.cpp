#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "arcball_camera.h"

using namespace globe;

// ═══════════════════════════════════════════════════════════════════
// Helper constants
// ═══════════════════════════════════════════════════════════════════

constexpr float PI        = 3.14159265358979323846f;
constexpr float EPSILON   = 1e-5f;
constexpr float VIEWPORT_W = 800.0f;
constexpr float VIEWPORT_H = 600.0f;

// ═══════════════════════════════════════════════════════════════════
// Vec3 math utilities
// ═══════════════════════════════════════════════════════════════════

TEST(Vec3Math, NormalizeUnitVector) {
    // Arrange
    Vec3 v{1.0f, 0.0f, 0.0f};

    // Act
    Vec3 result = normalize(v);

    // Assert
    EXPECT_NEAR(result.x, 1.0f, EPSILON);
    EXPECT_NEAR(result.y, 0.0f, EPSILON);
    EXPECT_NEAR(result.z, 0.0f, EPSILON);
}

TEST(Vec3Math, NormalizeArbitraryVector) {
    // Arrange
    Vec3 v{3.0f, 4.0f, 0.0f};

    // Act
    Vec3 result = normalize(v);

    // Assert — length should be 1
    float len = std::sqrt(result.x * result.x +
                          result.y * result.y +
                          result.z * result.z);
    EXPECT_NEAR(len, 1.0f, EPSILON);
    EXPECT_NEAR(result.x, 0.6f, EPSILON);
    EXPECT_NEAR(result.y, 0.8f, EPSILON);
}

TEST(Vec3Math, NormalizeZeroVectorReturnsSafe) {
    // Arrange
    Vec3 v{0.0f, 0.0f, 0.0f};

    // Act
    Vec3 result = normalize(v);

    // Assert — must not crash, returns zero vector
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 0.0f, EPSILON);
    EXPECT_NEAR(result.z, 0.0f, EPSILON);
}

TEST(Vec3Math, Length) {
    // Arrange
    Vec3 v{3.0f, 4.0f, 0.0f};

    // Act
    float len = length(v);

    // Assert
    EXPECT_NEAR(len, 5.0f, EPSILON);
}

TEST(Vec3Math, DotProduct) {
    // Arrange
    Vec3 a{1.0f, 0.0f, 0.0f};
    Vec3 b{0.0f, 1.0f, 0.0f};

    // Act
    float result = dot(a, b);

    // Assert — perpendicular => 0
    EXPECT_NEAR(result, 0.0f, EPSILON);
}

TEST(Vec3Math, DotProductParallel) {
    // Arrange
    Vec3 a{1.0f, 0.0f, 0.0f};
    Vec3 b{1.0f, 0.0f, 0.0f};

    // Act
    float result = dot(a, b);

    // Assert
    EXPECT_NEAR(result, 1.0f, EPSILON);
}

TEST(Vec3Math, CrossProduct) {
    // Arrange
    Vec3 a{1.0f, 0.0f, 0.0f};
    Vec3 b{0.0f, 1.0f, 0.0f};

    // Act
    Vec3 result = cross(a, b);

    // Assert — x cross y = z
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 0.0f, EPSILON);
    EXPECT_NEAR(result.z, 1.0f, EPSILON);
}

// ═══════════════════════════════════════════════════════════════════
// Happy Path — Arcball sphere mapping & rotation
// ═══════════════════════════════════════════════════════════════════

TEST(ArcballMapping, CenterMapsToFront) {
    // Arrange — mouse at exact center of viewport
    float cx = VIEWPORT_W / 2.0f;
    float cy = VIEWPORT_H / 2.0f;

    // Act
    Vec3 point = map_to_sphere(cx, cy, VIEWPORT_W, VIEWPORT_H);

    // Assert — center should map to (0, 0, 1) on the sphere
    EXPECT_NEAR(point.x, 0.0f, EPSILON);
    EXPECT_NEAR(point.y, 0.0f, EPSILON);
    EXPECT_NEAR(point.z, 1.0f, EPSILON);
}

TEST(ArcballMapping, ResultIsUnitLength) {
    // Arrange — arbitrary point inside viewport
    float sx = 300.0f;
    float sy = 200.0f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, VIEWPORT_W, VIEWPORT_H);
    float len = std::sqrt(point.x * point.x +
                          point.y * point.y +
                          point.z * point.z);

    // Assert — mapped point should always lie on the unit sphere
    EXPECT_NEAR(len, 1.0f, EPSILON);
}

TEST(ArcballMapping, RightOfCenterHasPositiveX) {
    // Arrange — mouse to the right of center
    float sx = VIEWPORT_W * 0.75f;
    float sy = VIEWPORT_H / 2.0f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, VIEWPORT_W, VIEWPORT_H);

    // Assert
    EXPECT_GT(point.x, 0.0f);
}

TEST(ArcballMapping, AboveCenterHasPositiveY) {
    // Arrange — mouse above center (screen y decreases upward)
    float sx = VIEWPORT_W / 2.0f;
    float sy = VIEWPORT_H * 0.25f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, VIEWPORT_W, VIEWPORT_H);

    // Assert — y should be positive (up)
    EXPECT_GT(point.y, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════
// Happy Path — Rotation computation
// ═══════════════════════════════════════════════════════════════════

TEST(ComputeRotation, HorizontalDragProducesVerticalAxis) {
    // Arrange — drag from center to right
    Vec3 from = map_to_sphere(VIEWPORT_W / 2.0f, VIEWPORT_H / 2.0f,
                              VIEWPORT_W, VIEWPORT_H);
    Vec3 to   = map_to_sphere(VIEWPORT_W / 2.0f + 50.0f, VIEWPORT_H / 2.0f,
                              VIEWPORT_W, VIEWPORT_H);
    Vec3 axis;
    float angle;

    // Act
    bool ok = compute_rotation(from, to, axis, angle);

    // Assert — rotation axis should be predominantly Y (vertical)
    EXPECT_TRUE(ok);
    EXPECT_GT(angle, 0.0f);
    EXPECT_GT(std::abs(axis.y), std::abs(axis.x));
}

TEST(ComputeRotation, VerticalDragProducesHorizontalAxis) {
    // Arrange — drag from center downward
    Vec3 from = map_to_sphere(VIEWPORT_W / 2.0f, VIEWPORT_H / 2.0f,
                              VIEWPORT_W, VIEWPORT_H);
    Vec3 to   = map_to_sphere(VIEWPORT_W / 2.0f, VIEWPORT_H / 2.0f + 50.0f,
                              VIEWPORT_W, VIEWPORT_H);
    Vec3 axis;
    float angle;

    // Act
    bool ok = compute_rotation(from, to, axis, angle);

    // Assert — rotation axis should be predominantly X (horizontal)
    EXPECT_TRUE(ok);
    EXPECT_GT(angle, 0.0f);
    EXPECT_GT(std::abs(axis.x), std::abs(axis.y));
}

TEST(ComputeRotation, LargerDragProducesLargerAngle) {
    // Arrange
    Vec3 center = map_to_sphere(VIEWPORT_W / 2.0f, VIEWPORT_H / 2.0f,
                                VIEWPORT_W, VIEWPORT_H);
    Vec3 small_drag = map_to_sphere(VIEWPORT_W / 2.0f + 20.0f,
                                    VIEWPORT_H / 2.0f,
                                    VIEWPORT_W, VIEWPORT_H);
    Vec3 large_drag = map_to_sphere(VIEWPORT_W / 2.0f + 100.0f,
                                    VIEWPORT_H / 2.0f,
                                    VIEWPORT_W, VIEWPORT_H);
    Vec3 axis1, axis2;
    float angle1, angle2;

    // Act
    compute_rotation(center, small_drag, axis1, angle1);
    compute_rotation(center, large_drag, axis2, angle2);

    // Assert
    EXPECT_GT(angle2, angle1);
}

// ═══════════════════════════════════════════════════════════════════
// Happy Path — ArcballCamera spherical coordinate rotation
// ═══════════════════════════════════════════════════════════════════

TEST(ArcballCamera_Rotate, DragRightIncreasesLongitude) {
    // Arrange
    ArcballCamera cam;
    float initial_lon = cam.longitude;

    // Act — positive dx = drag right
    cam.rotate(100.0f, 0.0f);

    // Assert
    EXPECT_GT(cam.longitude, initial_lon);
}

TEST(ArcballCamera_Rotate, DragDownDecreasesLatitude) {
    // Arrange
    ArcballCamera cam;
    float initial_lat = cam.latitude;

    // Act — positive dy = drag down
    cam.rotate(0.0f, 100.0f);

    // Assert
    EXPECT_LT(cam.latitude, initial_lat);
}

TEST(ArcballCamera_Rotate, SensitivityScalesRotation) {
    // Arrange
    ArcballCamera cam1, cam2;
    float low_sens  = 0.001f;
    float high_sens = 0.01f;

    // Act
    cam1.rotate(100.0f, 0.0f, low_sens);
    cam2.rotate(100.0f, 0.0f, high_sens);

    // Assert — higher sensitivity -> larger longitude change
    EXPECT_GT(std::abs(cam2.longitude), std::abs(cam1.longitude));
}

TEST(ArcballCamera_EyePosition, DefaultPositionOnPositiveZ) {
    // Arrange
    ArcballCamera cam;  // lat=0, lon=0

    // Act
    Vec3 eye = cam.get_eye_position();

    // Assert — at lat=0, lon=0 the eye should be on the +Z axis
    EXPECT_NEAR(eye.x, 0.0f, EPSILON);
    EXPECT_NEAR(eye.y, 0.0f, EPSILON);
    EXPECT_GT(eye.z, 0.0f);
    EXPECT_NEAR(eye.z, cam.distance, EPSILON);
}

TEST(ArcballCamera_EyePosition, EyeDistanceMatchesCameraDistance) {
    // Arrange
    ArcballCamera cam;
    cam.distance = 10.0f;
    cam.latitude = 0.3f;
    cam.longitude = 0.7f;

    // Act
    Vec3 eye = cam.get_eye_position();
    float dist = std::sqrt(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);

    // Assert
    EXPECT_NEAR(dist, 10.0f, EPSILON);
}

// ═══════════════════════════════════════════════════════════════════
// Edge Case — Cursor outside render window
// ═══════════════════════════════════════════════════════════════════

TEST(ArcballMapping_Edge, CursorOutsideLeftStillProducesUnitVector) {
    // Arrange — cursor far to the left of the window
    float sx = -200.0f;
    float sy = VIEWPORT_H / 2.0f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, VIEWPORT_W, VIEWPORT_H);
    float len = std::sqrt(point.x * point.x +
                          point.y * point.y +
                          point.z * point.z);

    // Assert — still on unit sphere (projected onto edge)
    EXPECT_NEAR(len, 1.0f, EPSILON);
}

TEST(ArcballMapping_Edge, CursorFarOutsideWindowStillSafe) {
    // Arrange — cursor extremely far outside
    float sx = -10000.0f;
    float sy = 10000.0f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, VIEWPORT_W, VIEWPORT_H);
    float len = std::sqrt(point.x * point.x +
                          point.y * point.y +
                          point.z * point.z);

    // Assert — no crash, still on unit sphere
    EXPECT_NEAR(len, 1.0f, EPSILON);
    EXPECT_FALSE(std::isnan(point.x));
    EXPECT_FALSE(std::isnan(point.y));
    EXPECT_FALSE(std::isnan(point.z));
}

TEST(ComputeRotation_Edge, IdenticalPointsReturnFalse) {
    // Arrange — no mouse movement
    Vec3 same = map_to_sphere(VIEWPORT_W / 2.0f, VIEWPORT_H / 2.0f,
                              VIEWPORT_W, VIEWPORT_H);
    Vec3 axis;
    float angle;

    // Act
    bool ok = compute_rotation(same, same, axis, angle);

    // Assert — no rotation when start == end
    EXPECT_FALSE(ok);
}

TEST(ArcballCamera_Edge, LatitudeClampedAtPoles) {
    // Arrange
    ArcballCamera cam;

    // Act — drag up a huge amount (should clamp near +90°)
    cam.rotate(0.0f, -100000.0f);

    // Assert — latitude must not exceed ±π/2
    EXPECT_LT(cam.latitude, PI / 2.0f);
    EXPECT_GT(cam.latitude, -PI / 2.0f);
}

TEST(ArcballCamera_Edge, LatitudeClampedAtSouthPole) {
    // Arrange
    ArcballCamera cam;

    // Act — drag down a huge amount
    cam.rotate(0.0f, 100000.0f);

    // Assert
    EXPECT_LT(cam.latitude, PI / 2.0f);
    EXPECT_GT(cam.latitude, -PI / 2.0f);
}

TEST(ArcballCamera_Edge, LongitudeWrapsAround) {
    // Arrange
    ArcballCamera cam;

    // Act — rotate many full circles
    for (int i = 0; i < 1000; ++i) {
        cam.rotate(100.0f, 0.0f);
    }

    // Assert — longitude should not overflow to inf
    EXPECT_FALSE(std::isinf(cam.longitude));
    EXPECT_FALSE(std::isnan(cam.longitude));
}

// ═══════════════════════════════════════════════════════════════════
// Error Case — NaN, division by zero, invalid input
// ═══════════════════════════════════════════════════════════════════

TEST(ArcballMapping_Error, NaNInputProducesSafeOutput) {
    // Arrange
    float nan_val = std::numeric_limits<float>::quiet_NaN();

    // Act
    Vec3 point = map_to_sphere(nan_val, nan_val, VIEWPORT_W, VIEWPORT_H);

    // Assert — must not crash, no NaN propagation
    EXPECT_FALSE(std::isnan(point.x));
    EXPECT_FALSE(std::isnan(point.y));
    EXPECT_FALSE(std::isnan(point.z));
}

TEST(ArcballMapping_Error, ZeroViewportProducesSafeOutput) {
    // Arrange — zero-size viewport (would cause division by zero)
    float sx = 100.0f;
    float sy = 100.0f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, 0.0f, 0.0f);

    // Assert — no crash, no NaN/Inf
    EXPECT_FALSE(std::isnan(point.x));
    EXPECT_FALSE(std::isnan(point.y));
    EXPECT_FALSE(std::isnan(point.z));
    EXPECT_FALSE(std::isinf(point.x));
    EXPECT_FALSE(std::isinf(point.y));
    EXPECT_FALSE(std::isinf(point.z));
}

TEST(ArcballMapping_Error, NegativeViewportProducesSafeOutput) {
    // Arrange
    float sx = 100.0f;
    float sy = 100.0f;

    // Act
    Vec3 point = map_to_sphere(sx, sy, -800.0f, -600.0f);

    // Assert
    EXPECT_FALSE(std::isnan(point.x));
    EXPECT_FALSE(std::isnan(point.y));
    EXPECT_FALSE(std::isnan(point.z));
}

TEST(ComputeRotation_Error, NaNVectorsReturnFalse) {
    // Arrange
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    Vec3 bad{nan_val, nan_val, nan_val};
    Vec3 good{0.0f, 0.0f, 1.0f};
    Vec3 axis;
    float angle;

    // Act
    bool ok = compute_rotation(bad, good, axis, angle);

    // Assert — invalid input should be rejected
    EXPECT_FALSE(ok);
}

TEST(ComputeRotation_Error, ZeroVectorsReturnFalse) {
    // Arrange
    Vec3 zero{0.0f, 0.0f, 0.0f};
    Vec3 good{0.0f, 0.0f, 1.0f};
    Vec3 axis;
    float angle;

    // Act
    bool ok = compute_rotation(zero, good, axis, angle);

    // Assert
    EXPECT_FALSE(ok);
}

TEST(ArcballCamera_Error, NaNDeltaDoesNotCorruptState) {
    // Arrange
    ArcballCamera cam;
    float original_lat = cam.latitude;
    float original_lon = cam.longitude;
    float nan_val = std::numeric_limits<float>::quiet_NaN();

    // Act
    cam.rotate(nan_val, nan_val);

    // Assert — camera state must remain valid
    EXPECT_FALSE(std::isnan(cam.latitude));
    EXPECT_FALSE(std::isnan(cam.longitude));
}

TEST(ArcballCamera_Error, InfDeltaDoesNotCorruptState) {
    // Arrange
    ArcballCamera cam;
    float inf_val = std::numeric_limits<float>::infinity();

    // Act
    cam.rotate(inf_val, inf_val);

    // Assert
    EXPECT_FALSE(std::isinf(cam.latitude));
    EXPECT_FALSE(std::isinf(cam.longitude));
    EXPECT_FALSE(std::isnan(cam.latitude));
    EXPECT_FALSE(std::isnan(cam.longitude));
}

TEST(ArcballCamera_Error, ZeroSensitivityProducesNoRotation) {
    // Arrange
    ArcballCamera cam;
    float original_lat = cam.latitude;
    float original_lon = cam.longitude;

    // Act
    cam.rotate(100.0f, 100.0f, 0.0f);

    // Assert — no change
    EXPECT_NEAR(cam.latitude, original_lat, EPSILON);
    EXPECT_NEAR(cam.longitude, original_lon, EPSILON);
}
