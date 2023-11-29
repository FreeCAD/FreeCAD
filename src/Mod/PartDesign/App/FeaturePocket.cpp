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
# include <BRepAlgoAPI_Common.hxx>
#include <Geom_Plane.hxx>
#include <gp_Pln.hxx>
#include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <gp_Dir.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Face.hxx>
#endif
#include <TopoDS.hxx>
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
    Refine.setValue(Standard_True);
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

    TopoDS_Shape profileShape;
    try {
        profileShape = getVerifiedFace();
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

        if (profileShape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Creating a face from sketch failed"));
        profileShape.Move(invObjLoc);

        std::string method(Type.getValueAsString());
        if (method == "UpToFirst" || method == "UpToFace") {
            if (base.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Extruding up to a face is only possible if the sketch is located on a face"));

            // Note: This will return an unlimited planar face if support is a datum plane
            TopoDS_Face supportFace = getSupportFace();
            supportFace.Move(invObjLoc);

            if (Reversed.getValue())
                dir.Reverse();

            // Find a valid face or datum plane to extrude up to
            TopoDS_Face upToFace;
            if (method == "UpToFace") {
                getFaceFromLinkSub(upToFace, UpToFace);
                upToFace.Move(invObjLoc);
            }
            getUpToFace(upToFace, base, profileShape, method, dir);
            addOffsetToFace(upToFace, dir, Offset.getValue());

            // BRepFeat_MakePrism(..., 2, 1) in combination with PerForm(upToFace) is buggy when the
            // prism that is being created is contained completely inside the base solid
            // In this case the resulting shape is empty. This is not a problem for the Pad or Pocket itself
            // but it leads to an invalid SubShape
            // The bug only occurs when the upToFace is limited (by a wire), not for unlimited upToFace. But
            // other problems occur with unlimited concave upToFace so it is not an option to always unlimit upToFace
            // Check supportface for limits, otherwise Perform() throws an exception
            TopExp_Explorer Ex(supportFace,TopAbs_WIRE);
            if (!Ex.More())
                supportFace = TopoDS_Face();
            TopoDS_Shape prism;
            PrismMode mode = PrismMode::CutFromBase;
            generatePrism(prism, method, base, profileShape, supportFace, upToFace, dir, mode, Standard_True);

            // Generate the prism for material above and below the pocket
            // As noted above, this BrepFeat_MakePrism is very sensitive, so we end up creating a bounding box,
            // intersecting planes with it, and then using the resulting two faces to create the prism. 

            // make a box just a little bigger than the real bounding box to avoid edge cases
            Base::BoundBox3d bbox = getBaseTopoShape().getBoundBox();
            BRepPrimAPI_MakeBox mkBox(bbox.LengthX()*1.1, bbox.LengthY()*1.1, bbox.LengthZ()*1.1);
            TopoDS_Shape box = mkBox.Shape();
            gp_Trsf gp1;
            gp1.SetTransformation(gp_Ax3(gp_Pnt(bbox.MinX*1.05,bbox.MinY*1.05,bbox.MinZ*1.05),gp_Dir(0,0,1)));
            TopLoc_Location boxLoc(gp1);
            box.Move(boxLoc.Inverted()); // move the box half the extra size we made it

            TopExp_Explorer xp;
            extendFace(supportFace, box);
            extendFace(upToFace, box);
            TopoDS_Face profileFace = TopoDS::Face(profileShape);
            extendFace(profileFace, box);
            // When using the face with BRepFeat_MakePrism::Perform(const TopoDS_Shape& Until)
            // then the algorithm expects that the 'NaturalRestriction' flag is set in order
            // to work as expected (see generatePrism())
            BRep_Builder builder;
            builder.NaturalRestriction(upToFace, Standard_True);

            TopoDS_Shape keepprism, keepprism2;
            generatePrism(keepprism, method, box, supportFace, supportFace, upToFace, dir, mode, Standard_True);
            try {
                generatePrism(keepprism2, method, box, supportFace, supportFace, profileFace, dir.Reversed(), mode, Standard_True);
                BRepAlgoAPI_Cut mkCut2(base, keepprism2);
                keepprism2 = mkCut2.Shape();
                if (!mkCut2.IsDone())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Cut of base object failed"));
                BRepAlgoAPI_Fuse mkFuse2(keepprism, keepprism2);
                keepprism = mkFuse2.Shape();
                if (!mkFuse2.IsDone()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Pocket: Fuse of result objects failed"));
                }
            } catch ( Base::RuntimeError& e) {
                // If the pocket is on the outsied edge, there isn't a second prism
            }
            if (keepprism.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Can't create removal prism"));

            // Invert the prism relative to the base
            BRepAlgoAPI_Cut mkCut(base, prism);
            TopoDS_Shape iprism = mkCut.Shape();
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Cut of base object failed"));
            // Get rid of the portion of the object above the pocket
            BRepAlgoAPI_Common mkCom(prism, keepprism);
            TopoDS_Shape keepshape = mkCom.Shape();
            if (!mkCom.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Cut of base object failed"));
            // generate the new object and fuse it in
            TopoDS_Shape shape = FeatureAddSub::subtractiveOp(base, iprism);
            BRepAlgoAPI_Fuse mkFuse(keepshape, shape);
            shape = mkFuse.Shape();
            if (!mkFuse.IsDone()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Pocket: Fuse of result objects failed"));
            }

            TopoDS_Shape result = refineShapeIfActive(shape);
            int prismCount = countSolids(prism);
            if (prismCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            }

            this->AddSubShape.setValue(getSolid(prism));
            // this->Shape.setValue(getSolid(result));
            this->Shape.setValue(result);
        }
        else {
            TopoDS_Shape prism;
            if (hasTaperedAngle()) {
                if (Reversed.getValue())
                    dir.Reverse();
                generateTaperedPrism(prism, profileShape, method, dir, L, L2, TaperAngle.getValue(), TaperAngle2.getValue(), Midplane.getValue());
            }
            else {
                generatePrism(prism, profileShape, method, dir, L, L2, Midplane.getValue(), Reversed.getValue());
            }
            TopoDS_Shape loseprism;
            TopoDS_Face profileFace = TopoDS::Face(profileShape);
            extendFace(profileFace, TopoDS_Shape());
            generatePrism(loseprism, profileFace, method, dir, L, L2, Midplane.getValue(), Reversed.getValue());
            if (loseprism.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Resulting shape is empty"));

            // set the subtractive shape property for later usage in e.g. pattern
            prism = refineShapeIfActive(prism);
            this->AddSubShape.setValue(prism);

            // Get rid of the portion of the object above the pocket
            BRepAlgoAPI_Cut mkCut(base, loseprism);
            TopoDS_Shape keepshape = mkCut.Shape();
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Cut of base object failed"));
      
            // generate the new object above the pocket plane and fuse it in
            TopoDS_Shape shape = FeatureAddSub::subtractiveOp(base, prism);
            BRepAlgoAPI_Fuse mkFuse(keepshape, shape);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pocket: Fuse of result objects failed"));

            TopoDS_Shape result = refineShapeIfActive(mkFuse.Shape());
            TopoDS_Shape solRes = this->getSolid(result); // we have to get the solids (fuse sometimes creates compounds)
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            int solidCount = countSolids(result);
            if (solidCount > 1)
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
            solRes = refineShapeIfActive(solRes);
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
