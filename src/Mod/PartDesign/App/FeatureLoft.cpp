// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <Precision.hxx>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <memory>


#include <boost/core/ignore_unused.hpp>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <App/SemanticDocumentState.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/FaceMakerCheese.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/SemanticSourceCollector.h>

#include "Mod/Part/App/TopoShapeOpCode.h"

#include "FeatureLoft.h"
#include "SemanticOpcode.h"

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::Loft, PartDesign::ProfileBased)

Loft::Loft()
{
    ADD_PROPERTY_TYPE(Sections, (nullptr), "Loft", App::Prop_None, "List of sections");
    Sections.setValue(nullptr);
    ADD_PROPERTY_TYPE(Ruled, (false), "Loft", App::Prop_None, "Create ruled surface");
    ADD_PROPERTY_TYPE(Closed, (false), "Loft", App::Prop_None, "Close Last to First Profile");
}

short Loft::mustExecute() const
{
    if (Sections.isTouched()) {
        return 1;
    }
    if (Ruled.isTouched()) {
        return 1;
    }
    if (Closed.isTouched()) {
        return 1;
    }

    // Shared by additive and subtractive loft restore republish.
    if (isSemanticRepublishPass(SemanticEmitter::graphFor(this))) {
        return 1;
    }

    return ProfileBased::mustExecute();
}

std::vector<Part::TopoShape> Loft::getSectionShape(
    const char* name,
    App::DocumentObject* obj,
    const std::vector<std::string>& subs,
    size_t expected_size
)
{
    auto useSketch = [](App::DocumentObject* obj, const std::vector<std::string>& subs) {
        // Be smart. If part of a sketch is selected, use the entire sketch unless it is a single
        // vertex - backward compatibility (#16630)
        if (!obj) {
            return false;
        }

        auto subName = subs.empty() ? "" : subs.front();
        return obj->isDerivedFrom<Part::Part2DObject>() && subName.find("Vertex") != 0;
    };

    std::vector<TopoShape> shapes;
    auto useEntireSketch = useSketch(obj, subs);
    if (subs.empty() || std::ranges::find(subs, std::string()) != subs.end() || useEntireSketch) {
        shapes.push_back(
            Part::Feature::getTopoShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform)
        );
        if (shapes.back().isNull()) {
            std::stringstream str;
            str << "Failed to get shape of " << name;
            if (obj) {
                auto doc = obj->getDocument();
                str << " " << App::SubObjectT(obj, "").getSubObjectFullName(doc->getName());
            }
            THROWM(Part::NullShapeException, str.str());
        }
    }
    else {
        for (const auto& sub : subs) {
            shapes.push_back(
                Part::Feature::getTopoShape(
                    obj,
                    Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                        | Part::ShapeOption::Transform,
                    sub.c_str()
                )
            );
            if (shapes.back().isNull()) {
                std::stringstream str;
                str << "Failed to get shape of " << name;
                if (obj) {
                    auto doc = obj->getDocument();
                    App::SubObjectT subObj(obj, sub.c_str());
                    str << " " << subObj.getSubObjectFullName(doc->getName());
                }
                THROWM(Part::NullShapeException, str.str());
            }
        }
    }
    auto compound = TopoShape(0).makeElementCompound(
        shapes,
        "",
        TopoShape::SingleShapeCompoundCreationPolicy::returnShape
    );
    auto wires = compound.getSubTopoShapes(TopAbs_WIRE);
    auto edges = compound.getSubTopoShapes(TopAbs_EDGE, TopAbs_WIRE);  // get free edges and make
                                                                       // wires from it
    if (!edges.empty()) {
        auto extra = TopoShape(0).makeElementWires(edges).getSubTopoShapes(TopAbs_WIRE);
        wires.insert(wires.end(), extra.begin(), extra.end());
    }
    const char* msg
        = "Sections need to have the same amount of wires or vertices as the base section";
    if (!wires.empty()) {
        if (expected_size && expected_size != wires.size()) {
            FC_THROWM(Base::CADKernelError, msg);
        }
        return wires;
    }
    auto vertices = compound.getSubTopoShapes(TopAbs_VERTEX);
    if (vertices.empty()) {
        FC_THROWM(
            Base::CADKernelError,
            "Invalid " << name << " shape, expecting either wires or vertices"
        );
    }
    if (expected_size && expected_size != vertices.size()) {
        FC_THROWM(Base::CADKernelError, msg);
    }
    return vertices;
}

App::DocumentObjectExecReturn* Loft::execute()
{
    // The Refine-only shortcut must not suppress restore republish: the maker
    // is needed to recreate durable history for both loft variants.
    const bool semanticRepublish = isSemanticRepublishPass(SemanticEmitter::graphFor(this));
    if (!semanticRepublish && onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    std::vector<TopoShape> wires;
    try {
        // Profile Face references must use the live semantic slot when available;
        // otherwise a stale dual-write subname can feed the loft maker.
        wires = getSectionShape("Profile", Profile.getValue(), getProfileSubValuesForMaker());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    // if the Base property has a valid shape, fuse the pipe into it
    TopoShape base;
    try {
        base = getBaseTopoShape();
    }
    catch (const Base::Exception&) {
    }

    auto hasher = getDocument()->getStringHasher();

    try {
        // setup the location
        this->positionByPrevious();
        auto invObjLoc = this->getLocation().Inverted();
        if (!base.isNull()) {
            base.move(invObjLoc);
        }

        // build up multisections
        auto multisections = Sections.getSubListValues();
        if (multisections.empty()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Loft: At least one section is needed")
            );
        }

        std::vector<std::vector<TopoShape>> wiresections;
        wiresections.reserve(wires.size());
        for (auto& wire : wires) {
            wiresections.emplace_back(1, wire);
        }

        for (const auto& subSet : multisections) {
            int i = 0;
            for (const auto& s :
                 getSectionShape("Section", subSet.first, subSet.second, wiresections.size())) {
                wiresections[i++].push_back(s);
            }
        }

        bool closed = Closed.getValue();
        // invalid for less then 3 sections
        if (multisections.size() < 2) {
            closed = false;
        }

        TopoShape result(0, hasher);
        std::vector<TopoShape> shapes;

        // build all shells. Keep live ThruSections until captureLoftMaker.
        // Store the exact AddWire/AddVertex TShapes (post-move) — those are
        // the Generated() keys. Do not use FaceMaker front or FirstShape.
        std::vector<TopoShape> shells;
        std::vector<std::unique_ptr<BRepOffsetAPI_ThruSections>> liveLofts;
        std::vector<TopoDS_Shape> firstAddWires;
        for (auto& sectionWires : wiresections) {
            for (auto& wire : sectionWires) {
                wire.move(invObjLoc);
            }
            auto mkLoft = std::make_unique<BRepOffsetAPI_ThruSections>(
                Standard_False,
                Ruled.getValue() ? Standard_True : Standard_False
            );
            const bool captureThis = firstAddWires.empty();
            for (auto& sh : sectionWires) {
                const auto& shape = sh.getShape();
                if (captureThis) {
                    firstAddWires.push_back(shape);
                }
                if (shape.ShapeType() == TopAbs_VERTEX) {
                    mkLoft->AddVertex(TopoDS::Vertex(shape));
                }
                else {
                    mkLoft->AddWire(TopoDS::Wire(shape));
                }
            }
            if (closed && !sectionWires.empty()
                && sectionWires.back().getShape().ShapeType() != TopAbs_VERTEX) {
                const TopoDS_Shape& firstProfile = sectionWires.front().getShape();
                if (firstProfile.ShapeType() == TopAbs_VERTEX) {
                    mkLoft->AddVertex(TopoDS::Vertex(firstProfile));
                }
                else {
                    mkLoft->AddWire(TopoDS::Wire(firstProfile));
                }
            }
            mkLoft->CheckCompatibility(Standard_True);
            mkLoft->Build();
            TopoShape shell(0, hasher);
            shell.makeElementShape(*mkLoft, sectionWires, Part::OpCodes::Loft);
            shells.push_back(shell);
            liveLofts.push_back(std::move(mkLoft));
        }

        // ThruSections makeElementShape shell sewer.Add receives, before
        // front/back append and before sew / makeElementSolid.
        const TopoShape preSewShell = shells.empty() ? TopoShape() : shells.front();

        // build the top and bottom face, sew the shell and build the final solid
        TopoShape front;
        if (wiresections[0].front().shapeType() != TopAbs_VERTEX) {
            front = getTopoShapeVerifiedFace();
            if (front.isNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Loft: Creating a face from sketch failed")
                );
            }
            front.move(invObjLoc);
        }

        TopoShape back;
        if (wiresections[0].back().shapeType() != TopAbs_VERTEX) {
            std::vector<TopoShape> backwires;
            for (auto& sectionWires : wiresections) {
                backwires.push_back(sectionWires.back());
            }
            const char* faceMaker[] = {
                "Part::FaceMakerBullseye",
                "Part::FaceMakerCheese",
                "Part::FaceMakerSimple",
                "Part::FaceMakerUnified",
            };
            for (size_t i = 0; i < std::size(faceMaker); i++) {
                try {
                    back = TopoShape(0).makeElementFace(backwires, nullptr, faceMaker[i]);
                    break;
                }
                catch (...) {
                    if (i == std::size(faceMaker) - 1) {
                        throw;
                    }
                    continue;
                }
            }
        }

        BRepBuilderAPI_Sewing sewer;
        bool sewRan = false;
        if (!front.isNull() || !back.isNull()) {
            sewer.SetTolerance(Precision::Confusion());
            if (!front.isNull()) {
                sewer.Add(front.getShape());
            }
            if (!back.isNull()) {
                sewer.Add(back.getShape());
            }
            for (auto& s : shells) {
                sewer.Add(s.getShape());
            }

            sewer.Perform();

            if (!front.isNull()) {
                shells.push_back(front);
            }
            if (!back.isNull()) {
                shells.push_back(back);
            }
            // equivalent of the removed: result = result.makeElementShape(sewer,shells);
            result = result.makeShapeWithElementMap(
                sewer.SewedShape(),
                Part::MapperSewing(sewer),
                shells,
                Part::OpCodes::Sewing
            );
            sewRan = true;
        }

        if (!result.countSubShapes(TopAbs_SHELL)) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Loft: Failed to create shell")
            );
        }
        shapes = result.getSubTopoShapes(TopAbs_SHELL);

        for (auto& s : shapes) {
            // build the solid
            s = s.makeElementSolid();
            BRepClass3d_SolidClassifier SC(s.getShape());
            SC.PerformInfinitePoint(Precision::Confusion());
            if (SC.State() == TopAbs_IN) {
                s.setShape(s.getShape().Reversed(), false);
            }
        }

        AddSubShape.setValue(result.makeElementCompound(
            shapes,
            nullptr,
            Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape
        ));

        if (shapes.size() > 1) {
            result.makeElementFuse(shapes);
        }
        else {
            result = shapes.front();
        }

        if (base.isNull()) {
            if (!isSingleSolidRuleSatisfied(result.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            Shape.setValue(getSolid(result));
            if (!liveLofts.empty()) {
                emitCapturedLoft(
                    liveLofts.front().get(),
                    preSewShell,
                    Shape.getShape(),
                    firstAddWires,
                    sewRan ? &sewer : nullptr
                );
            }
            return App::DocumentObject::StdReturn;
        }

        result.Tag = -getID();
        TopoShape boolOp(0, getDocument()->getStringHasher());
        std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkCut;
        const TopoDS_Shape toolOcc = result.getShape();

        try {
            if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
                // Live Cut + ElementMap (Batch A / FeaturePrimitive parity) so
                // Fillet can bind :M;CUT edges and stampElementMap can write ;:ST.
                auto* fcCut = new FCBRepAlgoAPI_Cut;
                mkCut.reset(fcCut);
                TopTools_ListOfShape args;
                TopTools_ListOfShape toolList;
                args.Append(base.getShape());
                toolList.Append(toolOcc);
                fcCut->SetArguments(args);
                fcCut->SetTools(toolList);
                const double fuzzy = FuzzyTolerance.getValue();
                if (fuzzy > 0.0) {
                    fcCut->SetFuzzyValue(fuzzy);
                }
                fcCut->Build();
                if (!fcCut->IsDone()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation")
                    );
                }
                boolOp.makeElementShape(*fcCut, {base, result}, Part::OpCodes::Cut);
            }
            else if (getAddSubType() == FeatureAddSub::Type::Additive) {
                boolOp.makeElementBoolean(
                    Part::OpCodes::Fuse,
                    {base, result},
                    nullptr,
                    FuzzyTolerance.getValue()
                );
            }
            else {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Unknown operation type")
                );
            }
        }
        catch (Standard_Failure&) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation")
            );
        }
        TopoShape solid = getSolid(boolOp);
        // lets check if the result is a solid
        if (solid.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid")
            );
        }
        // store shape before refinement. Skip refine when live Cut/loft makers
        // are retained so fromMaker TShapes still match the published solid.
        this->rawShape = boolOp;
        if (!mkCut && liveLofts.empty()) {
            boolOp = refineShapeIfActive(boolOp);
        }
        if (!isSingleSolidRuleSatisfied(boolOp.getShape())) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Result has multiple solids: enable 'Allow Compound' in the active body."
            ));
        }
        boolOp = getSolid(boolOp);
        Shape.setValue(boolOp);
        if (mkCut) {
            App::DocumentObject* baseObj = getBaseObject(/* silent = */ true);
            SemanticEmitter::publishSubtractiveCutHistory(
                this,
                mkCut.get(),
                &toolOcc,
                &base.getShape(),
                baseObj,
                Opcode::SubtractiveLoft,
                "subLoftDiag"
            );
            App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
            // I8 dual-write after Cut Bindings publish; EM14-U1 fail-closed
            // (lockstep uniqueBindingOnFeature / AG21-E1 — EM22-L1 / #30).
            SemanticEmitter::stampElementMap(Shape, graph, static_cast<App::ObjectId>(getID()));
        }
        else if (!liveLofts.empty()) {
            emitCapturedLoft(
                liveLofts.front().get(),
                preSewShell,
                Shape.getShape(),
                firstAddWires,
                sewRan ? &sewer : nullptr
            );
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Loft: A fatal error occurred when making the loft")
        );
    }
}

PROPERTY_SOURCE(PartDesign::AdditiveLoft, PartDesign::Loft)
AdditiveLoft::AdditiveLoft()
{
    defineAdditive();
}

PROPERTY_SOURCE(PartDesign::SubtractiveLoft, PartDesign::Loft)
SubtractiveLoft::SubtractiveLoft()
{
    defineSubtractive();
}


void Loft::clearSemanticCapture()
{
    lastLoftGenerated.clear();
    lastNamedFaceIndices.clear();
    lastLoftGeneratedEdges.clear();
    lastNamedEdgeIndices.clear();
}

void Loft::captureLoftMaker(
    void* occMaker,
    const TopoShape& preSewShell,
    const TopoShape& published,
    const std::vector<TopoDS_Shape>& addWireShapes,
    BRepBuilderAPI_Sewing* sewer
)
{
    int early = 0;
    int genFaceRaw = 0;
    int genEdgeRaw = 0;
    int fromMaker = 0;
    int fromHistN = 0;
    int nCurve = 0;
    int nVertex = 0;
    int nEdges = 0;
    int nVerts = 0;
    int nZ = 0;
    const int sew = sewer ? 1 : 0;

    auto finishDiag = [&]() {
        const std::size_t nf = Part::namedIndexCount(lastNamedFaceIndices);
        const std::size_t ne = Part::namedIndexCount(lastNamedEdgeIndices);
        lastLoftDiag = std::string("loftDiag early=") + std::to_string(early) + " preCompat=1 sew="
            + std::to_string(sew) + " inputs=" + std::to_string(addWireShapes.size())
            + " curveSeeds=" + std::to_string(nCurve) + " vertexSeeds=" + std::to_string(nVertex)
            + " edges=" + std::to_string(nEdges) + " verts=" + std::to_string(nVerts)
            + " genFaceRaw=" + std::to_string(genFaceRaw) + " genFace="
            + std::to_string(lastLoftGenerated.size()) + " genEdgeRaw=" + std::to_string(genEdgeRaw)
            + " genEdge=" + std::to_string(lastLoftGeneratedEdges.size())
            + " fromMaker=" + std::to_string(fromMaker) + " fromHist=" + std::to_string(fromHistN)
            + " namedFace=" + std::to_string(nf) + " namedFaceMiss="
            + std::to_string(lastNamedFaceIndices.size() - nf) + " namedEdge=" + std::to_string(ne)
            + " namedEdgeMiss=" + std::to_string(lastNamedEdgeIndices.size() - ne)
            + " zEdge=" + std::to_string(nZ);
    };

    if (!occMaker) {
        early = 1;
        finishDiag();
        return;
    }
    if (preSewShell.isNull()) {
        early = 2;
        finishDiag();
        return;
    }
    if (addWireShapes.empty()) {
        early = 3;
        finishDiag();
        return;
    }

    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    AfterExecuteRequest seeds;
    if (graph) {
        if (App::DocumentObject* profile = Profile.getValue()) {
            seeds = collectProfileSemanticSeeds();
        }
    }
    nCurve = static_cast<int>(seeds.curveSeeds.size());
    nVertex = static_cast<int>(seeds.vertexSeeds.size());

    // Generated() keys are the AddWire/AddVertex TShapes kept alive from
    // execute() (post-move). First shape is the profile wire.
    const TopoShape profile(addWireShapes.front());
    if (profile.isNull()) {
        early = 4;
        finishDiag();
        return;
    }
    const auto edges = profile.getSubTopoShapes(TopAbs_EDGE);
    const auto vertices = profile.getSubTopoShapes(TopAbs_VERTEX);
    nEdges = static_cast<int>(edges.size());
    nVerts = static_cast<int>(vertices.size());

    auto* maker = static_cast<BRepBuilderAPI_MakeShape*>(occMaker);

    auto alreadyHasEdge = [&](const TopoDS_Shape& s) -> bool {
        for (const auto& existing : lastLoftGeneratedEdges) {
            if (Part::sameOccShape(existing.shape, s)) {
                return true;
            }
        }
        return false;
    };

    // Direct maker->Generated(profileEdge) → unique side FACE on preSewShell.
    for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < edges.size(); ++i) {
        genFaceRaw += Part::countGeneratedOf(maker, edges[i].getShape(), TopAbs_FACE);
        const TopoDS_Shape face = Part::uniqueGeneratedFace(maker, edges[i].getShape());
        if (!face.IsNull()) {
            lastLoftGenerated.push_back({seeds.curveSeeds[i], face});
        }
    }

    // Vertical corners: Generated EDGE from profile VERTEX, 1 image (Pad).
    auto stashUniqueEdge = [&](const App::SemanticId& seed, const TopoDS_Shape& input) {
        if (!seed.valid() || input.IsNull()) {
            return;
        }
        genEdgeRaw += Part::countGeneratedOf(maker, input, TopAbs_EDGE);
        const auto images = Part::uniqueGeneratedThenModifiedEdgeImages(maker, input);
        if (images.size() == 1 && !alreadyHasEdge(images.front())) {
            lastLoftGeneratedEdges.push_back({seed, images.front()});
        }
    };
    if (!seeds.vertexSeeds.empty()) {
        for (std::size_t i = 0; i < seeds.vertexSeeds.size() && i < vertices.size(); ++i) {
            stashUniqueEdge(seeds.vertexSeeds[i], vertices[i].getShape());
        }
    }
    else {
        for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < vertices.size(); ++i) {
            stashUniqueEdge(seeds.curveSeeds[i], vertices[i].getShape());
        }
    }

    // Pad uniqueEdgeImages: unique EDGE image of each profile EDGE.
    for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < edges.size(); ++i) {
        genEdgeRaw += Part::countGeneratedOf(maker, edges[i].getShape(), TopAbs_EDGE);
        const auto images = Part::uniqueGeneratedThenModifiedEdgeImages(maker, edges[i].getShape());
        if (images.size() == 1 && !alreadyHasEdge(images.front())) {
            lastLoftGeneratedEdges.push_back({seeds.curveSeeds[i], images.front()});
        }
    }

    // Pad fallback: unique vertical EDGE of a Generated side face on preSewShell.
    Part::appendUniqueZParallelFaceRailEdges(preSewShell, lastLoftGenerated, lastLoftGeneratedEdges);

    // fromMaker fallback on preSewShell only (not published) if Generated walk
    // produced no Faces — Pad lastPrismGenerated empty path. Index later.
    {
        bool usedFromMaker = false;
        fromHistN = static_cast<int>(Part::appendFromMakerGeneratedWhenFacesEmpty(
            maker,
            preSewShell,
            seeds.curveSeeds,
            edges,
            seeds.vertexSeeds,
            vertices,
            lastLoftGenerated,
            lastLoftGeneratedEdges,
            &usedFromMaker
        ));
        fromMaker = usedFromMaker ? 1 : 0;
    }

    Part::refreshNamedIndicesFromSeededShapes(
        published,
        preSewShell,
        sewer,
        lastLoftGenerated,
        lastLoftGeneratedEdges,
        lastNamedFaceIndices,
        lastNamedEdgeIndices
    );

    // Edge binding ALWAYS: unique Z-parallel of each published Face, even when
    // vertexSeeds is non-empty (root cause 4). I13: 0 or N unnamed.
    nZ = static_cast<int>(
        Part::mergeUniqueZParallelEdgesOntoNamed(published, lastNamedFaceIndices, lastNamedEdgeIndices)
    );
    finishDiag();
}

void Loft::emitCapturedLoft(
    void* occMaker,
    const TopoShape& preSewShell,
    const TopoShape& published,
    const std::vector<TopoDS_Shape>& addWireShapes,
    BRepBuilderAPI_Sewing* sewer
)
{
    clearSemanticCapture();
    captureLoftMaker(occMaker, preSewShell, published, addWireShapes, sewer);
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    AfterExecuteRequest req;
    App::ObjectId fid = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
        if (App::DocumentObject* profile = Profile.getValue()) {
            req = collectProfileSemanticSeeds();
        }
    }
    req.namedFaceIndices = lastNamedFaceIndices;
    req.namedEdgeIndices = lastNamedEdgeIndices;
    req.allowSequentialFaceN = false;
    SemanticEmitter::afterExecute(
        graph,
        getAddSubType() == FeatureAddSub::Type::Subtractive ? Opcode::SubtractiveLoft : Opcode::Loft,
        fid,
        eval,
        req
    );
    // I8 dual-write after Bindings publish; EM14-U1 fail-closed if multi-eval
    // (lockstep uniqueBindingOnFeature / AG21-E1 — QUALITY-SWEEP #22/#30 EM22-L1).
    SemanticEmitter::stampElementMap(Shape, graph, fid);
    SemanticEmitter::appendAfterExecuteNote(lastLoftDiag);
    Base::Console().message("TESTS loftDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

void Loft::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    // property Sections had the App::PropertyLinkList and was changed to App::PropertyXLinkSubList
    if (prop == &Sections && strcmp(TypeName, "App::PropertyLinkList") == 0) {
        Sections.upgrade(reader, TypeName);
    }
    else {
        ProfileBased::handleChangedPropertyType(reader, TypeName, prop);
    }
}
