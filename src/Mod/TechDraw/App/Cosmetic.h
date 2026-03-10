/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <QColor>

#include <App/FeaturePython.h>
#include <Base/Color.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "Geometry.h"
#include "LineFormat.h"

class TopoDS_Edge;

namespace TechDraw {
class DrawViewPart;

//********** CosmeticEdge ******************************************************

class TechDrawExport CosmeticEdge : public Base::Persistence, public TechDraw::BaseGeom
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    CosmeticEdge();
    CosmeticEdge(const TechDraw::BaseGeomPtr* geometry);
    CosmeticEdge(const CosmeticEdge* ce);
    CosmeticEdge(const Base::Vector3d& p1, const Base::Vector3d& p2);
    CosmeticEdge(const TopoDS_Edge& e);
    CosmeticEdge(const TechDraw::BaseGeomPtr g);
    ~CosmeticEdge() override;

    void initialize();
    TopoDS_Edge TopoDS_EdgeFromVectors(const Base::Vector3d& pt1, const Base::Vector3d& pt2);
    TechDraw::BaseGeomPtr scaledGeometry(const double scale);
    TechDraw::BaseGeomPtr scaledAndRotatedGeometry(const double scale, const double rotDegrees);

    static TechDraw::BaseGeomPtr makeCanonicalLine(DrawViewPart* dvp, Base::Vector3d start, Base::Vector3d end);
    static TechDraw::BaseGeomPtr makeLineFromCanonicalPoints(Base::Vector3d start, Base::Vector3d end);

    LineFormat format() const { return m_format; }
    void setFormat(LineFormat newFormat) { m_format = newFormat; }

    std::string toString() const override;
    void dump(const char* title) const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;
    CosmeticEdge* clone() const;

    Base::Vector3d permaStart;         //persistent unscaled start/end points in View coords
    Base::Vector3d permaEnd;
    double permaRadius;
//    void unscaleEnds(double scale);
    TechDraw::BaseGeomPtr m_geometry;
    LineFormat m_format;

protected:
    Py::Object PythonObject;
};

//********** GeomFormat ********************************************************

// format specifier for geometric edges (Edge5)
class TechDrawExport GeomFormat: public Base::Persistence, public TechDraw::Tag
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    GeomFormat();
    explicit GeomFormat(const TechDraw::GeomFormat* gf);
    GeomFormat(const int idx,
               const LineFormat& fmt);
    ~GeomFormat() override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;
    GeomFormat* copy() const;
    GeomFormat* clone() const;

    std::string toString() const;
    void dump(const char* title) const;

    //std::string linkTag;
    int m_geomIndex;            //connection to edgeGeom
    LineFormat m_format;

protected:
    Py::Object PythonObject;
};

} //end namespace TechDraw