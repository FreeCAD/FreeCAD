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

#ifndef TECHDRAW_COSMETIC_VERTEX_H
#define TECHDRAW_COSMETIC_VERTEX_H

#include "PreCompiled.h"
#ifndef _PreComp_
    #include <App/FeaturePython.h>
    #include <Base/Persistence.h>
    #include <Base/Vector3D.h>
#endif

#include "Geometry.h"


class TopoDS_Edge;

namespace TechDraw {
class DrawViewPart;

class TechDrawExport CosmeticVertex: public Base::Persistence, public TechDraw::Vertex
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    CosmeticVertex();
    CosmeticVertex(const CosmeticVertex* cv);
    CosmeticVertex(Base::Vector3d loc);
    ~CosmeticVertex() override = default;

    void move(Base::Vector3d newPos);
    void moveRelative(Base::Vector3d movement);

    std::string toString() const;
    void dump(const char* title) override;
    Base::Vector3d scaled(double factor);

    static bool restoreCosmetic();

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;
    CosmeticVertex* copy() const;
    CosmeticVertex* clone() const;

    Base::Vector3d permaPoint;           //permanent, unscaled value
    int            linkGeom;             //connection to corresponding "geom" Vertex (fragile - index based!)
                                         //better to do reverse search for CosmeticTag in vertex geometry
    App::Color     color;
    double         size;
    int            style;
    bool           visible;              //base class vertex also has visible property

    boost::uuids::uuid getTag() const;
    std::string getTagAsString() const override;

protected:
    //Uniqueness
    void createNewTag();
    void assignTag(const TechDraw::CosmeticVertex* cv);

    boost::uuids::uuid tag;

    Py::Object PythonObject;


};

} //end namespace TechDraw

#endif  // TECHDRAW_COSMETIC_VERTEX_H
