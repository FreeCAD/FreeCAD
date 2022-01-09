/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cmath>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepGProp.hxx>
# include <BRepLib_FindSurface.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <gp_Pln.hxx>
# include <gp_Trsf.hxx>
# include <GProp_GProps.hxx>
# include <Precision.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include "FeatureExtrusion.h"
#include <App/Application.h>
#include <Base/Tools.h>
#include <Base/Exception.h>
#include "Part2DObject.h"

using namespace Part;

PROPERTY_SOURCE(Part::Extrusion, Part::Feature)

const char* Extrusion::eDirModeStrings[] = {
    "Custom",
    "Edge",
    "Normal",
    NULL };

Extrusion::Extrusion()
{
    ADD_PROPERTY_TYPE(Base, (0), "Extrude", App::Prop_None, "Shape to extrude");
    ADD_PROPERTY_TYPE(Dir, (Base::Vector3d(0.0, 0.0, 1.0)), "Extrude", App::Prop_None, "Direction of extrusion (also magnitude, if both lengths are zero).");
    ADD_PROPERTY_TYPE(DirMode, (dmCustom), "Extrude", App::Prop_None, "Sets, how Dir is updated.");
    DirMode.setEnums(eDirModeStrings);
    ADD_PROPERTY_TYPE(DirLink, (nullptr), "Extrude", App::Prop_None, "Link to edge defining extrusion direction.");
    ADD_PROPERTY_TYPE(LengthFwd, (0.0), "Extrude", App::Prop_None, "Length of extrusion along direction. If both LengthFwd and LengthRev are zero, magnitude of Dir is used.");
    ADD_PROPERTY_TYPE(LengthRev, (0.0), "Extrude", App::Prop_None, "Length of additional extrusion, against direction.");
    ADD_PROPERTY_TYPE(Solid, (false), "Extrude", App::Prop_None, "If true, extruding a wire yields a solid. If false, a shell.");
    ADD_PROPERTY_TYPE(Reversed, (false), "Extrude", App::Prop_None, "Set to true to swap the direction of extrusion.");
    ADD_PROPERTY_TYPE(Symmetric, (false), "Extrude", App::Prop_None, "If true, extrusion is done in both directions to a total of LengthFwd. LengthRev is ignored.");
    ADD_PROPERTY_TYPE(TaperAngle, (0.0), "Extrude", App::Prop_None, "Sets the angle of slope (draft) to apply to the sides. The angle is for outward taper; negative value yields inward tapering.");
    ADD_PROPERTY_TYPE(TaperAngleRev, (0.0), "Extrude", App::Prop_None, "Taper angle of reverse part of extrusion.");
    ADD_PROPERTY_TYPE(FaceMakerClass, ("Part::FaceMakerExtrusion"), "Extrude", App::Prop_None, "If Solid is true, this sets the facemaker class to use when converting wires to faces. Otherwise, ignored."); //default for old documents. See setupObject for default for new extrusions.
}

short Extrusion::mustExecute() const
{
    if (Base.isTouched() ||
        Dir.isTouched() ||
        DirMode.isTouched() ||
        DirLink.isTouched() ||
        LengthFwd.isTouched() ||
        LengthRev.isTouched() ||
        Solid.isTouched() ||
        Reversed.isTouched() ||
        Symmetric.isTouched() ||
        TaperAngle.isTouched() ||
        TaperAngleRev.isTouched() ||
        FaceMakerClass.isTouched())
        return 1;
    return 0;
}

bool Extrusion::fetchAxisLink(const App::PropertyLinkSub& axisLink, Base::Vector3d& basepoint, Base::Vector3d& dir)
{
    if (!axisLink.getValue())
        return false;

    auto linked = axisLink.getValue();

    TopoDS_Shape axEdge;
    if (axisLink.getSubValues().size() > 0 && axisLink.getSubValues()[0].length() > 0) {
        axEdge = Feature::getTopoShape(linked).getSubShape(axisLink.getSubValues()[0].c_str());
    }
    else {
        axEdge = Feature::getShape(linked);
    }

    if (axEdge.IsNull())
        throw Base::ValueError("DirLink shape is null");
    if (axEdge.ShapeType() != TopAbs_EDGE)
        throw Base::TypeError("DirLink shape is not an edge");

    BRepAdaptor_Curve crv(TopoDS::Edge(axEdge));
    gp_Pnt startpoint;
    gp_Pnt endpoint;
    if (crv.GetType() == GeomAbs_Line) {
        startpoint = crv.Value(crv.FirstParameter());
        endpoint = crv.Value(crv.LastParameter());
        if (axEdge.Orientation() == TopAbs_REVERSED)
            std::swap(startpoint, endpoint);
    }
    else {
        throw Base::TypeError("DirLink edge is not a line.");
    }
    basepoint.Set(startpoint.X(), startpoint.Y(), startpoint.Z());
    gp_Vec vec = gp_Vec(startpoint, endpoint);
    dir.Set(vec.X(), vec.Y(), vec.Z());
    return true;
}

Extrusion::ExtrusionParameters Extrusion::computeFinalParameters()
{
    Extrusion::ExtrusionParameters result;
    Base::Vector3d dir;
    switch (this->DirMode.getValue()) {
    case dmCustom:
        dir = this->Dir.getValue();
        break;
    case dmEdge: {
        bool fetched;
        Base::Vector3d base;
        fetched = fetchAxisLink(this->DirLink, base, dir);
        if (!fetched)
            throw Base::ValueError("DirMode is set to use edge, but no edge is linked.");
        this->Dir.setValue(dir);
    } break;
    case dmNormal:
        dir = calculateShapeNormal(this->Base);
        this->Dir.setValue(dir);
        break;
    default:
        throw Base::ValueError("Unexpected enum value");
    }
    if (dir.Length() < Precision::Confusion())
        throw Base::ValueError("Direction is zero-length");
    result.dir = gp_Dir(dir.x, dir.y, dir.z);
    if (this->Reversed.getValue())
        result.dir.Reverse();

    result.lengthFwd = this->LengthFwd.getValue();
    result.lengthRev = this->LengthRev.getValue();
    if (fabs(result.lengthFwd) < Precision::Confusion()
        && fabs(result.lengthRev) < Precision::Confusion()) {
        result.lengthFwd = dir.Length();
    }

    if (this->Symmetric.getValue()) {
        result.lengthRev = result.lengthFwd * 0.5;
        result.lengthFwd = result.lengthFwd * 0.5;
    }

    if (fabs(result.lengthFwd + result.lengthRev) < Precision::Confusion())
        throw Base::ValueError("Total length of extrusion is zero.");

    result.solid = this->Solid.getValue();

    result.taperAngleFwd = this->TaperAngle.getValue() * M_PI / 180.0;
    if (fabs(result.taperAngleFwd) > M_PI * 0.5 - Precision::Angular())
        throw Base::ValueError("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");
    result.taperAngleRev = this->TaperAngleRev.getValue() * M_PI / 180.0;
    if (fabs(result.taperAngleRev) > M_PI * 0.5 - Precision::Angular())
        throw Base::ValueError("Magnitude of taper angle matches or exceeds 90 degrees. That is too much.");

    result.faceMakerClass = this->FaceMakerClass.getValue();

    return result;
}

Base::Vector3d Extrusion::calculateShapeNormal(const App::PropertyLink& shapeLink)
{
    App::DocumentObject* docobj = 0;
    Base::Matrix4D mat;
    TopoDS_Shape sh = Feature::getShape(shapeLink.getValue(), 0, false, &mat, &docobj);

    if (!docobj)
        throw Base::ValueError("calculateShapeNormal: link is empty");

    //special case for sketches and the like: no matter what shape they have, use their local Z axis.
    if (docobj->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        Base::Vector3d OZ(0.0, 0.0, 1.0);
        Base::Vector3d result;
        Base::Rotation(mat).multVec(OZ, result);
        return result;
    }

    if (sh.IsNull())
        throw NullShapeException("calculateShapeNormal: link points to a valid object, but its shape is null.");

    //find plane
    BRepLib_FindSurface planeFinder(sh, -1, /*OnlyPlane=*/true);
    if (!planeFinder.Found())
        throw Base::ValueError("Can't find normal direction, because the shape is not on a plane.");

    //find plane normal and return result.
    GeomAdaptor_Surface surf(planeFinder.Surface());
    gp_Dir normal = surf.Plane().Axis().Direction();

    //now we know the plane. But if there are faces, the
    //plane normal direction is not dependent on face orientation (because findPlane only uses edges).
    //let's fix that.
    TopExp_Explorer ex(sh, TopAbs_FACE);
    if (ex.More()) {
        BRepAdaptor_Surface surf(TopoDS::Face(ex.Current()));
        normal = surf.Plane().Axis().Direction();
        if (ex.Current().Orientation() == TopAbs_REVERSED) {
            normal.Reverse();
        }
    }

    return Base::Vector3d(normal.X(), normal.Y(), normal.Z());
}

TopoShape Extrusion::extrudeShape(const TopoShape& source, const Extrusion::ExtrusionParameters& params)
{
    TopoDS_Shape result;
    gp_Vec vec = gp_Vec(params.dir).Multiplied(params.lengthFwd + params.lengthRev);//total vector of extrusion

    if (std::fabs(params.taperAngleFwd) >= Precision::Angular() ||
        std::fabs(params.taperAngleRev) >= Precision::Angular()) {
        //Tapered extrusion!
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        TopoDS_Shape myShape = source.getShape();
        if (myShape.IsNull())
            Standard_Failure::Raise("Cannot extrude empty shape");
        // #0000910: Circles Extrude Only Surfaces, thus use BRepBuilderAPI_Copy
        myShape = BRepBuilderAPI_Copy(myShape).Shape();

        std::list<TopoDS_Shape> drafts;
        makeDraft(params, myShape, drafts);
        if (drafts.empty()) {
            Standard_Failure::Raise("Drafting shape failed");
        }
        else if (drafts.size() == 1) {
            result = drafts.front();
        }
        else {
            TopoDS_Compound comp;
            BRep_Builder builder;
            builder.MakeCompound(comp);
            for (std::list<TopoDS_Shape>::iterator it = drafts.begin(); it != drafts.end(); ++it)
                builder.Add(comp, *it);
            result = comp;
        }
    }
    else {
        //Regular (non-tapered) extrusion!
        TopoDS_Shape myShape = source.getShape();
        if (myShape.IsNull())
            Standard_Failure::Raise("Cannot extrude empty shape");

        // #0000910: Circles Extrude Only Surfaces, thus use BRepBuilderAPI_Copy
        myShape = BRepBuilderAPI_Copy(myShape).Shape();

        //apply reverse part of extrusion by shifting the source shape
        if (fabs(params.lengthRev) > Precision::Confusion()) {
            gp_Trsf mov;
            mov.SetTranslation(gp_Vec(params.dir) * (-params.lengthRev));
            TopLoc_Location loc(mov);
            myShape.Move(loc);
        }

        //make faces from wires
        if (params.solid) {
            //test if we need to make faces from wires. If there are faces - we don't.
            TopExp_Explorer xp(myShape, TopAbs_FACE);
            if (xp.More()) {
                //source shape has faces. Just extrude as-is.
            }
            else {
                std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(params.faceMakerClass.c_str());

                if (myShape.ShapeType() == TopAbs_COMPOUND)
                    mkFace->useCompound(TopoDS::Compound(myShape));
                else
                    mkFace->addShape(myShape);
                mkFace->Build();
                myShape = mkFace->Shape();
            }
        }

        //extrude!
        BRepPrimAPI_MakePrism mkPrism(myShape, vec);
        result = mkPrism.Shape();
    }

    if (result.IsNull())
        throw NullShapeException("Result of extrusion is null shape.");
    return TopoShape(result);
}

App::DocumentObjectExecReturn* Extrusion::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    try {
        Extrusion::ExtrusionParameters params = computeFinalParameters();
        TopoShape result = extrudeShape(Feature::getShape(link), params);
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Extrusion::makeDraft(const ExtrusionParameters& params, const TopoDS_Shape& shape, std::list<TopoDS_Shape>& drafts)
{
    std::vector<std::vector<TopoDS_Shape>> wiresections;

    auto addWiresToWireSections =
        [&shape](std::vector<std::vector<TopoDS_Shape>>& wiresections) -> size_t {
        TopExp_Explorer ex;
        size_t i = 0;
        for (ex.Init(shape, TopAbs_WIRE); ex.More(); ex.Next(), ++i) {
            wiresections.push_back(std::vector<TopoDS_Shape>());
            wiresections[i].push_back(TopoDS::Wire(ex.Current()));
        }
        return i;
    };

    double distanceFwd = tan(params.taperAngleFwd)*params.lengthFwd;
    double distanceRev = tan(params.taperAngleRev)*params.lengthRev;
    gp_Vec vecFwd = gp_Vec(params.dir) * params.lengthFwd;
    gp_Vec vecRev = gp_Vec(params.dir.Reversed()) * params.lengthRev;

    bool bFwd = fabs(params.lengthFwd) > Precision::Confusion();
    bool bRev = fabs(params.lengthRev) > Precision::Confusion();
    // only if there is a 2nd direction and the negated angle is not equal to the first one
    // we don't need to add the source shape as loft section
    bool bMid = !bFwd || !bRev || -1.0 * params.taperAngleFwd != params.taperAngleRev;

    if (shape.IsNull())
        Standard_Failure::Raise("Not a valid shape");

    // store all wires of the shape into an array
    size_t numWires = addWiresToWireSections(wiresections);
    if (numWires == 0)
        Standard_Failure::Raise("Extrusion: Input must not only consist if a vertex");

    // to store the sections for the loft
    std::list<TopoDS_Wire> list_of_sections;
    std::vector<std::vector<TopoDS_Shape>> extrusionSections;

    // we need for all found wires an offset copy of them
    // we store them in an array
    TopoDS_Wire offsetWire;
    for (auto& wire : wiresections)
        extrusionSections.push_back(std::vector<TopoDS_Shape>());
    size_t rows = 0;
    int numEdges = 0;
    int numInnerWires = 0;

    // we need to find out what are ounter wires and what are inner ones
    // methods like checking the center of mass etc. don't help us here
    // therefore build a prism with every wire, then subtract every prism the others
    // if the subtraction changes the initial prism, the subtracted prism has an inner wire
    std::vector<TopoDS_Shape> resultPrisms;
    std::vector<bool> isInnerWire;
    TopoDS_Shape singlePrism;
    // first build the prisms
    for (auto& wireVector : wiresections) {
        for (auto& singleWire : wireVector) {
            BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(singleWire));
            auto tempFace = mkFace.Shape();
            BRepPrimAPI_MakePrism mkPrism(tempFace, vecFwd);
            if(!mkPrism.IsDone())
                Standard_Failure::Raise("Extrusion: Generating prism failed");
            singlePrism = mkPrism.Shape();
            resultPrisms.push_back(singlePrism);
        }
    }
    // now subtract them
    // if the moment of inertia changes, we have an inner wire
    GProp_GProps tempProperties;
    Standard_Real momentOfInertiaInitial;
    Standard_Real momentOfInertiaFinal;
    std::vector<bool>::iterator itInner = isInnerWire.begin();
    bool isInner;
    for (auto itOuter = resultPrisms.begin(); itOuter != resultPrisms.end(); ++itOuter) {
        isInner = false;
        for (auto itInner = resultPrisms.begin(); itInner != resultPrisms.end(); ++itInner) {
            if (itOuter == itInner)
                continue;
            // get center of mass of first shape
            BRepGProp::VolumeProperties(*itInner, tempProperties);
            momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), params.dir));
            BRepAlgoAPI_Cut mkCut(*itInner, *itOuter);
            if (!mkCut.IsDone())
                Standard_Failure::Raise("Extrusion: Cut out failed");
            BRepGProp::VolumeProperties(mkCut.Shape(), tempProperties);
            momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), params.dir));
            // if the whole shape was cut away the resulting shape is not Null but its MomentOfInertia is 0.0
            // therefore we have an inner wire if the MomentOfInertia is not zero and changed
            if ((momentOfInertiaInitial != momentOfInertiaFinal)
                && (momentOfInertiaFinal > Precision::Confusion())) {
                isInner = true;
                ++numInnerWires;
                break;
            }
        }
        isInnerWire.push_back(isInner);
    }
    // if all wires are inner ones, we take the first one and issue a warning
    if ((numWires - numInnerWires) == 0) {
        isInnerWire[0] = false;
        Base::Console().Warning("Extrusion: could not determine what structure is the outer one.\n\
                                 The first input one will now be taken as outer one.\n");
    }

    // at first create an offset wire for reversed part of extrusion
    if (bRev) {
        // this offsetWire must be the first loft section
        // create an offset for all source wires
        rows = 0;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                // count number of edges
                numEdges = 0;
                TopExp_Explorer xp(singleWire, TopAbs_EDGE);
                while (xp.More()) {
                    numEdges++;
                    xp.Next();
                }
                // create an offset copy of the wire
                if (!isInnerWire[rows]) {
                    // this is an outer wire
                    createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, numEdges, true, offsetWire);
                }
                else {
                    // inner wires must be reversed and get the negated offset
                    createTaperedPrismOffset(TopoDS::Wire(singleWire.Reversed()), vecRev, -distanceRev, numEdges, true, offsetWire);
                }
                if (offsetWire.IsNull())
                    return;
                extrusionSections[rows].push_back(offsetWire);
            }
            ++rows;
        }
    }

    // Add the source wire as middle section. It is important to add them after the reversed part. 
    if (bMid) {
        // transfer all source wires as they are to the array from which we build the shells
        rows = 0;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                extrusionSections[rows].push_back(singleWire);
            }
            rows++;
        }
    }

    // finally add the forward extrusion offset wire
    if (bFwd) {
        // this offsetWire must be the last loft section
        rows = 0;
        for (auto& wireVector : wiresections) {
            for (auto& singleWire : wireVector) {
                // count number of edges
                numEdges = 0;
                TopExp_Explorer xp(singleWire, TopAbs_EDGE);
                while (xp.More()) {
                    numEdges++;
                    xp.Next();
                }
                // create an offset copy of the wire
                if (!isInnerWire[rows]) {
                    // this is an outer wire
                    createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, numEdges, false, offsetWire);
                }
                else {
                    // inner wires must be reversed and get the negated offset
                    createTaperedPrismOffset(TopoDS::Wire(singleWire.Reversed()), vecFwd, -distanceFwd, numEdges, false, offsetWire);
                }
                if (offsetWire.IsNull())
                    return;
                extrusionSections[rows].push_back(offsetWire);
            }
            ++rows;
        }
    }

    try {
        // build all shells
        std::vector<TopoDS_Shape> shells;

        for (auto& wires : extrusionSections) {
            BRepOffsetAPI_ThruSections mkTS(params.solid, /*ruled=*/Standard_True, Precision::Confusion());

            for (auto& singleWire : wires) {
                if (singleWire.ShapeType() == TopAbs_VERTEX)
                    mkTS.AddVertex(TopoDS::Vertex(singleWire));
                else
                    mkTS.AddWire(TopoDS::Wire(singleWire));
            }
            mkTS.Build();
            if (!mkTS.IsDone())
                Standard_Failure::Raise("Extrusion: Loft could not be built");
            
            shells.push_back(mkTS.Shape());
        }

        if (params.solid) {
            // we only need to cut if we have inner wires
            if (numInnerWires > 0) {
                TopoDS_Shape result;
                BRep_Builder builder;
                TopoDS_Compound compOuter;
                TopoDS_Shape outerCutShape;
                // when there is more than one outer wire make a compound of the outer shells
                rows = 0;
                if ((numWires - numInnerWires) > 1) {
                    builder.MakeCompound(compOuter);
                    for (auto it = shells.begin(); it != shells.end(); ++it) {
                        if (!isInnerWire[rows])
                            builder.Add(compOuter, *it);
                        ++rows;
                    }
                    outerCutShape = compOuter;
                }
                else { // take the shell directly without a compound
                    for (auto it = shells.begin(); it != shells.end(); ++it) {
                        if (!isInnerWire[rows]) {
                            outerCutShape = *it;
                            break;
                        }
                        ++rows;
                    }
                }
                // now the compound of the inner, no matter if we only have one inner
                TopoDS_Compound compInner;
                builder.MakeCompound(compInner);
                rows = 0;
                for (auto it = shells.begin(); it != shells.end(); ++it) {
                    if (isInnerWire[rows])
                        builder.Add(compInner, *it);
                    ++rows;
                }
                // cut the inner from the outer
                BRepAlgoAPI_Cut mkCutFinal(outerCutShape, compInner);
                if (!mkCutFinal.IsDone())
                    Standard_Failure::Raise("Extrusion: Cut of inner structures failed");
                result = mkCutFinal.Shape();
                // de-assemble the result to return every solid independently
                for (TopoDS_Iterator anExp(result); anExp.More(); anExp.Next()) {
                    if (anExp.Value().ShapeType() == TopAbs_SOLID)
                        drafts.push_back(anExp.Value());
                }
                if (drafts.empty()) // should never happen, we already checked the success of the cut of solids
                    Standard_Failure::Raise("Extrusion: The extrusion result is no solid shape");
            } else
                // we already have the results
                for (auto it = shells.begin(); it != shells.end(); ++it)
                    drafts.push_back(*it);
        }
        else { // no solid
            BRepBuilderAPI_Sewing sewer;
            sewer.SetTolerance(Precision::Confusion());
            for (TopoDS_Shape& s : shells)
                sewer.Add(s);
            sewer.Perform();
            drafts.push_back(sewer.SewedShape());
        } 
    }
    catch (Standard_Failure& e) {
        throw Base::RuntimeError(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        throw Base::RuntimeError(e.what());
    }
    catch (...) {
        throw Base::CADKernelError("Extrusion: A fatal error occurred when making the loft");
    }
}

void Extrusion::createTaperedPrismOffset(TopoDS_Wire sourceWire,
                                         const gp_Vec& translation,
                                         double offset,
                                         int numEdges,
                                         bool isSecond,
                                         TopoDS_Wire& result) {

    // if the wire consists of a single edge which has applied a placement
    // then this placement must be reset because otherwise the
    // BRepOffsetAPI_MakeOffset shows weird behaviour by applying the placement
    gp_Trsf tempTransform;
    tempTransform.SetTranslation(translation);
    TopLoc_Location loc(tempTransform);
    TopoDS_Wire movedSourceWire = TopoDS::Wire(sourceWire.Moved(loc));

    TopoDS_Shape offsetShape;
    if (fabs(offset) > Precision::Confusion()) {
        TopLoc_Location edgeLocation;
        if (numEdges == 1) {
            // create a new wire from the input wire to determine its location
            // to reset the location after the offet operation
            BRepBuilderAPI_MakeWire mkWire;
            TopExp_Explorer xp(sourceWire, TopAbs_EDGE);
            while (xp.More()) {
                TopoDS_Edge edge = TopoDS::Edge(xp.Current());
                edgeLocation = edge.Location();
                edge.Location(TopLoc_Location());
                mkWire.Add(edge);
                xp.Next();
            }
            movedSourceWire = mkWire.Wire();
        }
        // create the offset shape
        BRepOffsetAPI_MakeOffset mkOffset;
        mkOffset.Init(GeomAbs_Arc);
        mkOffset.Init(GeomAbs_Intersection);
        mkOffset.AddWire(movedSourceWire);
        try {
            mkOffset.Perform(offset);
            offsetShape = mkOffset.Shape();
        }
        catch (const Base::Exception& e) {
            throw Base::RuntimeError(e.what());
            result = TopoDS_Wire();
        }
        if (!mkOffset.IsDone()) {
            Standard_Failure::Raise("Extrusion: Offset could not be created");
            result = TopoDS_Wire();
        }
        if (numEdges == 1) {
            // we need to move the offset wire first back to its original position
            offsetShape.Move(edgeLocation);
            // now apply the translation
            offsetShape = offsetShape.Moved(loc);
        }
    }
    else {
        offsetShape = movedSourceWire;
    }
    if (offsetShape.IsNull()) {
        if (isSecond)
            Base::Console().Error("Extrusion: end face of tapered against extrusion is empty\n");
        else
            Base::Console().Error("Extrusion: end face of tapered along extrusion is empty\n");
    }
    // assure we return a wire and no edge
    TopAbs_ShapeEnum type = offsetShape.ShapeType();
    if (type == TopAbs_WIRE) {
        result = TopoDS::Wire(offsetShape);
    }
    else if (type == TopAbs_EDGE) {
        BRepBuilderAPI_MakeWire mkWire2(TopoDS::Edge(offsetShape));
        result = mkWire2.Wire();
    }
    else {
        // this happens usually if type == TopAbs_COMPOUND and means the angle is too small
        // FIXME: since this is a common mistake users will quickly do, issue a warning dialog
        // Standard_Failure::Raise or App::DocumentObjectExecReturn will not output the message to the user
        result = TopoDS_Wire();
        if (isSecond)
            Base::Console().Error("Extrusion: type of against extrusion end face is not supported.\n" \
                "This means most probably that the against taper angle is too large.\n");
        else
            Base::Console().Error("Extrusion: type of along extrusion is not supported.\n" \
                "This means most probably that the along taper angle is too large.\n");
    }
}

//----------------------------------------------------------------

TYPESYSTEM_SOURCE(Part::FaceMakerExtrusion, Part::FaceMakerCheese)

std::string FaceMakerExtrusion::getUserFriendlyName() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker", "Part Extrude facemaker"));
}

std::string FaceMakerExtrusion::getBriefExplanation() const
{
    return std::string(QT_TRANSLATE_NOOP("Part_FaceMaker", "Supports making faces with holes, does not support nesting."));
}

void FaceMakerExtrusion::Build()
{
    this->NotDone();
    this->myGenerated.Clear();
    this->myShapesToReturn.clear();
    this->myShape = TopoDS_Shape();
    TopoDS_Shape inputShape;
    if (mySourceShapes.empty())
        throw Base::ValueError("No input shapes!");
    if (mySourceShapes.size() == 1) {
        inputShape = mySourceShapes[0];
    }
    else {
        TopoDS_Builder builder;
        TopoDS_Compound cmp;
        builder.MakeCompound(cmp);
        for (const TopoDS_Shape& sh : mySourceShapes) {
            builder.Add(cmp, sh);
        }
        inputShape = cmp;
    }

    std::vector<TopoDS_Wire> wires;
    TopTools_IndexedMapOfShape mapOfWires;
    TopExp::MapShapes(inputShape, TopAbs_WIRE, mapOfWires);

    // if there are no wires then check also for edges
    if (mapOfWires.IsEmpty()) {
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(inputShape, TopAbs_EDGE, mapOfEdges);
        for (int i = 1; i <= mapOfEdges.Extent(); i++) {
            BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(mapOfEdges.FindKey(i)));
            wires.push_back(mkWire.Wire());
        }
    }
    else {
        wires.reserve(mapOfWires.Extent());
        for (int i = 1; i <= mapOfWires.Extent(); i++) {
            wires.push_back(TopoDS::Wire(mapOfWires.FindKey(i)));
        }
    }

    if (!wires.empty()) {
        //try {
        TopoDS_Shape res = FaceMakerCheese::makeFace(wires);
        if (!res.IsNull())
            this->myShape = res;
        //}
        //catch (...) {

        //}
    }

    this->Done();

}

void Part::Extrusion::setupObject()
{
    Part::Feature::setupObject();
    this->FaceMakerClass.setValue("Part::FaceMakerBullseye"); //default for newly created features
}
