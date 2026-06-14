// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include "SketchGeometry.h"
#include <Mod/Part/App/Geometry.h>

using namespace Sketcher;

namespace Sketcher
{

template<typename GeometryT>
class SketchGeometryT: public SketchGeometry
{
public:
    using GeomType = GeometryT;
    bool supports(const Part::Geometry* geo) const override
    {
        return geo->is<GeomType>();
    }
};

class SketchPoint: public SketchGeometryT<Part::GeomPoint>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto geomPoint = dynamic_cast<const GeomType*>(geo);
        if (PosId == PointPos::start || PosId == PointPos::mid || PosId == PointPos::end) {
            return geomPoint->getPoint();
        }

        return Base::Vector3d();
    }
};

class SketchLineSegment: public SketchGeometryT<Part::GeomLineSegment>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto lineSeg = dynamic_cast<const GeomType*>(geo);
        switch (PosId) {
            case PointPos::start: {
                return lineSeg->getStartPoint();
            }
            case PointPos::end: {
                return lineSeg->getEndPoint();
            }
            default:
                break;
        }
        return Base::Vector3d();
    }
};

class SketchCircle: public SketchGeometryT<Part::GeomCircle>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto circle = dynamic_cast<const GeomType*>(geo);
        auto pt = circle->getCenter();
        if (PosId != PointPos::mid) {
            pt.x += circle->getRadius();
        }
        return pt;
    }
};

class SketchEllipse: public SketchGeometryT<Part::GeomEllipse>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto ellipse = dynamic_cast<const GeomType*>(geo);
        auto pt = ellipse->getCenter();
        if (PosId != PointPos::mid) {
            pt += ellipse->getMajorAxisDir() * ellipse->getMajorRadius();
        }
        return pt;
    }
};

class SketchArcOfCircle: public SketchGeometryT<Part::GeomArcOfCircle>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto aoc = dynamic_cast<const GeomType*>(geo);
        const bool emulateCCW = true;
        switch (PosId) {
            case PointPos::start: {
                return aoc->getStartPoint(emulateCCW);
            }
            case PointPos::end: {
                return aoc->getEndPoint(emulateCCW);
            }
            case PointPos::mid: {
                return aoc->getCenter();
            }
            default:
                break;
        }
        return Base::Vector3d();
    }
};

class SketchArcOfEllipse: public SketchGeometryT<Part::GeomArcOfEllipse>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto aoe = dynamic_cast<const GeomType*>(geo);
        const bool emulateCCW = true;
        switch (PosId) {
            case PointPos::start: {
                return aoe->getStartPoint(emulateCCW);
            }
            case PointPos::end: {
                return aoe->getEndPoint(emulateCCW);
            }
            case PointPos::mid: {
                return aoe->getCenter();
            }
            default:
                break;
        }
        return Base::Vector3d();
    }
};

class SketchArcOfHyperbola: public SketchGeometryT<Part::GeomArcOfHyperbola>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto aoh = dynamic_cast<const GeomType*>(geo);
        switch (PosId) {
            case PointPos::start: {
                return aoh->getStartPoint();
            }
            case PointPos::end: {
                return aoh->getEndPoint();
            }
            case PointPos::mid: {
                return aoh->getCenter();
            }
            default:
                break;
        }
        return Base::Vector3d();
    }
};

class SketchArcOfParabola: public SketchGeometryT<Part::GeomArcOfParabola>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto aop = dynamic_cast<const GeomType*>(geo);
        switch (PosId) {
            case PointPos::start: {
                return aop->getStartPoint();
            }
            case PointPos::end: {
                return aop->getEndPoint();
            }
            case PointPos::mid: {
                return aop->getCenter();
            }
            default:
                break;
        }
        return Base::Vector3d();
    }
};

class SketchBSplineCurve: public SketchGeometryT<Part::GeomBSplineCurve>
{
public:
    Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId) const override
    {
        auto bsp = dynamic_cast<const GeomType*>(geo);
        switch (PosId) {
            case PointPos::start: {
                return bsp->getStartPoint();
            }
            case PointPos::end: {
                return bsp->getEndPoint();
            }
            default:
                break;
        }
        return Base::Vector3d();
    }
};

}  // namespace Sketcher

std::list<SketchGeometryPtr> SketchGeometryType::sketchGeoms;  // NOLINT

void SketchGeometryType::init()
{
    static bool init = true;
    if (init) {
        init = false;
        addType(std::make_shared<SketchPoint>());
        addType(std::make_shared<SketchLineSegment>());
        addType(std::make_shared<SketchCircle>());
        addType(std::make_shared<SketchEllipse>());
        addType(std::make_shared<SketchArcOfCircle>());
        addType(std::make_shared<SketchArcOfEllipse>());
        addType(std::make_shared<SketchArcOfHyperbola>());
        addType(std::make_shared<SketchArcOfParabola>());
        addType(std::make_shared<SketchBSplineCurve>());
    }
}

void SketchGeometryType::addType(const SketchGeometryPtr& type)
{
    sketchGeoms.emplace_back(type);
}

Base::Vector3d SketchGeometryType::getPoint(const Part::Geometry* geo, PointPos PosId)
{
    for (const auto& it : sketchGeoms) {
        if (it->supports(geo)) {
            return it->getPoint(geo, PosId);
        }
    }

    return Base::Vector3d();
}
