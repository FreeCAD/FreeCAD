/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <gp_Ax2.hxx>
# include <Law_Function.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Wire.hxx>
# include <TopTools_HSequenceOfShape.hxx>
#endif

#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/FaceMakerCheese.h>

#include "FeaturePipe.h"


using namespace PartDesign;

const char* Pipe::TypeEnums[] = {"FullPath", "UpToFace", nullptr};
const char* Pipe::TransitionEnums[] = {"Transformed", "Right corner", "Round corner", nullptr};
const char* Pipe::ModeEnums[] = {"Standard", "Fixed", "Frenet", "Auxiliary", "Binormal", nullptr};
const char* Pipe::TransformEnums[] = {
    "Constant", "Multisection", "Linear", "S-shape", "Interpolation", nullptr};


PROPERTY_SOURCE(PartDesign::Pipe, PartDesign::ProfileBased)

Pipe::Pipe()
{
    ADD_PROPERTY_TYPE(Sections, (nullptr), "Sweep", App::Prop_None, "List of sections");
    Sections.setValue(nullptr);
    ADD_PROPERTY_TYPE(Spine, (nullptr), "Sweep", App::Prop_None, "Path to sweep along");
    ADD_PROPERTY_TYPE(SpineTangent, (false), "Sweep", App::Prop_None,
        "Include tangent edges into path");
    ADD_PROPERTY_TYPE(AuxillerySpine, (nullptr), "Sweep", App::Prop_None,
        "Secondary path to orient sweep");
    ADD_PROPERTY_TYPE(AuxillerySpineTangent, (false), "Sweep", App::Prop_None,
        "Include tangent edges into secondary path");
    ADD_PROPERTY_TYPE(AuxilleryCurvelinear, (true), "Sweep", App::Prop_None,
        "Calculate normal between equidistant points on both spines");
    ADD_PROPERTY_TYPE(Mode, (long(0)), "Sweep", App::Prop_None, "Profile mode");
    ADD_PROPERTY_TYPE(Binormal, (Base::Vector3d()), "Sweep", App::Prop_None,
        "Binormal vector for corresponding orientation mode");
    ADD_PROPERTY_TYPE(Transition, (long(0)), "Sweep", App::Prop_None, "Transition mode");
    ADD_PROPERTY_TYPE(Transformation, (long(0)), "Sweep", App::Prop_None,
        "Section transformation mode");
    Mode.setEnums(ModeEnums);
    Transition.setEnums(TransitionEnums);
    Transformation.setEnums(TransformEnums);
}

short Pipe::mustExecute() const
{
    if (Sections.isTouched())
        return 1;
    if (Spine.isTouched())
        return 1;
    if (Mode.isTouched())
        return 1;
    if (Transition.isTouched())
        return 1;
    return ProfileBased::mustExecute();
}

App::DocumentObjectExecReturn *Pipe::execute()
{
    auto getSectionShape = [](App::DocumentObject* feature,
                              const std::vector<std::string>& subs) -> TopoDS_Shape {
        if (!feature || !feature->isDerivedFrom(Part::Feature::getClassTypeId()))
            throw Base::TypeError("Pipe: Invalid profile/section");

        auto subName = subs.empty() ? "" : subs.front();

        // only take the entire shape when we have a sketch selected, but
        // not a point of the sketch
        if (feature->isDerivedFrom(Part::Part2DObject::getClassTypeId())
            && subName.compare(0, 6, "Vertex") != 0)
            return static_cast<Part::Part2DObject*>(feature)->Shape.getValue();
        else {
            if (subName.empty())
                throw Base::ValueError("Pipe: No valid subelement linked in Part::Feature");
            return static_cast<Part::Feature*>(feature)->Shape.getShape().getSubShape(
                subName.c_str());
        }
    };

    auto addWiresToWireSections =
        [](TopoDS_Shape& section, std::vector<std::vector<TopoDS_Shape>>& wiresections) -> size_t {
        TopExp_Explorer ex;
        size_t i = 0;
        bool initialWireSectionsEmpty = wiresections.empty();
        for (ex.Init(section, TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
            // if profile was just a point then this is where we can first set our list
            if (i >= wiresections.size()) {
                if (initialWireSectionsEmpty)
                    wiresections.emplace_back(1, ex.Current());
                else
                    throw Base::ValueError(
                        "Pipe: Sections need to have the same amount of inner wires (except "
                        "profile and last section, which can be points)");
            }
            else
                wiresections[i].push_back(TopoDS::Wire(ex.Current()));
        }
        return i;
    };

    // TODO: currently we can only allow planar faces, so add that check.
    // The reason for this is that with other faces in front, we could not use the
    // current simulate approach and build the start and end face from the wires.
    // As the shell begins always at the spine and not the profile, the sketchshape
    // cannot be used directly as front face. We would need a method to translate
    // the front shape to match the shell starting position somehow...
    std::vector<TopoDS_Wire> wires;
    TopoDS_Shape profilePoint;

    // if the Base property has a valid shape, fuse the pipe into it
    TopoDS_Shape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
        base = TopoDS_Shape();
    }

    try {
        // setup the location
        this->positionByPrevious();
        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        if (!base.IsNull())
            base.Move(invObjLoc);

        // setup the profile section
        TopoDS_Shape profileShape = getSectionShape(Profile.getValue(),
                                                    Profile.getSubValues());
        if (profileShape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pipe: Could not obtain profile shape"));

        // build the paths
        App::DocumentObject* spine = Spine.getValue();
        if (!(spine && spine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "No spine linked"));

        std::vector<std::string> subedge = Spine.getSubValues();
        TopoDS_Shape path;
        const Part::TopoShape& shape = static_cast<Part::Feature*>(spine)->Shape.getValue();
        buildPipePath(shape, subedge, path);
        path.Move(invObjLoc);

        // auxiliary
        TopoDS_Shape auxpath;
        if (Mode.getValue() == 3) {
            App::DocumentObject* auxspine = AuxillerySpine.getValue();
            if (!(auxspine && auxspine->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())))
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "No auxiliary spine linked."));
            std::vector<std::string> auxsubedge = AuxillerySpine.getSubValues();

            const Part::TopoShape& auxshape =
                static_cast<Part::Feature*>(auxspine)->Shape.getValue();
            buildPipePath(auxshape, auxsubedge, auxpath);
            auxpath.Move(invObjLoc);
        }

        // build up multisections
        auto multisections = Sections.getSubListValues();
        std::vector<std::vector<TopoDS_Shape>> wiresections;

        size_t numWires = addWiresToWireSections(profileShape, wiresections);
        if (numWires == 0) {
            // profileShape had no wires so only other valid option is single point section
            TopExp_Explorer ex;
            size_t i = 0;
            for (ex.Init(profileShape, TopAbs_VERTEX); ex.More(); ex.Next(), ++i)
                profilePoint = ex.Current();
            if (i > 1)
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                    "Pipe: Only one isolated point is needed if using a sketch with isolated "
                    "points for section"));
        }

        if (!profilePoint.IsNull() && (Transformation.getValue() != 1 || multisections.empty()))
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                "Pipe: At least one section is needed when using a single point for profile"));

        // maybe we need a scaling law
        Handle(Law_Function) scalinglaw;

        bool isLastSectionVertex = false;

        // see if we shall use multiple sections
        if (Transformation.getValue() == 1) {
            // TODO: we need to order the sections to prevent occ from crashing,
            // as makepipeshell connects the sections in the order of adding
            for (auto& subSet : multisections) {
                if (!subSet.first->isDerivedFrom(Part::Feature::getClassTypeId()))
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                        "Pipe: All sections need to be part features"));

                // if the section is an object's face then take just the face
                TopoDS_Shape shape = getSectionShape(subSet.first, subSet.second);
                if (shape.IsNull())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                        "Pipe: Could not obtain section shape"));

                size_t nWiresAdded = addWiresToWireSections(shape, wiresections);
                if (nWiresAdded == 0) {
                    TopExp_Explorer ex;
                    size_t i = 0;
                    for (ex.Init(shape, TopAbs_VERTEX); ex.More(); ex.Next(), ++i) {
                        if (isLastSectionVertex)
                            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                                "Pipe: Only the profile and last section can be vertices"));
                        isLastSectionVertex = true;
                        for (auto& wires : wiresections)
                            wires.push_back(ex.Current());
                    }
                }

                if (!isLastSectionVertex && nWiresAdded < wiresections.size())
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                        "Multisections need to have the same amount of inner wires as the base "
                        "section"));
            }
        }
        /*//build the law functions instead
        else if (Transformation.getValue() == 2) {
            if (ScalingData.getValues().size()<1)
                return new App::DocumentObjectExecReturn("No valid data given for linear scaling mode");

            Handle(Law_Linear) lin = new Law_Linear();
            lin->Set(0, 1, 1, ScalingData[0].x);

            scalinglaw = lin;
        }
        else if (Transformation.getValue() == 3) {
            if (ScalingData.getValues().size()<1)
                return new App::DocumentObjectExecReturn("No valid data given for S-shape scaling mode");

            Handle(Law_S) s = new Law_S();
            s->Set(0, 1, ScalingData[0].y, 1, ScalingData[0].x, ScalingData[0].z);

            scalinglaw = s;
        }*/

        // Verify that path is not a null shape
        if (path.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception", "Path must not be a null shape"));

        // build all shells
        std::vector<TopoDS_Shape> shells;

        TopoDS_Shape copyProfilePoint(profilePoint);
        if (!profilePoint.IsNull())
            copyProfilePoint.Move(invObjLoc);

        std::vector<TopoDS_Wire> frontwires, backwires;
        for (auto& wires : wiresections) {
            BRepOffsetAPI_MakePipeShell mkPS(TopoDS::Wire(path));
            setupAlgorithm(mkPS, auxpath);

            if (!scalinglaw) {
                if (!profilePoint.IsNull())
                    mkPS.Add(copyProfilePoint);

                for (auto& wire : wires) {
                    wire.Move(invObjLoc);
                    mkPS.Add(wire);
                }
            }
            else {
                if (!profilePoint.IsNull())
                    mkPS.SetLaw(copyProfilePoint, scalinglaw);

                for (auto& wire : wires)  {
                    wire.Move(invObjLoc);
                    mkPS.SetLaw(wire, scalinglaw);
                }
            }

            if (!mkPS.IsReady())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Pipe could not be built"));

            shells.push_back(mkPS.Shape());

            if (!mkPS.Shape().Closed()) {
                // shell is not closed - use simulate to get the end wires
                TopTools_ListOfShape sim;
                mkPS.Simulate(2, sim);

                // Note that while we call them front and back, these sections
                // appear to correspond to the front or back of the path. When one
                // or both ends of the pipe are points, one or both of these wires
                // (and eventually faces) will be null.
                frontwires.push_back(TopoDS::Wire(sim.First()));
                backwires.push_back(TopoDS::Wire(sim.Last()));
            }
        }

        BRepBuilderAPI_MakeSolid mkSolid;

        if (!frontwires.empty() || !backwires.empty()) {
            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());

            // build the end faces, sew the shell and build the final solid
            if (!frontwires.empty()) {
                TopoDS_Shape front = Part::FaceMakerCheese::makeFace(frontwires);
                sewer.Add(front);
            }
            if (!backwires.empty()) {
                TopoDS_Shape back  = Part::FaceMakerCheese::makeFace(backwires);
                sewer.Add(back);
            }
            for (TopoDS_Shape& s : shells)
                sewer.Add(s);

            sewer.Perform();
            mkSolid.Add(TopoDS::Shell(sewer.SewedShape()));
        } else {
            // shells are already closed - add them directly
            for (TopoDS_Shape& s : shells) {
                mkSolid.Add(TopoDS::Shell(s));
            }
        }

        if (!mkSolid.IsDone())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result is not a solid"));

        TopoDS_Shape result = mkSolid.Shape();
        BRepClass3d_SolidClassifier SC(result);
        SC.PerformInfinitePoint(Precision::Confusion());
        if (SC.State() == TopAbs_IN) {
            result.Reverse();
        }

        //result.Move(invObjLoc);
        AddSubShape.setValue(result);

        if (base.IsNull()) {
            if (getAddSubType() == FeatureAddSub::Subtractive)
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Pipe: There is nothing to subtract from"));

            result = refineShapeIfActive(result);
            Shape.setValue(getSolid(result));
            return App::DocumentObject::StdReturn;
        }

        if (getAddSubType() == FeatureAddSub::Additive) {

            BRepAlgoAPI_Fuse mkFuse(base, result);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Adding the pipe failed"));
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                    "Result has multiple solids: that is not currently supported."));
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if (getAddSubType() == FeatureAddSub::Subtractive) {

            BRepAlgoAPI_Cut mkCut(base, result);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Subtracting the pipe failed"));
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));

            int solidCount = countSolids(boolOp);
            if (solidCount > 1) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception",
                    "Result has multiple solids: that is not currently supported."));
            }

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "A fatal error occurred when making the pipe"));
    }
}

void Pipe::setupAlgorithm(BRepOffsetAPI_MakePipeShell& mkPipeShell, TopoDS_Shape& auxshape) {

    mkPipeShell.SetTolerance(Precision::Confusion());

    switch(Transition.getValue()) {
        case 0:
            mkPipeShell.SetTransitionMode(BRepBuilderAPI_Transformed);
            break;
        case 1:
            mkPipeShell.SetTransitionMode(BRepBuilderAPI_RightCorner);
            break;
        case 2:
            mkPipeShell.SetTransitionMode(BRepBuilderAPI_RoundCorner);
            break;
    }

    bool auxiliary = false;
    const Base::Vector3d& bVec = Binormal.getValue();
    switch(Mode.getValue()) {
        case 1:
            mkPipeShell.SetMode(gp_Ax2(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0)));
            break;
        case 2:
            mkPipeShell.SetMode(true);
            break;
        case 3:
            auxiliary = true;
            break;
        case 4:
            mkPipeShell.SetMode(gp_Dir(bVec.x, bVec.y, bVec.z));
            break;
    }

    if (auxiliary) {
        mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue());
        // mkPipeShell.SetMode(TopoDS::Wire(auxshape), AuxilleryCurvelinear.getValue(),
        // BRepFill_ContactOnBorder);
    }
}


void Pipe::getContinuousEdges(Part::TopoShape /*TopShape*/, std::vector<std::string>& /*SubNames*/)
{
    /*
    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeEdge;
    TopExp::MapShapesAndAncestors(TopShape.getShape(), TopAbs_EDGE, TopAbs_EDGE, mapEdgeEdge);
    TopExp::MapShapes(TopShape.getShape(), TopAbs_EDGE, mapOfEdges);

    Base::Console().Message("Initial edges:\n");
    for (int i=0; i<SubNames.size(); ++i)
        Base::Console().Message("Subname: %s\n", SubNames[i].c_str());

    unsigned int i = 0;
    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.compare(0, 4, "Edge") == 0) {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeEdge.FindFromKey(edge);

            if (los.Extent() != 2)
            {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            i++;
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin()+i);
        }
    }

    Base::Console().Message("Final edges:\n");
    for (int i=0; i<SubNames.size(); ++i)
        Base::Console().Message("Subname: %s\n", SubNames[i].c_str());
    */
}

void Pipe::buildPipePath(const Part::TopoShape& shape, const std::vector<std::string>& subedge,
                         TopoDS_Shape& path)
{
    if (!shape.getShape().IsNull()) {
        try {
            if (!subedge.empty()) {
                //if (SpineTangent.getValue())
                    //getContinuousEdges(shape, subedge);

                BRepBuilderAPI_MakeWire mkWire;
                for (const auto & it : subedge) {
                    TopoDS_Shape subshape = shape.getSubShape(it.c_str());
                    mkWire.Add(TopoDS::Edge(subshape));
                }
                path = mkWire.Wire();
            }
            else if (shape.getShape().ShapeType() == TopAbs_EDGE) {
                path = shape.getShape();
            }
            else if (shape.getShape().ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeWire mkWire(TopoDS::Wire(shape.getShape()));
                path = mkWire.Wire();
            }
            else if (shape.getShape().ShapeType() == TopAbs_COMPOUND) {
                TopoDS_Iterator it(shape.getShape());
                for (; it.More(); it.Next()) {
                    if (it.Value().IsNull())
                        throw Base::ValueError(QT_TRANSLATE_NOOP("Exception", "Invalid element in spine."));
                    if ((it.Value().ShapeType() != TopAbs_EDGE) &&
                        (it.Value().ShapeType() != TopAbs_WIRE)) {
                        throw Base::TypeError(QT_TRANSLATE_NOOP("Exception", "Element in spine is neither an edge nor a wire."));
                    }
                }

                Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
                Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
                for (TopExp_Explorer xp(shape.getShape(), TopAbs_EDGE); xp.More(); xp.Next())
                    hEdges->Append(xp.Current());

                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(
                    hEdges, Precision::Confusion(), Standard_True, hWires);
                int len = hWires->Length();
                if (len != 1)
                    throw Base::ValueError(QT_TRANSLATE_NOOP("Exception", "Spine is not connected."));
                path = hWires->Value(1);
            }
            else {
                throw Base::TypeError(QT_TRANSLATE_NOOP("Exception", "Spine is neither an edge nor a wire."));
            }
        }
        catch (Standard_Failure&) {
            throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Invalid spine."));
        }
    }
}

PROPERTY_SOURCE(PartDesign::AdditivePipe, PartDesign::Pipe)
AdditivePipe::AdditivePipe() {
    addSubType = Additive;
}

PROPERTY_SOURCE(PartDesign::SubtractivePipe, PartDesign::Pipe)
SubtractivePipe::SubtractivePipe() {
    addSubType = Subtractive;
}

void Pipe::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName,
                                     App::Property* prop)
{
    // property Sections had the App::PropertyLinkList and was changed to App::PropertyXLinkSubList
    if (prop == &Sections && strcmp(TypeName, "App::PropertyLinkList") == 0) {
        Sections.upgrade(reader, TypeName);
    }
    else {
        ProfileBased::handleChangedPropertyType(reader, TypeName, prop);
    }
}
