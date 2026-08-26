// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak                                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA 02111-1307, USA                                 *
 *                                                                         *
 ***************************************************************************/

#include "DimensionDatumPlacement.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>

#include <Base/Precision.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DimensionOption.h"
#include "Utils.h"
#include "ViewProviderSketchGeometryExtension.h"

namespace SketcherGui::DimensionDatumPlacementDetail
{

using Sketcher::getRadiusCenterCircleArc;
using Sketcher::isArcOfCircle;
using Sketcher::isCircleOrArc;
using Sketcher::isLineSegment;

std::optional<Base::Vector3d> projectPointOntoLine(
    const Base::Vector3d& point,
    const Part::GeomLineSegment& line
)
{
    const Base::Vector3d start = line.getStartPoint();
    const Base::Vector3d direction = line.getEndPoint() - start;
    if (direction.Length() <= Base::Precision::Confusion()) {
        // A collapsed line has no stable perpendicular projection.
        return std::nullopt;
    }

    Base::Vector3d projected;
    projected.ProjectToLine(point - start, direction);
    projected += point;
    return projected;
}

Base::Vector3d radialDirection(
    const Base::Vector3d& from,
    Base::Vector3d fallbackLineDirection = Base::Vector3d()
)
{
    Base::Vector3d direction = from;
    if (direction.Length() <= Base::Precision::Confusion()) {
        direction = fallbackLineDirection;
        if (direction.Length() <= Base::Precision::Confusion()) {
            return Base::Vector3d(1.0, 0.0, 0.0);
        }
        direction.RotateZ(std::numbers::pi / 2.0);
    }
    return direction.Normalize();
}

double weightRepresentationFactor(const Part::Geometry& geometry)
{
    if (!geometry.hasExtension(ViewProviderSketchGeometryExtension::getClassTypeId())) {
        return 1.0;
    }

    const auto extension = std::static_pointer_cast<const ViewProviderSketchGeometryExtension>(
        geometry.getExtension(ViewProviderSketchGeometryExtension::getClassTypeId()).lock()
    );
    return extension ? extension->getRepresentationFactor() : 1.0;
}

std::optional<DimensionDatumEndpoints> pointGeometryEndpoints(
    const Base::Vector3d& point,
    const Part::Geometry& geometry
)
{
    if (isLineSegment(geometry)) {
        const auto& line = static_cast<const Part::GeomLineSegment&>(geometry);
        if (const auto projected = projectPointOntoLine(point, line)) {
            return DimensionDatumEndpoints {point, *projected};
        }
        return std::nullopt;
    }

    if (isCircleOrArc(geometry)) {
        const auto [radius, center] = getRadiusCenterCircleArc(&geometry);
        return DimensionDatumEndpoints {point, center + radius * radialDirection(point - center)};
    }
    return std::nullopt;
}

std::optional<DimensionDatumEndpoints> roundLineEndpoints(
    const Part::Geometry& round,
    const Part::GeomLineSegment& line
)
{
    const auto [radius, center] = getRadiusCenterCircleArc(&round);
    const auto projected = projectPointOntoLine(center, line);
    if (!projected) {
        return std::nullopt;
    }

    const Base::Vector3d lineDirection = line.getEndPoint() - line.getStartPoint();
    const Base::Vector3d roundPoint = center
        + radius * radialDirection(*projected - center, lineDirection);
    return DimensionDatumEndpoints {*projected, roundPoint};
}

std::optional<DimensionDatumEndpoints> twoGeometryEndpoints(
    const Sketcher::GeoListFacade& geometry,
    const Sketcher::Constraint& constraint
)
{
    const Part::Geometry* firstGeometry = geometry.getGeometryFromGeoId(constraint.First);
    const Part::Geometry* secondGeometry = geometry.getGeometryFromGeoId(constraint.Second);
    if (!firstGeometry || !secondGeometry) {
        return std::nullopt;
    }

    if (constraint.FirstPos != Sketcher::PointPos::none) {
        const Base::Vector3d point = geometry.getPoint(constraint.First, constraint.FirstPos);
        return pointGeometryEndpoints(point, *secondGeometry);
    }
    if (isCircleOrArc(*firstGeometry) && isLineSegment(*secondGeometry)) {
        return roundLineEndpoints(
            *firstGeometry,
            static_cast<const Part::GeomLineSegment&>(*secondGeometry)
        );
    }
    if (isLineSegment(*firstGeometry) && isCircleOrArc(*secondGeometry)) {
        return roundLineEndpoints(
            *secondGeometry,
            static_cast<const Part::GeomLineSegment&>(*firstGeometry)
        );
    }
    if (isCircleOrArc(*firstGeometry) && isCircleOrArc(*secondGeometry)) {
        DimensionDatumEndpoints endpoints;
        GetCirclesMinimalDistance(firstGeometry, secondGeometry, endpoints.first, endpoints.second);
        return endpoints;
    }
    return std::nullopt;
}

double defaultRoundAngle(const Part::Geometry& geometry, Sketcher::ConstraintType type)
{
    if (isArcOfCircle(geometry)) {
        const auto& arc = static_cast<const Part::GeomArcOfCircle&>(geometry);
        double startAngle = 0.0;
        double endAngle = 0.0;
        arc.getRange(startAngle, endAngle, /*emulateCCW=*/true);
        return (startAngle + endAngle) / 2.0;
    }
    return type == Sketcher::Diameter ? std::numbers::pi / 4.0 : 0.0;
}

std::optional<DimensionDatumEndpoints> singleGeometryEndpoints(
    const Part::Geometry& geometry,
    const Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition
)
{
    if (isLineSegment(geometry)) {
        const auto& line = static_cast<const Part::GeomLineSegment&>(geometry);
        return DimensionDatumEndpoints {line.getStartPoint(), line.getEndPoint()};
    }
    if (!isCircleOrArc(geometry)) {
        return std::nullopt;
    }

    const auto [radius, center] = getRadiusCenterCircleArc(&geometry);
    const double defaultAngle = defaultRoundAngle(geometry, constraint.Type);
    const Base::Vector3d centerToLabel = Base::Vector3d(labelPosition.x, labelPosition.y, 0.0)
        - center;
    double angle = constraint.LabelPosition;
    if (isArcOfCircle(geometry) && isAutoDatumLabelPosition(angle)) {
        angle = defaultAngle;
    }
    else if (centerToLabel.Length() > Base::Precision::Confusion()) {
        angle = std::atan2(centerToLabel.y, centerToLabel.x);
    }
    else if (isAutoDatumLabelPosition(angle)) {
        angle = defaultAngle;
    }

    const Base::Vector3d radial(std::cos(angle), std::sin(angle), 0.0);
    Base::Vector3d first = center;
    if (constraint.Type == Sketcher::Diameter) {
        first -= radius * radial;
    }
    const double displayRadius = constraint.Type == Sketcher::Weight
        ? radius * weightRepresentationFactor(geometry)
        : radius;
    return DimensionDatumEndpoints {first, center + displayRadius * radial};
}

std::optional<DimensionDatumEndpoints> resolveDatumEndpoints(
    const Sketcher::GeoListFacade& geometry,
    const Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition
)
{
    if (constraint.SecondPos != Sketcher::PointPos::none) {
        return DimensionDatumEndpoints {
            geometry.getPoint(constraint.First, constraint.FirstPos),
            geometry.getPoint(constraint.Second, constraint.SecondPos),
        };
    }
    if (constraint.Second != Sketcher::GeoEnum::GeoUndef) {
        return twoGeometryEndpoints(geometry, constraint);
    }
    if (constraint.FirstPos != Sketcher::PointPos::none) {
        return DimensionDatumEndpoints {
            Base::Vector3d(),
            geometry.getPoint(constraint.First, constraint.FirstPos),
        };
    }
    if (constraint.First == Sketcher::GeoEnum::GeoUndef) {
        return std::nullopt;
    }

    const Part::Geometry* firstGeometry = geometry.getGeometryFromGeoId(constraint.First);
    return firstGeometry ? singleGeometryEndpoints(*firstGeometry, constraint, labelPosition)
                         : std::nullopt;
}

struct LineEndpoints
{
    Base::Vector2d start;
    Base::Vector2d end;
};

[[nodiscard]] LineEndpoints lineEndpoints(const Part::GeomLineSegment& line)
{
    return {
        Base::Vector2d(line.getStartPoint().x, line.getStartPoint().y),
        Base::Vector2d(line.getEndPoint().x, line.getEndPoint().y),
    };
}

[[nodiscard]] std::optional<Base::Vector2d> lineIntersection(
    const LineEndpoints& first,
    const LineEndpoints& second
)
{
    Base::Vector2d intersection;
    return Base::Line2d(first.start, first.end)
               .Intersect(Base::Line2d(second.start, second.end), intersection)
        ? std::optional<Base::Vector2d> {intersection}
        : std::nullopt;
}

namespace LinearDatumLabelPlacement
{
[[nodiscard]] std::optional<Base::Vector2d> computeLabelPosition(
    const Sketcher::SketchObject& sketch,
    const Sketcher::Constraint& constraint
)
{
    if (constraint.Type != Sketcher::DistanceX && constraint.Type != Sketcher::DistanceY
        && constraint.Type != Sketcher::Distance) {
        return std::nullopt;
    }

    const auto projectPoint = [](const Base::Vector3d& point) {
        return Base::Vector2d(point.x, point.y);
    };

    const auto endpoints
        = resolveDatumEndpoints(sketch.getGeoListFacade(), constraint, Base::Vector2d());
    if (!endpoints) {
        return std::nullopt;
    }
    Base::Vector2d firstPoint = projectPoint(endpoints->first);
    Base::Vector2d secondPoint = projectPoint(endpoints->second);

    const double eps = Base::Precision::Confusion();
    Base::Vector2d labelDirection(0.0, 0.0);

    switch (constraint.Type) {
        case Sketcher::DistanceX:
            if (secondPoint.x < firstPoint.x - eps) {
                std::swap(firstPoint, secondPoint);
            }
            labelDirection = Base::Vector2d(0.0, -1.0);
            break;
        case Sketcher::DistanceY:
            if (secondPoint.y < firstPoint.y - eps) {
                std::swap(firstPoint, secondPoint);
            }
            if (secondPoint.x > firstPoint.x + eps) {
                labelDirection = Base::Vector2d(1.0, 0.0);
            }
            else if (secondPoint.x < firstPoint.x - eps) {
                labelDirection = Base::Vector2d(-1.0, 0.0);
            }
            else {
                labelDirection = Base::Vector2d(1.0, 0.0);
            }
            break;
        case Sketcher::Distance:
            if (secondPoint.y < firstPoint.y - eps
                || (std::abs(secondPoint.y - firstPoint.y) <= eps
                    && secondPoint.x < firstPoint.x - eps)) {
                std::swap(firstPoint, secondPoint);
            }
            {
                const Base::Vector2d span = secondPoint - firstPoint;
                const double spanLength = span.Length();
                if (spanLength <= eps) {
                    return std::nullopt;
                }
                labelDirection = span.x >= 0.0
                    ? Base::Vector2d(-span.y / spanLength, span.x / spanLength)
                    : Base::Vector2d(span.y / spanLength, -span.x / spanLength);
            }
            break;
        default:
            return std::nullopt;
    }

    const Base::Vector2d span = secondPoint - firstPoint;
    return (firstPoint + secondPoint) * 0.5
        + labelDirection
        * (constraint.LabelDistance
           + 0.5 * std::abs(span.x * labelDirection.x + span.y * labelDirection.y));
}
}  // namespace LinearDatumLabelPlacement

enum class SegmentIntersection
{
    Reject,
    Allow,
};

namespace AngularDatumLabelPlacement
{
[[nodiscard]] std::optional<Base::Vector2d> computeLabelPosition(
    const Sketcher::SketchObject& sketch,
    const Sketcher::Constraint& constraint,
    SegmentIntersection segmentIntersection = SegmentIntersection::Reject
)
{
    if (constraint.Type != Sketcher::Angle) {
        return std::nullopt;
    }

    const bool firstIsAxis = constraint.First == Sketcher::GeoEnum::HAxis
        || constraint.First == Sketcher::GeoEnum::VAxis;
    const bool secondIsAxis = constraint.Second == Sketcher::GeoEnum::HAxis
        || constraint.Second == Sketcher::GeoEnum::VAxis;

    Base::Vector2d vertex;
    Base::Vector2d rayPoint1;
    Base::Vector2d rayPoint2;
    double radius = 0.0;

    const auto vertexOnSegment = [](const Base::Vector2d& segmentStart,
                                    const Base::Vector2d& segmentSpan,
                                    const Base::Vector2d& point) {
        const double tolerance = Base::Precision::Confusion();
        const Base::Vector2d segmentToPoint = point - segmentStart;

        const double crossProduct = segmentSpan.x * segmentToPoint.y
            - segmentSpan.y * segmentToPoint.x;
        const double dotProduct = segmentToPoint.x * segmentSpan.x + segmentToPoint.y * segmentSpan.y;
        const double segmentLengthSquared = segmentSpan.Sqr();

        return std::abs(crossProduct) <= tolerance && dotProduct >= -tolerance
            && dotProduct <= segmentLengthSquared + tolerance;
    };

    if (constraint.Second == Sketcher::GeoEnum::GeoUndef) {
        const auto* arc = freecad_cast<const Part::GeomArcOfCircle*>(
            sketch.getGeometry(constraint.First)
        );

        if (!arc) {
            return std::nullopt;
        }

        const Base::Vector2d center(arc->getCenter().x, arc->getCenter().y);
        const double radius = arc->getRadius();
        const double middleAngle = defaultRoundAngle(*arc, constraint.Type);
        return center
            + Base::Vector2d(std::cos(middleAngle), std::sin(middleAngle))
            * (radius + constraint.LabelDistance);
    }
    else if (firstIsAxis != secondIsAxis) {
        const int axisGeoId = firstIsAxis ? constraint.First : constraint.Second;
        const auto* lineSegment = freecad_cast<const Part::GeomLineSegment*>(
            sketch.getGeometry(firstIsAxis ? constraint.Second : constraint.First)
        );
        if (!lineSegment) {
            return std::nullopt;
        }

        const auto [startPoint, endPoint] = lineEndpoints(*lineSegment);
        const Base::Vector2d lineSpan = endPoint - startPoint;
        const LineEndpoints axis {
            {0.0, 0.0},
            axisGeoId == Sketcher::GeoEnum::HAxis ? Base::Vector2d(1.0, 0.0)
                                                  : Base::Vector2d(0.0, 1.0),
        };
        const auto intersection = lineIntersection(LineEndpoints {startPoint, endPoint}, axis);
        if (!intersection) {
            return std::nullopt;
        }
        vertex = *intersection;

        const bool intersectionOnSegment = vertexOnSegment(startPoint, lineSpan, vertex);
        if (intersectionOnSegment && segmentIntersection == SegmentIntersection::Reject) {
            return std::nullopt;
        }

        if (intersectionOnSegment) {
            const auto linePos = firstIsAxis ? constraint.SecondPos : constraint.FirstPos;
            rayPoint2 = linePos == Sketcher::PointPos::start ? endPoint : startPoint;
        }
        else {
            rayPoint2 = (startPoint - vertex).Sqr() <= (endPoint - vertex).Sqr() ? startPoint
                                                                                 : endPoint;
        }

        radius = (rayPoint2 - vertex).Length();
        if (radius <= Base::Precision::Confusion()) {
            return std::nullopt;
        }

        rayPoint1 = axisGeoId == Sketcher::GeoEnum::HAxis
            ? vertex + Base::Vector2d((rayPoint2 - vertex).x >= 0.0 ? radius : -radius, 0.0)
            : vertex + Base::Vector2d(0.0, (rayPoint2 - vertex).y >= 0.0 ? radius : -radius);
    }
    else {
        const auto* firstLine = freecad_cast<const Part::GeomLineSegment*>(
            sketch.getGeometry(constraint.First)
        );
        const auto* secondLine = freecad_cast<const Part::GeomLineSegment*>(
            sketch.getGeometry(constraint.Second)
        );
        if (!firstLine || !secondLine) {
            return std::nullopt;
        }

        const auto [firstStart, firstEnd] = lineEndpoints(*firstLine);
        const auto [secondStart, secondEnd] = lineEndpoints(*secondLine);
        const Base::Vector2d firstSpan = firstEnd - firstStart;
        const Base::Vector2d secondSpan = secondEnd - secondStart;
        const auto intersection = lineIntersection(
            LineEndpoints {firstStart, firstEnd},
            LineEndpoints {secondStart, secondEnd}
        );
        if (!intersection) {
            return std::nullopt;
        }
        vertex = *intersection;
        const bool intersectionOnSegments = vertexOnSegment(firstStart, firstSpan, vertex)
            && vertexOnSegment(secondStart, secondSpan, vertex);
        if (intersectionOnSegments && segmentIntersection == SegmentIntersection::Reject) {
            return std::nullopt;
        }

        if (intersectionOnSegments) {
            rayPoint1 = constraint.FirstPos == Sketcher::PointPos::start ? firstEnd : firstStart;
            rayPoint2 = constraint.SecondPos == Sketcher::PointPos::start ? secondEnd : secondStart;
        }
        else {
            rayPoint1 = (firstStart - vertex).Sqr() <= (firstEnd - vertex).Sqr() ? firstStart
                                                                                 : firstEnd;
            rayPoint2 = (secondStart - vertex).Sqr() <= (secondEnd - vertex).Sqr() ? secondStart
                                                                                   : secondEnd;
        }
        radius = std::min((rayPoint1 - vertex).Length(), (rayPoint2 - vertex).Length());
        if (radius <= Base::Precision::Confusion()) {
            return std::nullopt;
        }
    }

    Base::Vector2d firstDirection = rayPoint1 - vertex;
    Base::Vector2d secondDirection = rayPoint2 - vertex;
    if (firstDirection.Length() <= Base::Precision::Confusion()
        || secondDirection.Length() <= Base::Precision::Confusion()) {
        return std::nullopt;
    }

    firstDirection.Normalize();
    secondDirection.Normalize();
    Base::Vector2d position = firstDirection + secondDirection;
    if (position.Length() <= Base::Precision::Confusion()) {
        position = firstDirection.Perpendicular(false);
    }
    else {
        position.Normalize();
    }

    return vertex + position * (radius + constraint.LabelDistance);
}
}  // namespace AngularDatumLabelPlacement

bool applyArcLengthPlacement(
    const Part::GeomArcOfCircle& arc,
    Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition
)
{
    const double angle = defaultRoundAngle(arc, constraint.Type);
    const Base::Vector2d direction(std::cos(angle), std::sin(angle));
    const Base::Vector3d center = arc.getCenter();
    constraint.LabelDistance = (labelPosition - Base::Vector2d(center.x, center.y)) * direction;
    return true;
}

bool applyLinearPlacement(
    const Sketcher::SketchObject& sketch,
    Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition,
    double labelOffset
)
{
    if (constraint.Type == Sketcher::Distance && constraint.Second == Sketcher::GeoEnum::GeoUndef
        && constraint.FirstPos == Sketcher::PointPos::none) {
        const Part::Geometry* geometry = sketch.getGeometry(constraint.First);
        if (geometry && isArcOfCircle(*geometry)) {
            return applyArcLengthPlacement(
                static_cast<const Part::GeomArcOfCircle&>(*geometry),
                constraint,
                labelPosition
            );
        }
    }

    const auto endpoints = resolveDatumEndpoints(sketch.getGeoListFacade(), constraint, labelPosition);
    if (!endpoints) {
        return false;
    }

    const Base::Vector3d datumDirection = endpoints->second - endpoints->first;
    if (datumDirection.Length() <= Base::Precision::Confusion()) {
        return false;
    }

    const Base::Vector3d label(labelPosition.x, labelPosition.y, 0.0);
    Base::Vector3d direction;
    if (constraint.Type == Sketcher::Distance || constraint.Type == Sketcher::Radius
        || constraint.Type == Sketcher::Diameter || constraint.Type == Sketcher::Weight) {
        direction = datumDirection;
        direction.Normalize();
    }
    else if (constraint.Type == Sketcher::DistanceX) {
        direction = Base::Vector3d(
            datumDirection.x >= std::numeric_limits<float>::epsilon() ? 1.0 : -1.0,
            0.0,
            0.0
        );
    }
    else if (constraint.Type == Sketcher::DistanceY) {
        direction = Base::Vector3d(
            0.0,
            datumDirection.y >= std::numeric_limits<float>::epsilon() ? 1.0 : -1.0,
            0.0
        );
    }
    else {
        return false;
    }

    if (constraint.Type == Sketcher::Radius || constraint.Type == Sketcher::Diameter
        || constraint.Type == Sketcher::Weight) {
        const Base::Vector3d labelVector = label - endpoints->second;
        double distance = labelVector * direction;
        if (distance > labelOffset) {
            distance -= labelOffset;
        }
        constraint.LabelDistance = distance;
        constraint.LabelPosition = std::atan2(direction.y, direction.x);
        return true;
    }

    const Base::Vector3d normal(-direction.y, direction.x, 0.0);
    constraint.LabelDistance = (label - endpoints->second) * normal - labelOffset;
    constraint.LabelPosition = (label - (endpoints->second + endpoints->first) / 2.0) * direction;
    return true;
}

bool applyAnglePlacement(
    const Sketcher::SketchObject& sketch,
    Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition
)
{
    Base::Vector3d vertex;
    constexpr double angleLabelDistanceScale = 0.5;

    if (constraint.Second != Sketcher::GeoEnum::GeoUndef) {
        if (constraint.Third == Sketcher::GeoEnum::GeoUndef) {
            const Part::Geometry* firstGeometry = sketch.getGeometry(constraint.First);
            const Part::Geometry* secondGeometry = sketch.getGeometry(constraint.Second);
            if (!firstGeometry || !secondGeometry || !isLineSegment(*firstGeometry)
                || !isLineSegment(*secondGeometry)) {
                return false;
            }

            const auto& firstLine = static_cast<const Part::GeomLineSegment&>(*firstGeometry);
            const auto& secondLine = static_cast<const Part::GeomLineSegment&>(*secondGeometry);
            const auto intersection
                = lineIntersection(lineEndpoints(firstLine), lineEndpoints(secondLine));
            if (!intersection) {
                return false;
            }
            vertex = Base::Vector3d(intersection->x, intersection->y, 0.0);
        }
        else {
            vertex = sketch.getPoint(constraint.Third, constraint.ThirdPos);
        }
    }
    else if (constraint.First != Sketcher::GeoEnum::GeoUndef) {
        const Part::Geometry* geometry = sketch.getGeometry(constraint.First);
        if (!geometry) {
            return false;
        }
        if (isLineSegment(*geometry)) {
            const auto& line = static_cast<const Part::GeomLineSegment&>(*geometry);
            vertex = (line.getEndPoint() + line.getStartPoint()) / 2.0;
        }
        else if (isArcOfCircle(*geometry)) {
            const auto& arc = static_cast<const Part::GeomArcOfCircle&>(*geometry);
            const double middleAngle = defaultRoundAngle(arc, constraint.Type);
            const Base::Vector2d arcDirection(std::cos(middleAngle), std::sin(middleAngle));
            const Base::Vector3d center = arc.getCenter();
            const Base::Vector2d centerToLabel = labelPosition - Base::Vector2d(center.x, center.y);
            constraint.LabelDistance = angleLabelDistanceScale * (centerToLabel * arcDirection);
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }

    const Base::Vector3d label(labelPosition.x, labelPosition.y, 0.0);
    constraint.LabelDistance = angleLabelDistanceScale * (label - vertex).Length();
    return true;
}

}  // namespace SketcherGui::DimensionDatumPlacementDetail

namespace SketcherGui
{

std::optional<DimensionDatumEndpoints> resolveDimensionDatumEndpoints(
    const Sketcher::GeoListFacade& geometry,
    const Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition
)
{
    return DimensionDatumPlacementDetail::resolveDatumEndpoints(geometry, constraint, labelPosition);
}

std::optional<Base::Vector2d> defaultDimensionDatumLabelPosition(
    const Sketcher::SketchObject& sketch,
    const Sketcher::Constraint& constraint
)
{
    if (constraint.Type == Sketcher::Distance || constraint.Type == Sketcher::DistanceX
        || constraint.Type == Sketcher::DistanceY) {
        return DimensionDatumPlacementDetail::LinearDatumLabelPlacement::computeLabelPosition(
            sketch,
            constraint
        );
    }
    else if (constraint.Type == Sketcher::Angle) {
        return DimensionDatumPlacementDetail::AngularDatumLabelPlacement::computeLabelPosition(
            sketch,
            constraint
        );
    }
    return std::nullopt;
}

bool prepareDimensionDatumPlacement(
    const Sketcher::SketchObject& sketch,
    Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition,
    double labelOffset
)
{
    if (constraint.Type == Sketcher::Distance || constraint.Type == Sketcher::DistanceX
        || constraint.Type == Sketcher::DistanceY || constraint.Type == Sketcher::Radius
        || constraint.Type == Sketcher::Diameter || constraint.Type == Sketcher::Weight) {
        return DimensionDatumPlacementDetail::applyLinearPlacement(
            sketch,
            constraint,
            labelPosition,
            labelOffset
        );
    }
    if (constraint.Type == Sketcher::Angle) {
        return DimensionDatumPlacementDetail::applyAnglePlacement(sketch, constraint, labelPosition);
    }
    return false;
}

bool prepareDimensionDatumPlacement(const Sketcher::SketchObject& sketch, Sketcher::Constraint& constraint)
{
    const auto position = constraint.Type == Sketcher::Angle
        ? DimensionDatumPlacementDetail::AngularDatumLabelPlacement::computeLabelPosition(
              sketch,
              constraint,
              DimensionDatumPlacementDetail::SegmentIntersection::Allow
          )
        : defaultDimensionDatumLabelPosition(sketch, constraint);
    return position && prepareDimensionDatumPlacement(sketch, constraint, *position);
}

}  // namespace SketcherGui
