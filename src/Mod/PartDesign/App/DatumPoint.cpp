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
# include <gp_Circ.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_Circle.hxx>
# include <Geom2d_Line.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Surface.hxx>
# include <Geom2d_Line.hxx>
# include <GeomAPI_IntCS.hxx>
# include <GeomAPI_IntSS.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Standard_Version.hxx>
# include <QObject>
#endif
// TODO Cleanup headers (2015-09-04, Fat-Zer)

#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include "Mod/Part/App/PrimitiveFeature.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;
using namespace Attacher;

// ============================================================================

PROPERTY_SOURCE(PartDesign::Point, Part::Datum)

Point::Point()
{
    this->setAttacher(new AttachEnginePoint);
    this->makeShape();
}

Point::~Point()
{
}

void Point::onChanged(const App::Property* prop)
{
    if(prop == &(this->Shape)){
        //fix for #0002758 Datum point moves to (0,0,0) when reopening the file.
        //bypass Part::Feature's onChanged, which may alter Placement property to match shape's placement.
        //This is to prevent loss of correct Placement when restoring Shape from file.
        App::GeoFeature::onChanged(prop);
        return;
    }
    Superclass::onChanged(prop);
}

void Point::onDocumentRestored()
{
    //fix for #0002758 Datum point moves to (0,0,0) when reopening the file.
    //recreate shape, as the restored one has old Placement burned into it.
    this->makeShape();
    Superclass::onDocumentRestored();
}

void Point::makeShape()
{
    // Create a shape, which will be used by Sketcher, attachables, and whatever. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeVertex builder(gp_Pnt(0,0,0));
    if (!builder.IsDone())
        return;
    Part::TopoShape tshape(builder.Shape());
    tshape.setPlacement(this->Placement.getValue());
    Shape.setValue(tshape);
}

Base::Vector3d Point::getPoint()
{
    return Placement.getValue().getPosition();
}
