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
# include <BRep_Tool.hxx>
# include <gp_GTrsf.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Handle_Geom_Curve.hxx>
# include <Handle_Geom_Surface.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <GeomAPI_IntCS.hxx>
# include <GeomAPI_IntSS.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Standard_Version.hxx>
#endif
// TODO Cleanup headers (2015-09-04, Fat-Zer)
#include <QObject>
#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;
using namespace Attacher;

// ============================================================================

PROPERTY_SOURCE(PartDesign::Plane, Part::Datum)

Plane::Plane()
{
    this->setAttacher(new AttachEngine3D);
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    Shape.setValue(builder.Shape());

}

Plane::~Plane()
{
}

void Plane::onChanged(const App::Property *prop)
{
    Part::Datum::onChanged(prop);
}

Base::Vector3d Plane::getNormal()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,0,1), normal);
    return normal;
}
