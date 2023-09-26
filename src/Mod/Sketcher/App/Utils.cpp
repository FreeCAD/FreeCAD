/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <App/Application.h>
#include <Base/Tools.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "Utils.h"


using namespace std;
using namespace Sketcher;

bool Sketcher::isCircle(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomCircle::getClassTypeId();
}

bool Sketcher::isArcOfCircle(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomArcOfCircle::getClassTypeId();
}

bool Sketcher::isEllipse(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomEllipse::getClassTypeId();
}

bool Sketcher::isArcOfEllipse(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomArcOfEllipse::getClassTypeId();
}

bool Sketcher::isLineSegment(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomLineSegment::getClassTypeId();
}

bool Sketcher::isArcOfHyperbola(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId();
}

bool Sketcher::isArcOfParabola(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomArcOfParabola::getClassTypeId();
}

bool Sketcher::isBSplineCurve(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomBSplineCurve::getClassTypeId();
}

bool Sketcher::isPoint(const Part::Geometry& geom)
{
    return geom.getTypeId() == Part::GeomPoint::getClassTypeId();
}

Base::Vector2d Sketcher::startPoint2d(const Part::Geometry* geo)
{
    return getPoint2d(geo, PointPos::start);
}

Base::Vector2d Sketcher::endPoint2d(const Part::Geometry* geo)
{
    return getPoint2d(geo, PointPos::end);
}

Base::Vector2d Sketcher::centerPoint2d(const Part::Geometry* geo)
{
    return getPoint2d(geo, PointPos::mid);
}

Base::Vector3d Sketcher::startPoint3d(const Part::Geometry* geo)
{
    return getPoint3d(geo, PointPos::start);
}

Base::Vector3d Sketcher::endPoint3d(const Part::Geometry* geo)
{
    return getPoint3d(geo, PointPos::end);
}

Base::Vector3d Sketcher::centerPoint3d(const Part::Geometry* geo)
{
    return getPoint3d(geo, PointPos::mid);
}

Base::Vector2d Sketcher::getPoint2d(const Part::Geometry* geo, PointPos PosId)
{
    Base::Vector3d p = getPoint3d(geo, PosId);
    return Base::Vector2d(p.x, p.y);
}

Base::Vector3d Sketcher::getPoint3d(const Part::Geometry* geo, PointPos PosId)
{
    if (isPoint(*geo)) {
        return (static_cast<const Part::GeomPoint*>(geo))->getPoint();
    }
    else if (isLineSegment(*geo)) {
        const auto* lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
        if (PosId == PointPos::start) {
            return lineSeg->getStartPoint();
        }
        else if (PosId == PointPos::end) {
            return lineSeg->getEndPoint();
        }
        else if (PosId == PointPos::mid) {
            return (lineSeg->getStartPoint() + lineSeg->getEndPoint()) / 2;
        }
    }
    else if (isCircle(*geo)) {
        const auto* circle = static_cast<const Part::GeomCircle*>(geo);
        auto pt = circle->getCenter();
        if (PosId != PointPos::mid) {
            pt.x += circle->getRadius();
        }
        return pt;
    }
    else if (isEllipse(*geo)) {
        const auto* ellipse = static_cast<const Part::GeomEllipse*>(geo);
        auto pt = ellipse->getCenter();
        if (PosId != PointPos::mid) {
            pt += ellipse->getMajorAxisDir() * ellipse->getMajorRadius();
        }
        return pt;
    }
    else if (isArcOfCircle(*geo)) {
        const auto* aoc = static_cast<const Part::GeomArcOfCircle*>(geo);
        if (PosId == PointPos::start) {
            return aoc->getStartPoint(/*emulateCCW=*/true);
        }
        else if (PosId == PointPos::end) {
            return aoc->getEndPoint(/*emulateCCW=*/true);
        }
        else if (PosId == PointPos::mid) {
            return aoc->getCenter();
        }
    }
    else if (isArcOfEllipse(*geo)) {
        const auto* aoc = static_cast<const Part::GeomArcOfEllipse*>(geo);
        if (PosId == PointPos::start) {
            return aoc->getStartPoint(/*emulateCCW=*/true);
        }
        else if (PosId == PointPos::end) {
            return aoc->getEndPoint(/*emulateCCW=*/true);
        }
        else if (PosId == PointPos::mid) {
            return aoc->getCenter();
        }
    }
    else if (isArcOfHyperbola(*geo)) {
        const auto* aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);
        if (PosId == PointPos::start) {
            return aoh->getStartPoint();
        }
        else if (PosId == PointPos::end) {
            return aoh->getEndPoint();
        }
        else if (PosId == PointPos::mid) {
            return aoh->getCenter();
        }
    }
    else if (isArcOfParabola(*geo)) {
        const auto* aop = static_cast<const Part::GeomArcOfParabola*>(geo);
        if (PosId == PointPos::start) {
            return aop->getStartPoint();
        }
        else if (PosId == PointPos::end) {
            return aop->getEndPoint();
        }
        else if (PosId == PointPos::mid) {
            return aop->getCenter();
        }
    }
    else if (isBSplineCurve(*geo)) {
        const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
        if (PosId == PointPos::start) {
            return bsp->getStartPoint();
        }
        else if (PosId == PointPos::end) {
            return bsp->getEndPoint();
        }
    }
    return Base::Vector3d();
}

double Sketcher::getRadius(const Part::Geometry* geo)
{
    if (isCircle(*geo)) {
        return static_cast<const Part::GeomCircle*>(geo)->getRadius();
    }
    else if (isEllipse(*geo)) {
        return static_cast<const Part::GeomEllipse*>(geo)->getMajorRadius();
    }
    else if (isArcOfCircle(*geo)) {
        return static_cast<const Part::GeomArcOfCircle*>(geo)->getRadius();
    }
    else if (isArcOfEllipse(*geo)) {
        return static_cast<const Part::GeomArcOfEllipse*>(geo)->getMajorRadius();
    }
    return 0.;
}
