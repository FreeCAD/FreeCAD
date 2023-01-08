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

#ifndef TECHDRAW_DIMENSIONGEOMETRY_h_
#define TECHDRAW_DIMENSIONGEOMETRY_h_

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <string>

#include <Base/Vector3D.h>

namespace TechDraw
{
class DrawViewPart;

//a convenient container for linear dimension end points
class TechDrawExport pointPair
{
public:
    pointPair() = default;
    pointPair(Base::Vector3d point0, Base::Vector3d point1) { m_first = point0; m_second = point1; }
    pointPair(const pointPair& pp);

    pointPair& operator= (const pointPair& pp) {
        m_first = pp.first();
        m_second = pp.second();
        return *this;
    }

    Base::Vector3d first() const { return m_first; }
    void first(Base::Vector3d newFirst) { m_first = newFirst; }
    Base::Vector3d second() const { return m_second; }
    void second(Base::Vector3d newSecond) { m_second = newSecond; }

    void move(Base::Vector3d offset);
    void project(DrawViewPart* dvp);
    void mapToPage(DrawViewPart* dvp);
    void invertY();
    void dump(std::string text) const;

private:
    Base::Vector3d m_first;
    Base::Vector3d m_second;
};

//a convenient container for angular dimension points
class TechDrawExport anglePoints
{
public:
    anglePoints();
    anglePoints(Base::Vector3d apex, Base::Vector3d point0, Base::Vector3d point1) {
        vertex(apex); first(point0); second(point1); }
    anglePoints(const anglePoints& ap);

    anglePoints& operator= (const anglePoints& ap);

    pointPair ends() const { return m_ends; }
    void ends(pointPair newEnds) { m_ends = newEnds; }
    Base::Vector3d first() const { return m_ends.first(); }
    void first(Base::Vector3d newFirst) { m_ends.first(newFirst); }
    Base::Vector3d second() const { return m_ends.second(); }
    void second(Base::Vector3d newSecond) { m_ends.second(newSecond); }
    Base::Vector3d vertex() const { return m_vertex; }
    void vertex(Base::Vector3d newVertex) { m_vertex = newVertex; }

    void move(Base::Vector3d offset);
    void project(DrawViewPart* dvp);
    void mapToPage(DrawViewPart* dvp);
    void invertY();
    void dump(std::string text) const;
    
private:
    pointPair m_ends;
    Base::Vector3d m_vertex;
};

//a convenient container for diameter or radius dimension points
class TechDrawExport arcPoints
{
public:
    arcPoints();
    arcPoints(const arcPoints& ap);

    arcPoints& operator= (const arcPoints& ap);

    void move(Base::Vector3d offset);
    void project(DrawViewPart* dvp);
    void mapToPage(DrawViewPart* dvp);
    void invertY();
    void dump(std::string text) const;

//TODO: setters and getters
    bool isArc;
    double radius;
    Base::Vector3d center;
    pointPair onCurve;
    pointPair arcEnds;
    Base::Vector3d midArc;
    bool arcCW;
};

}   //end namespace TechDraw

#endif
