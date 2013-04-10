/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <cfloat>
# include <BRepLib.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <gp_GTrsf.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_Line.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <Standard_Version.hxx>
#endif


#include "DatumFeature.h"
#include <Base/Tools.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif


namespace PartDesign {
    const App::PropertyFloatConstraint::Constraints angleRange = {0.0f,360.0f,1.0f};
}

using namespace PartDesign;


PROPERTY_SOURCE_ABSTRACT(PartDesign::Datum, PartDesign::Feature)

Datum::Datum(void)
{
    ADD_PROPERTY_TYPE(References,(0,0),"Vertex",(App::PropertyType)(App::Prop_None),"References defining the vertex");
    touch();
}

Datum::~Datum()
{
}

short Datum::mustExecute(void) const
{
    if (References.isTouched())
        return 1;
    return Feature::mustExecute();
}

void Datum::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        try {
            App::DocumentObjectExecReturn *ret = recompute();
            delete ret;
        }
        catch (...) {
        }
    }
    PartDesign::Feature::onChanged(prop);
}

PROPERTY_SOURCE(PartDesign::Vertex, PartDesign::Datum)

Vertex::Vertex()
{
}

Vertex::~Vertex()
{
}

short Vertex::mustExecute() const
{
    return PartDesign::Datum::mustExecute();
}

App::DocumentObjectExecReturn *Vertex::execute(void)
{
    gp_Pnt point(0,0,0);
    // TODO: Find the point
    
    BRepBuilderAPI_MakeVertex MakeVertex(point);
    const TopoDS_Vertex& vertex = MakeVertex.Vertex();
    this->Shape.setValue(vertex);

    return App::DocumentObject::StdReturn;
}


PROPERTY_SOURCE(PartDesign::Line, PartDesign::Datum)

Line::Line()
{
}

Line::~Line()
{
}

short Line::mustExecute() const
{
    return PartDesign::Datum::mustExecute();
}

App::DocumentObjectExecReturn *Line::execute(void)
{
    gp_Pnt point1(0,0,0);

    gp_Pnt point2(10,10,10);

    BRepBuilderAPI_MakeEdge mkEdge(point1, point2);
    if (!mkEdge.IsDone())
        return new App::DocumentObjectExecReturn("Failed to create edge");
    const TopoDS_Edge& edge = mkEdge.Edge();
    this->Shape.setValue(edge);

    return App::DocumentObject::StdReturn;
}


PROPERTY_SOURCE(PartDesign::Plane, PartDesign::Datum)

Plane::Plane()
{
    ADD_PROPERTY_TYPE(Offset,(10.0),"Plane",App::Prop_None,"The offset from the reference");
    ADD_PROPERTY_TYPE(Angle ,(0.0),"Plane",App::Prop_None,"The angle to the reference");
}

short Plane::mustExecute() const
{
    if (Offset.isTouched() ||
        Angle.isTouched() )
        return 1;
    return PartDesign::Datum::mustExecute();
}

App::DocumentObjectExecReturn *Plane::execute(void)
{
    double O = this->Offset.getValue();
    double A = this->Angle.getValue();

    if (fabs(A) > 360.0)
      return new App::DocumentObjectExecReturn("Angle too large (please use -360.0 .. +360.0)");

    gp_Pnt pnt(0.0,0.0,0.0);
    gp_Dir dir(0.0,0.0,1.0);
    Handle_Geom_Plane aPlane = new Geom_Plane(pnt, dir);
    BRepBuilderAPI_MakeFace mkFace(aPlane, 0.0, 100.0, 0.0, 100.0
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
    );

    TopoDS_Shape ResultShape = mkFace.Shape();
    this->Shape.setValue(ResultShape);

    return App::DocumentObject::StdReturn;
}
