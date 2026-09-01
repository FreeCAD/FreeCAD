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

#include "DimensionOption.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include <Base/Precision.h>
#include <Base/Type.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "CommandConstraints.h"
#include "DimensionDatumPlacement.h"
#include "Utils.h"

namespace SketcherGui::DimensionOptionDetail
{

using Sketcher::getRadiusCenterCircleArc;
using Sketcher::isCircleOrArc;
using Sketcher::isLineSegment;

constexpr double kDimensionOptionAngleOffset = std::numbers::pi / 4.0;

bool isPointReference(const DimensionReference& ref)
{
    return ref.Pos != Sketcher::PointPos::none;
}

bool isValidDimensionOption(const DimensionOption& option)
{
    const auto refIsValid = [](const DimensionReference& ref) {
        return ref.GeoId != Sketcher::GeoEnum::GeoUndef;
    };

    if (option.constraintType == Sketcher::None
        || !std::all_of(option.refs.begin(), option.refs.end(), refIsValid)) {
        return false;
    }

    switch (option.constraintType) {
        case Sketcher::Distance:
        case Sketcher::DistanceX:
        case Sketcher::DistanceY:
            return !option.refs.empty() && option.refs.size() <= 2;
        case Sketcher::Radius:
        case Sketcher::Diameter:
            return option.refs.size() == 1;
        case Sketcher::Angle:
            return !option.refs.empty() && option.refs.size() <= 3;
        default:
            return false;
    }
}

struct DistanceSegment
{
    Base::Vector2d a;
    Base::Vector2d b;
    double value {0.0};
};

Base::Vector2d toVector2d(const Base::Vector3d& p)
{
    return Base::Vector2d(p.x, p.y);
}

bool shouldSwapDirectLinearEndpoints(const Base::Vector2d& a, const Base::Vector2d& b)
{
    if (b.x < a.x - Base::Precision::Confusion()) {
        return true;
    }
    if (std::abs(b.x - a.x) <= Base::Precision::Confusion()
        && b.y < a.y - Base::Precision::Confusion()) {
        return true;
    }
    return false;
}

bool shouldSwapProjectedEndpoints(
    const Base::Vector2d& a,
    const Base::Vector2d& b,
    Sketcher::ConstraintType constraintType
)
{
    switch (constraintType) {
        case Sketcher::DistanceX:
            return b.x < a.x - Base::Precision::Confusion();
        case Sketcher::DistanceY:
            return b.y < a.y - Base::Precision::Confusion();
        default:
            return false;
    }
}

bool isAxisGeoId(int geoId)
{
    return geoId == Sketcher::GeoEnum::HAxis || geoId == Sketcher::GeoEnum::VAxis;
}

bool isLineGeometry(const Part::Geometry* geometry)
{
    return geometry && isLineSegment(*geometry);
}

bool isRoundGeometry(const Part::Geometry* geometry)
{
    return geometry && isCircleOrArc(*geometry);
}

DistanceSegment makeDistanceSegment(const Base::Vector3d& a, const Base::Vector3d& b, double value)
{
    return DistanceSegment {toVector2d(a), toVector2d(b), value};
}

std::optional<DistanceSegment> directDistanceSegment(
    const Sketcher::SketchObject& sketch,
    const DimensionReference& first,
    const DimensionReference& second
)
{
    const bool firstIsPoint = first.Pos != Sketcher::PointPos::none;
    const bool secondIsPoint = second.Pos != Sketcher::PointPos::none;

    if (!firstIsPoint && secondIsPoint) {
        return directDistanceSegment(sketch, second, first);
    }

    Sketcher::Constraint constraint;
    constraint.Type = Sketcher::Distance;
    constraint.First = first.GeoId;
    constraint.FirstPos = first.Pos;
    constraint.Second = second.GeoId;
    constraint.SecondPos = second.Pos;
    const auto endpoints
        = resolveDimensionDatumEndpoints(sketch.getGeoListFacade(), constraint, Base::Vector2d());
    if (!endpoints) {
        return std::nullopt;
    }

    return makeDistanceSegment(
        endpoints->first,
        endpoints->second,
        (endpoints->second - endpoints->first).Length()
    );
}

bool isLineLikeSelection(const Sketcher::SketchObject& sketch, const DimensionReference& ref)
{
    if (isPointReference(ref)) {
        return false;
    }
    if (isAxisGeoId(ref.GeoId)) {
        return true;
    }
    return isLineGeometry(sketch.getGeometry(ref.GeoId));
}

std::optional<DimensionOption> buildDirectDistanceOption(std::vector<DimensionReference> refs)
{
    if (refs.size() < 2) {
        return std::nullopt;
    }
    return DimensionOption {Sketcher::Distance, std::move(refs)};
}

std::vector<DimensionOption> buildLinearOptions(
    std::initializer_list<Sketcher::ConstraintType> order,
    const std::vector<DimensionReference>& refs,
    std::optional<DimensionOption> directOption
)
{
    std::vector<DimensionOption> result;
    for (const auto constraintType : order) {
        if (constraintType == Sketcher::Distance && directOption) {
            result.push_back(std::move(*directOption));
        }
        else if (constraintType == Sketcher::DistanceX || constraintType == Sketcher::DistanceY) {
            result.push_back({constraintType, refs});
        }
    }
    return result;
}

std::vector<DimensionOption> buildLineOptions(const Sketcher::SketchObject& sketch, int geoId)
{
    const auto* line = freecad_cast<const Part::GeomLineSegment*>(sketch.getGeometry(geoId));
    if (!line) {
        return {};
    }

    const std::vector<DimensionReference> refs {
        DimensionReference {geoId, Sketcher::PointPos::start},
        DimensionReference {geoId, Sketcher::PointPos::end}
    };
    const auto alignedType
        = getAlignedDistanceConstraintType(line->getStartPoint(), line->getEndPoint());
    if (alignedType != Sketcher::None) {
        return {{alignedType, refs}};
    }
    return buildLinearOptions(
        {Sketcher::Distance, Sketcher::DistanceX, Sketcher::DistanceY},
        refs,
        buildDirectDistanceOption(refs)
    );
}

std::vector<DimensionOption> buildRoundOptions(Sketcher::SketchObject* sketch, int geoId)
{
    std::vector<DimensionOption> result;

    const Part::Geometry* geometry = sketch->getGeometry(geoId);
    if (!geometry || !isCircleOrArc(*geometry)) {
        return result;
    }

    const DimensionReference ref {geoId, Sketcher::PointPos::none};
    result.push_back({Sketcher::Radius, {ref}});
    result.push_back({Sketcher::Diameter, {ref}});

    if (const auto* arc = freecad_cast<const Part::GeomArcOfCircle*>(geometry)) {
        result.push_back({Sketcher::Angle, {ref}});
    }

    return result;
}

std::vector<DimensionOption> buildTwoLineOptions(
    Sketcher::SketchObject* sketch,
    int firstGeoId,
    int secondGeoId
)
{
    Sketcher::PointPos firstPos = Sketcher::PointPos::none;
    Sketcher::PointPos secondPos = Sketcher::PointPos::none;
    double angleValue = 0.0;
    if (!calculateAngle(sketch, firstGeoId, secondGeoId, firstPos, secondPos, angleValue)) {
        return {};
    }

    if (angleValue == 0.0) {
        const std::vector<DimensionReference> refs {
            DimensionReference {secondGeoId, Sketcher::PointPos::start},
            DimensionReference {firstGeoId, Sketcher::PointPos::none}
        };
        if (auto option = buildDirectDistanceOption(refs)) {
            return {std::move(*option)};
        }
    }
    else {
        return {{
            Sketcher::Angle,
            {
                DimensionReference {firstGeoId, firstPos},
                DimensionReference {secondGeoId, secondPos},
            },
        }};
    }

    return {};
}

std::vector<DimensionOption> buildTwoPointOptions(
    const Sketcher::SketchObject& sketch,
    const DimensionReference& firstRef,
    const DimensionReference& secondRef
)
{
    const std::vector<DimensionReference> refs {firstRef, secondRef};
    const auto alignedType = getAlignedDistanceConstraintType(
        sketch.getPoint(firstRef.GeoId, firstRef.Pos),
        sketch.getPoint(secondRef.GeoId, secondRef.Pos)
    );
    if (alignedType != Sketcher::None) {
        return {{alignedType, refs}};
    }
    return buildLinearOptions(
        {Sketcher::Distance, Sketcher::DistanceX, Sketcher::DistanceY},
        refs,
        buildDirectDistanceOption(refs)
    );
}

std::vector<DimensionOption> buildSinglePointOptions(const DimensionReference& pointRef)
{
    const DimensionReference origin {Sketcher::GeoEnum::RtPnt, Sketcher::PointPos::start};
    const std::vector<DimensionReference> refs {origin, pointRef};
    return {{Sketcher::DistanceX, refs}, {Sketcher::DistanceY, refs}};
}

std::vector<DimensionOption> buildPointAxisOptions(const DimensionReference& pointRef, int axisGeoId)
{
    const DimensionReference axisRef {axisGeoId, Sketcher::PointPos::start};
    if (axisGeoId == Sketcher::GeoEnum::HAxis) {
        return {{Sketcher::DistanceY, {axisRef, pointRef}}};
    }
    if (axisGeoId == Sketcher::GeoEnum::VAxis) {
        return {{Sketcher::DistanceX, {axisRef, pointRef}}};
    }
    return {};
}

struct DimensionConstraintKey
{
    Sketcher::ConstraintType type;
    std::vector<DimensionReference> refs;

    bool operator==(const DimensionConstraintKey&) const = default;
};

enum class RoundSizeEquivalence
{
    Distinct,
    Equivalent,
};

bool isLinearDimensionType(Sketcher::ConstraintType type)
{
    return type == Sketcher::Distance || type == Sketcher::DistanceX || type == Sketcher::DistanceY;
}

DimensionConstraintKey makeDimensionConstraintKey(
    const Sketcher::Constraint& constraint,
    RoundSizeEquivalence roundSizeEquivalence = RoundSizeEquivalence::Distinct
)
{
    const bool isRoundSize = constraint.Type == Sketcher::Radius
        || constraint.Type == Sketcher::Diameter;
    const auto keyType = isRoundSize && roundSizeEquivalence == RoundSizeEquivalence::Equivalent
        ? Sketcher::Radius
        : constraint.Type;
    DimensionConstraintKey key {keyType, {}};

    if (isRoundSize) {
        key.refs.emplace_back(constraint.First, Sketcher::PointPos::none);
    }
    else if (constraint.Second != Sketcher::GeoEnum::GeoUndef) {
        key.refs.emplace_back(constraint.First, constraint.FirstPos);
        key.refs.emplace_back(constraint.Second, constraint.SecondPos);
        if (constraint.Third != Sketcher::GeoEnum::GeoUndef) {
            key.refs.emplace_back(constraint.Third, constraint.ThirdPos);
        }
    }
    else if (constraint.Type == Sketcher::Distance && constraint.FirstPos == Sketcher::PointPos::none) {
        key.refs.emplace_back(constraint.First, Sketcher::PointPos::start);
        key.refs.emplace_back(constraint.First, Sketcher::PointPos::end);
    }
    else if (isLinearDimensionType(constraint.Type) && constraint.FirstPos != Sketcher::PointPos::none) {
        key.refs.push_back(DimensionReference::RtPnt);
        key.refs.emplace_back(constraint.First, constraint.FirstPos);
    }
    else {
        key.refs.emplace_back(constraint.First, constraint.FirstPos);
    }

    if (key.refs.size() == 2 && std::less<DimensionReference> {}(key.refs[1], key.refs[0])) {
        std::swap(key.refs[0], key.refs[1]);
    }
    return key;
}

void filterPreviewOptions(const Sketcher::SketchObject& sketch, std::vector<DimensionOption>& options)
{
    const auto& constraints = sketch.Constraints.getValues();
    std::vector<DimensionConstraintKey> existingKeys;
    existingKeys.reserve(constraints.size());
    for (const auto* constraint : constraints) {
        if (constraint) {
            existingKeys.push_back(
                makeDimensionConstraintKey(*constraint, RoundSizeEquivalence::Equivalent)
            );
        }
    }

    std::vector<DimensionConstraintKey> candidateKeys;
    std::vector<DimensionOption> unique;
    unique.reserve(options.size());
    for (auto& option : options) {
        const auto preview = buildDimensionConstraint(sketch, option);
        if (!preview) {
            continue;
        }

        auto key = makeDimensionConstraintKey(*preview);
        const auto existingKey = makeDimensionConstraintKey(*preview, RoundSizeEquivalence::Equivalent);
        const auto contains = [&](const auto& keys) {
            return std::find(keys.begin(), keys.end(), key) != keys.end();
        };
        if (std::find(existingKeys.begin(), existingKeys.end(), existingKey) != existingKeys.end()
            || contains(candidateKeys)) {
            continue;
        }

        candidateKeys.push_back(std::move(key));
        unique.push_back(std::move(option));
    }

    options = std::move(unique);
}

}  // namespace SketcherGui::DimensionOptionDetail

namespace SketcherGui
{

using DimensionOptionDetail::buildDirectDistanceOption;
using DimensionOptionDetail::buildLineOptions;
using DimensionOptionDetail::buildPointAxisOptions;
using DimensionOptionDetail::buildRoundOptions;
using DimensionOptionDetail::buildSinglePointOptions;
using DimensionOptionDetail::buildTwoLineOptions;
using DimensionOptionDetail::buildTwoPointOptions;
using DimensionOptionDetail::filterPreviewOptions;
using DimensionOptionDetail::isAxisGeoId;
using DimensionOptionDetail::isLineGeometry;
using DimensionOptionDetail::isLineLikeSelection;
using DimensionOptionDetail::isPointReference;
using DimensionOptionDetail::isRoundGeometry;

std::vector<DimensionOption> buildDimensionOptions(
    Sketcher::SketchObject* sketch,
    const std::vector<DimensionReference>& selectionRefs
)
{
    if (!sketch || selectionRefs.empty() || selectionRefs.size() > 2) {
        return {};
    }

    std::vector<DimensionOption> options;
    const auto setDistanceOption = [&](std::vector<DimensionReference> refs) {
        if (auto option = buildDirectDistanceOption(std::move(refs))) {
            options = {std::move(*option)};
        }
    };

    if (selectionRefs.size() == 1) {
        const auto& item = selectionRefs.front();
        if (isPointReference(item)) {
            options = buildSinglePointOptions(item);
        }
        else if (!isAxisGeoId(item.GeoId)) {
            const Part::Geometry* geometry = sketch->getGeometry(item.GeoId);
            if (isLineGeometry(geometry)) {
                options = buildLineOptions(*sketch, item.GeoId);
            }
            else if (isRoundGeometry(geometry)) {
                options = buildRoundOptions(sketch, item.GeoId);
            }
        }
    }
    else {
        auto first = selectionRefs[0];
        auto second = selectionRefs[1];
        if (isPointReference(first) && isPointReference(second)) {
            options = buildTwoPointOptions(*sketch, first, second);
        }
        else {
            if (isPointReference(second)) {
                std::swap(first, second);
            }
            if (isPointReference(first)) {
                if (isAxisGeoId(second.GeoId)) {
                    options = buildPointAxisOptions(first, second.GeoId);
                }
                else {
                    const Part::Geometry* geometry = sketch->getGeometry(second.GeoId);
                    if (isLineGeometry(geometry) || isRoundGeometry(geometry)) {
                        setDistanceOption({first, second});
                    }
                }
            }
            else if (isLineLikeSelection(*sketch, first) && isLineLikeSelection(*sketch, second)) {
                options = buildTwoLineOptions(sketch, first.GeoId, second.GeoId);
            }
            else {
                const Part::Geometry* firstGeometry = isAxisGeoId(first.GeoId)
                    ? nullptr
                    : sketch->getGeometry(first.GeoId);
                const Part::Geometry* secondGeometry = isAxisGeoId(second.GeoId)
                    ? nullptr
                    : sketch->getGeometry(second.GeoId);
                const bool firstRound = isRoundGeometry(firstGeometry);
                const bool secondRound = isRoundGeometry(secondGeometry);
                const bool firstLine = isLineGeometry(firstGeometry);
                const bool secondLine = isLineGeometry(secondGeometry);
                if ((firstRound && secondRound) || (firstRound && secondLine)) {
                    setDistanceOption({first, second});
                }
                else if (firstLine && secondRound) {
                    setDistanceOption({second, first});
                }
            }
        }
    }

    filterPreviewOptions(*sketch, options);
    return options;
}

namespace DimensionOptionDetail
{

void canonicalizeLinearConstraint(
    const Sketcher::SketchObject& sketch,
    const DimensionOption& option,
    Sketcher::Constraint& constraint
)
{
    if (option.refs.size() < 2 || !isPointReference(option.refs[0])
        || !isPointReference(option.refs[1])) {
        return;
    }

    const auto segment = directDistanceSegment(sketch, option.refs[0], option.refs[1]);
    if (!segment) {
        return;
    }

    bool swap = false;
    if (option.constraintType == Sketcher::DistanceX || option.constraintType == Sketcher::DistanceY) {
        swap = shouldSwapProjectedEndpoints(segment->a, segment->b, option.constraintType);
    }
    else if (option.constraintType == Sketcher::Distance) {
        swap = shouldSwapDirectLinearEndpoints(segment->a, segment->b);
    }

    if (!swap) {
        return;
    }

    std::swap(constraint.First, constraint.Second);
    std::swap(constraint.FirstPos, constraint.SecondPos);
}

}  // namespace DimensionOptionDetail

std::unique_ptr<Sketcher::Constraint> buildDimensionConstraint(
    const Sketcher::SketchObject& sketch,
    const DimensionOption& option,
    CircleDistanceMode circleDistanceMode
)
{
    if (!DimensionOptionDetail::isValidDimensionOption(option)) {
        return {};
    }

    auto constraint = std::make_unique<Sketcher::Constraint>();
    constraint->Type = option.constraintType;
    constraint->isDriving = true;
    if (!option.refs.empty()) {
        constraint->First = option.refs[0].GeoId;
        constraint->FirstPos = option.refs[0].Pos;
    }
    if (option.refs.size() > 1) {
        constraint->Second = option.refs[1].GeoId;
        constraint->SecondPos = option.refs[1].Pos;
    }
    if (option.refs.size() > 2) {
        constraint->Third = option.refs[2].GeoId;
        constraint->ThirdPos = option.refs[2].Pos;
    }

    double value = 0.0;
    bool signedCircleDistance = false;
    if (option.constraintType == Sketcher::Distance || option.constraintType == Sketcher::DistanceX
        || option.constraintType == Sketcher::DistanceY) {
        if (option.refs.size() == 1 && option.constraintType == Sketcher::Distance) {
            const auto* arc = freecad_cast<const Part::GeomArcOfCircle*>(
                sketch.getGeometry(option.refs.front().GeoId)
            );
            if (!arc) {
                return {};
            }
            value = arc->getAngle(false) * arc->getRadius();
        }
        else {
            if (option.refs.size() < 2) {
                return {};
            }
            const auto* firstGeometry = option.refs[0].Pos == Sketcher::PointPos::none
                ? sketch.getGeometry(option.refs[0].GeoId)
                : nullptr;
            const auto* secondGeometry = option.refs[1].Pos == Sketcher::PointPos::none
                ? sketch.getGeometry(option.refs[1].GeoId)
                : nullptr;
            signedCircleDistance = circleDistanceMode == CircleDistanceMode::Signed
                && option.constraintType == Sketcher::Distance && firstGeometry && secondGeometry
                && Sketcher::isCircleOrArc(*firstGeometry)
                && Sketcher::isCircleOrArc(*secondGeometry);
            if (signedCircleDistance) {
                value = getCirclesSignedDistance(firstGeometry, secondGeometry);
            }
            if (!signedCircleDistance) {
                const auto segment = DimensionOptionDetail::directDistanceSegment(
                    sketch,
                    option.refs[0],
                    option.refs[1]
                );
                if (!segment) {
                    return {};
                }
                value = option.constraintType == Sketcher::DistanceX
                    ? std::abs(segment->b.x - segment->a.x)
                    : option.constraintType == Sketcher::DistanceY
                    ? std::abs(segment->b.y - segment->a.y)
                    : segment->value;
            }
        }
    }
    else if (option.constraintType == Sketcher::Radius || option.constraintType == Sketcher::Diameter) {
        const auto* geometry = sketch.getGeometry(option.refs.front().GeoId);
        if (!geometry || !Sketcher::isCircleOrArc(*geometry)) {
            return {};
        }
        const auto radius = std::get<0>(Sketcher::getRadiusCenterCircleArc(geometry));
        value = option.constraintType == Sketcher::Diameter ? 2.0 * radius : radius;
    }
    else if (option.constraintType == Sketcher::Angle) {
        if (option.refs.size() == 1) {
            const auto* arc = freecad_cast<const Part::GeomArcOfCircle*>(
                sketch.getGeometry(option.refs.front().GeoId)
            );
            if (!arc) {
                return {};
            }
            value = arc->getAngle(true);
        }
        else {
            int first = option.refs[0].GeoId;
            int second = option.refs[1].GeoId;
            auto firstPos = option.refs[0].Pos;
            auto secondPos = option.refs[1].Pos;
            if (!calculateAngle(&sketch, first, second, firstPos, secondPos, value)) {
                return {};
            }
            constraint->First = first;
            constraint->FirstPos = firstPos;
            constraint->Second = second;
            constraint->SecondPos = secondPos;
        }
    }
    if (!std::isfinite(value) || (!signedCircleDistance && value <= Base::Precision::Confusion())) {
        return {};
    }
    constraint->setValue(value);

    if (option.constraintType == Sketcher::Radius || option.constraintType == Sketcher::Diameter) {
        double angle = 0.0;
        bool isArc = false;
        if (!option.refs.empty()) {
            const Part::Geometry* geometry = sketch.getGeometry(option.refs.front().GeoId);
            if (const auto* arc = freecad_cast<const Part::GeomArcOfCircle*>(geometry)) {
                double startAngle = 0.0;
                double endAngle = 0.0;
                arc->getRange(startAngle, endAngle, /*emulateCCW=*/true);
                angle = 0.5 * (startAngle + endAngle);
                isArc = true;
            }
        }
        if (isArc) {
            constraint->LabelPosition = option.constraintType == Sketcher::Diameter
                ? angle + 2.0 * DimensionOptionDetail::kDimensionOptionAngleOffset
                : angle + DimensionOptionDetail::kDimensionOptionAngleOffset;
        }
        else {
            constraint->LabelPosition = option.constraintType == Sketcher::Diameter
                ? angle + DimensionOptionDetail::kDimensionOptionAngleOffset
                : angle;
        }
    }
    else if (option.constraintType == Sketcher::Angle && option.refs.size() == 1) {
        const Part::Geometry* geometry = sketch.getGeometry(option.refs.front().GeoId);
        if (const auto* arc = freecad_cast<const Part::GeomArcOfCircle*>(geometry)) {
            double startAngle = 0.0;
            double endAngle = 0.0;
            arc->getRange(startAngle, endAngle, /*emulateCCW=*/true);
            constraint->LabelPosition = 0.5 * (startAngle + endAngle)
                - DimensionOptionDetail::kDimensionOptionAngleOffset;
        }
    }
    DimensionOptionDetail::canonicalizeLinearConstraint(sketch, option, *constraint);
    return constraint;
}

}  // namespace SketcherGui
