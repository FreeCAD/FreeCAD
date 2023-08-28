/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRep_Builder.hxx>
# include <BRepFeat_MakePrism.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <gp_Dir.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include <App/Document.h>
#include <Base/Tools.h>
#include <Mod/Part/App/ExtrusionHelper.h>

#include "FeatureExtrude.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::FeatureExtrude, PartDesign::ProfileBased)

App::PropertyQuantityConstraint::Constraints FeatureExtrude::signedLengthConstraint = { -DBL_MAX, DBL_MAX, 1.0 };
double FeatureExtrude::maxAngle = 90 - Base::toDegrees<double>(Precision::Angular());
App::PropertyAngle::Constraints FeatureExtrude::floatAngle = { -maxAngle, maxAngle, 1.0 };

FeatureExtrude::FeatureExtrude() = default;

short FeatureExtrude::mustExecute() const
{
    if (Placement.isTouched() ||
        Type.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        TaperAngle.isTouched() ||
        TaperAngle2.isTouched() ||
        UseCustomVector.isTouched() ||
        Direction.isTouched() ||
        ReferenceAxis.isTouched() ||
        AlongSketchNormal.isTouched() ||
        Offset.isTouched() ||
        UpToFace.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

Base::Vector3d FeatureExtrude::computeDirection(const Base::Vector3d& sketchVector)
{
    Base::Vector3d extrudeDirection;

    if (!UseCustomVector.getValue()) {
        if (!ReferenceAxis.getValue()) {
            // use sketch's normal vector for direction
            extrudeDirection = sketchVector;
            AlongSketchNormal.setReadOnly(true);
        }
        else {
            // update Direction from ReferenceAxis
            App::DocumentObject* pcReferenceAxis = ReferenceAxis.getValue();
            const std::vector<std::string>& subReferenceAxis = ReferenceAxis.getSubValues();
            Base::Vector3d base;
            Base::Vector3d dir;
            getAxis(pcReferenceAxis, subReferenceAxis, base, dir, ForbiddenAxis::NotPerpendicularWithNormal);
            switch (addSubType) {
            case Type::Additive:
                extrudeDirection = dir;
                break;
            case Type::Subtractive:
                extrudeDirection = -dir;
                break;
            }
        }
    }
    else {
        // use the given vector
        // if null vector, use sketchVector
        if ((fabs(Direction.getValue().x) < Precision::Confusion())
            && (fabs(Direction.getValue().y) < Precision::Confusion())
            && (fabs(Direction.getValue().z) < Precision::Confusion())) {
            Direction.setValue(sketchVector);
        }
        extrudeDirection = Direction.getValue();
    }

    // disable options of UseCustomVector
    Direction.setReadOnly(!UseCustomVector.getValue());
    ReferenceAxis.setReadOnly(UseCustomVector.getValue());
    // UseCustomVector allows AlongSketchNormal but !UseCustomVector does not forbid it
    if (UseCustomVector.getValue())
        AlongSketchNormal.setReadOnly(false);

    // explicitly set the Direction so that the dialog shows also the used direction
    // if the sketch's normal vector was used
    Direction.setValue(extrudeDirection);
    return extrudeDirection;
}

bool FeatureExtrude::hasTaperedAngle() const
{
    return fabs(TaperAngle.getValue()) > Base::toRadians(Precision::Angular()) ||
           fabs(TaperAngle2.getValue()) > Base::toRadians(Precision::Angular());
}

void FeatureExtrude::generatePrism(TopoDS_Shape& prism,
                                   const TopoDS_Shape& sketchshape,
                                   const std::string& method,
                                   const gp_Dir& direction,
                                   const double L,
                                   const double L2,
                                   const bool midplane,
                                   const bool reversed)
{
    if (method == "Length" || method == "TwoLengths" || method == "ThroughAll") {
        double Ltotal = L;
        double Loffset = 0.;
        if (method == "ThroughAll")
            Ltotal = getThroughAllLength();


        if (method == "TwoLengths") {
            Ltotal += L2;
            if (reversed)
                Loffset = -L;
            else
                Loffset = -L2;
        }
        else if (midplane) {
            Loffset = -Ltotal / 2;
        }

        TopoDS_Shape from = sketchshape;
        if (method == "TwoLengths" || midplane) {
            gp_Trsf mov;
            mov.SetTranslation(Loffset * gp_Vec(direction));
            TopLoc_Location loc(mov);
            from = sketchshape.Moved(loc);
        }
        else if (reversed) {
            Ltotal *= -1.0;
        }

        if (fabs(Ltotal) < Precision::Confusion()) {
            if (addSubType == Type::Additive)
                throw Base::ValueError("Cannot create a pad with a height of zero.");
            else
                throw Base::ValueError("Cannot create a pocket with a depth of zero.");
        }

        // Without taper angle we create a prism because its shells are in every case no B-splines and can therefore
        // be use as support for further features like Pads, Lofts etc. B-spline shells can break certain features,
        // see e.g. https://forum.freecad.org/viewtopic.php?p=560785#p560785
        // It is better not to use BRepFeat_MakePrism here even if we have a support because the
        // resulting shape creates problems with Pocket
        BRepPrimAPI_MakePrism PrismMaker(from, Ltotal * gp_Vec(direction), Standard_False, Standard_True); // finite prism
        if (!PrismMaker.IsDone())
            throw Base::RuntimeError("ProfileBased: Length: Could not extrude the sketch!");
        prism = PrismMaker.Shape();
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method '"
            << method << "' for generatePrism()";
        throw Base::RuntimeError(str.str());
    }
}

void FeatureExtrude::generatePrism(TopoDS_Shape& prism,
                                   const std::string& method,
                                   const TopoDS_Shape& baseshape,
                                   const TopoDS_Shape& profileshape,
                                   const TopoDS_Face& supportface,
                                   const TopoDS_Face& uptoface,
                                   const gp_Dir& direction,
                                   PrismMode Mode,
                                   Standard_Boolean Modify)
{
    if (method == "UpToFirst" || method == "UpToFace") {
        BRepFeat_MakePrism PrismMaker;
        TopoDS_Shape base = baseshape;
        for (TopExp_Explorer xp(profileshape, TopAbs_FACE); xp.More(); xp.Next()) {
            PrismMaker.Init(base, xp.Current(), supportface, direction, Mode, Modify);
            PrismMaker.Perform(uptoface);
            if (!PrismMaker.IsDone())
                throw Base::RuntimeError("ProfileBased: Up to face: Could not extrude the sketch!");

            base = PrismMaker.Shape();
            if (Mode == PrismMode::None)
                Mode = PrismMode::FuseWithBase;
        }

        prism = base;
    }
    else if (method == "UpToLast") {
        BRepFeat_MakePrism PrismMaker;
        prism = baseshape;
        for (TopExp_Explorer xp(profileshape, TopAbs_FACE); xp.More(); xp.Next()) {
            PrismMaker.Init(baseshape, xp.Current(), supportface, direction, PrismMode::None, Modify);

            //Each face needs 2 prisms because if uptoFace is intersected twice the first one ends too soon
            for (int i=0; i<2; i++){
                if (i==0){
                    PrismMaker.Perform(uptoface);
                }else{
                    PrismMaker.Perform(uptoface, uptoface);
                }

                if (!PrismMaker.IsDone())
                    throw Base::RuntimeError("ProfileBased: Up to face: Could not extrude the sketch!");
                auto onePrism = PrismMaker.Shape();

                BRepAlgoAPI_Fuse fuse(prism, onePrism);
                prism = fuse.Shape();
            }
        }
    }
    else {
        std::stringstream str;
        str << "ProfileBased: Internal error: Unknown method '"
            << method << "' for generatePrism()";
        throw Base::RuntimeError(str.str());
    }
}

void FeatureExtrude::generateTaperedPrism(TopoDS_Shape& prism,
                                          const TopoDS_Shape& sketchshape,
                                          const std::string& method,
                                          const gp_Dir& direction,
                                          const double L,
                                          const double L2,
                                          const double angle,
                                          const double angle2,
                                          const bool midplane)
{
    std::list<TopoDS_Shape> drafts;
    bool isSolid = true; // in PD we only generate solids, while Part Extrude can also create only shells
    bool isPartDesign = true; // there is an OCC bug with single-edge wires (circles) we need to treat differently for PD and Part
    if (method == "ThroughAll") {
        Part::ExtrusionHelper::makeDraft(sketchshape, direction, getThroughAllLength(),
            0.0, Base::toRadians(angle), 0.0, isSolid, drafts, isPartDesign);
    }
    else if (method == "TwoLengths") {
        Part::ExtrusionHelper::makeDraft(sketchshape, direction, L, L2,
            Base::toRadians(angle), Base::toRadians(angle2), isSolid, drafts, isPartDesign);
    }
    else if (method == "Length") {
        if (midplane) {
            Part::ExtrusionHelper::makeDraft(sketchshape, direction, L / 2, L / 2,
                Base::toRadians(angle), Base::toRadians(angle), isSolid, drafts, isPartDesign);
        }
        else
            Part::ExtrusionHelper::makeDraft(sketchshape, direction, L, 0.0,
                Base::toRadians(angle), 0.0, isSolid, drafts, isPartDesign);
    }

    if (drafts.empty()) {
        throw Base::RuntimeError("Creation of tapered object failed");
    }
    else if (drafts.size() == 1) {
        prism = drafts.front();
    }
    else {
        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);
        for (const auto & draft : drafts)
            builder.Add(comp, draft);
        prism = comp;
    }
}

void FeatureExtrude::updateProperties(const std::string &method)
{
    // disable settings that are not valid on the current method
    // disable everything unless we are sure we need it
    bool isLengthEnabled = false;
    bool isLength2Enabled = false;
    bool isOffsetEnabled = false;
    bool isMidplaneEnabled = false;
    bool isReversedEnabled = false;
    bool isUpToFaceEnabled = false;
    bool isTaperVisible = false;
    bool isTaper2Visible = false;
    if (method == "Length") {
        isLengthEnabled = true;
        isTaperVisible = true;
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == "UpToLast") {
        isOffsetEnabled = true;
        isReversedEnabled = true;
    }
    else if (method == "ThroughAll") {
        isMidplaneEnabled = true;
        isReversedEnabled = !Midplane.getValue();
    }
    else if (method == "UpToFirst") {
        isOffsetEnabled = true;
        isReversedEnabled = true;
    }
    else if (method == "UpToFace") {
        isOffsetEnabled = true;
        isReversedEnabled = true;
        isUpToFaceEnabled = true;
    }
    else if (method == "TwoLengths") {
        isLengthEnabled = true;
        isLength2Enabled = true;
        isTaperVisible = true;
        isTaper2Visible = true;
        isReversedEnabled = true;
    }

    Length.setReadOnly(!isLengthEnabled);
    AlongSketchNormal.setReadOnly(!isLengthEnabled);
    Length2.setReadOnly(!isLength2Enabled);
    Offset.setReadOnly(!isOffsetEnabled);
    TaperAngle.setReadOnly(!isTaperVisible);
    TaperAngle2.setReadOnly(!isTaper2Visible);
    Midplane.setReadOnly(!isMidplaneEnabled);
    Reversed.setReadOnly(!isReversedEnabled);
    UpToFace.setReadOnly(!isUpToFaceEnabled);
}
