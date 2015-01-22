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
#endif

#include <Base/Exception.h>
#include <Base/Tools.h>

#include "FeatureBSurf.h"

using namespace Surface;

PROPERTY_SOURCE(Surface::BSurf, Part::Feature)

BSurf::BSurf(): Feature()
{
  ADD_PROPERTY(filltype,(1));
  ADD_PROPERTY(aBList,(0,"Dummy"));
}


//Check if any components of the surface have been modified
short BSurf::mustExecute() const
{
    if (aBList.isTouched() ||
        filltype.isTouched())
    {
        return 1;
    }
    return 0;
}

GeomFill_FillingStyle BSurf::getFillingStyle()
{
    //Identify filling style
    int ftype = filltype.getValue();
    if(ftype==StretchStyle) {return GeomFill_StretchStyle;}
    else if(ftype==CoonsStyle) {return GeomFill_CoonsStyle;}
    else if(ftype==CurvedStyle) {return GeomFill_CurvedStyle;}
    else {Standard_Failure::Raise("Filling style must be 1 (Stretch), 2 (Coons), or 3 (Curved).");}
}


void BSurf::getWire(TopoDS_Wire& aWire)
{
    Handle(ShapeFix_Wire) aShFW = new ShapeFix_Wire;
    Handle(ShapeExtend_WireData) aWD = new ShapeExtend_WireData;

    if(aBList.getSize()>4 || aBList.getSize()<2){Standard_Failure::Raise("Only 2-4 curves are allowed");return;}

    for(int i=0; i<aBList.getSize(); i++){

        Part::TopoShape ts; //Curve TopoShape
        TopoDS_Shape sub;   //Curve TopoDS_Shape
        TopoDS_Edge etmp;   //Curve TopoDS_Edge

        //Get Edge
        App::PropertyLinkSubList::SubSet set = aBList[i];

        if(set.obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {

            ts = static_cast<Part::Feature*>(set.obj)->Shape.getShape();

            //we want only the subshape which is linked
            sub = ts.getSubShape(set.sub);
            // make a copy of the shape and the underlying geometry to avoid to affect the input shapes
            BRepBuilderAPI_Copy copy(sub);
            sub = copy.Shape();

            if(sub.ShapeType() == TopAbs_EDGE) {  //Check Shape type and assign edge
                etmp = TopoDS::Edge(sub);
            }
            else {
                Standard_Failure::Raise("Curves must be type TopoDS_Edge");
                return; //Raise exception
            }
            aWD->Add(etmp);

        }
        else{Standard_Failure::Raise("Curve not from Part::Feature");return;}

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
