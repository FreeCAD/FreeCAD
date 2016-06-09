/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller    <Nathan.A.Mill[at]gmail.com> *
 *                           Balázs Bámer                                  *
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
#include <ShapeFix_Wire.hxx>
#include <ShapeExtend_WireData.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <Geom_BezierCurve.hxx>
#include <GeomFill_BezierCurves.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureBSurf.h"

using namespace Surface;

void ShapeValidator::initValidator(void)
{
    willBezier = willBSpline = false;
    edgeCount = 0;
}

// shows error message if the shape is not an edge
void ShapeValidator::checkEdge(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE)
    {
        Standard_Failure::Raise("Shape is not an edge.");
    }

    TopoDS_Edge etmp = TopoDS::Edge(shape);   //Curve TopoDS_Edge
    TopLoc_Location heloc; // this will be output
    Standard_Real u0;// contains output
    Standard_Real u1;// contains output
    Handle_Geom_Curve c_geom = BRep_Tool::Curve(etmp,heloc,u0,u1); //The geometric curve
    Handle_Geom_BezierCurve bez_geom = Handle_Geom_BezierCurve::DownCast(c_geom); //Try to get Bezier curve
    if (bez_geom.IsNull())
    {
        // this one is not Bezier, we hope it can be converted into b-spline
        if (willBezier) {
            // already found the other type, fail
            Standard_Failure::Raise("Mixing Bezier and non-Bezier curves is not allowed.");
        }
        // we will create b-spline surface
        willBSpline = true;
    }
    else
    {
        // this one is Bezier
        if (willBSpline) {
            // already found the other type, fail
            Standard_Failure::Raise("Mixing Bezier and non-Bezier curves is not allowed.");
        }
        // we will create Bezier surface
        willBezier = true;
    }

    edgeCount++;
}

void ShapeValidator::checkAndAdd(const TopoDS_Shape &shape, Handle(ShapeExtend_WireData) *aWD)
{
    checkEdge(shape);
    if (aWD != NULL)
    {
        BRepBuilderAPI_Copy copier(shape);
        // make a copy of the shape and the underlying geometry to avoid to affect the input shapes
        (*aWD)->Add(TopoDS::Edge(copier.Shape()));
    }
}

void ShapeValidator::checkAndAdd(const Part::TopoShape &ts, const char *subName, Handle(ShapeExtend_WireData) *aWD)
{
    try {
        // unwrap the wire
        if((!ts._Shape.IsNull()) && ts._Shape.ShapeType() == TopAbs_WIRE)
        {
            TopoDS_Wire wire = TopoDS::Wire(ts._Shape);
            for (TopExp_Explorer wireExplorer (wire, TopAbs_EDGE); wireExplorer.More(); wireExplorer.Next())
            {
                checkAndAdd(wireExplorer.Current(), aWD);
            }
        }
        else
        {
            if(subName != NULL && *subName != 0)
            {
                //we want only the subshape which is linked
                checkAndAdd(ts.getSubShape(subName), aWD);
            }
            else
            {
                checkAndAdd(ts._Shape, aWD);
            }
        }
    }
    catch(Standard_Failure) { // any OCC exception means an unappropriate shape in the selection
        Standard_Failure::Raise("Wrong shape type.");
    }
}


PROPERTY_SOURCE(Surface::BSurf, Part::Feature)

const char* BSurf::FillTypeEnums[]    = {"Invalid", "Stretched", "Coons", "Curved", NULL};

BSurf::BSurf(): Feature()
{
    ADD_PROPERTY(FillType, ((long)0));
    ADD_PROPERTY(BoundaryList, (0, "Dummy"));
    FillType.setStatus(App::Property::ReadOnly, true); // read-only in property editor
    FillType.setEnums(FillTypeEnums);
}


//Check if any components of the surface have been modified
short BSurf::mustExecute() const
{
    if (BoundaryList.isTouched() ||
        FillType.isTouched())
    {
        return 1;
    }
    return 0;
}

GeomFill_FillingStyle BSurf::getFillingStyle()
{
    //Identify filling style
    int ftype = FillType.getValue();
    if (ftype==StretchStyle)
        return GeomFill_StretchStyle;
    else if(ftype==CoonsStyle)
        return GeomFill_CoonsStyle;
    else if(ftype==CurvedStyle)
        return GeomFill_CurvedStyle;
    else
        Standard_Failure::Raise("Filling style must be 1 (Stretch), 2 (Coons), or 3 (Curved).");
    throw; // this is to shut up the compiler
}


void BSurf::getWire(TopoDS_Wire& aWire)
{
    Handle(ShapeFix_Wire) aShFW = new ShapeFix_Wire;
    Handle(ShapeExtend_WireData) aWD = new ShapeExtend_WireData;

    std::vector<App::PropertyLinkSubList::SubSet> boundary = BoundaryList.getSubListValues();
    if(boundary.size() > 4) // if too many not even try
    {
        Standard_Failure::Raise("Only 2-4 curves are allowed");
    }

    initValidator();
    for(std::size_t i = 0; i < boundary.size(); i++)
    {
        App::PropertyLinkSubList::SubSet set = boundary[i];

        if(set.first->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {

            for (auto jt: set.second) {
                const Part::TopoShape &ts = static_cast<Part::Feature*>(set.first)->Shape.getShape();
                checkAndAdd(ts, jt.c_str(), &aWD);
            }
        }
        else {
            Standard_Failure::Raise("Curve not from Part::Feature");
        }
    }

    if(edgeCount < 2 || edgeCount > 4)
    {
        Standard_Failure::Raise("Only 2-4 curves are allowed");
    }

    //Reorder the curves and fix the wire if required

    aShFW->Load(aWD); //Load in the wire
    aShFW->FixReorder(); //Fix the order of the edges if required
    aShFW->ClosedWireMode() = Standard_True; //Enables closed wire mode
    aShFW->FixConnected(); //Fix connection between wires
    aShFW->FixSelfIntersection(); //Fix Self Intersection
    aShFW->Perform(); //Perform the fixes

    aWire = aShFW->Wire(); //Healed Wire

    if(aWire.IsNull()) {
        Standard_Failure::Raise("Wire unable to be constructed");
    }
}

void BSurf::createFace(const Handle_Geom_BoundedSurface &aSurface)
{
    BRepBuilderAPI_MakeFace aFaceBuilder;
    Standard_Real u1, u2, v1, v2;
    // transfer surface bounds to face
    aSurface->Bounds(u1, u2, v1, v2);
    aFaceBuilder.Init(aSurface, u1, u2, v1, v2, Precision::Confusion());

    TopoDS_Face aFace = aFaceBuilder.Face();

    if(!aFaceBuilder.IsDone()) {
        Standard_Failure::Raise("Face unable to be constructed");
    }
    if (aFace.IsNull()) {
        Standard_Failure::Raise("Resulting Face is null");
    }
    this->Shape.setValue(aFace);
}

void BSurf::correcteInvalidFillType()
{
    int ftype = FillType.getValue();
    if(ftype == InvalidStyle)
    {
        FillType.setValue(StretchStyle);
    }
}
