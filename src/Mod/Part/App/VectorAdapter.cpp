/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Application.h>
#include <Mod/Part/PartGlobal.h>
#include "VectorAdapter.h"
#include "Base/Console.h"
#include <string>

#include "PrimitiveFeature.h"
#include "PartFeature.h"

#include <Standard_Type.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ElementarySurface.hxx>
#include <TopExp.hxx>
#include <Geom_Line.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <BRep_Tool.hxx>


namespace Part {


VectorAdapter::VectorAdapter() : status(false), vector()
{
}

VectorAdapter::VectorAdapter(const TopoDS_Face &faceIn, const gp_Vec &pickedPointIn) :
  status(false), vector(), origin(pickedPointIn)
{
  Handle(Geom_Surface) surface = BRep_Tool::Surface(faceIn);
  if (surface->IsKind(STANDARD_TYPE(Geom_ElementarySurface)))
  {
    Handle(Geom_ElementarySurface) eSurface = Handle(Geom_ElementarySurface)::DownCast(surface);
    gp_Dir direction = eSurface->Axis().Direction();
    vector = direction;
    vector.Normalize();
    if (faceIn.Orientation() == TopAbs_REVERSED) {
      vector.Reverse();
    }
    if (surface->IsKind(STANDARD_TYPE(Geom_CylindricalSurface)) ||
      surface->IsKind(STANDARD_TYPE(Geom_SphericalSurface))
    )
    {
      origin = eSurface->Axis().Location().XYZ();
      projectOriginOntoVector(pickedPointIn);
    }
    else {
      origin = pickedPointIn + vector;
    }
    status = true;
  }
}

VectorAdapter::VectorAdapter(const TopoDS_Edge &edgeIn, const gp_Vec &pickedPointIn) :
  status(false), vector(), origin(pickedPointIn)
{
  TopoDS_Vertex firstVertex = TopExp::FirstVertex(edgeIn, Standard_True);
  TopoDS_Vertex lastVertex = TopExp::LastVertex(edgeIn, Standard_True);
  vector = convert(lastVertex) - convert(firstVertex);
  if (vector.Magnitude() < Precision::Confusion()) {
    return;
  }
  vector.Normalize();

  status = true;
  projectOriginOntoVector(pickedPointIn);
}

VectorAdapter::VectorAdapter(const TopoDS_Vertex &vertex1In, const TopoDS_Vertex &vertex2In) :
  status(false), vector(), origin()
{
  vector = convert(vertex2In) - convert(vertex1In);
  vector.Normalize();

  //build origin half way.
  gp_Vec tempVector = (convert(vertex2In) - convert(vertex1In));
  double mag = tempVector.Magnitude();
  tempVector.Normalize();
  tempVector *= (mag / 2.0);
  origin = tempVector + convert(vertex1In);

  status = true;
}

VectorAdapter::VectorAdapter(const gp_Vec &vector1, const gp_Vec &vector2) :
  status(false), vector(), origin()
{
  vector = vector2- vector1;
  vector.Normalize();

  //build origin half way.
  gp_Vec tempVector = vector2 - vector1;
  double mag = tempVector.Magnitude();
  tempVector.Normalize();
  tempVector *= (mag / 2.0);
  origin = tempVector + vector1;

  status = true;
}

void VectorAdapter::projectOriginOntoVector(const gp_Vec &pickedPointIn)
{
  Handle(Geom_Curve) heapLine = new Geom_Line(origin.XYZ(), vector.XYZ());
  gp_Pnt tempPoint(pickedPointIn.XYZ());
  GeomAPI_ProjectPointOnCurve projection(tempPoint, heapLine);
  if (projection.NbPoints() < 1) {
    return;
  }
  origin.SetXYZ(projection.Point(1).XYZ());
}

VectorAdapter::operator gp_Lin() const
{
  gp_Pnt tempOrigin;
  tempOrigin.SetXYZ(origin.XYZ());
  return gp_Lin(tempOrigin, gp_Dir(vector));
}


/*convert a vertex to vector*/
gp_Vec VectorAdapter::convert(const TopoDS_Vertex &vertex)
{
  gp_Pnt point = BRep_Tool::Pnt(vertex);
  gp_Vec out(point.X(), point.Y(), point.Z());
  return out;
}



}

