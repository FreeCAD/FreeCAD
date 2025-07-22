/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

//! a class to assist with exporting sketches to dxf


#include "PreCompiled.h"

#ifndef _PreComp_
#include <HLRBRep_Algo.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Placement.h>
#include <Base/Vector3D.h>

#include <Mod/Part/App/PartFeature.h>

#include "SketchExportHelper.h"

using namespace Import;

//! project a shape so that it is represented as a flat shape on the XY plane.  Z coordinate
//! information is lost in this process, so it should only be used for flat objects like sketches.
//! Note: this only returns hard and outline edges.  Seam, smooth, isoparametric and hidden lines
//! are not returned.
TopoDS_Shape SketchExportHelper::projectShape(const TopoDS_Shape& inShape,
                                              const gp_Ax2& projectionCS)
{
    Handle(HLRBRep_Algo) brep_hlr = new HLRBRep_Algo();
    brep_hlr->Add(inShape);
    HLRAlgo_Projector projector(projectionCS);
    brep_hlr->Projector(projector);
    brep_hlr->Update();
    brep_hlr->Hide();
    HLRBRep_HLRToShape hlrToShape(brep_hlr);
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    if (!hlrToShape.VCompound().IsNull()) {
        builder.Add(comp, hlrToShape.VCompound());
    }
    if (!hlrToShape.OutLineVCompound().IsNull()) {
        builder.Add(comp, hlrToShape.OutLineVCompound());
    }
    return comp;
}


//! true if obj is a sketch
bool SketchExportHelper::isSketch(App::DocumentObject* obj)
{
    // Use name to lookup to avoid dependency on Sketcher module
    return obj->isDerivedFrom(Base::Type::fromName("Sketcher::SketchObject"));
}


//! return a version of a sketch's geometry mapped to the OXYZ coordinate system
//! preferred by dxf
TopoDS_Shape SketchExportHelper::getFlatSketchXY(App::DocumentObject* obj)
{
    // since we can't reference Sketcher module here, we will cast obj to
    // a Part::Feature instead
    auto sketch = dynamic_cast<Part::Feature*>(obj);
    if (!sketch || !isSketch(obj)) {
        return {};
    }

    auto plm = sketch->Placement.getValue();
    Base::Rotation rot = plm.getRotation();

    // get the sketch normal
    Base::Vector3d stdZ {0.0, 0.0, 1.0};
    Base::Vector3d sketchNormal;
    rot.multVec(stdZ, sketchNormal);
    Base::Vector3d stdX {1.0, 0.0, 0.0};
    Base::Vector3d sketchX;
    rot.multVec(stdX, sketchX);

    // get the sketch origin
    Base::Vector3d position = plm.getPosition();
    gp_Ax2 projectionCS(gp_Pnt(position.x, position.y, position.z),
                        gp_Dir(sketchNormal.x, sketchNormal.y, sketchNormal.z),
                        gp_Dir(sketchX.x, sketchX.y, sketchX.z));
    const TopoDS_Shape& shape = sketch->Shape.getValue();
    return projectShape(shape, projectionCS);
}
