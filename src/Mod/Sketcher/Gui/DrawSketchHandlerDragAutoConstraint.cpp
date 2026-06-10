// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak <furkan1795@gmail.com>          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include <QWidget>

#include <Precision.hxx>
#include <Base/Vector3D.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandlerDragAutoConstraint.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

using namespace SketcherGui;
using namespace Sketcher;

namespace
{
constexpr double DragAutoConstraintSnapDistanceFactor = 0.25;
}

bool DrawSketchHandlerDragAutoConstraint::canSuggestFor(const std::vector<GeoElementId>& dragged) const
{
    return sketchgui && sketchgui->Autoconstraints.getValue() && dragged.size() == 1
        && dragged.front().Pos != PointPos::none;
}

void DrawSketchHandlerDragAutoConstraint::initDragging(const std::vector<GeoElementId>& dragged)
{
    suggestedConstraints.clear();

    if (QWidget* widget = getCursorWidget()) {
        oldCursor = widget->cursor();
    }

    SketchObject* obj = getSketchObject();
    if (!obj || !canSuggestFor(dragged)) {
        return;
    }

    updateCursor();
    startPos = toVector2d(obj->getPoint(dragged.front().GeoId, dragged.front().Pos));
}

void DrawSketchHandlerDragAutoConstraint::addAutoConstraint(ConstraintType type, int geoId, PointPos posId)
{
    AutoConstraint constr;
    constr.Type = type;
    constr.GeoId = geoId;
    constr.PosId = posId;
    suggestedConstraints.push_back(constr);
}

void DrawSketchHandlerDragAutoConstraint::clear()
{
    suggestedConstraints.clear();
    unsetCursor();
}

bool DrawSketchHandlerDragAutoConstraint::hasMoved(const Base::Vector2d& actualPos) const
{
    return (actualPos - startPos).Sqr() > Precision::SquareConfusion();
}

Base::Vector2d DrawSketchHandlerDragAutoConstraint::getDirection(
    const GeoElementId& dragged,
    const Base::Vector2d& pos
) const
{
    const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(dragged.GeoId);
    if (!geo || !geo->isDerivedFrom<Part::GeomLineSegment>()) {
        return Base::Vector2d(0.0, 0.0);
    }

    const auto* line = static_cast<const Part::GeomLineSegment*>(geo);

    Base::Vector2d startPoint = toVector2d(line->getStartPoint());
    Base::Vector2d endPoint = toVector2d(line->getEndPoint());

    if (dragged.Pos == PointPos::start) {
        startPoint = pos;
    }
    else if (dragged.Pos == PointPos::end) {
        endPoint = pos;
    }

    return endPoint - startPoint;
}

bool DrawSketchHandlerDragAutoConstraint::isExistingConstraint(
    const GeoElementId& dragged,
    const AutoConstraint& constraint
) const
{
    auto samePoint = [](int geoId1, PointPos posId1, int geoId2, PointPos posId2) {
        return geoId1 == geoId2 && posId1 == posId2;
    };

    auto isDraggedPoint = [&](int geoId, PointPos posId) {
        return samePoint(geoId, posId, dragged.GeoId, dragged.Pos);
    };

    const auto& constraints = sketchgui->getSketchObject()->Constraints.getValues();
    for (const Constraint* c : constraints) {
        if (!c) {
            continue;
        }

        switch (constraint.Type) {
            case Coincident:
                if (c->Type == Coincident
                    && ((isDraggedPoint(c->First, c->FirstPos)
                         && samePoint(c->Second, c->SecondPos, constraint.GeoId, constraint.PosId))
                        || (isDraggedPoint(c->Second, c->SecondPos)
                            && samePoint(c->First, c->FirstPos, constraint.GeoId, constraint.PosId)))) {
                    return true;
                }
                break;

            case PointOnObject:
                if (c->Type == PointOnObject && isDraggedPoint(c->First, c->FirstPos)
                    && c->Second == constraint.GeoId) {
                    return true;
                }
                break;

            case Symmetric: {
                if (c->Type != Symmetric || !isDraggedPoint(c->Third, c->ThirdPos)) {
                    break;
                }

                const bool sameLineStartEnd
                    = samePoint(c->First, c->FirstPos, constraint.GeoId, PointPos::start)
                    && samePoint(c->Second, c->SecondPos, constraint.GeoId, PointPos::end);
                const bool sameLineEndStart
                    = samePoint(c->First, c->FirstPos, constraint.GeoId, PointPos::end)
                    && samePoint(c->Second, c->SecondPos, constraint.GeoId, PointPos::start);

                if (sameLineStartEnd || sameLineEndStart) {
                    return true;
                }
                break;
            }

            case Horizontal:
            case Vertical: {
                const int targetGeoId = constraint.GeoId != GeoEnum::GeoUndef ? constraint.GeoId
                                                                              : dragged.GeoId;
                if (c->Type == constraint.Type && c->First == targetGeoId) {
                    return true;
                }
                break;
            }

            default:
                break;
        }
    }

    return false;
}

void DrawSketchHandlerDragAutoConstraint::removeInvalidConstraints(const GeoElementId& dragged)
{
    suggestedConstraints.erase(
        std::remove_if(
            suggestedConstraints.begin(),
            suggestedConstraints.end(),
            [this, &dragged](const AutoConstraint& constraint) {
                const bool isSelfPoint = constraint.Type == Coincident
                    && constraint.GeoId == dragged.GeoId && constraint.PosId == dragged.Pos;
                const bool isSelfObject = constraint.Type == PointOnObject
                    && constraint.GeoId == dragged.GeoId;

                return isSelfPoint || isSelfObject || isExistingConstraint(dragged, constraint);
            }
        ),
        suggestedConstraints.end()
    );
}

void DrawSketchHandlerDragAutoConstraint::update(
    const std::vector<GeoElementId>& draggedElements,
    const Base::Vector2d& /*pos*/
)
{
    suggestedConstraints.clear();

    SketchObject* obj = getSketchObject();
    if (!obj || !canSuggestFor(draggedElements)) {
        unsetCursor();
        return;
    }

    const auto& dragged = draggedElements.front();
    const Base::Vector2d actualPos = toVector2d(
        sketchgui->getSolvedSketch().getPoint(dragged.GeoId, dragged.Pos)
    );

    if (!hasMoved(actualPos)) {
        unsetCursor();
        return;
    }

    const double snapDistance = std::max(
        Precision::Confusion(),
        DragAutoConstraintSnapDistanceFactor * sketchgui->getScaleFactor()
    );

    auto isDraggedPoint = [&dragged](int geoId, PointPos posId) {
        return dragged.GeoId == geoId && dragged.Pos == posId;
    };

    int bestPointGeoId = GeoEnum::GeoUndef;
    PointPos bestPointPos = PointPos::none;
    double bestPointDist = snapDistance;

    const int highestVertex = obj->getHighestVertexIndex();
    for (int vertexIndex = 0; vertexIndex <= highestVertex; ++vertexIndex) {
        int geoId = GeoEnum::GeoUndef;
        PointPos posId = PointPos::none;
        obj->getGeoVertexIndex(vertexIndex, geoId, posId);

        if (geoId == GeoEnum::GeoUndef || isDraggedPoint(geoId, posId)) {
            continue;
        }

        const double dist = (actualPos - toVector2d(obj->getPoint(geoId, posId))).Length();
        if (dist < bestPointDist) {
            bestPointDist = dist;
            bestPointGeoId = geoId;
            bestPointPos = posId;
        }
    }

    if (bestPointGeoId != GeoEnum::GeoUndef) {
        addAutoConstraint(Coincident, bestPointGeoId, bestPointPos);
    }
    else if (actualPos.Length() < snapDistance) {
        addAutoConstraint(Coincident, GeoEnum::RtPnt, PointPos::start);
    }
    else {
        struct CurveCandidate
        {
            int geoId = GeoEnum::GeoUndef;
            double distance = std::numeric_limits<double>::max();
            bool lineCenter = false;
        };

        CurveCandidate bestCurve;

        auto considerCurve = [&](int geoId, double distance, bool lineCenter = false) {
            if (geoId == dragged.GeoId || distance >= snapDistance || distance >= bestCurve.distance) {
                return;
            }

            bestCurve.geoId = geoId;
            bestCurve.distance = distance;
            bestCurve.lineCenter = lineCenter;
        };

        for (int geoId = 0; geoId <= obj->getHighestCurveIndex(); ++geoId) {
            const Part::Geometry* geo = obj->getGeometry(geoId);
            if (!geo) {
                continue;
            }

            if (geo->is<Part::GeomLineSegment>()) {
                const auto* line = static_cast<const Part::GeomLineSegment*>(geo);

                const Base::Vector2d a = toVector2d(line->getStartPoint());
                const Base::Vector2d b = toVector2d(line->getEndPoint());
                const Base::Vector2d ab = b - a;
                const double len2 = ab * ab;

                if (len2 <= Precision::SquareConfusion()) {
                    continue;
                }

                double t = ((actualPos - a) * ab) / len2;
                t = std::max(0.0, std::min(1.0, t));

                const Base::Vector2d projection = a + ab * t;
                const double distance = (actualPos - projection).Length();
                considerCurve(geoId, distance, isLineCenterAutoConstraint(geoId, actualPos));
            }
            else if (geo->is<Part::GeomCircle>()) {
                const auto* circle = static_cast<const Part::GeomCircle*>(geo);

                const Base::Vector2d center = toVector2d(circle->getCenter());
                const double distance = std::abs((actualPos - center).Length() - circle->getRadius());

                considerCurve(geoId, distance);
            }
        }

        if (bestCurve.geoId != GeoEnum::GeoUndef) {
            addAutoConstraint(bestCurve.lineCenter ? Symmetric : PointOnObject, bestCurve.geoId);
        }
        else if (std::abs(actualPos.y) < snapDistance) {
            addAutoConstraint(PointOnObject, GeoEnum::HAxis);
        }
        else if (std::abs(actualPos.x) < snapDistance) {
            addAutoConstraint(PointOnObject, GeoEnum::VAxis);
        }
    }

    const Base::Vector2d dir = getDirection(dragged, actualPos);
    if (dir.Sqr() > Precision::SquareConfusion()) {
        using std::numbers::pi;
        constexpr double angleDevRad = Base::toRadians<double>(2);

        AutoConstraint constr;
        constr.Type = None;
        constr.GeoId = GeoEnum::GeoUndef;
        constr.PosId = PointPos::none;

        const double angle = std::abs(atan2(dir.y, dir.x));
        if (angle < angleDevRad || (pi - angle) < angleDevRad) {
            constr.Type = Horizontal;
        }
        else if (std::abs(angle - pi / 2) < angleDevRad) {
            constr.Type = Vertical;
        }

        if (constr.Type != None) {
            suggestedConstraints.push_back(constr);
        }
    }

    removeInvalidConstraints(dragged);

    if (suggestedConstraints.empty()) {
        unsetCursor();
    }
    else {
        renderSuggestConstraintsCursor(suggestedConstraints);
    }
}

void DrawSketchHandlerDragAutoConstraint::create(const std::vector<GeoElementId>& dragged)
{
    if (!canSuggestFor(dragged) || suggestedConstraints.empty()) {
        return;
    }

    createAutoConstraints(suggestedConstraints, dragged.front().GeoId, dragged.front().Pos, false);
}
