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

#ifndef TECHDRAW_COSMETIC_H
#define TECHDRAW_COSMETIC_H

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include "Geometry.h"


class TopoDS_Edge;

namespace TechDraw {
class DrawViewPart;


//general purpose line format specifier
class TechDrawExport LineFormat
{
public:
    LineFormat();
    LineFormat(const int style,
               const double weight,
               const App::Color& color,
               const bool visible);
    ~LineFormat() = default;

    int m_style;
    double m_weight;
    App::Color m_color;
    bool m_visible;

    static double getDefEdgeWidth();
    static App::Color getDefEdgeColor();
    static int getDefEdgeStyle();

    void dump(const char* title);
    std::string toString() const;
};

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

    std::string toString() const override;
    void dump(const char* title) const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;
    CosmeticEdge* copy() const;
    CosmeticEdge* clone() const;

    Base::Vector3d permaStart;         //persistent unscaled start/end points in View coords
    Base::Vector3d permaEnd;
    double permaRadius;
//    void unscaleEnds(double scale);
    TechDraw::BaseGeomPtr m_geometry;
    LineFormat m_format;

    boost::uuids::uuid getTag() const;
    std::string getTagAsString() const override;

protected:
    //Uniqueness
    void createNewTag();
    void assignTag(const TechDraw::CosmeticEdge* ce);
    boost::uuids::uuid tag;

    Py::Object PythonObject;

};

//********** GeomFormat ********************************************************

// format specifier for geometric edges (Edge5)
class TechDrawExport GeomFormat: public Base::Persistence
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

    //Uniqueness
    boost::uuids::uuid getTag() const;
    virtual std::string getTagAsString() const;

protected:
    void createNewTag();
    void assignTag(const TechDraw::GeomFormat* gf);

    boost::uuids::uuid tag;
    Py::Object PythonObject;
};

} //end namespace TechDraw

#endif //TECHDRAW_COSMETIC_H
