/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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
# include <gp_Ax3.hxx>
# include <gp_Trsf.hxx>
# include <gp_Vec.hxx>
#endif

#include <Base/Console.h>

#include "DimensionGeometry.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"


using namespace TechDraw;
using DU = DrawUtil;

pointPair::pointPair(const pointPair& pp)
{
    first(pp.first());
    second(pp.second());
}

//move the points by offset
void pointPair::move(Base::Vector3d offset)
{
    m_first = m_first - offset;
    m_second = m_second - offset;
}

// project the points onto the dvp's paper plane.  Points are still in R3 coords.
void pointPair::project(DrawViewPart* dvp)
{
    Base::Vector3d normal = DrawUtil::toVector3d(dvp->getProjectionCS().Direction());
    Base::Vector3d stdOrigin(0.0, 0.0, 0.0);
    m_first = m_first.ProjectToPlane(stdOrigin, normal) * dvp->getScale();
    m_second = m_second.ProjectToPlane(stdOrigin, normal) * dvp->getScale();
}

// map the points onto the dvp's XY coordinate system
void pointPair::mapToPage(DrawViewPart* dvp)
{
    gp_Trsf xOXYZ;
    gp_Ax3 OXYZ;
    xOXYZ.SetTransformation(OXYZ, gp_Ax3(dvp->getRotatedCS()));

    gp_Vec gvFirst = DU::togp_Vec(m_first).Transformed(xOXYZ);
    m_first = DU::toVector3d(gvFirst);
    gp_Vec gvSecond = DU::togp_Vec(m_second).Transformed(xOXYZ);
    m_second = DU::toVector3d(gvSecond);
}

void pointPair::invertY()
{
    m_first = DU::invertY(m_first);
    m_second = DU::invertY(m_second);
}

void pointPair::dump(std::string text) const
{
    Base::Console().Message("pointPair - %s\n", text.c_str());
    Base::Console().Message("pointPair - first: %s  second: %s\n",
                            DU::formatVector(first()).c_str(), DU::formatVector(second()).c_str());
}

anglePoints::anglePoints()
{
    m_ends.first(Base::Vector3d(0.0, 0.0, 0.0));
    m_ends.second(Base::Vector3d(0.0, 0.0, 0.0));
    m_vertex = Base::Vector3d(0.0, 0.0, 0.0);
}

anglePoints::anglePoints(const anglePoints& ap) : m_ends(ap.ends()), m_vertex(ap.vertex()) {}

anglePoints& anglePoints::operator=(const anglePoints& ap)
{
    m_ends = ap.ends();
    m_vertex = ap.vertex();
    return *this;
}

// move the points by offset
void anglePoints::move(Base::Vector3d offset)
{
    m_ends.move(offset);
    m_vertex = m_vertex - offset;
}

// project the points onto the dvp's paper plane.  Points are still in R3 coords.
void anglePoints::project(DrawViewPart* dvp)
{
    Base::Vector3d normal = DrawUtil::toVector3d(dvp->getProjectionCS().Direction());
    Base::Vector3d stdOrigin(0.0, 0.0, 0.0);
    m_ends.project(dvp);
    m_vertex = m_vertex.ProjectToPlane(stdOrigin, normal) * dvp->getScale();
}

// map the points onto the dvp's XY coordinate system
void anglePoints::mapToPage(DrawViewPart* dvp)
{
    m_ends.mapToPage(dvp);

    gp_Trsf xOXYZ;
    gp_Ax3 OXYZ;
    xOXYZ.SetTransformation(OXYZ, gp_Ax3(dvp->getRotatedCS()));
    gp_Vec gvVertex = DU::togp_Vec(m_vertex).Transformed(xOXYZ);
    m_vertex = DU::toVector3d(gvVertex);
}

// map the points onto the coordinate system used for drawing where -Y direction is "up"
void anglePoints::invertY()
{
    m_ends.invertY();
    m_vertex = DU::invertY(m_vertex);
}

void anglePoints::dump(std::string text) const
{
    Base::Console().Message("anglePoints - %s\n", text.c_str());
    Base::Console().Message("anglePoints - ends - first: %s  second: %s\n",
                            DU::formatVector(first()).c_str(), DU::formatVector(second()).c_str());
    Base::Console().Message("anglePoints - vertex: %s\n", DU::formatVector(vertex()).c_str());
}

arcPoints::arcPoints()
{
    isArc = false;
    radius = 0.0;
    center = Base::Vector3d(0.0, 0.0, 0.0);
    onCurve.first(Base::Vector3d(0.0, 0.0, 0.0));
    onCurve.second(Base::Vector3d(0.0, 0.0, 0.0));
    arcEnds.first(Base::Vector3d(0.0, 0.0, 0.0));
    arcEnds.second(Base::Vector3d(0.0, 0.0, 0.0));
    midArc = Base::Vector3d(0.0, 0.0, 0.0);
    arcCW = false;
}

arcPoints::arcPoints(const arcPoints& ap)
    : isArc(ap.isArc), radius(ap.radius), center(ap.center), onCurve(ap.onCurve),
      arcEnds(ap.arcEnds), midArc(ap.midArc), arcCW(ap.arcCW)
{}

arcPoints& arcPoints::operator=(const arcPoints& ap)
{
    isArc = ap.isArc;
    radius = ap.radius;
    center = ap.center;
    onCurve = ap.onCurve;
    arcEnds = ap.arcEnds;
    midArc = ap.midArc;
    arcCW = ap.arcCW;
    return *this;
}

void arcPoints::move(Base::Vector3d offset)
{
    center = center - offset;
    onCurve.first(onCurve.first() - offset);
    onCurve.second(onCurve.second() - offset);
    arcEnds.first(arcEnds.first() - offset);
    arcEnds.second(arcEnds.second() - offset);
    midArc = midArc - offset;
}

void arcPoints::project(DrawViewPart* dvp)
{
    radius = radius * dvp->getScale();
    Base::Vector3d normal = DrawUtil::toVector3d(dvp->getProjectionCS().Direction());
    Base::Vector3d stdOrigin(0.0, 0.0, 0.0);
    center = center.ProjectToPlane(stdOrigin, normal) * dvp->getScale();
    onCurve.first(onCurve.first().ProjectToPlane(stdOrigin, normal) * dvp->getScale());
    onCurve.second(onCurve.second().ProjectToPlane(stdOrigin, normal) * dvp->getScale());
    arcEnds.first(arcEnds.first().ProjectToPlane(stdOrigin, normal) * dvp->getScale());
    arcEnds.second(arcEnds.second().ProjectToPlane(stdOrigin, normal) * dvp->getScale());
    midArc = midArc.ProjectToPlane(stdOrigin, normal) * dvp->getScale();
}

void arcPoints::mapToPage(DrawViewPart* dvp)
{
    gp_Trsf xOXYZ;
    gp_Ax3 OXYZ;
    xOXYZ.SetTransformation(OXYZ, gp_Ax3(dvp->getRotatedCS()));

    gp_Vec gvCenter = DU::togp_Vec(center).Transformed(xOXYZ);
    center = DU::toVector3d(gvCenter);
    gp_Vec gvOnCurve1 = DU::togp_Vec(onCurve.first()).Transformed(xOXYZ);
    onCurve.first(DU::toVector3d(gvOnCurve1));
    gp_Vec gvOnCurve2 = DU::togp_Vec(onCurve.second()).Transformed(xOXYZ);
    onCurve.second(DU::toVector3d(gvOnCurve2));
    gp_Vec gvArcEnds1 = DU::togp_Vec(arcEnds.first()).Transformed(xOXYZ);
    arcEnds.first(DU::toVector3d(gvArcEnds1));
    gp_Vec gvArcEnds2 = DU::togp_Vec(arcEnds.second()).Transformed(xOXYZ);
    arcEnds.second(DU::toVector3d(gvArcEnds2));
    gp_Vec gvMidArc = DU::togp_Vec(midArc).Transformed(xOXYZ);
    midArc = DU::toVector3d(gvMidArc);
}

void arcPoints::invertY()
{
    center = DU::invertY(center);
    onCurve.invertY();
    arcEnds.invertY();
    midArc = DU::invertY(midArc);
}

void arcPoints::dump(std::string text) const
{
    Base::Console().Message("arcPoints - %s\n", text.c_str());
    Base::Console().Message("arcPoints - radius: %.3f center: %s\n", radius,
                            DrawUtil::formatVector(center).c_str());
    Base::Console().Message("arcPoints - isArc: %d arcCW: %d\n", isArc, arcCW);
    Base::Console().Message("arcPoints - onCurve: %s  %s\n",
                            DrawUtil::formatVector(onCurve.first()).c_str(),
                            DrawUtil::formatVector(onCurve.second()).c_str());
    Base::Console().Message("arcPoints - arcEnds: %s  %s\n",
                            DrawUtil::formatVector(arcEnds.first()).c_str(),
                            DrawUtil::formatVector(arcEnds.second()).c_str());
    Base::Console().Message("arcPoints - midArc: %s\n", DrawUtil::formatVector(midArc).c_str());
}
