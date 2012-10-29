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
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <Handle_Geom_Surface.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Wire.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <Precision.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <BRepAlgoAPI_Common.hxx>
#endif

#include <Base/Placement.h>
#include <App/Document.h>

#include "FeaturePad.h"


using namespace PartDesign;

const char* Pad::TypeEnums[]= {"Length","UpToLast","UpToFirst","UpToFace","TwoLengths",NULL};

PROPERTY_SOURCE(PartDesign::Pad, PartDesign::Additive)

Pad::Pad()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Length,(100.0));
    ADD_PROPERTY(Length2,(100.0));
    ADD_PROPERTY(FaceName,(""));
}

short Pad::mustExecute() const
{
    if (Placement.isTouched() ||
        Length.isTouched() ||
        Length2.isTouched() ||
        FaceName.isTouched())
        return 1;
    return Additive::mustExecute();
}

App::DocumentObjectExecReturn *Pad::execute(void)
{
    // Validate parameters
    double L = Length.getValue();
    if ((std::string(Type.getValueAsString()) == "Length") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Length of pad too small");
    double L2 = Length2.getValue();
    if ((std::string(Type.getValueAsString()) == "TwoLengths") && (L < Precision::Confusion()))
        return new App::DocumentObjectExecReturn("Second length of pad too small");

    Part::Part2DObject* sketch = 0;
    std::vector<TopoDS_Wire> wires;
    try {
        sketch = getVerifiedSketch();
        wires = getSketchWires();
    } catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopoDS_Shape support;
    try {
        support = getSupportShape();
    } catch (const Base::Exception&) {
        // ignore, because support isn't mandatory
        support = TopoDS_Shape();
    }

    // get the Sketch plane
    Base::Placement SketchPos = sketch->Placement.getValue();
    Base::Rotation SketchOrientation = SketchPos.getRotation();
    Base::Vector3d SketchVector(0,0,1);
    SketchOrientation.multVec(SketchVector,SketchVector);

    TopoDS_Shape aFace = makeFace(wires);
    if (aFace.IsNull())
        return new App::DocumentObjectExecReturn("Creating a face from sketch failed");

    this->positionBySketch();
    TopLoc_Location invObjLoc = this->getLocation().Inverted();

    try {
        // extrude the face to a solid
        TopoDS_Shape prism;
        bool isSolid = false; // support is a solid?
        bool isSolidChecked = false; // not checked yet

        if ((std::string(Type.getValueAsString()) == "UpToLast") ||
            (std::string(Type.getValueAsString()) == "UpToFirst") ||
            (std::string(Type.getValueAsString()) == "UpToFace"))
        {
            TopoDS_Face upToFace;
            gp_Dir dir(SketchVector.x,SketchVector.y,SketchVector.z);

            if ((std::string(Type.getValueAsString()) == "UpToLast") ||
                (std::string(Type.getValueAsString()) == "UpToFirst"))
            {
                // Check for valid support object
                if (support.IsNull())
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: No valid support in Sketch");
                TopExp_Explorer xp (support, TopAbs_SOLID);
                if (!xp.More())
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: Support shape is not a solid");
                isSolid = true;
                isSolidChecked = true;

                TopoDS_Shape origFace = makeFace(wires); // original sketch face before moving one unit
                std::vector<Part::cutFaces> cfaces = Part::findAllFacesCutBy(support, origFace, dir);
                if (cfaces.empty())
                      return new App::DocumentObjectExecReturn("No faces found in this direction");

                // Find nearest/furthest face
                std::vector<Part::cutFaces>::const_iterator it, it_near, it_far;
                it_near = it_far = cfaces.begin();
                for (it = cfaces.begin(); it != cfaces.end(); it++)
                    if (it->distsq > it_far->distsq)
                        it_far = it;
                    else if (it->distsq < it_near->distsq)
                        it_near = it;
                upToFace = (std::string(Type.getValueAsString()) == "UpToLast" ? it_far->face : it_near->face);
            } else {
                if (FaceName.isEmpty())
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: No face selected");

                // Get active object, this is the object that the user referenced when he clicked on the face!
                App::DocumentObject* baseLink = this->getDocument()->getActiveObject();

                if (!baseLink)
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: No object linked");
                if (!baseLink->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: Linked object is not a Part object");
                Part::Feature *base = static_cast<Part::Feature*>(baseLink);
                const Part::TopoShape& baseShape = base->Shape.getShape();
                if (baseShape._Shape.IsNull())
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: Cannot work on invalid shape");

                TopoDS_Shape sub = baseShape.getSubShape(FaceName.getValue());
                if (!sub.IsNull() && sub.ShapeType() == TopAbs_FACE)
                    upToFace = TopoDS::Face(sub);
                else
                    return new App::DocumentObjectExecReturn("Cannot extrude up to face: Selection is not a face");

                // Validate face
                // TODO: This would also exclude faces that are valid but not cut by the line
                // So for now we trust to the intelligence of the user when picking the face
                /*std::vector<cutFaces> cfaces = findAllFacesCutBy(upToFace, origFace, dir);
                if (cfaces.empty())
                      return new App::DocumentObjectExecReturn("No faces found in this direction");*/
            }

            // Create semi-infinite prism from sketch in direction dir
            // Hack, because the two lines commented out below do NOT work!!!
            SketchVector *= 1E6;
            gp_Vec vec(SketchVector.x,SketchVector.y,SketchVector.z);
            vec.Transform(invObjLoc.Transformation());
            BRepPrimAPI_MakePrism PrismMaker(aFace.Moved(invObjLoc),vec,0,1); // very long, but finite prism
            //dir.Transform(invObjLoc.Transformation());
            //BRepPrimAPI_MakePrism PrismMaker(aFace.Moved(invObjLoc),dir,0,0,1);
            if (!PrismMaker.IsDone())
                return new App::DocumentObjectExecReturn("Cannot extrude up to face: Could not extrude the sketch!");

            // Cut off the prism at the face we found
            // Grab any point from the sketch
            TopExp_Explorer exp;
            exp.Init(aFace, TopAbs_VERTEX);
            if (!exp.More())
                return new App::DocumentObjectExecReturn("Cannot extrude up to face: Sketch without points?");
            gp_Pnt aPnt = BRep_Tool::Pnt(TopoDS::Vertex(exp.Current()));

            // Create a halfspace from the face, extending in direction of sketch plane
            BRepPrimAPI_MakeHalfSpace mkHalfSpace(upToFace, aPnt);
            if (!mkHalfSpace.IsDone())
                return new App::DocumentObjectExecReturn("Cannot extrude up to face: HalfSpace creation failed");

            // Find common material between halfspace and prism
            BRepAlgoAPI_Common mkCommon(PrismMaker.Shape(), mkHalfSpace.Solid().Moved(invObjLoc));
            if (!mkCommon.IsDone())
                return new App::DocumentObjectExecReturn("Cannot extrude up to face: Common creation failed");

            prism = this->getSolid(mkCommon.Shape());
            if (prism.IsNull())
                return new App::DocumentObjectExecReturn("Cannot extrude up to face: Resulting shape is not a solid");
        } else if ((std::string(Type.getValueAsString()) == "Length") ||
                   (std::string(Type.getValueAsString()) == "TwoLengths")) {
            if (std::string(Type.getValueAsString()) == "Length") {
                if (Midplane.getValue()) {
                    // Move face by half the extrusion distance to get pad symmetric to sketch plane
                    gp_Trsf mov;
                    mov.SetTranslation(gp_Vec(SketchVector.x,SketchVector.y,SketchVector.z) * (-1.0) * L/2.0);
                    TopLoc_Location loc(mov);
                    aFace.Move(loc);
                } else if (Reversed.getValue()) { // negative direction
                    SketchVector *= -1.0;
                }

                // lengthen the vector
                SketchVector *= L;
            } else {
                // Move face by the second length to get pad extending to both sides of sketch plane
                gp_Trsf mov;
                mov.SetTranslation(gp_Vec(SketchVector.x,SketchVector.y,SketchVector.z) * (-1.0) * L2);
                TopLoc_Location loc(mov);
                aFace.Move(loc);

                // lengthen the vector
                SketchVector *= (L + L2);
            }

            // create the extrusion
            gp_Vec vec(SketchVector.x,SketchVector.y,SketchVector.z);
            vec.Transform(invObjLoc.Transformation());
            BRepPrimAPI_MakePrism PrismMaker(aFace.Moved(invObjLoc),vec,0,1); // finite prism
            if (!PrismMaker.IsDone())
                return new App::DocumentObjectExecReturn("Could not extrude the sketch!");
            prism = PrismMaker.Shape();
        } else {
            return new App::DocumentObjectExecReturn("Internal error: Unknown type for Pad feature");
        }

        // set the additive shape property for later usage in e.g. pattern
        this->AddShape.setValue(prism);

        // if the sketch has a support fuse them to get one result object (PAD!)
        if (!support.IsNull()) {

            if (!isSolidChecked) { // we haven't checked for solid, yet
                if (!support.IsNull()) {
                    TopExp_Explorer xp;
                    xp.Init(support,TopAbs_SOLID);
                    for (;xp.More(); xp.Next()) {
                        isSolid = true;
                        break;
                    }
                }

                if (!isSolid)
                    return new App::DocumentObjectExecReturn("Support is not a solid");
            }

            // Let's call algorithm computing a fuse operation:
            BRepAlgoAPI_Fuse mkFuse(support.Moved(invObjLoc), prism);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Pad: Fusion with support failed");
            TopoDS_Shape result = mkFuse.Shape();
            // we have to get the solids (fuse create seldomly compounds)
            TopoDS_Shape solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.IsNull())
                return new App::DocumentObjectExecReturn("Pad: Resulting shape is not a solid");
            this->Shape.setValue(solRes);
        }
        else {
            TopoDS_Shape result = this->getSolid(prism);
            // set the additive shape property for later usage in e.g. pattern
            this->AddShape.setValue(result);
            this->Shape.setValue(result);
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

