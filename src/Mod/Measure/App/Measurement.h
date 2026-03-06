// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <gp_Pnt.hxx>
#include <TopAbs_ShapeEnum.hxx>

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include <Base/BaseClass.h>
#include <Base/Vector3D.h>
#include <Mod/Measure/MeasureGlobal.h>


class TopoDS_Shape;
namespace Measure
{
enum class MeasureType
{
    Volumes,           // Measure the Volume(s)
    Edges,             // Measure the Edge(s)
    Line,              // One Line
    TwoLines,          // Two lines
    TwoParallelLines,  // Two parallel lines
    Circle,            // One circle
    CircleArc,         // One circle arc
    TwoCircles,
    CircleToEdge,
    CircleToSurface,
    CircleToCylinder,
    Surfaces,         // Measure the surface(s)
    Cylinder,         // One Cylinder
    CylinderSection,  // One cylinder section
    TwoCylinders,
    Cone,       // One Cone
    Sphere,     // One Sphere
    Torus,      // One Torus
    Plane,      // One Plane
    TwoPlanes,  // One Plane
    Points,
    PointToPoint,  // Measure between TWO points
    PointToEdge,   // Measure between ONE point and ONE edge
    PointToCircle,
    PointToSurface,  // Measure between ONE point and ONE surface
    PointToCylinder,
    EdgeToEdge,  // Measure between TWO edges
    Invalid
};

class MeasureExport Measurement: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    App::PropertyLinkSubList References3D;

public:
    Measurement();
    ~Measurement() override;

    void clear();
    bool has3DReferences();

    /// Add a reference
    int addReference3D(App::DocumentObject* obj, const std::string& subName);
    int addReference3D(App::DocumentObject* obj, const char* subName);

    MeasureType getType();
    MeasureType findType();

    // from base class
    PyObject* getPyObject() override;
    virtual unsigned int getMemSize() const;

    // Methods for distances (edge length, two points, edge and a point
    double length() const;
    Base::Vector3d delta() const;  // when would client use delta??
    double lineLineDistance() const;
    double circleCenterDistance() const;
    double planePlaneDistance() const;
    double cylinderAxisDistance() const;

    // Calculates the radius for an arc or circular edge
    double radius() const;

    // Calculates the diameter for a circle or a cylinder
    double diameter() const;

    // Calculates the angle between two edges
    double angle(const Base::Vector3d& param = Base::Vector3d(0, 0, 0)) const;  // param is never used???

    // Calculate the center of mass
    Base::Vector3d massCenter() const;

    // Calculate the volume of selected volumes
    double volume() const;

    // Calculate the area of selection
    double area() const;

    static Base::Vector3d toVector3d(const gp_Pnt gp)
    {
        return Base::Vector3d(gp.X(), gp.Y(), gp.Z());
    }

    bool planesAreParallel() const;
    bool linesAreParallel() const;

protected:
    // Hint parameter helps sort out compound shapes by specifying a subelement type
    // use hint = TopAbs_COMPOUND to give no hint
    TopoDS_Shape getShape(
        App::DocumentObject* obj,
        const char* subName,
        TopAbs_ShapeEnum hint = TopAbs_COMPOUND
    ) const;

private:
    MeasureType measureType;
    Py::SmartPtr PythonObject;
};


}  // namespace Measure
