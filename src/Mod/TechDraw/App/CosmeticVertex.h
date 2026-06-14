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

#include <App/FeaturePython.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

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
    CosmeticVertex(const Base::Vector3d& loc);
    ~CosmeticVertex() override = default;

    Base::Vector3d point() const  { return permaPoint; };
    void point(Base::Vector3d newPoint) { permaPoint = newPoint; }
    void move(const Base::Vector3d& newPos);
    void moveRelative(const Base::Vector3d& movement);

    std::string toString() const;
    void dump(const char* title) override;
    Base::Vector3d scaled(const double factor) const;
    Base::Vector3d rotatedAndScaled(const double scale, const double rotDegrees) const;

    static Base::Vector3d makeCanonicalPoint(DrawViewPart* dvp, Base::Vector3d point, bool unscale = true);
    static Base::Vector3d makeCanonicalPointInverted(DrawViewPart* dvp, Base::Vector3d invertedPoint, bool unscale = true);
    static bool restoreCosmetic();

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;

    PyObject *getPyObject() override;
    CosmeticVertex* copy() const;
    CosmeticVertex* clone() const;

    Base::Vector3d permaPoint{Base::Vector3d()};           //permanent, unscaled value
    int            linkGeom{-1};             //connection to corresponding "geom" Vertex (fragile - index based!)
                                         //better to do reverse search for CosmeticTag in vertex geometry
    Base::Color     color{Base::Color()};
    double         size{1.0};
    int            style{1};
    bool           visible{true};              //base class vertex also has visible property

protected:
    Py::Object PythonObject;
};

} //end namespace TechDraw