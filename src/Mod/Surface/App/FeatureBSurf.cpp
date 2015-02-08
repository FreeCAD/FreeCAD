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
#include <GeomFill_BezierCurves.hxx>
#include <Geom_BoundedSurface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopExp_Explorer.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureBSurf.h"

using namespace Surface;

PROPERTY_SOURCE(Surface::BSurf, Part::Feature)

const char* BSurf::FillTypeEnums[]    = {"Invalid", "Sretched", "Coons", "Curved", NULL};

BSurf::BSurf(): Feature()
{
    ADD_PROPERTY(FillType, ((long)0));
    ADD_PROPERTY(BoundaryList, (0, "Dummy"));
    FillType.StatusBits |= 4; // read-only in property editor
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
    if(ftype==StretchStyle) {return GeomFill_StretchStyle;}
    else if(ftype==CoonsStyle) {return GeomFill_CoonsStyle;}
    else if(ftype==CurvedStyle) {return GeomFill_CurvedStyle;}
    else {Standard_Failure::Raise("Filling style must be 1 (Stretch), 2 (Coons), or 3 (Curved).");}
}


void BSurf::getWire(TopoDS_Wire& aWire)
{
    Handle(ShapeFix_Wire) aShFW = new ShapeFix_Wire;
    Handle(ShapeExtend_WireData) aWD = new ShapeExtend_WireData;

    int boundaryListSize = BoundaryList.getSize();
    if(boundaryListSize > 4) // if too many not even try
    {
        Standard_Failure::Raise("Only 2-4 curves are allowed");
        return;
    }

    BRepBuilderAPI_Copy copier;
    edgeCount = 0;
    for(int i = 0; i < boundaryListSize; i++)
    {
        App::PropertyLinkSubList::SubSet set = BoundaryList[i];

        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {

            const Part::TopoShape &ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();
            if(ts._Shape.ShapeType() == TopAbs_WIRE)
            {
                const TopoDS_Wire &wire = TopoDS::Wire(ts._Shape);
                // resolve the wire, we need edges from now on
                for (TopExp_Explorer wireExplorer (wire, TopAbs_EDGE); wireExplorer.More(); wireExplorer.Next())
                {
                    // make a copy of the shape and the underlying geometry to avoid to affect the input shapes
                    copier.Perform(wireExplorer.Current());
                    aWD->Add(TopoDS::Edge(copier.Shape()));
                    edgeCount++;
                }
            }
            else
            {
                //we want only the subshape which is linked
                const TopoDS_Shape &sub = ts.getSubShape(set.sub);
                // make a copy of the shape and the underlying geometry to avoid to affect the input shapes
                copier.Perform(sub);
                const TopoDS_Shape &copy = copier.Shape();

                if(copy.ShapeType() == TopAbs_EDGE) {  //Check Shape type and assign edge
                    aWD->Add(TopoDS::Edge(copy));
                    edgeCount++;
                }
                else {
                    Standard_Failure::Raise("Curves must be of type TopoDS_Edge or TopoDS_Wire");
                    return; //Raise exception
                }
            }
        }
        else{Standard_Failure::Raise("Curve not from Part::Feature");return;}
    }
    if(edgeCount < 2 || edgeCount > 4)
    {
        Standard_Failure::Raise("Only 2-4 curves are allowed");
        return;
    }

    //Reorder the curves and fix the wire if required

    aShFW->Load(aWD); //Load in the wire
    aShFW->FixReorder(); //Fix the order of the edges if required
    aShFW->ClosedWireMode() = Standard_True; //Enables closed wire mode
    aShFW->FixConnected(); //Fix connection between wires
    aShFW->FixSelfIntersection(); //Fix Self Intersection
    aShFW->Perform(); //Perform the fixes

    aWire = aShFW->Wire(); //Healed Wire

    if(aWire.IsNull()){Standard_Failure::Raise("Wire unable to be constructed");return;}
}

void BSurf::createFace(const Handle_Geom_BoundedSurface &aSurface)
{
    BRepBuilderAPI_MakeFace aFaceBuilder;
    Standard_Real u1, u2, v1, v2;
    // transfer surface bounds to face
    aSurface->Bounds(u1, u2, v1, v2);
    aFaceBuilder.Init(aSurface, u1, u2, v1, v2, Precision::Confusion());

    TopoDS_Face aFace = aFaceBuilder.Face();

    if(!aFaceBuilder.IsDone()) { Standard_Failure::Raise("Face unable to be constructed");}
    if (aFace.IsNull()) { Standard_Failure::Raise("Resulting Face is null"); }
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
