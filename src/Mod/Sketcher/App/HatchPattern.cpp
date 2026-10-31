// SPDX-License-Identifier: LGPL-2.1-or-later
#include <algorithm>
#include <cmath>
#include <string>
#include <Base/Exception.h>
#include "HatchPattern.h"

using namespace Sketcher;

namespace
{
// Section-lining symbols of ANSI Y14.2 as distributed in acad.pat (inches). ISO 128-3
// hatching for general use is the same 45° thin line family as ANSI31.
constexpr double ansiUnit = 0.125;
}  // namespace

const std::vector<HatchPattern>& Sketcher::hatchPatterns()
{
    static const std::vector<HatchPattern> patterns {
        {"ANSI31", ansiUnit, {{45, 0, 0, 0, .125, {}}}},
        {"ANSI32", ansiUnit, {{45, 0, 0, 0, .375, {}}, {45, .176776695, 0, 0, .375, {}}}},
        {"ANSI33", ansiUnit, {{45, 0, 0, 0, .25, {}}, {45, .176776695, 0, 0, .25, {.125, -.0625}}}},
        {"ANSI34",
         ansiUnit,
         {{45, 0, 0, 0, .75, {}},
          {45, .176776695, 0, 0, .75, {}},
          {45, .353553391, 0, 0, .75, {}},
          {45, .530330086, 0, 0, .75, {}}}},
        {"ANSI35",
         ansiUnit,
         {{45, 0, 0, 0, .25, {}}, {45, .176776695, 0, 0, .25, {.3125, -.0625, 0, -.0625}}}},
        {"ANSI36", ansiUnit, {{45, 0, 0, .21875, .125, {.3125, -.0625, 0, -.0625}}}},
        {"ANSI37", ansiUnit, {{45, 0, 0, 0, .125, {}}, {135, 0, 0, 0, .125, {}}}},
        {"ANSI38", ansiUnit, {{45, 0, 0, 0, .125, {}}, {135, 0, 0, .25, .125, {.3125, -.1875}}}},
        // Common drafting patterns from acad.pat. They are conventions, not standards.
        {"NET", ansiUnit, {{0, 0, 0, 0, .125, {}}, {90, 0, 0, 0, .125, {}}}},
        {"BRICK",
         ansiUnit,
         {{0, 0, 0, 0, .25, {}}, {90, 0, 0, 0, .5, {.25, -.25}}, {90, .25, 0, 0, .5, {-.25, .25}}}},
        {"EARTH",
         ansiUnit,
         {{0, 0, 0, .25, .25, {.25, -.25}},
          {0, 0, .09375, .25, .25, {.25, -.25}},
          {0, 0, .1875, .25, .25, {.25, -.25}},
          {90, .03125, .21875, .25, .25, {.25, -.25}},
          {90, .125, .21875, .25, .25, {.25, -.25}},
          {90, .21875, .21875, .25, .25, {.25, -.25}}}},
        {"HBONE",
         ansiUnit,
         {{45, 0, 0, .25, .25, {.75, -.25}}, {135, .1767766953, .1767766953, .25, -.25, {.75, -.25}}}},
        {"CROSS",
         ansiUnit,
         {{0, 0, 0, .25, .25, {.125, -.375}}, {90, .0625, -.0625, .25, .25, {.125, -.375}}}},
        {"HONEY",
         ansiUnit,
         {{0, 0, 0, .1875, .108253175, {.125, -.25}},
          {120, 0, 0, .1875, .108253175, {.125, -.25}},
          {60, 0, 0, .1875, .108253175, {-.25, .125}}}},
        {"INSUL",
         ansiUnit,
         {{0, 0, 0, 0, .375, {}},
          {0, 0, .125, 0, .375, {.125, -.125}},
          {0, 0, .25, 0, .375, {.125, -.125}}}},
        {"DOTS", ansiUnit, {{0, 0, 0, .03125, .0625, {0, -.0625}}}},
        // Architectural patterns are drawn in larger units: one unit per millimetre of
        // spacing keeps the stones and grains readable at the default spacing.
        {"AR-CONC",
         1.0,
         {{50, 0, 0, 4.12975034, -5.89789472, {.75, -8.25}},
          {355, 0, 0, -2.03781207, 7.37236840, {.6, -6.6}},
          {100.45144446, .59771681, -.05229344, 5.7305871, -6.9397673, {.63740192, -7.01142112}},
          {46.1842, 0, 2, 6.19462554, -8.84684596, {1.125, -12.375}},
          {96.63563549, .88936745, 1.86206693, 8.59588239, -10.40964966, {.95610342, -10.5171376}},
          {351.18416399, 0, 2, 7.74327494, 11.05855746, {.9, -9.90000001}},
          {21, 1, 1.5, 4.12975034, -5.89789472, {.75, -8.25}},
          {326, 1, 1.5, -2.03781207, 7.37236840, {.6, -6.6}},
          {71.45144474, 1.49742254, 1.16448426, 5.7305871, -6.9397673, {.6374019, -7.01142112}},
          {37.5, 0, 0, 2.123, 2.567, {0, -6.52, 0, -6.7, 0, -6.625}},
          {7.5, 0, 0, 3.123, 3.567, {0, -3.82, 0, -6.37, 0, -2.525}},
          {-32.5, -2.23, 0, 4.6234, 2.678, {0, -2.5, 0, -7.8, 0, -10.35}},
          {-42.5, -3.23, 0, 3.6234, 4.678, {0, -3.25, 0, -5.18, 0, -7.35}}}},
        {"AR-SAND",
         1.0,
         {{37.5, 0, 0, 1.123, 1.567, {0, -1.52, 0, -1.7, 0, -1.625}},
          {7.5, 0, 0, 2.123, 2.567, {0, -.82, 0, -1.37, 0, -.525}},
          {-32.5, -1.23, 0, 2.6234, 1.678, {0, -.5, 0, -1.8, 0, -2.35}},
          {-42.5, -1.23, 0, 1.6234, 2.678, {0, -.25, 0, -1.18, 0, -1.35}}}},
    };
    return patterns;
}

const HatchPattern* Sketcher::findHatchPattern(std::string_view name)
{
    const auto& patterns = hatchPatterns();
    const auto it = std::find_if(patterns.begin(), patterns.end(), [name](const auto& pattern) {
        return name == pattern.name;
    });
    return it == patterns.end() ? nullptr : &*it;
}

std::vector<PlacedHatchLines> Sketcher::placeHatchPattern(
    std::string_view name,
    const Base::Vector3d& position,
    double rotation,
    double spacing
)
{
    const auto* pattern = findHatchPattern(name);
    if (!pattern) {
        throw Base::ValueError("Unknown hatch pattern: " + std::string(name));
    }
    const double scale = spacing / pattern->unit;
    const double turn = rotation * std::acos(-1.0) / 180.0;
    const double c = std::cos(turn);
    const double s = std::sin(turn);
    std::vector<PlacedHatchLines> result;
    for (const auto& family : pattern->families) {
        const double angle = family.angle * std::acos(-1.0) / 180.0 + turn;
        const Base::Vector3d d(std::cos(angle), std::sin(angle), 0);
        const Base::Vector3d n(-d.y, d.x, 0);
        PlacedHatchLines lines;
        lines.direction = d;
        lines.origin = position
            + Base::Vector3d(c * family.x - s * family.y, s * family.x + c * family.y, 0) * scale;
        lines.offset = (d * family.dx + n * family.dy) * scale;
        for (double dash : family.dashes) {
            lines.dashes.push_back(dash * scale);
        }
        result.push_back(std::move(lines));
    }
    return result;
}
