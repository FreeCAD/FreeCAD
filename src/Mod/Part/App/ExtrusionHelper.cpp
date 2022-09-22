/***************************************************************************
 *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepGProp.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <gp_Ax1.hxx>
# include <gp_Dir.hxx>
# include <gp_Trsf.hxx>
# include <gp_Pln.hxx>
# include <GProp_GProps.hxx>
# include <Precision.hxx>
# include <ShapeFix_Wire.hxx> 
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>

#include "BRepOffsetAPI_MakeOffsetFix.h"
#include "ExtrusionHelper.h"
#include "FeatureExtrusion.h"
#include "Geometry.h"


using namespace Part;

ExtrusionHelper::ExtrusionHelper()
{
}

void ExtrusionHelper::makeDraft(const TopoDS_Shape& shape,
                                const gp_Dir& direction,
                                const double LengthFwd,
                                const double LengthRev,
                                const double AngleFwd,
                                const double AngleRev,
                                bool isSolid,
                                std::list<TopoDS_Shape>& drafts,
                                bool isPartDesign)
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

    double distanceFwd = tan(AngleFwd) * LengthFwd;
    double distanceRev = tan(AngleRev) * LengthRev;
    gp_Vec vecFwd = gp_Vec(direction) * LengthFwd;
    gp_Vec vecRev = gp_Vec(direction.Reversed()) * LengthRev;

    bool bFwd = fabs(LengthFwd) > Precision::Confusion();
    bool bRev = fabs(LengthRev) > Precision::Confusion();
    // only if there is a 2nd direction and the negated angle is equal to the first one
    // we can omit the source shape as loft section
    bool bMid = !bFwd || !bRev || -1.0 * AngleFwd != AngleRev;

    if (shape.IsNull())
        Standard_Failure::Raise("Not a valid shape");

    // store all wires of the shape into an array
    size_t numWires = addWiresToWireSections(wiresections);
    if (numWires == 0)
        Standard_Failure::Raise("Extrusion: Input must not only consist if a vertex");

    // to store the sections for the loft
    std::list<TopoDS_Wire> list_of_sections;

    // we need for all found wires an offset copy of them
    // we store them in an array
    TopoDS_Wire offsetWire;
    std::vector<std::vector<TopoDS_Shape>> extrusionSections(wiresections.size(), std::vector<TopoDS_Shape>());
    size_t rows = 0;
    int numEdges = 0;

    // We need to find out what are outer wires and what are inner ones
    // methods like checking the center of mass etc. don't help us here.
    // As solution we build a prism with every wire, then subtract every prism from each other.
    // If the moment of inertia changes by a subtraction, we have an inner wire prism.
    // 
    // first build the prisms
    std::vector<TopoDS_Shape> resultPrisms;
    TopoDS_Shape singlePrism;
    for (auto& wireVector : wiresections) {
        for (auto& singleWire : wireVector) {
            BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(singleWire));
            auto tempFace = mkFace.Shape();
            BRepPrimAPI_MakePrism mkPrism(tempFace, vecFwd);
            if (!mkPrism.IsDone())
                Standard_Failure::Raise("Extrusion: Generating prism failed");
            singlePrism = mkPrism.Shape();
            resultPrisms.push_back(singlePrism);
        }
    }
    // create an array with false to store later which wires are inner ones
    std::vector<bool> isInnerWire(resultPrisms.size(), false);
    std::vector<bool> checklist(resultPrisms.size(), true);
    // finally check reecursively for inner wires
    checkInnerWires(isInnerWire, direction, checklist, false, resultPrisms);

    // count the number of inner wires
    int numInnerWires = 0;
    for (auto isInner : isInnerWire) {
        if (isInner)
            ++numInnerWires;
    }

    // at first create offset wires for the reversed part of extrusion
    // it is important that these wires are the first loft section
    if (bRev) {
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
                    createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, true, offsetWire);
                }
                else {
                    // there is an OCC bug with single-edge wires (circles), see inside createTaperedPrismOffset
                    if (numEdges > 1 || !isPartDesign)
                        // inner wires must get the negated offset
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, -distanceRev, true, offsetWire);
                    else
                        // circles in PartDesign must not get the negated offset
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecRev, distanceRev, true, offsetWire);
                }
                if (offsetWire.IsNull())
                    return;
                extrusionSections[rows].push_back(offsetWire);
            }
            ++rows;
        }
    }

    // add the source wire as middle section
    // it is important to add them after the reversed part
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

    // finally add the forward extrusion offset wires
    // these wires must be the last loft section
    if (bFwd) {
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
                    createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, false, offsetWire);
                }
                else {
                    // there is an OCC bug with single-edge wires (circles), see inside createTaperedPrismOffset
                    if (numEdges > 1 || !isPartDesign)
                        // inner wires must get the negated offset
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, -distanceFwd, false, offsetWire);
                    else
                        // circles in PartDesign must not get the negated offset
                        createTaperedPrismOffset(TopoDS::Wire(singleWire), vecFwd, distanceFwd, false, offsetWire);
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
            BRepOffsetAPI_ThruSections mkTS(isSolid, /*ruled=*/Standard_True, Precision::Confusion());

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

        if (isSolid) {
            // we only need to cut if we have inner wires
            if (numInnerWires > 0) {
                // we take every outer wire prism and cut subsequently all inner wires prisms from it
                // every resulting shape is the final drafted extrusion shape
                GProp_GProps tempProperties;
                Standard_Real momentOfInertiaInitial;
                Standard_Real momentOfInertiaFinal;
                std::vector<bool>::iterator isInnerWireIterator = isInnerWire.begin();
                std::vector<bool>::iterator isInnerWireIteratorLoop;
                for (auto itOuter = shells.begin(); itOuter != shells.end(); ++itOuter) {
                    if (*isInnerWireIterator) {
                        ++isInnerWireIterator;
                        continue;
                    }
                    isInnerWireIteratorLoop = isInnerWire.begin();
                    for (auto itInner = shells.begin(); itInner != shells.end(); ++itInner) {
                        if (itOuter == itInner || !*isInnerWireIteratorLoop) {
                            ++isInnerWireIteratorLoop;
                            continue;
                        }
                        // get MomentOfInertia of first shape
                        BRepGProp::VolumeProperties(*itOuter, tempProperties);
                        momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
                        BRepAlgoAPI_Cut mkCut(*itOuter, *itInner);
                        if (!mkCut.IsDone())
                            Standard_Failure::Raise("Extrusion: Final cut out failed");
                        BRepGProp::VolumeProperties(mkCut.Shape(), tempProperties);
                        momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
                        // if the whole shape was cut away the resulting shape is not Null but its MomentOfInertia is 0.0
                        // therefore we have a valid cut if the MomentOfInertia is not zero and changed
                        if ((momentOfInertiaInitial != momentOfInertiaFinal)
                            && (momentOfInertiaFinal > Precision::Confusion())) {
                            // immediately update the outer shape since more inner wire prism might cut it
                            *itOuter = mkCut.Shape();
                        }
                        ++isInnerWireIteratorLoop;
                    }
                    drafts.push_back(*itOuter);
                    ++isInnerWireIterator;
                }
            }
            else
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

void ExtrusionHelper::checkInnerWires(std::vector<bool>& isInnerWire, const gp_Dir direction,
                                      std::vector<bool>&checklist, bool forInner, std::vector<TopoDS_Shape> prisms)
{
    // store the number of wires to be checked
    size_t numCheckWiresInitial = 0;
    for (auto checks : checklist) {
        if (checks)
            ++numCheckWiresInitial;
    }
    GProp_GProps tempProperties;
    Standard_Real momentOfInertiaInitial;
    Standard_Real momentOfInertiaFinal;
    size_t numCheckWires = 0;
    std::vector<bool>::iterator isInnerWireIterator = isInnerWire.begin();
    std::vector<bool>::iterator toCheckIterator = checklist.begin();
    // create an array with false used later to store what can be cancelled from the checklist
    std::vector<bool> toDisable(checklist.size(), false);
    int outer = -1;
    // we cut every prism to be checked from the other to be checked ones
    // if nothing happens, a prism can be cancelled from the checklist
    for (auto itOuter = prisms.begin(); itOuter != prisms.end(); ++itOuter) {
        ++outer;
        if (!*toCheckIterator) {
            ++isInnerWireIterator;
            ++toCheckIterator;
            continue;
        }
        auto toCheckIteratorInner = checklist.begin();
        bool saveIsInnerWireIterator = *isInnerWireIterator;
        for (auto itInner = prisms.begin(); itInner != prisms.end(); ++itInner) {
            if (itOuter == itInner || !*toCheckIteratorInner) {
                ++toCheckIteratorInner;
                continue;
            }
            // get MomentOfInertia of first shape
            BRepGProp::VolumeProperties(*itInner, tempProperties);
            momentOfInertiaInitial = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
            BRepAlgoAPI_Cut mkCut(*itInner, *itOuter);
            if (!mkCut.IsDone())
                Standard_Failure::Raise("Extrusion: Cut out failed");
            BRepGProp::VolumeProperties(mkCut.Shape(), tempProperties);
            momentOfInertiaFinal = tempProperties.MomentOfInertia(gp_Ax1(gp_Pnt(), direction));
            // if the whole shape was cut away the resulting shape is not Null but its MomentOfInertia is 0.0
            // therefore we have an inner wire if the MomentOfInertia is not zero and changed
            if ((momentOfInertiaInitial != momentOfInertiaFinal)
                && (momentOfInertiaFinal > Precision::Confusion())) {
                *isInnerWireIterator = !forInner;
                ++numCheckWires;
                *toCheckIterator = true;
                break;
            }
            ++toCheckIteratorInner;
        }
        if (saveIsInnerWireIterator == *isInnerWireIterator)
            // nothing was changed and we can remove it from the list to be checked
            // but we cannot do this before the foor loop was fully run
            toDisable[outer] = true;
        ++isInnerWireIterator;
        ++toCheckIterator;
    }

    // cancel prisms from the checklist whose wire state did not change
    size_t i = 0;
    for (auto disable : toDisable) {
        if (disable)
            checklist[i] = false;
        ++i;
    }

    // if all wires are inner ones, we take the first one as outer and issue a warning
    if (numCheckWires == isInnerWire.size()) {
        isInnerWire[0] = false;
        checklist[0] = false;
        --numCheckWires;
        Base::Console().Warning("Extrusion: could not determine what structure is the outer one.\n\
                                 The first input one will now be taken as outer one.\n");
    }

    // There can be cases with several wires all intersecting each other.
    // Then it is impossible to find out what wire is an inner one
    // and we can only treat all wires in the checklist as outer ones.
    if (numCheckWiresInitial == numCheckWires) {
        i = 0;
        for (auto checks : checklist) {
            if (checks) {
                isInnerWire[i] = false;
                checklist[i] = false;
                --numCheckWires;
            }
            ++i;
        }
        Base::Console().Warning("Extrusion: too many self-intersection structures!\n\
                                 Impossible to determine what structure is an inner one.\n\
                                 All undeterminable structures will therefore be taken as outer ones.\n");
    }

    // recursively call the function until all wires are checked
    if (numCheckWires > 1)
        checkInnerWires(isInnerWire, direction, checklist, !forInner, prisms);
}

void ExtrusionHelper::createTaperedPrismOffset(TopoDS_Wire sourceWire,
                                               const gp_Vec& translation,
                                               double offset,
                                               bool isSecond,
                                               TopoDS_Wire& result) {

    // if the wire consists of a single edge which has applied a placement
    // then this placement must be reset because otherwise
    // BRepOffsetAPI_MakeOffset shows weird behaviour by applying the placement, see
    // https://dev.opencascade.org/content/brepoffsetapimakeoffset-wire-and-face-odd-occt-740
    // therefore we use here the workaround of BRepOffsetAPI_MakeOffsetFix and not BRepOffsetAPI_MakeOffset

    gp_Trsf tempTransform;
    tempTransform.SetTranslation(translation);
    TopLoc_Location loc(tempTransform);
    TopoDS_Wire movedSourceWire = TopoDS::Wire(sourceWire.Moved(loc));

    TopoDS_Shape offsetShape;
    if (fabs(offset) > Precision::Confusion()) {
        TopLoc_Location edgeLocation;
        // create the offset shape
        BRepOffsetAPI_MakeOffsetFix mkOffset;
        mkOffset.Init(GeomAbs_Arc);
        mkOffset.Init(GeomAbs_Intersection);
        mkOffset.AddWire(movedSourceWire);
        try {
            mkOffset.Perform(offset);
            offsetShape = mkOffset.Shape();
        }
        catch (const Base::Exception& e) {
            throw Base::RuntimeError(e.what());
        }
        if (!mkOffset.IsDone()) {
            Standard_Failure::Raise("Extrusion: Offset could not be created");
        }
    }
    else {
        offsetShape = movedSourceWire;
    }
    if (offsetShape.IsNull()) {
        if (isSecond)
            Base::Console().Error("Extrusion: end face of tapered against extrusion is empty\n" \
                "This means most probably that the against taper angle is too large or small.\n");
        else
            Base::Console().Error("Extrusion: end face of tapered along extrusion is empty\n" \
                "This means most probably that the along taper angle is too large or small.\n");
        Standard_Failure::Raise("Extrusion: end face of tapered extrusion is empty");
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
        // since this is a common mistake users will quickly do, issue a warning dialog
        // FIXME: Standard_Failure::Raise or App::DocumentObjectExecReturn don't output the message to the user
        result = TopoDS_Wire();
        if (isSecond)
            Base::Console().Error("Extrusion: type of against extrusion end face is not supported.\n" \
                "This means most probably that the against taper angle is too large or small.\n");
        else
            Base::Console().Error("Extrusion: type of along extrusion is not supported.\n" \
                "This means most probably that the along taper angle is too large or small.\n");
    }

}

static TopoShape makEDraftUsingPipe(const std::vector<TopoShape> &_wires,
                                    App::StringHasherRef hasher)
{
    std::vector<TopoShape> shells;
    std::vector<TopoShape> frontwires, backwires;
    
    if (_wires.size() < 2)
        throw Base::CADKernelError("Not enough wire section");

    std::vector<TopoShape> wires;
    wires.reserve(_wires.size());
    for (auto &wire : _wires) {
        // Make a copy to work around OCCT bug on offset circular shapes
        wires.push_back(wire.makECopy());
    }
    GeomLineSegment line;
    Base::Vector3d pstart, pend;
    wires.front().getCenterOfGravity(pstart);
    gp_Pln pln;
    if (wires.back().findPlane(pln)) {
        auto dir = pln.Position().Direction();
        auto base = pln.Location();
        pend = pstart;
        pend.ProjectToPlane(Base::Vector3d(base.X(), base.Y(), base.Z()),
                            Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
    } else
        wires.back().getCenterOfGravity(pend);
    line.setPoints(pstart, pend);

    BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(line.toShape()));
    BRepOffsetAPI_MakePipeShell mkPS(mkWire.Wire());
    mkPS.SetTolerance(Precision::Confusion());
    mkPS.SetTransitionMode(BRepBuilderAPI_Transformed);
    mkPS.SetMode(false);

    for (auto &wire : wires)
        mkPS.Add(TopoDS::Wire(wire.getShape()));

    if (!mkPS.IsReady())
        throw Base::CADKernelError("Shape could not be built");

    TopoShape result(0,hasher);
    result.makEShape(mkPS,wires);

    if (!mkPS.Shape().Closed()) {
        // shell is not closed - use simulate to get the end wires
        TopTools_ListOfShape sim;
        mkPS.Simulate(2, sim);

        TopoShape front(sim.First());
        if(front.countSubShapes(TopAbs_EDGE)==wires.front().countSubShapes(TopAbs_EDGE)) {
            front = wires.front();
            front.setShape(sim.First(),false);
        }else
            front.Tag = -wires.front().Tag;
        TopoShape back(sim.Last());
        if(back.countSubShapes(TopAbs_EDGE)==wires.back().countSubShapes(TopAbs_EDGE)) {
            back = wires.back();
            back.setShape(sim.Last(),false);
        }else
            back.Tag = -wires.back().Tag;

        // build the end faces, sew the shell and build the final solid
        front = front.makEFace();
        back = back.makEFace();

        BRepBuilderAPI_Sewing sewer;
        sewer.SetTolerance(Precision::Confusion());
        sewer.Add(front.getShape());
        sewer.Add(back.getShape());
        sewer.Add(result.getShape());

        sewer.Perform();
        result = result.makEShape(sewer);
    }

    result = result.makESolid();

    BRepClass3d_SolidClassifier SC(result.getShape());
    SC.PerformInfinitePoint(Precision::Confusion());
    if (SC.State() == TopAbs_IN) {
        result.setShape(result.getShape().Reversed(),false);
    }
    return result;
}

void ExtrusionHelper::makEDraft(const ExtrusionParameters& params,
                                const TopoShape& _shape, 
                                std::vector<TopoShape>& drafts,
                                App::StringHasherRef hasher)
{
    double distanceFwd = tan(params.taperAngleFwd)*params.lengthFwd;
    double distanceRev = tan(params.taperAngleRev)*params.lengthRev;

    gp_Vec vecFwd = gp_Vec(params.dir)*params.lengthFwd;
    gp_Vec vecRev = gp_Vec(params.dir.Reversed())*params.lengthRev;

    bool bFwd = fabs(params.lengthFwd) > Precision::Confusion();
    bool bRev = fabs(params.lengthRev) > Precision::Confusion();
    bool bMid = !bFwd || !bRev || params.lengthFwd*params.lengthRev > 0.0; //include the source shape as loft section?

    TopoShape shape = _shape;
    TopoShape sourceWire;
    if (shape.isNull())
        Standard_Failure::Raise("Not a valid shape");

    if (params.solid && !shape.hasSubShape(TopAbs_FACE))
        shape = shape.makEFace(nullptr, params.faceMakerClass.c_str());

    if (shape.shapeType() == TopAbs_FACE) {
        std::vector<TopoShape> wires;
        TopoShape outerWire = shape.splitWires(&wires, TopoShape::ReorientForward);
        if (outerWire.isNull())
            Standard_Failure::Raise("Missing outer wire");
        if (wires.empty())
            shape = outerWire;
        else {
            unsigned pos = drafts.size();
            makEDraft(params, outerWire, drafts, hasher);
            if (drafts.size() != pos+1)
                Standard_Failure::Raise("Failed to make drafted extrusion");
            std::vector<TopoShape> inner;
            TopoShape innerWires(0, hasher);
            innerWires.makECompound(wires,"",false);
            ExtrusionParameters copy = params;
            copy.taperAngleFwd = params.innerTaperAngleFwd;
            copy.taperAngleRev = params.innerTaperAngleRev;
            makEDraft(copy, innerWires, inner, hasher);
            if (inner.empty())
                Standard_Failure::Raise("Failed to make drafted extrusion with inner hole");
            inner.insert(inner.begin(), drafts.back());
            drafts.back().makECut(inner);
            return;
        }
    }

    if (shape.shapeType() == TopAbs_WIRE) {
        ShapeFix_Wire aFix;
        aFix.Load(TopoDS::Wire(shape.getShape()));
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        sourceWire.setShape(aFix.Wire());
        sourceWire.Tag = shape.Tag;
        sourceWire.mapSubElement(shape);
    }
    else if (shape.shapeType() == TopAbs_COMPOUND) {
        for(auto &s : shape.getSubTopoShapes())
            makEDraft(params, s, drafts, hasher);
    }
    else {
        Standard_Failure::Raise("Only a wire or a face is supported");
    }

    if (!sourceWire.isNull()) {
        std::vector<TopoShape> list_of_sections;
        auto makeOffset = [&sourceWire](const gp_Vec& translation, double offset) -> TopoShape {
            gp_Trsf mat;
            mat.SetTranslation(translation);
            TopoShape offsetShape(sourceWire.makETransform(mat,"RV"));
            if (fabs(offset)>Precision::Confusion())
                offsetShape = offsetShape.makEOffset2D(offset, TopoShape::JoinType::Intersection);
            return offsetShape;
        };

        //first. add wire for reversed part of extrusion
        if (bRev){
            auto offsetShape = makeOffset(vecRev, distanceRev);
            if (offsetShape.isNull())
                Standard_Failure::Raise("Tapered shape is empty");
            TopAbs_ShapeEnum type = offsetShape.getShape().ShapeType();
            if (type == TopAbs_WIRE) {
                list_of_sections.push_back(offsetShape);
            }
            else if (type == TopAbs_EDGE) {
                list_of_sections.push_back(offsetShape.makEWires());
            }
            else {
                Standard_Failure::Raise("Tapered shape type is not supported");
            }
        }

        //next. Add source wire as middle section. Order is important.
        if (bMid){
            list_of_sections.push_back(sourceWire);
        }

        //finally. Forward extrusion offset wire.
        if (bFwd){
            auto offsetShape = makeOffset(vecFwd, distanceFwd);
            if (offsetShape.isNull())
                Standard_Failure::Raise("Tapered shape is empty");
            TopAbs_ShapeEnum type = offsetShape.getShape().ShapeType();
            if (type == TopAbs_WIRE) {
                list_of_sections.push_back(offsetShape);
            }
            else if (type == TopAbs_EDGE) {
                list_of_sections.push_back(offsetShape.makEWires());
            }
            else {
                Standard_Failure::Raise("Tapered shape type is not supported");
            }
        }

        try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            if (params.usepipe) {
                drafts.push_back(makEDraftUsingPipe(list_of_sections, hasher));
                return;
            }

            //make loft
            BRepOffsetAPI_ThruSections mkGenerator(
                    params.solid ? Standard_True : Standard_False, /*ruled=*/Standard_True);
            for(auto &s : list_of_sections)
                mkGenerator.AddWire(TopoDS::Wire(s.getShape()));

            mkGenerator.Build();
            drafts.push_back(TopoShape(0,hasher).makEShape(mkGenerator,list_of_sections));
        }
        catch (Standard_Failure &){
            throw;
        }
        catch (...) {
            throw Base::CADKernelError("Unknown exception from BRepOffsetAPI_ThruSections");
        }
    }
}
