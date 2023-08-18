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
# include <BRepAlgoAPI_Cut.hxx>
# include <gp_Dir.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Exception.h>

#include "FeaturePocket.h"

using namespace PartDesign;

/* TRANSLATOR PartDesign::Pocket */

const char* Pocket::TypeEnums[]= {"Length", "ThroughAll", "UpToFirst", "UpToFace", "TwoLengths", nullptr};

PROPERTY_SOURCE(PartDesign::Pocket, PartDesign::FeatureExtrude)

Pocket::Pocket()
{
    addSubType = FeatureAddSub::Subtractive;

    ADD_PROPERTY_TYPE(Type, ((long)0), "Pocket", App::Prop_None, "Pocket type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length, (5.0), "Pocket", App::Prop_None, "Pocket length");
    ADD_PROPERTY_TYPE(Length2, (5.0), "Pocket", App::Prop_None, "Pocket length in 2nd direction");
    ADD_PROPERTY_TYPE(UseCustomVector, (false), "Pocket", App::Prop_None, "Use custom vector for pocket direction");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 1.0, 1.0)), "Pocket", App::Prop_None, "Pocket direction vector");
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Pocket", App::Prop_None, "Reference axis of direction");
    ADD_PROPERTY_TYPE(AlongSketchNormal, (true), "Pocket", App::Prop_None, "Measure pocket length along the sketch normal direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Pocket", App::Prop_None, "Face where pocket will end");
    ADD_PROPERTY_TYPE(Offset, (0.0), "Pocket", App::Prop_None, "Offset from face in which pocket will end");
    Offset.setConstraints(&signedLengthConstraint);
    ADD_PROPERTY_TYPE(TaperAngle, (0.0), "Pocket", App::Prop_None, "Taper angle");
    TaperAngle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(TaperAngle2, (0.0), "Pocket", App::Prop_None, "Taper angle for 2nd direction");
    TaperAngle2.setConstraints(&floatAngle);

    // Remove the constraints and keep the type to allow to accept negative values
    // https://forum.freecad.org/viewtopic.php?f=3&t=52075&p=448410#p447636
    Length2.setConstraints(nullptr);
}

App::DocumentObjectExecReturn *Pocket::execute()
{
    // Handle legacy features, these typically have Type set to 3 (previously NULL, now UpToFace),
    // empty FaceName (because it didn't exist) and a value for Length
    if (std::string(Type.getValueAsString()) == "UpToFace" &&
        (!UpToFace.getValue() && Length.getValue() > Precision::Confusion()))
        Type.setValue("Length");

    double L = Length.getValue();
    double L2 = Length2.getValue();

    TopoDS_Shape profileshape;
    try {
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
        std::string text(QT_TRANSLATE_NOOP("Exception", ("The requested feature cannot be created. The reason may be that:\n"
                                    "  - the active Body does not contain a base shape, so there is no\n"
                                    "  material to be removed;\n"
                                    "  - the selected sketch does not belong to the active Body.")));
        return new App::DocumentObjectExecReturn(text);
    }

    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal();

    // turn around for pockets
    SketchVector *= -1;

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        Base::Vector3d pocketDirection = computeDirection(SketchVector);

        // create vector in pocketing direction with length 1
        gp_Dir dir(pocketDirection.x, pocketDirection.y, pocketDirection.z);

        // The length of a gp_Dir is 1 so the resulting pocket would have
        // the length L in the direction of dir. But we want to have its height in the
        // direction of the normal vector.
        // Therefore we must multiply L by the factor that is necessary
        // to make dir as long that its projection to the SketchVector
        // equals the SketchVector.
        // This is the scalar product of both vectors.
        // Since the pocket length cannot be negative, the factor must not be negative.

        double factor = fabs(dir * gp_Dir(SketchVector.x, SketchVector.y, SketchVector.z));

        // factor would be zero if vectors are orthogonal
        if (factor < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Creation failed because direction is orthogonal to sketch's normal vector"));

        // perform the length correction if not along custom vector
        if (AlongSketchNormal.getValue()) {
            L = L / factor;
            L2 = L2 / factor;
        }

        dir.Transform(invObjLoc.Transformation());

        if (profileshape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Creating a face from sketch failed"));
        profileshape.Move(invObjLoc);

        std::string method(Type.getValueAsString());
        if (method == "UpToFirst" || method == "UpToFace") {
            if (base.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Extruding up to a face is only possible if the sketch is located on a face"));

            // Note: This will return an unlimited planar face if support is a datum plane
            TopoDS_Face supportface = getSupportFace();
            supportface.Move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            // Find a valid face or datum plane to extrude up to
            TopoDS_Face upToFace;
            if (method == "UpToFace") {
                getFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            getUpToFace(upToFace, base, profileshape, method, dir);
            addOffsetToFace(upToFace, dir, Offset.getValue());

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
            TopoDS_Shape prism;
            PrismMode mode = PrismMode::CutFromBase;
            generatePrism(prism, method, base, profileshape, supportface, upToFace, dir, mode, Standard_True);

            // And the really expensive way to get the SubShape...
            BRepAlgoAPI_Cut mkCut(base, prism);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Up to face: Could not get SubShape!"));
            // FIXME: In some cases this affects the Shape property: It is set to the same shape as the SubShape!!!!
            TopoDS_Shape result = refineShapeIfActive(mkCut.Shape());
            this->AddSubShape.setValue(result);

            int prismCount = countSolids(prism);
            if (prismCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }

            this->Shape.setValue(getSolid(prism));
        }
        else {
            TopoDS_Shape prism;
            if (hasTaperedAngle()) {
                if (Reversed.getValue())
                    dir.Reverse();
                generateTaperedPrism(prism, profileshape, method, dir, L, L2, TaperAngle.getValue(), TaperAngle2.getValue(), Midplane.getValue());
            }
            else {
                generatePrism(prism, profileshape, method, dir, L, L2, Midplane.getValue(), Reversed.getValue());
            }

            if (prism.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Resulting shape is empty"));

            // set the subtractive shape property for later usage in e.g. pattern
            prism = refineShapeIfActive(prism);
            this->AddSubShape.setValue(prism);

            // Cut the SubShape out of the base feature
            BRepAlgoAPI_Cut mkCut(base, prism);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Cut out of base feature failed"));
            TopoDS_Shape result = mkCut.Shape();
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));

            }
            solRes = refineShapeIfActive(solRes);
            remapSupportShape(solRes);
            this->Shape.setValue(getSolid(solRes));
        }

        // eventually disable some settings that are not valid for the current method
        updateProperties(method);

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face" &&
            (std::string(Type.getValueAsString()) == "UpToFirst" || std::string(Type.getValueAsString()) == "UpToFace"))
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed "
                "for making a pocket up to a face."));
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}
