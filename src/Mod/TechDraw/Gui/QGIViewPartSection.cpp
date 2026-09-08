/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.       *
 ***************************************************************************/

#include <algorithm>
#include <cmath>
#include <vector>

#include <QPolygonF>

#include <App/PropertyStandard.h>
#include <Mod/TechDraw/App/DrawComplexSection.h>

#include "QGICMark.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGIVertex.h"
#include "QGIViewPart.h"
#include "Rez.h"

using namespace TechDrawGui;

namespace
{
QPolygonF clipPolygonToHalfPlane(const QPolygonF& polygon,
                                 const QPointF& boundaryPoint,
                                 const QPointF& inwardNormal)
{
    QPolygonF result;
    if (polygon.isEmpty()) {
        return result;
    }

    constexpr double tolerance = 1.0e-7;
    const auto signedDistance = [&](const QPointF& point) {
        return QPointF::dotProduct(point - boundaryPoint, inwardNormal);
    };
    for (qsizetype index = 0; index < polygon.size(); ++index) {
        const QPointF current = polygon.at(index);
        const QPointF next = polygon.at((index + 1) % polygon.size());
        const double currentDistance = signedDistance(current);
        const double nextDistance = signedDistance(next);
        const bool currentInside = currentDistance >= -tolerance;
        const bool nextInside = nextDistance >= -tolerance;

        if (currentInside) {
            result.append(current);
        }
        if (currentInside == nextInside) {
            continue;
        }
        const double denominator = currentDistance - nextDistance;
        if (std::abs(denominator) <= tolerance) {
            continue;
        }
        const double ratio = currentDistance / denominator;
        result.append(current + (next - current) * ratio);
    }
    return result;
}
}

QPainterPath QGIViewPart::partialSectionClipPath() const
{
    const auto* section =
        dynamic_cast<const TechDraw::DrawComplexSection*>(getViewObject());
    if (!section || !section->isPartialSection()
        || section->ShowOutsidePartialBoundaries.getValue()) {
        return {};
    }

    struct Boundary
    {
        QPointF point;
        QPointF inwardNormal;
    };
    std::vector<Boundary> boundaries;
    const auto guiPoint = [](const Base::Vector3d& point) {
        return QPointF(Rez::guiX(point.x), Rez::guiX(point.y));
    };

    if (section->ProjectionStrategy.getValue() == 0) {
        const auto points = section->partialSectionBoundaryPoints();
        const auto directions = section->partialSectionBoundaryDirections();
        bool useStart = true;
        bool useEnd = true;
        if (const App::DocumentObject* profile = section->getGeneratedProfile()) {
            const auto* startProperty =
                dynamic_cast<const App::PropertyBool*>(
                    profile->getPropertyByName("PartialSectionStart"));
            const auto* endProperty =
                dynamic_cast<const App::PropertyBool*>(
                    profile->getPropertyByName("PartialSectionEnd"));
            if (startProperty || endProperty) {
                useStart = startProperty && startProperty->getValue();
                useEnd = endProperty && endProperty->getValue();
            }
        }
        if (useStart && directions.first.Sqr() > 1.0e-12) {
            boundaries.push_back(
                {guiPoint(points.first),
                 QPointF(directions.first.x, directions.first.y)});
        }
        if (useEnd && directions.second.Sqr() > 1.0e-12) {
            boundaries.push_back(
                {guiPoint(points.second),
                 QPointF(directions.second.x, directions.second.y)});
        }
    }
    else {
        const auto info = section->alignedSectionBoundaryInfo();
        if (info.valid) {
            const QPointF normal(info.normal.x, info.normal.y);
            const QPointF start = guiPoint(info.startPoint);
            const QPointF end = guiPoint(info.endPoint);
            const bool startIsMinimum =
                QPointF::dotProduct(start, normal)
                <= QPointF::dotProduct(end, normal);
            if (info.startPartial) {
                boundaries.push_back(
                    {start, startIsMinimum ? normal : -normal});
            }
            if (info.endPartial) {
                boundaries.push_back(
                    {end, startIsMinimum ? -normal : normal});
            }
        }
    }
    if (boundaries.empty()) {
        return {};
    }

    QRectF geometryBounds;
    bool haveBounds = false;
    for (const TechDraw::BaseGeomPtr& geom : section->getEdgeGeometry()) {
        const QRectF pathBounds = drawPainterPath(geom).boundingRect();
        if (pathBounds.isEmpty()) {
            continue;
        }
        geometryBounds = haveBounds
            ? geometryBounds.united(pathBounds) : pathBounds;
        haveBounds = true;
    }
    for (const Boundary& boundary : boundaries) {
        const QRectF pointBounds(boundary.point, QSizeF(0.0, 0.0));
        geometryBounds = haveBounds
            ? geometryBounds.united(pointBounds) : pointBounds;
        haveBounds = true;
    }
    if (!haveBounds) {
        return {};
    }

    const double extent = std::max(
        {geometryBounds.width(), geometryBounds.height(), Rez::guiX(100.0)});
    geometryBounds.adjust(-extent, -extent, extent, extent);
    QPolygonF polygon;
    polygon << geometryBounds.topLeft()
            << geometryBounds.topRight()
            << geometryBounds.bottomRight()
            << geometryBounds.bottomLeft();
    for (const Boundary& boundary : boundaries) {
        polygon = clipPolygonToHalfPlane(
            polygon, boundary.point, boundary.inwardNormal);
        if (polygon.isEmpty()) {
            return {};
        }
    }

    QPainterPath clipPath;
    clipPath.setFillRule(Qt::WindingFill);
    clipPath.addPolygon(polygon);
    clipPath.closeSubpath();
    return clipPath;
}

void QGIViewPart::applyPartialSectionClip()
{
    const QPainterPath clip = partialSectionClipPath();
    if (clip.isEmpty()) {
        return;
    }

    const QList<QGraphicsItem*> children = childItems();
    for (QGraphicsItem* child : children) {
        if (auto* face = dynamic_cast<QGIFace*>(child)) {
            face->setPaintClip(clip);
        }
        else if (auto* edge = dynamic_cast<QGIEdge*>(child)) {
            if (edge->getProjIndex() >= 0) {
                edge->setPaintClip(clip);
            }
        }
        else if ((dynamic_cast<QGIVertex*>(child)
                  || dynamic_cast<QGICMark*>(child))
                 && !clip.contains(child->pos())) {
            delete child;
        }
    }
}
