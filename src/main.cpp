#include "arcball_camera.h"
#include "raylib.h"

int main() {
    constexpr int SCREEN_W = 800;
    constexpr int SCREEN_H = 600;
    constexpr float SPHERE_RADIUS = 1.5f;

    InitWindow(SCREEN_W, SCREEN_H, "3D Earth — Arcball Camera");
    SetTargetFPS(60);

    globe::ArcballCamera arcball;
    arcball.distance = 5.0f;

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
                DrawSphere({0, 0, 0}, SPHERE_RADIUS, {40, 80, 160, 255});
                // Wireframe overlay
                DrawSphereWires({0, 0, 0}, SPHERE_RADIUS + 0.002f,
                                16, 16, {100, 160, 255, 120});
            EndMode3D();

            DrawText("Drag to rotate", 10, 10, 20, LIGHTGRAY);
            DrawFPS(SCREEN_W - 90, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
