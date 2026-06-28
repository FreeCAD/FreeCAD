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

#pragma once

#include <string>

#include <Base/Vector3D.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


namespace TechDraw
{
class DrawViewPart;

//a convenient container for linear dimension end points
class TechDrawExport pointPair
{
public:
    pointPair() = default;
    pointPair(const Base::Vector3d& point0, const Base::Vector3d& point1)
        : m_first(point0)
        , m_second(point1){};

    pointPair(const Base::Vector3d& point0, const Base::Vector3d& point1,
              const Base::Vector3d& extensionPoint0, const Base::Vector3d& extensionPoint1)
        : m_first(point0)
        , m_second(point1)
        , m_useOverrideFirst(true)
        , m_overrideFirst(extensionPoint0)
        , m_useOverrideSecond(true)
        , m_overrideSecond(extensionPoint1){};

    pointPair(const pointPair& pp);

    pointPair& operator=(const pointPair& pp) {
        m_first = pp.first();
        m_second = pp.second();
        overrideFirst(pp.extensionLineFirst());
        overrideSecond(pp.extensionLineSecond());
        return *this;
    }

    Base::Vector3d first() const { return m_first; }
    void first(Base::Vector3d newFirst) { m_first = newFirst; }
    Base::Vector3d second() const { return m_second; }
    void second(Base::Vector3d newSecond) { m_second = newSecond; }
    // extension line specific points
    Base::Vector3d extensionLineFirst() const { return m_useOverrideFirst ? m_overrideFirst : m_first; }
    void overrideFirst(Base::Vector3d newFirst) { m_useOverrideFirst = true;  m_overrideFirst = newFirst; }
    Base::Vector3d extensionLineSecond() const { return m_useOverrideSecond ? m_overrideSecond : m_second; }
    void overrideSecond(Base::Vector3d newSecond) { m_useOverrideSecond = true;  m_overrideSecond = newSecond; }
    void setExtensionLine(const pointPair& pp);

    void move(const Base::Vector3d& offset);
    void project(const DrawViewPart* dvp);
    void mapToPage(const DrawViewPart* dvp);
    void invertY();
    void scale(double factor);
    void dump(const std::string& text) const;

    pointPair toCanonicalForm(DrawViewPart* dvp) const;
    pointPair toDisplayForm(DrawViewPart* dvp) const;


private:
    Base::Vector3d m_first;
    Base::Vector3d m_second;
    // extension line specific points
    bool m_useOverrideFirst = false;
    Base::Vector3d m_overrideFirst;
    bool m_useOverrideSecond = false;
    Base::Vector3d m_overrideSecond;
};

//a convenient container for angular dimension points
class TechDrawExport anglePoints
{
public:
    anglePoints();
    anglePoints(const Base::Vector3d& apex, const Base::Vector3d& point0, Base::Vector3d point1) {
        vertex(apex); first(point0); second(point1); }
    anglePoints(const anglePoints& ap);

    anglePoints& operator= (const anglePoints& ap);

    pointPair ends() const { return m_ends; }
    void ends(const pointPair& newEnds) { m_ends = newEnds; }
    Base::Vector3d first() const { return m_ends.first(); }
    void first(const Base::Vector3d& newFirst) { m_ends.first(newFirst); }
    Base::Vector3d second() const { return m_ends.second(); }
    void second(const Base::Vector3d& newSecond) { m_ends.second(newSecond); }
    Base::Vector3d vertex() const { return m_vertex; }
    void vertex(const Base::Vector3d& newVertex) { m_vertex = newVertex; }

    void move(const Base::Vector3d& offset);
    void project(const DrawViewPart* dvp);
    void mapToPage(const DrawViewPart* dvp);
    void invertY();
    void dump(const std::string& text) const;

    anglePoints toCanonicalForm(DrawViewPart* dvp) const;
    anglePoints toDisplayForm(DrawViewPart* dvp) const;


private:
    pointPair m_ends;
    Base::Vector3d m_vertex;
};

//a convenient container for diameter or radius dimension points
class TechDrawExport arcPoints
{
public:
    arcPoints();
    arcPoints(const arcPoints& ap) = default;

    arcPoints& operator= (const arcPoints& ap);

    void move(const Base::Vector3d& offset);
    void project(const DrawViewPart* dvp);
    void mapToPage(const DrawViewPart* dvp);
    void invertY();
    void dump(const std::string& text) const;

    arcPoints toCanonicalForm(DrawViewPart* dvp) const;
    arcPoints toDisplayForm(DrawViewPart* dvp) const;

//TODO: setters and getters
    bool isArc;
    double radius;
    Base::Vector3d center;
    pointPair onCurve;
    pointPair arcEnds;
    Base::Vector3d midArc;
    bool arcCW;
};

//a convenient container for area dimension
class TechDrawExport areaPoint
{
public:
    areaPoint();
    areaPoint(const areaPoint& ap) = default;

    areaPoint& operator= (const areaPoint& ap);

    void move(const Base::Vector3d& offset);
    void project(const DrawViewPart* dvp);
    void invertY();
    void dump(const std::string& text) const;

    double getFilledArea() const { return area; }
    double getActualArea() const { return actualArea; }
    Base::Vector3d getCenter() const { return center; }

    double area{0};             // this is the outer area without considering holes
    double actualArea{0};       // this is the net area after holes are removed
    Base::Vector3d center;      // this is geometric center of the outer face
};

}   //end namespace TechDraw