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
# include <BRepAlgoAPI_Fuse.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "FeaturePad.h"

using namespace PartDesign;

const char* Pad::TypeEnums[]= {"Length", "UpToLast", "UpToFirst", "UpToFace", "TwoLengths", nullptr};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::FeatureExtrude)

Pad::Pad()
{
    addSubType = FeatureAddSub::Additive;

    ADD_PROPERTY_TYPE(Type, (0L), "Pad", App::Prop_None, "Pad type");
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(Length, (10.0), "Pad", App::Prop_None, "Pad length");
    ADD_PROPERTY_TYPE(Length2, (10.0), "Pad", App::Prop_None, "Pad length in 2nd direction");
    ADD_PROPERTY_TYPE(UseCustomVector, (false), "Pad", App::Prop_None, "Use custom vector for pad direction");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 1.0, 1.0)), "Pad", App::Prop_None, "Pad direction vector");
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Pad", App::Prop_None, "Reference axis of direction");
    ADD_PROPERTY_TYPE(AlongSketchNormal, (true), "Pad", App::Prop_None, "Measure pad length along the sketch normal direction");
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Pad", App::Prop_None, "Face where pad will end");
    ADD_PROPERTY_TYPE(Offset, (0.0), "Pad", App::Prop_None, "Offset from face in which pad will end");
    Offset.setConstraints(&signedLengthConstraint);
    ADD_PROPERTY_TYPE(TaperAngle, (0.0), "Pad", App::Prop_None, "Taper angle");
    TaperAngle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(TaperAngle2, (0.0), "Pad", App::Prop_None, "Taper angle for 2nd direction");
    TaperAngle2.setConstraints(&floatAngle);

    // Remove the constraints and keep the type to allow to accept negative values
    // https://forum.freecad.org/viewtopic.php?f=3&t=52075&p=448410#p447636
    Length2.setConstraints(nullptr);
}

App::DocumentObjectExecReturn *Pad::execute()
{
    double L = Length.getValue();
    double L2 = Length2.getValue();

    // if midplane is true, disable reversed and vice versa
    bool hasMidplane = Midplane.getValue();
    bool hasReversed = Reversed.getValue();
    Midplane.setReadOnly(hasReversed);
    Reversed.setReadOnly(hasMidplane);

    TopoDS_Shape sketchshape;
    try {
        sketchshape = getVerifiedFace();
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    }
    catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal();

    try {
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();

        base.Move(invObjLoc);

        Base::Vector3d paddingDirection = computeDirection(SketchVector);

        // create vector in padding direction with length 1
        gp_Dir dir(paddingDirection.x, paddingDirection.y, paddingDirection.z);

        // The length of a gp_Dir is 1 so the resulting pad would have
        // the length L in the direction of dir. But we want to have its height in the
        // direction of the normal vector.
        // Therefore we must multiply L by the factor that is necessary
        // to make dir as long that its projection to the SketchVector
        // equals the SketchVector.
        // This is the scalar product of both vectors.
        // Since the pad length cannot be negative, the factor must not be negative.

        double factor = fabs(dir * gp_Dir(SketchVector.x, SketchVector.y, SketchVector.z));

        // factor would be zero if vectors are orthogonal
        if (factor < Precision::Confusion())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pad: Creation failed because direction is orthogonal to sketch's normal vector"));

        // perform the length correction if not along custom vector
        if (AlongSketchNormal.getValue()) {
            L = L / factor;
            L2 = L2 / factor;
        }

        dir.Transform(invObjLoc.Transformation());

        if (sketchshape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pad: Creating a face from sketch failed"));
        sketchshape.Move(invObjLoc);

        TopoDS_Shape prism;
        std::string method(Type.getValueAsString());
        if (method == "UpToFirst" || method == "UpToLast" || method == "UpToFace") {
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
            getUpToFace(upToFace, base, sketchshape, method, dir);
            addOffsetToFace(upToFace, dir, Offset.getValue());

            // TODO: Write our own PrismMaker which does not depend on a solid base shape
            if (base.IsNull()) {
                //generatePrism(prism, sketchshape, "Length", dir, length, 0.0, false, false);
                base = sketchshape;
                supportface = TopoDS::Face(sketchshape);
                TopExp_Explorer Ex(supportface,TopAbs_WIRE);
                if (!Ex.More())
                    supportface = TopoDS_Face();

                PrismMode mode = PrismMode::None;
                generatePrism(prism, method, base, sketchshape, supportface, upToFace, dir, mode, Standard_True);
                base.Nullify();
            }
            else {
                // A support object is always required and we need to use BRepFeat_MakePrism
                // Problem: For Pocket/UpToFirst (or an equivalent Pocket/UpToFace) the resulting shape is invalid
                // because the feature does not add any material. This only happens with the "2" option, though
                // Note: It might be possible to pass a shell or a compound containing multiple faces
                // as the Until parameter of Perform()
                // Note: Multiple independent wires are not supported, we should check for that and
                // warn the user
                // FIXME: If the support shape is not the previous solid in the tree, then there will be unexpected results
                // Check supportface for limits, otherwise Perform() throws an exception
                TopExp_Explorer Ex(supportface,TopAbs_WIRE);
                if (!Ex.More())
                    supportface = TopoDS_Face();
                PrismMode mode = PrismMode::None;
                generatePrism(prism, method, base, sketchshape, supportface, upToFace, dir, mode, Standard_True);
            }
        }
        else {
            if (hasTaperedAngle()) {
                if (hasReversed)
                    dir.Reverse();
                generateTaperedPrism(prism, sketchshape, method, dir, L, L2, TaperAngle.getValue(), TaperAngle2.getValue(), hasMidplane);
            }
            else {
                generatePrism(prism, sketchshape, method, dir, L, L2, hasMidplane, hasReversed);
            }
        }

        if (prism.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pad: Resulting shape is empty"));

        // set the additive shape property for later usage in e.g. pattern
        prism = refineShapeIfActive(prism);
        this->AddSubShape.setValue(prism);

        if (!base.IsNull()) {
            // Let's call algorithm computing a fuse operation:
            BRepAlgoAPI_Fuse mkFuse(base, prism);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pad: Fusion with base feature failed"));
            TopoDS_Shape result = mkFuse.Shape();
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            int solidCount = countSolids(result);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }

            solRes = refineShapeIfActive(solRes);
            this->Shape.setValue(getSolid(solRes));
        }
        else {
            int solidCount = countSolids(prism);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }

           this->Shape.setValue(getSolid(prism));
        }

        // eventually disable some settings that are not valid for the current method
        updateProperties(method);

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face")
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed."));
        else
            return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

}
