#include "geojson_parser.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace globe {

// ── Internal helpers ──────────────────────────────────────────────

/// Convert a GeoJSON coordinate ring [[lon,lat], ...] to Vec3 points.
static std::vector<Vec3> convert_ring(const json& coords, float radius) {
    std::vector<Vec3> ring;
    ring.reserve(coords.size());
    for (const auto& point : coords) {
        if (!point.is_array() || point.size() < 2) continue;
        float lon = point[0].get<float>();  // GeoJSON: [lon, lat]
        float lat = point[1].get<float>();
        ring.push_back(latlon_to_cartesian(lat, lon, radius));
    }
    return ring;
}

/// Process a single Polygon geometry's coordinates.
static void process_polygon(const json& coordinates, float radius,
                            std::vector<std::vector<Vec3>>& out_rings) {
    for (const auto& ring_coords : coordinates) {
        if (!ring_coords.is_array()) continue;
        auto ring = convert_ring(ring_coords, radius);
        if (!ring.empty()) {
            out_rings.push_back(std::move(ring));
        }
    }
}

/// Process a single Feature and append to countries list.
static void process_feature(const json& feature, float radius,
                            std::vector<CountryPolygon>& countries) {
    // Skip features without geometry
    if (!feature.contains("geometry") || feature["geometry"].is_null()) {
        return;
    }

    const auto& geometry = feature["geometry"];
    if (!geometry.contains("type") || !geometry.contains("coordinates")) {
        return;
    }

    std::string geom_type = geometry["type"].get<std::string>();
    const auto& coordinates = geometry["coordinates"];

    // Extract country name from properties
    std::string name;
    if (feature.contains("properties") && !feature["properties"].is_null()) {
        const auto& props = feature["properties"];
        if (props.contains("name")) {
            name = props["name"].get<std::string>();
        }
    }

    CountryPolygon country;
    country.name = std::move(name);

    if (geom_type == "Polygon") {
        process_polygon(coordinates, radius, country.rings);
    } else if (geom_type == "MultiPolygon") {
        for (const auto& polygon_coords : coordinates) {
            if (!polygon_coords.is_array()) continue;
            process_polygon(polygon_coords, radius, country.rings);
        }
    }

    countries.push_back(std::move(country));
}

// ── Public API ────────────────────────────────────────────────────

ParseResult parse_geojson_string(const std::string& json_str, float radius) {
    ParseResult result;

    if (json_str.empty()) {
        result.success = false;
        result.error_message = "Empty JSON string";
        return result;
    }

    try {
        json data = json::parse(json_str);

        // Must be a FeatureCollection
        if (!data.contains("type") ||
            data["type"].get<std::string>() != "FeatureCollection") {
            result.success = false;
            result.error_message = "Expected type 'FeatureCollection'";
            return result;
        }

        if (!data.contains("features") || !data["features"].is_array()) {
            result.success = false;
            result.error_message = "Missing 'features' array";
            return result;
        }

        for (const auto& feature : data["features"]) {
            process_feature(feature, radius, result.countries);
        }

        result.success = true;
    } catch (const json::parse_error& e) {
        result.success = false;
        result.error_message = std::string("JSON parse error: ") + e.what();
    } catch (const json::type_error& e) {
        result.success = false;
        result.error_message = std::string("JSON type error: ") + e.what();
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Error: ") + e.what();
    }

    return result;
}

ParseResult parse_geojson_file(const std::string& file_path, float radius) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {{}, false, "Cannot open file: " + file_path};
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return parse_geojson_string(ss.str(), radius);
}

}  // namespace globe
