#pragma once

#include "game_lab.h"
#include <optional>

namespace collision {
struct Point { double x, y; };
struct Segment { Point a, b; };
using World = std::vector<Segment>;
struct Settings {
    double radius = 30.0; // Provisional circular body, not a recovered original radius.
    double skin = 0.0001; // Numerical separation in world units.
    unsigned maxContacts = 8; // Stop safely if a move encounters more constraints.
};

// Copies static global segments; 0x1000 connections and animated doors are excluded.
World staticWorld(const game_lab::Level& level);
bool insidePolygon(Point point, const game_lab::Sector& sector, const game_lab::Polygon& polygon);
struct Location { unsigned sector, polygon; };
std::optional<Location> locate(const game_lab::Level& level, Point point,
                               std::optional<unsigned> preferredSector = {});
double distanceToSegment(Segment segment, Point point);
double signedDistanceToLine(Segment segment, Point point);
bool overlaps(Segment segment, Point center, double radius);
// Ray parameter along direction (world distance for a unit vector), including collinear walls.
std::optional<double> rayDistance(Segment segment, Point start, Point direction, double range);
std::optional<double> rayDistance(const World& world, Point start, Point direction, double range);
// Continuous circle/segment sweep followed by tangential sliding. No SDL dependency.
Point move(const World& world, Point start, Point displacement, const Settings& settings = {});
} // namespace collision
