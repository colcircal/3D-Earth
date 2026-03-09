#include "arcball_camera.h"
#include "geojson_parser.h"
#include "raylib.h"

#include <vector>

int main() {
    constexpr int SCREEN_W = 800;
    constexpr int SCREEN_H = 600;
    constexpr float SPHERE_RADIUS = 1.5f;

    InitWindow(SCREEN_W, SCREEN_H, "3D Earth — World Map");
    SetTargetFPS(60);

    // ── Load GeoJSON ─────────────────────────────────────
    globe::ParseResult geo = globe::parse_geojson_file("world.json", SPHERE_RADIUS + 0.003f);
    if (!geo.success) {
        TraceLog(LOG_WARNING, "GeoJSON load failed: %s", geo.error_message.c_str());
    }

    // Pre-convert to Raylib Vector3 for fast rendering
    struct RingData {
        std::vector<Vector3> points;
    };
    std::vector<std::vector<RingData>> all_countries;

    for (const auto& country : geo.countries) {
        std::vector<RingData> country_rings;
        for (const auto& ring : country.rings) {
            RingData rd;
            rd.points.reserve(ring.size());
            for (const auto& pt : ring) {
                rd.points.push_back({pt.x, pt.y, pt.z});
            }
            country_rings.push_back(std::move(rd));
        }
        all_countries.push_back(std::move(country_rings));
    }

    globe::ArcballCamera arcball;
    arcball.distance = 5.0f;
    arcball.initial_distance = 5.0f;

    Vector2 prev_mouse = {0};
    bool dragging = false;

    while (!WindowShouldClose()) {
        // ── Input: mouse drag ────────────────────────────
        Vector2 mouse = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging = true;
            prev_mouse = mouse;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            dragging = false;
        }

        if (dragging) {
            float dx = mouse.x - prev_mouse.x;
            float dy = mouse.y - prev_mouse.y;
            arcball.rotate(dx, dy);
            prev_mouse = mouse;
        }

        // ── Input: mouse wheel zoom ─────────────────────
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            arcball.zoom(wheel);
        }

        // ── Camera position from spherical coordinates ───
        globe::Vec3 eye = arcball.get_eye_position();
        Camera3D camera = {0};
        camera.position = {eye.x, eye.y, eye.z};
        camera.target   = {0.0f, 0.0f, 0.0f};
        camera.up       = {0.0f, 1.0f, 0.0f};
        camera.fovy     = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        // ── Render ───────────────────────────────────────
        BeginDrawing();
            ClearBackground({20, 20, 30, 255});

            BeginMode3D(camera);
                // Solid sphere
                DrawSphere({0, 0, 0}, SPHERE_RADIUS, {30, 60, 120, 255});

                // Country borders as line strips
                for (const auto& country_rings : all_countries) {
                    for (const auto& rd : country_rings) {
                        for (size_t i = 0; i + 1 < rd.points.size(); ++i) {
                            DrawLine3D(rd.points[i], rd.points[i + 1],
                                       {200, 220, 255, 200});
                        }
                    }
                }
            EndMode3D();

            if (!geo.success) {
                DrawText("world.json not found — place it next to the exe",
                         10, SCREEN_H - 30, 16, RED);
            }
            DrawText("Drag to rotate | Scroll to zoom", 10, 10, 20, LIGHTGRAY);
            DrawFPS(SCREEN_W - 90, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
