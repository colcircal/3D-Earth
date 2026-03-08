cmake_minimum_required(VERSION 3.20)
project(3D-Earth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies via FetchContent ──────────────────────────────────
include(FetchContent)

# Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
# Windows: force shared CRT to avoid link errors
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Core library (pure logic, no graphics) ─────────────────────────
add_library(globe_core STATIC
    src/arcball_camera.cpp
    src/geojson_parser.cpp
)
target_include_directories(globe_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(globe_core PUBLIC nlohmann_json::nlohmann_json)

# ── Main executable ────────────────────────────────────────────────
add_executable(3D-Earth
    src/main.cpp
)
target_link_libraries(3D-Earth PRIVATE globe_core raylib)

# ── Tests ──────────────────────────────────────────────────────────
enable_testing()

add_executable(globe_tests
    tests/test_arcball_camera.cpp
    tests/test_geojson_parser.cpp
)
target_link_libraries(globe_tests PRIVATE globe_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(globe_tests)
