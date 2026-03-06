/***************************************************************************
 *   Copyright (c) 2023 WandererFan  <wandererfan@gmail.com>               *
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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsPathItem>
#include <QPen>

#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/Geometry.h>

namespace TechDrawGui
{

class PATPathMaker
{
public:
    explicit PATPathMaker(QGraphicsItem* parent = nullptr, double lineWidth = 0.50, double fillScale = 1.0);
    ~PATPathMaker() = default;

    void setLineWidth(double width);
    void setScale(double scale) { m_fillScale = scale; }
    void setPen(QPen pen) { m_pen = pen; }
    QPen getPen()  { return m_pen; }
    void setParent(QGraphicsItem* parent) { m_parent = parent; }

    void lineSetToFillItems(TechDraw::LineSet& ls);

protected:
    QGraphicsPathItem* geomToLine(TechDraw::BaseGeomPtr base, TechDraw::LineSet& ls);
    QGraphicsPathItem* simpleLine(TechDraw::BaseGeomPtr base);
    QGraphicsPathItem* lineFromPoints(Base::Vector3d start, Base::Vector3d end, TechDraw::DashSpec ds);
    std::vector<double> offsetDash(const std::vector<double> dv, const double offset);
    QPainterPath dashedPPath(const std::vector<double> dv, const Base::Vector3d start, const Base::Vector3d end);
    double calcOffset(TechDraw::BaseGeomPtr g, TechDraw::LineSet ls);
    std::vector<double> decodeDashSpec(TechDraw::DashSpec d);

private:
    QGraphicsItem* m_parent;
    QPen m_pen;

    std::vector<TechDraw::LineSet> m_lineSets;
    std::vector<TechDraw::DashSpec> m_dashSpecs;
    std::vector<QGraphicsPathItem*> m_fillItems;


    double m_fillScale;
    double m_lineWidth;
    long int m_segCount;
    long int m_maxSeg;
};

}