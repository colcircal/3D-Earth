#include <gtest/gtest.h>
#include <cmath>
#include <string>
#include "geojson_parser.h"
#include "arcball_camera.h"

using namespace globe;

// ═══════════════════════════════════════════════════════════════════
// Helper constants
// ═══════════════════════════════════════════════════════════════════

constexpr float PI      = 3.14159265358979323846f;
constexpr float EPSILON = 1e-3f;
constexpr float RADIUS  = 1.5f;

// ═══════════════════════════════════════════════════════════════════
// Helper: minimal GeoJSON strings
// ═══════════════════════════════════════════════════════════════════

static const std::string VALID_POLYGON_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [{
    "type": "Feature",
    "properties": { "name": "TestLand" },
    "geometry": {
      "type": "Polygon",
      "coordinates": [[
        [127.0, 37.0],
        [128.0, 37.0],
        [128.0, 38.0],
        [127.0, 38.0],
        [127.0, 37.0]
      ]]
    }
  }]
})";

static const std::string VALID_MULTIPOLYGON_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [{
    "type": "Feature",
    "properties": { "name": "IslandNation" },
    "geometry": {
      "type": "MultiPolygon",
      "coordinates": [
        [[[10.0, 20.0], [11.0, 20.0], [11.0, 21.0], [10.0, 20.0]]],
        [[[30.0, 40.0], [31.0, 40.0], [31.0, 41.0], [30.0, 40.0]]]
      ]
    }
  }]
})";

static const std::string DATELINE_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [{
    "type": "Feature",
    "properties": { "name": "DatelineLand" },
    "geometry": {
      "type": "Polygon",
      "coordinates": [[
        [179.0, 50.0],
        [180.0, 50.0],
        [-179.0, 50.0],
        [-180.0, 50.0],
        [179.0, 50.0]
      ]]
    }
  }]
})";

static const std::string EMPTY_COORDINATES_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [{
    "type": "Feature",
    "properties": { "name": "EmptyLand" },
    "geometry": {
      "type": "Polygon",
      "coordinates": []
    }
  }]
})";

static const std::string NO_PROPERTIES_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [{
    "type": "Feature",
    "geometry": {
      "type": "Polygon",
      "coordinates": [[[10.0, 20.0], [11.0, 20.0], [11.0, 21.0], [10.0, 20.0]]]
    }
  }]
})";

static const std::string NO_GEOMETRY_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [{
    "type": "Feature",
    "properties": { "name": "NoGeom" }
  }]
})";

static const std::string WRONG_TYPE_GEOJSON = R"({
  "type": "Feature",
  "properties": { "name": "NotACollection" },
  "geometry": {
    "type": "Polygon",
    "coordinates": [[[10.0, 20.0], [11.0, 20.0], [11.0, 21.0], [10.0, 20.0]]]
  }
})";

static const std::string MULTIPLE_COUNTRIES_GEOJSON = R"({
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": { "name": "CountryA" },
      "geometry": {
        "type": "Polygon",
        "coordinates": [[[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 0.0]]]
      }
    },
    {
      "type": "Feature",
      "properties": { "name": "CountryB" },
      "geometry": {
        "type": "Polygon",
        "coordinates": [[[10.0, 10.0], [11.0, 10.0], [11.0, 11.0], [10.0, 10.0]]]
      }
    }
  ]
})";

// ═══════════════════════════════════════════════════════════════════
// latlon_to_cartesian unit tests
// ═══════════════════════════════════════════════════════════════════

TEST(LatlonToCartesian, EquatorPrimeMeridian) {
    // Arrange — lat=0, lon=0 → should be on +Z axis
    // Act
    Vec3 p = latlon_to_cartesian(0.0f, 0.0f, RADIUS);

    // Assert
    EXPECT_NEAR(p.x, 0.0f, EPSILON);
    EXPECT_NEAR(p.y, 0.0f, EPSILON);
    EXPECT_NEAR(p.z, RADIUS, EPSILON);
}

TEST(LatlonToCartesian, NorthPole) {
    // Arrange — lat=90 → top of sphere
    // Act
    Vec3 p = latlon_to_cartesian(90.0f, 0.0f, RADIUS);

    // Assert
    EXPECT_NEAR(p.x, 0.0f, EPSILON);
    EXPECT_NEAR(p.y, RADIUS, EPSILON);
    EXPECT_NEAR(p.z, 0.0f, EPSILON);
}

TEST(LatlonToCartesian, SouthPole) {
    // Arrange
    // Act
    Vec3 p = latlon_to_cartesian(-90.0f, 0.0f, RADIUS);

    // Assert
    EXPECT_NEAR(p.x, 0.0f, EPSILON);
    EXPECT_NEAR(p.y, -RADIUS, EPSILON);
    EXPECT_NEAR(p.z, 0.0f, EPSILON);
}

TEST(LatlonToCartesian, Equator90East) {
    // Arrange — lon=90 on equator → +X axis
    // Act
    Vec3 p = latlon_to_cartesian(0.0f, 90.0f, RADIUS);

    // Assert
    EXPECT_NEAR(p.x, RADIUS, EPSILON);
    EXPECT_NEAR(p.y, 0.0f, EPSILON);
    EXPECT_NEAR(p.z, 0.0f, EPSILON);
}

TEST(LatlonToCartesian, ResultLiesOnSphere) {
    // Arrange — Seoul approximate coordinates
    float lat = 37.5665f;
    float lon = 126.978f;

    // Act
    Vec3 p = latlon_to_cartesian(lat, lon, RADIUS);
    float dist = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

    // Assert — point must lie on sphere surface
    EXPECT_NEAR(dist, RADIUS, EPSILON);
}

TEST(LatlonToCartesian, SeoulIsInCorrectHemisphere) {
    // Arrange — Seoul: lat ~37.5 (north), lon ~127 (east)
    // Act
    Vec3 p = latlon_to_cartesian(37.5665f, 126.978f, RADIUS);

    // Assert — y > 0 (north), x < 0 (east of 90° in our coord system → sin(127°) > 0 → x > 0)
    EXPECT_GT(p.y, 0.0f);    // northern hemisphere
    EXPECT_GT(p.x, 0.0f);    // lon > 90° but sin(127°) > 0
}

// ═══════════════════════════════════════════════════════════════════
// Happy Path — GeoJSON parsing
// ═══════════════════════════════════════════════════════════════════

TEST(GeoJSONParser_Happy, ParseValidPolygon) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(VALID_POLYGON_GEOJSON, RADIUS);

    // Assert
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error_message.empty());
    EXPECT_EQ(result.countries.size(), 1u);
    EXPECT_EQ(result.countries[0].name, "TestLand");
    EXPECT_EQ(result.countries[0].rings.size(), 1u);
    EXPECT_EQ(result.countries[0].rings[0].size(), 5u);
}

TEST(GeoJSONParser_Happy, ParseValidMultiPolygon) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(VALID_MULTIPOLYGON_GEOJSON, RADIUS);

    // Assert
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.countries.size(), 1u);
    EXPECT_EQ(result.countries[0].name, "IslandNation");
    // MultiPolygon with 2 polygons, each with 1 ring
    EXPECT_EQ(result.countries[0].rings.size(), 2u);
}

TEST(GeoJSONParser_Happy, CoordinatesAreTransformedTo3D) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(VALID_POLYGON_GEOJSON, RADIUS);

    // Assert — first point [127, 37] should be on sphere
    ASSERT_TRUE(result.success);
    ASSERT_FALSE(result.countries.empty());
    ASSERT_FALSE(result.countries[0].rings.empty());
    ASSERT_FALSE(result.countries[0].rings[0].empty());

    Vec3 first = result.countries[0].rings[0][0];
    float dist = std::sqrt(first.x * first.x + first.y * first.y + first.z * first.z);
    EXPECT_NEAR(dist, RADIUS, EPSILON);

    // Verify it matches direct latlon_to_cartesian call
    Vec3 expected = latlon_to_cartesian(37.0f, 127.0f, RADIUS);
    EXPECT_NEAR(first.x, expected.x, EPSILON);
    EXPECT_NEAR(first.y, expected.y, EPSILON);
    EXPECT_NEAR(first.z, expected.z, EPSILON);
}

TEST(GeoJSONParser_Happy, CountryNameFromProperties) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(VALID_POLYGON_GEOJSON, RADIUS);

    // Assert
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.countries[0].name, "TestLand");
}

TEST(GeoJSONParser_Happy, MultipleCountries) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(MULTIPLE_COUNTRIES_GEOJSON, RADIUS);

    // Assert
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.countries.size(), 2u);
    EXPECT_EQ(result.countries[0].name, "CountryA");
    EXPECT_EQ(result.countries[1].name, "CountryB");
}

// ═══════════════════════════════════════════════════════════════════
// Edge Case — special geometries
// ═══════════════════════════════════════════════════════════════════

TEST(GeoJSONParser_Edge, DatelinePolygonPointsOnSphere) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(DATELINE_GEOJSON, RADIUS);

    // Assert — all points should lie on sphere regardless of ±180° longitude
    ASSERT_TRUE(result.success);
    ASSERT_FALSE(result.countries.empty());
    for (const auto& ring : result.countries[0].rings) {
        for (const auto& pt : ring) {
            float dist = std::sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
            EXPECT_NEAR(dist, RADIUS, EPSILON);
            EXPECT_FALSE(std::isnan(pt.x));
            EXPECT_FALSE(std::isnan(pt.y));
            EXPECT_FALSE(std::isnan(pt.z));
        }
    }
}

TEST(GeoJSONParser_Edge, EmptyCoordinatesProducesNoRings) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(EMPTY_COORDINATES_GEOJSON, RADIUS);

    // Assert — should succeed but country has no rings
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.countries.size(), 1u);
    EXPECT_TRUE(result.countries[0].rings.empty());
}

TEST(GeoJSONParser_Edge, NoPropertiesUsesEmptyName) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(NO_PROPERTIES_GEOJSON, RADIUS);

    // Assert — should succeed with empty name
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.countries.size(), 1u);
    EXPECT_TRUE(result.countries[0].name.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Error Case — malformed / missing data
// ═══════════════════════════════════════════════════════════════════

TEST(GeoJSONParser_Error, EmptyStringFails) {
    // Arrange & Act
    ParseResult result = parse_geojson_string("", RADIUS);

    // Assert
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(GeoJSONParser_Error, InvalidJsonFails) {
    // Arrange & Act
    ParseResult result = parse_geojson_string("{not valid json!!!", RADIUS);

    // Assert
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(GeoJSONParser_Error, NoGeometryFeatureSkipped) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(NO_GEOMETRY_GEOJSON, RADIUS);

    // Assert — should succeed but skip the geometry-less feature
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.countries.empty());
}

TEST(GeoJSONParser_Error, WrongTopLevelType) {
    // Arrange & Act
    ParseResult result = parse_geojson_string(WRONG_TYPE_GEOJSON, RADIUS);

    // Assert — not a FeatureCollection
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(GeoJSONParser_Error, NonexistentFileFails) {
    // Arrange & Act
    ParseResult result = parse_geojson_file("/nonexistent/path/world.json", RADIUS);

    // Assert
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}
