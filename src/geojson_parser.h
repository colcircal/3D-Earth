#pragma once

#include "arcball_camera.h"
#include <string>
#include <vector>

namespace globe {

/// A single country/territory parsed from GeoJSON.
struct CountryPolygon {
    std::string name;
    std::vector<std::vector<Vec3>> rings;  // each ring is a closed polyline
};

/// Result of a GeoJSON parse operation.
struct ParseResult {
    std::vector<CountryPolygon> countries;
    bool success = false;
    std::string error_message;
};

/// Parse GeoJSON from a string (useful for testing without file I/O).
/// @param json_str  Raw JSON string in GeoJSON FeatureCollection format.
/// @param radius    Sphere radius for coordinate transformation.
ParseResult parse_geojson_string(const std::string& json_str, float radius);

/// Parse GeoJSON from a file.
/// @param file_path  Path to the .json file.
/// @param radius     Sphere radius for coordinate transformation.
ParseResult parse_geojson_file(const std::string& file_path, float radius);

}  // namespace globe
