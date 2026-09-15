// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cmath>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include "GroupGeometry.h"

std::vector<std::unique_ptr<Part::Geometry>> Sketcher::transformGroupGeometry(
    const std::vector<Part::Geometry*>& geometry,
    const Base::Vector3d& start,
    const Base::Vector3d& end,
    bool height
)
{
    Bnd_Box bounds;
    for (const auto* geo : geometry) {
        if (!geo
            || !(
                geo->is<Part::GeomPoint>() || geo->is<Part::GeomLineSegment>()
                || geo->is<Part::GeomCircle>() || geo->is<Part::GeomArcOfCircle>()
                || geo->is<Part::GeomEllipse>() || geo->is<Part::GeomArcOfEllipse>()
                || geo->is<Part::GeomArcOfHyperbola>() || geo->is<Part::GeomArcOfParabola>()
                || geo->is<Part::GeomBSplineCurve>()
            )) {
            throw Base::TypeError("Unsupported geometry in group source");
        }
        BRepBndLib::AddOptimal(geo->toShape(), bounds, false, false);
    }
    if (bounds.IsVoid() || bounds.IsOpen()) {
        throw Base::ValueError("The source file contains no finite geometry");
    }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    const double size = height ? ymax - ymin : xmax - xmin;
    const auto direction = end - start;
    if (size < Precision::Confusion() || direction.Length() < Precision::Confusion()) {
        throw Base::ValueError("The group width or height must be greater than zero");
    }
    if (std::abs(zmin) > Precision::Confusion() || std::abs(zmax) > Precision::Confusion()) {
        throw Base::ValueError("Group geometry must lie in the sketch XY plane");
    }
    const double scale = direction.Length() / size;
    const double angle = std::atan2(direction.y, direction.x) - (height ? M_PI / 2 : 0);
    const double c = scale * std::cos(angle);
    const double s = scale * std::sin(angle);
    Base::Matrix4D matrix;
    matrix[0][0] = c;
    matrix[0][1] = -s;
    matrix[1][0] = s;
    matrix[1][1] = c;
    matrix[2][2] = scale;
    matrix[0][3] = start.x - c * xmin + s * ymin;
    matrix[1][3] = start.y - s * xmin - c * ymin;

    std::vector<std::unique_ptr<Part::Geometry>> result;
    result.reserve(geometry.size());
    for (const auto* geo : geometry) {
        auto copy = std::unique_ptr<Part::Geometry>(geo->copy());
        copy->transform(matrix);
        result.push_back(std::move(copy));
    }
    return result;
}
