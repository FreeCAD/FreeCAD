/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <Standard_Failure.hxx>
# include <TopoDS_Solid.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <gp_Pln.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
#endif

// TODO Cleanup headers (2015-09-04, Fat-Zer)
#include <Base/Exception.h>
#include "App/Document.h"
#include "App/OriginFeature.h"
#include "Body.h"
#include "Feature.h"
#include "Mod/Part/App/DatumFeature.h"

#include <Base/Console.h>


namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Feature,Part::Feature)

Feature::Feature()
{
    ADD_PROPERTY(BaseFeature,(0));
    Placement.StatusBits.set(3, true);
}

short Feature::mustExecute() const
{
    if (BaseFeature.isTouched())
        return 1;
    return Part::Feature::mustExecute();
}

TopoDS_Shape Feature::getSolid(const TopoDS_Shape& shape)
{
    if (shape.IsNull())
        Standard_Failure::Raise("Shape is null");
    TopExp_Explorer xp;
    xp.Init(shape,TopAbs_SOLID);
    for (;xp.More(); xp.Next()) {
        return xp.Current();
    }

    return TopoDS_Shape();
}

const gp_Pnt Feature::getPointFromFace(const TopoDS_Face& f)
{
    if (!f.Infinite()) {
        TopExp_Explorer exp;
        exp.Init(f, TopAbs_VERTEX);
        if (exp.More())
            return BRep_Tool::Pnt(TopoDS::Vertex(exp.Current()));
        // Else try the other method
    }

    // TODO: Other method, e.g. intersect X,Y,Z axis with the (unlimited?) face?
    // Or get a "corner" point if the face is limited?
    throw Base::Exception("getPointFromFace(): Not implemented yet for this case");
}

Part::Feature* Feature::getBaseObject(bool silent) const {
    App::DocumentObject* BaseLink = BaseFeature.getValue();
    Part::Feature* BaseObject = nullptr;
    const char *err = nullptr;

    if (BaseLink) {
        if (BaseLink->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            BaseObject = static_cast<Part::Feature*>(BaseLink);
        }
        if (!BaseObject) {
            err =  "No base feature linked";
        }
    } else {
        err = "Base property not set";
    }

    // If the funtion not in silent mode throw the exception discribing the error
    if (!silent && err) {
        throw Base::Exception(err);
    }

    return BaseObject;
}

const TopoDS_Shape& Feature::getBaseShape() const {
    const Part::Feature* BaseObject = getBaseObject();

    const TopoDS_Shape& result = BaseObject->Shape.getValue();
    if (result.IsNull())
        throw Base::Exception("Base feature's shape is invalid");
    TopExp_Explorer xp (result, TopAbs_SOLID);
    if (!xp.More())
        throw Base::Exception("Base feature's shape is not a solid");

    return result;
}

const Part::TopoShape Feature::getBaseTopoShape() const {
    const Part::Feature* BaseObject = getBaseObject();

    const Part::TopoShape& result = BaseObject->Shape.getShape();
    if (result._Shape.IsNull())
        throw Base::Exception("Base feature's TopoShape is invalid");

    return result;
}

bool Feature::isDatum(const App::DocumentObject* feature)
{
    return feature->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()) ||
           feature->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId());
}

gp_Pln Feature::makePlnFromPlane(const App::DocumentObject* obj)
{
    const App::GeoFeature* plane = static_cast<const App::GeoFeature*>(obj);
    if (plane == NULL)
        throw Base::Exception("Feature: Null object");

    Base::Vector3d pos = plane->Placement.getValue().getPosition();
    Base::Rotation rot = plane->Placement.getValue().getRotation();
    Base::Vector3d normal(0,0,1);
    rot.multVec(normal, normal);
    return gp_Pln(gp_Pnt(pos.x,pos.y,pos.z), gp_Dir(normal.x,normal.y,normal.z));
}

TopoDS_Shape Feature::makeShapeFromPlane(const App::DocumentObject* obj)
{
    BRepBuilderAPI_MakeFace builder(makePlnFromPlane(obj));
    if (!builder.IsDone())
        throw Base::Exception("Feature: Could not create shape from base plane");

    return builder.Shape();
}

}
