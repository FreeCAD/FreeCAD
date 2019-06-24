/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <Bnd_Box.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepBndLib.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopoDS_Solid.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <BRepAlgoAPI_Common.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <App/Document.h>

#include "FeaturePocket.h"


using namespace PartDesign;

/* TRANSLATOR PartDesign::Pocket */

const char* Pocket::TypeEnums[]= {"Length","ThroughAll","UpToFirst","UpToFace","TwoLengths",NULL};

PROPERTY_SOURCE(PartDesign::Pocket, PartDesign::ProfileBased)

Pocket::Pocket()
{
    addSubType = FeatureAddSub::Subtractive;

    ADD_PROPERTY_TYPE(Type,((long)0),"Pocket",App::Prop_None,"Pocket type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length,(100.0),"Pocket",App::Prop_None,"Pocket length");
    ADD_PROPERTY_TYPE(Length2,(100.0),"Pocket",App::Prop_None,"P");
    ADD_PROPERTY_TYPE(UpToFace,(0),"Pocket",App::Prop_None,"Face where pocket will end");
    ADD_PROPERTY_TYPE(Offset,(0.0),"Pocket",App::Prop_None,"Offset from face in which pocket will end");
    static const App::PropertyQuantityConstraint::Constraints signedLengthConstraint = {-DBL_MAX, DBL_MAX, 1.0};
    Offset.setConstraints ( &signedLengthConstraint );
}

short Pocket::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Pocket::execute(void)
{
    // Handle legacy features, these typically have Type set to 3 (previously NULL, now UpToFace),
    // empty FaceName (because it didn't exist) and a value for Length
    if (std::string(Type.getValueAsString()) == "UpToFace" &&
        (UpToFace.getValue() == NULL && Length.getValue() > Precision::Confusion()))
        Type.setValue("Length");

    // Validate parameters
    double L = Length.getValue();
    if ((std::string(Type.getValueAsString()) == "Length") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Pocket: Length of pocket too small");

    double L2 = Length2.getValue();
    if ((std::string(Type.getValueAsString()) == "TwoLengths") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Pocket: Second length of pocket too small");

    Part::Feature* obj = 0;
    TopoDS_Shape profileshape;
    try {
        obj = getVerifiedObject();
        profileshape = getVerifiedFace();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    }
    catch (const Base::Exception&) {
        std::string text(QT_TR_NOOP("The requested feature cannot be created. The reason may be that:\n\n"
                                    "  \xe2\x80\xa2 the active Body does not contain a base shape, so there is no\n"
                                    "  material to be removed;\n"
                                    "  \xe2\x80\xa2 the selected sketch does not belong to the active Body."));
        return new App::DocumentObjectExecReturn(text);
    }

    // get the Sketch plane
    Base::Placement SketchPos    = obj->Placement.getValue();
    Base::Vector3d  SketchVector = getProfileNormal();

    // turn around for pockets
    SketchVector *= -1;

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        gp_Dir dir(SketchVector.x,SketchVector.y,SketchVector.z);
        dir.Transform(invObjLoc.Transformation());

        if (profileshape.IsNull())
            return new App::DocumentObjectExecReturn("Pocket: Creating a face from sketch failed");
        profileshape.Move(invObjLoc);

        std::string method(Type.getValueAsString());
        if (method == "UpToFirst" || method == "UpToFace") {
            if (base.IsNull())
                return new App::DocumentObjectExecReturn("Pocket: Extruding up to a face is only possible if the sketch is located on a face");

            // Note: This will return an unlimited planar face if support is a datum plane
            TopoDS_Face supportface = getSupportFace();
            supportface.Move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            // Find a valid face or datum plane to extrude up to
            TopoDS_Face upToFace;
            if (method == "UpToFace") {
                getUpToFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            getUpToFace(upToFace, base, supportface, profileshape, method, dir, Offset.getValue());

            // BRepFeat_MakePrism(..., 2, 1) in combination with PerForm(upToFace) is buggy when the
            // prism that is being created is contained completely inside the base solid
            // In this case the resulting shape is empty. This is not a problem for the Pad or Pocket itself
            // but it leads to an invalid SubShape
            // The bug only occurs when the upToFace is limited (by a wire), not for unlimited upToFace. But
            // other problems occur with unlimited concave upToFace so it is not an option to always unlimit upToFace
            // Check supportface for limits, otherwise Perform() throws an exception
            TopExp_Explorer Ex(supportface,TopAbs_WIRE);
            if (!Ex.More())
                supportface = TopoDS_Face();
#if 0
            BRepFeat_MakePrism PrismMaker;
            PrismMaker.Init(base, profileshape, supportface, dir, 0, 1);
            PrismMaker.Perform(upToFace);

            if (!PrismMaker.IsDone())
                return new App::DocumentObjectExecReturn("Pocket: Up to face: Could not extrude the sketch!");
            TopoDS_Shape prism = PrismMaker.Shape();
#else
            TopoDS_Shape prism;
            generatePrism(prism, method, base, profileshape, supportface, upToFace, dir, 0, 1);
#endif

            // And the really expensive way to get the SubShape...
            BRepAlgoAPI_Cut mkCut(base, prism);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Pocket: Up to face: Could not get SubShape!");
            // FIXME: In some cases this affects the Shape property: It is set to the same shape as the SubShape!!!!
            TopoDS_Shape result = refineShapeIfActive(mkCut.Shape());
            this->AddSubShape.setValue(result);

            int prismCount = countSolids(prism);
            if (prismCount > 1) {
                return new App::DocumentObjectExecReturn("Pocket: Result has multiple solids. This is not supported at this time.");
            }

            this->Shape.setValue(getSolid(prism));
        } else {
            TopoDS_Shape prism;
            generatePrism(prism, profileshape, method, dir, L, L2,
                        Midplane.getValue(), Reversed.getValue());

            if (prism.IsNull())
                return new App::DocumentObjectExecReturn("Pocket: Resulting shape is empty");

            // set the subtractive shape property for later usage in e.g. pattern
            prism = refineShapeIfActive(prism);
            this->AddSubShape.setValue(prism);

            // Cut the SubShape out of the base feature
            BRepAlgoAPI_Cut mkCut(base, prism);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Pocket: Cut out of base feature failed");
            TopoDS_Shape result = mkCut.Shape();
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn("Pocket: Resulting shape is not a solid");

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn("Pocket: Result has multiple solids. This is not supported at this time.");

            }
            solRes = refineShapeIfActive(solRes);
            remapSupportShape(solRes);
            this->Shape.setValue(getSolid(solRes));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        if (std::string(e.GetMessageString()) == "TopoDS::Face" &&
            (std::string(Type.getValueAsString()) == "UpToFirst" || std::string(Type.getValueAsString()) == "UpToFace"))
            return new App::DocumentObjectExecReturn("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed "
                "for making a pocket up to a face.");
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}
