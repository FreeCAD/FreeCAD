/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/Exception.h>
#include <Mod/Part/App/Geometry.h>

#include "SolverGeometryExtension.h"


using namespace Sketcher;

//---------- Geometry Extension
TYPESYSTEM_SOURCE(Sketcher::SolverGeometryExtension, Part::GeometryExtension)

SolverGeometryExtension::SolverGeometryExtension()
    : Start(SolverGeometryExtension::Dependent)
    , Mid(SolverGeometryExtension::Dependent)
    , End(SolverGeometryExtension::Dependent)
{}

void SolverGeometryExtension::copyAttributes(Part::GeometryExtension* cpy) const
{
    Part::GeometryExtension::copyAttributes(cpy);
    static_cast<SolverGeometryExtension*>(cpy)->Edge = this->Edge;
    static_cast<SolverGeometryExtension*>(cpy)->Start = this->Start;
    static_cast<SolverGeometryExtension*>(cpy)->End = this->End;
    static_cast<SolverGeometryExtension*>(cpy)->Mid = this->Mid;
}

std::unique_ptr<Part::GeometryExtension> SolverGeometryExtension::copy() const
{
    auto cpy = std::make_unique<SolverGeometryExtension>();

    copyAttributes(cpy.get());

#if defined(__GNUC__) && (__GNUC__ <= 4)
    return std::move(cpy);
#else
    return cpy;
#endif
}

PyObject* SolverGeometryExtension::getPyObject()
{
    THROWM(Base::NotImplementedError, "SolverGeometryExtension does not have a Python counterpart");
}

SolverGeometryExtension::PointParameterStatus
SolverGeometryExtension::getPoint(Sketcher::PointPos pos) const
{
    if (pos == Sketcher::PointPos::start) {
        return getStartPoint();
    }
    if (pos == Sketcher::PointPos::end) {
        return getEndPoint();
    }
    if (pos == Sketcher::PointPos::mid) {
        return getMidPoint();
    }

    THROWM(Base::ValueError, "SolverGeometryExtension - getPoint: Edge is not a point");
}

void SolverGeometryExtension::notifyAttachment(Part::Geometry* geo)
{
    // maps type to number of solver parameters taken by the edge
    static std::map<Base::Type, int> edgeParamMap = {
        {Part::GeomPoint::getClassTypeId(), 0},
        {Part::GeomLineSegment::getClassTypeId(), 0},
        {Part::GeomArcOfCircle::getClassTypeId(), 3},
        {Part::GeomCircle::getClassTypeId(), 1},
        {Part::GeomArcOfEllipse::getClassTypeId(), 5},
        {Part::GeomEllipse::getClassTypeId(), 3},
        {Part::GeomArcOfHyperbola::getClassTypeId(), 5},
        {Part::GeomArcOfParabola::getClassTypeId(), 4},
        {Part::GeomBSplineCurve::getClassTypeId(), 0}  // is dynamic
    };

    GeometryType = geo->getTypeId();

    auto result = edgeParamMap.find(GeometryType);

    if (result == edgeParamMap.end()) {
        THROWM(Base::TypeError,
               "SolverGeometryExtension - notifyAttachment - Geometry not supported!!");
    }

    auto nedgeparams = (*result).second;

    if (nedgeparams > 0) {
        Edge.init(nedgeparams);
    }
}

void SolverGeometryExtension::ensureType(const Base::Type& type)
{
    if (GeometryType != type) {
        THROWM(Base::TypeError,
               "SolverGeometryExtension - requested edge parameters do not match underlying type!");
    }
}

SolverGeometryExtension::Point& SolverGeometryExtension::getPoint()
{
    ensureType(Part::GeomPoint::getClassTypeId());
    return static_cast<Point&>(Edge);
}

SolverGeometryExtension::Line& SolverGeometryExtension::getLine()
{
    ensureType(Part::GeomLineSegment::getClassTypeId());
    return static_cast<Line&>(Edge);
}

SolverGeometryExtension::Arc& SolverGeometryExtension::getArc()
{
    ensureType(Part::GeomArcOfCircle::getClassTypeId());
    return static_cast<Arc&>(Edge);
}

SolverGeometryExtension::Circle& SolverGeometryExtension::getCircle()
{
    ensureType(Part::GeomCircle::getClassTypeId());
    return static_cast<Circle&>(Edge);
}

SolverGeometryExtension::ArcOfEllipse& SolverGeometryExtension::getArcOfEllipse()
{
    ensureType(Part::GeomArcOfEllipse::getClassTypeId());
    return static_cast<ArcOfEllipse&>(Edge);
}

SolverGeometryExtension::Ellipse& SolverGeometryExtension::getEllipse()
{
    ensureType(Part::GeomEllipse::getClassTypeId());
    return static_cast<Ellipse&>(Edge);
}

SolverGeometryExtension::ArcOfHyperbola& SolverGeometryExtension::getArcOfHyperbola()
{
    ensureType(Part::GeomArcOfHyperbola::getClassTypeId());
    return static_cast<ArcOfHyperbola&>(Edge);
}

SolverGeometryExtension::ArcOfParabola& SolverGeometryExtension::getArcOfParabola()
{
    ensureType(Part::GeomArcOfParabola::getClassTypeId());
    return static_cast<ArcOfParabola&>(Edge);
}

SolverGeometryExtension::BSpline& SolverGeometryExtension::getBSpline()
{
    ensureType(Part::GeomBSplineCurve::getClassTypeId());
    return static_cast<BSpline&>(Edge);
}
