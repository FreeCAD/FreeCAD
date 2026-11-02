// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <string_view>
#include <vector>
#include <Base/Vector3D.h>
#include <Mod/Sketcher/SketcherGlobal.h>

namespace Sketcher
{
/// One line family of an AutoCAD/ISO PAT definition, in the pattern's own units:
/// `angle, x, y, dx, dy [, dash...]`. dx shifts successive lines along the line
/// direction, dy is the perpendicular pitch, positive dashes are drawn, negative dashes
/// are gaps and zero is a dot.
struct HatchLineFamily
{
    double angle;
    double x;
    double y;
    double dx;
    double dy;
    std::vector<double> dashes;
};

/// A named hatch pattern. The families are the published PAT data, so the TechDraw
/// counterpart can embed exactly what the sketch draws. `unit` is the pattern length
/// that the annotation's Spacing maps to: the pitch of the general-purpose ANSI31 lines
/// for every drafting pattern, so their relative proportions stay standard.
struct HatchPattern
{
    const char* name;
    double unit;
    std::vector<HatchLineFamily> families;
};

/// A line family placed in sketch coordinates: the lines pass through
/// `origin + k * offset` along `direction` (unit length) and the dash sequence starts at
/// each of those points. Lengths are in sketch millimetres.
struct PlacedHatchLines
{
    Base::Vector3d origin;
    Base::Vector3d direction;
    Base::Vector3d offset;
    std::vector<double> dashes;
};

/// The pattern scaled so that its unit is `spacing`, turned by `rotation` degrees about
/// `position`, and moved there. Throws for an unknown pattern.
SketcherExport std::vector<PlacedHatchLines> placeHatchPattern(
    std::string_view name,
    const Base::Vector3d& position,
    double rotation,
    double spacing
);

/// The patterns in presentation order. Names are stable file identifiers.
SketcherExport const std::vector<HatchPattern>& hatchPatterns();
SketcherExport const HatchPattern* findHatchPattern(std::string_view name);
/// Used for new hatches and for files saved before patterns existed.
constexpr const char* defaultHatchPattern = "ANSI31";
}  // namespace Sketcher
