// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <unordered_set>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <BRep_Builder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepFeat_MakePrism.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <TopoDS.hxx>
#include <gp_Ax2.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <App/ObjectIdentifier.h>
#include <App/SemanticDocumentState.h>
#include <Base/Converter.h>
#include <Base/Tools.h>
#include <Mod/Sketcher/App/SketchEntityId.h>
#include <Base/Reader.h>
#include <Base/ProgramVersion.h>
#include <Mod/Part/App/ExtrusionHelper.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/SemanticSourceCollector.h>
#include <Mod/Part/App/Tools.h>
#include "Mod/Part/App/TopoShapeOpCode.h"
#include <Mod/Part/App/PartFeature.h>

#include "FeatureExtrude.h"
#include "SemanticOpcode.h"
#include <Mod/Sketcher/App/SketchSemanticSeed.h>

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

const char* FeatureExtrude::SideTypesEnums[] = {"One side", "Two sides", "Symmetric", nullptr};

PROPERTY_SOURCE(PartDesign::FeatureExtrude, PartDesign::ProfileBased)

App::PropertyQuantityConstraint::Constraints FeatureExtrude::signedLengthConstraint
    = {-std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 1.0};
double FeatureExtrude::maxAngle = 90 - Base::toDegrees<double>(Precision::Angular());
App::PropertyAngle::Constraints FeatureExtrude::floatAngle = {-maxAngle, maxAngle, 1.0};

FeatureExtrude::FeatureExtrude()
{
    ADD_PROPERTY_TYPE(
        UseLegacyTaperDirection,
        (false),
        "Compatibility",
        App::Prop_Hidden,
        "Use legacy profile copying for tapered extrusions"
    );
}

namespace
{

/// Exactly one Face seed on Profile AttachmentSupport (sketch-on-cap).
/// 0 or >1 Face seeds → unnamed (I13). Never first-Binding-wins.
App::SemanticId uniqueNamedProfileFace(const App::DocumentObject* profile)
{
    if (!profile || !profile->isDerivedFrom<Part::Part2DObject>()) {
        return {};
    }
    const auto* sketch = static_cast<const Part::Part2DObject*>(profile);
    return uniqueNamedFace(sketch->AttachmentSupport.getSemanticRefs());
}

}  // namespace

void FeatureExtrude::clearSemanticCapture()
{
    lastPrismGenerated.clear();
    lastNamedFaceIndices.clear();
    lastPrismGeneratedEdges.clear();
    lastNamedEdgeIndices.clear();
    lastCutRemnant = {};
}

void FeatureExtrude::refreshNamedIndices(const TopoShape& published)
{
    lastNamedFaceIndices.clear();
    lastNamedFaceIndices.reserve(lastPrismGenerated.size());
    for (const auto& p : lastPrismGenerated) {
        lastNamedFaceIndices.push_back(Part::indexOnPublishedPartnerCoplanar(published, p.shape));
    }
    lastNamedEdgeIndices.clear();
    lastNamedEdgeIndices.reserve(lastPrismGeneratedEdges.size());
    for (const auto& p : lastPrismGeneratedEdges) {
        lastNamedEdgeIndices.push_back(Part::indexOnPublishedPartnerCoplanar(published, p.shape));
    }
}

void FeatureExtrude::capturePrismMaker(void* occMaker, const TopoShape& prism, const TopoShape& sketch)
{
    if (prism.isNull() || sketch.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    AfterExecuteRequest seeds;
    if (graph) {
        if (App::DocumentObject* profile = Profile.getValue()) {
            seeds = collectProfileSemanticSeeds();
        }
    }
    const auto edges = sketch.getSubTopoShapes(TopAbs_EDGE);
    const auto faces = sketch.getSubTopoShapes(TopAbs_FACE);
    const auto vertices = sketch.getSubTopoShapes(TopAbs_VERTEX);

    // Closed rectangle still has a profile region when MakeInternals is off
    // (lastInternalRegionStamps empty). Mint the same regionKey internals
    // would use (I8 reuse via ensureRegionSeed, not a second heap).
    if (graph && seeds.regionSeeds.empty() && !faces.empty()) {
        std::vector<std::string> names;
        const unsigned long nEdges = faces[0].countSubShapes(TopAbs_EDGE);
        names.reserve(static_cast<std::size_t>(nEdges));
        for (unsigned long i = 1; i <= nEdges; ++i) {
            Data::MappedName mapped = faces[0].getMappedName(
                Data::IndexedName::fromConst("Edge", static_cast<int>(i)));
            if (mapped) {
                names.push_back(mapped.toString());
            }
        }
        const auto ids = Sketcher::SketchEntityIdMap::entityIdsFromMappedNames(names);
        if (!ids.empty()) {
            App::ObjectId sketchObjectId = 0;
            if (App::DocumentObject* profile = Profile.getValue()) {
                sketchObjectId = static_cast<App::ObjectId>(profile->getID());
            }
            App::EvalSerial currentEval = 0;
            if (App::Document* doc = getDocument()) {
                currentEval = doc->semanticState().currentEval();
            }
            const std::string key = Sketcher::SketchEntityIdMap::regionKey(
                ids, Sketcher::SketchEntityIdMap::RoleInterior);
            const App::SemanticId region = Sketcher::SketchSemanticSeeds::ensureRegionSeed(
                *graph, sketchObjectId, currentEval, key);
            if (region.valid()) {
                seeds.regionSeeds.push_back(region);
            }
        }
    }

    std::deque<TopoDS_Shape> held;
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < edges.size(); ++i) {
        held.push_back(edges[i].getShape());
        inputs.push_back({seeds.curveSeeds[i], &held.back()});
    }
    for (std::size_t i = 0; i < seeds.regionSeeds.size() && i < faces.size(); ++i) {
        held.push_back(faces[i].getShape());
        inputs.push_back({seeds.regionSeeds[i], &held.back()});
    }
    // Vertex seeds when present; else zip prism edges onto curve seeds 1:1.
    if (!seeds.vertexSeeds.empty()) {
        for (std::size_t i = 0; i < seeds.vertexSeeds.size() && i < vertices.size(); ++i) {
            held.push_back(vertices[i].getShape());
            inputs.push_back({seeds.vertexSeeds[i], &held.back()});
        }
    }
    else {
        for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < vertices.size(); ++i) {
            held.push_back(vertices[i].getShape());
            inputs.push_back({seeds.curveSeeds[i], &held.back()});
        }
    }

    auto indexOf = [&prism](const void* occ) -> App::ElementIndex {
        if (!occ) {
            return {};
        }
        return Part::indexOnPublishedPartnerCoplanar(prism, *static_cast<const TopoDS_Shape*>(occ));
    };

    if (occMaker) {
        auto* maker = static_cast<BRepBuilderAPI_MakeShape*>(occMaker);
        for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < edges.size(); ++i) {
            const TopoDS_Shape& e = edges[i].getShape();
            for (TopTools_ListIteratorOfListOfShape it(maker->Generated(e)); it.More(); it.Next()) {
                if (it.Value().ShapeType() == TopAbs_FACE) {
                    lastPrismGenerated.push_back({seeds.curveSeeds[i], it.Value()});
                }
            }
        }
        // Vertical corners: Generated EDGE from sketch VERTEX, 1 image.
        // Several images → leave unnamed (do not invent EdgeN).
        auto stashUniqueEdge = [&](const App::SemanticId& seed, const TopoDS_Shape& input) {
            if (!seed.valid() || input.IsNull()) {
                return;
            }
            TopoDS_Shape unique;
            int n = 0;
            for (TopTools_ListIteratorOfListOfShape it(maker->Generated(input)); it.More();
                 it.Next()) {
                if (it.Value().ShapeType() == TopAbs_EDGE) {
                    ++n;
                    unique = it.Value();
                }
            }
            if (n == 1 && !unique.IsNull()) {
                lastPrismGeneratedEdges.push_back({seed, unique});
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
        // Sketch EDGE images on the prism: Modified-first (top) via
        // Part::uniqueModifiedThenGeneratedEdgeImages. :U bottom is a sibling Binding, not a fallback
        // when images.empty() — do not put top+:U in the same vector
        // (caller drops size!=1 and would unnamed both). 1 image → name it.
        // 0 or N → unnamed (I13). Vertical corners stay on the vertex stash.
        for (std::size_t i = 0; i < seeds.curveSeeds.size() && i < edges.size(); ++i) {
            const auto images =
                Part::uniqueModifiedThenGeneratedEdgeImages(maker, edges[i].getShape(), prism);
            if (images.size() == 1) {
                bool dup = false;
                for (const auto& existing : lastPrismGeneratedEdges) {
                    if (Part::sameOccShape(existing.shape, images.front())) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) {
                    lastPrismGeneratedEdges.push_back({seeds.curveSeeds[i], images.front()});
                }
            }
            std::vector<TopoDS_Shape> stashed;
            stashed.reserve(lastPrismGeneratedEdges.size());
            for (const auto& existing : lastPrismGeneratedEdges) {
                stashed.push_back(existing.shape);
            }
            const TopoDS_Shape partner =
                Part::uniquePartnerEdgeExcluding(edges[i].getShape(), prism, stashed);
            if (!partner.IsNull()) {
                lastPrismGeneratedEdges.push_back({seeds.curveSeeds[i], partner});
            }
        }
        if (auto* mkPrism = dynamic_cast<BRepPrimAPI_MakePrism*>(maker)) {
            if (!seeds.regionSeeds.empty()) {
                const TopoDS_Shape first = mkPrism->FirstShape();
                const TopoDS_Shape last = mkPrism->LastShape();
                if (!first.IsNull() && first.ShapeType() == TopAbs_FACE) {
                    lastPrismGenerated.push_back({seeds.regionSeeds.front(), first});
                }
                if (!last.IsNull() && last.ShapeType() == TopAbs_FACE) {
                    lastPrismGenerated.push_back({seeds.regionSeeds.front(), last});
                }
            }
        }
        // BRepBuilderAPI_MakeShape has no History() on this OCCT. fromMaker
        // (Generated/Modified/IsDeleted) only. Empty -> no invented FaceN (I13).
        const Part::HistoryTable fromHist =
            Part::SemanticHistoryAdapter::fromMaker(maker, inputs, indexOf);
        if (lastPrismGenerated.empty()) {
            for (const auto& rec : fromHist) {
                if (rec.kind != App::EventKind::Generated
                    || !Part::isNamedIndex(rec.toIndex)  // PD32-E1
                    || rec.toIndex.type != "Face") {
                    continue;
                }
                TopoDS_Shape s = prism.findShape(TopAbs_FACE, rec.toIndex.index);
                if (!s.IsNull()) {
                    lastPrismGenerated.push_back({rec.fromSeed, s});
                }
            }
        }
        // fromMaker Edge rows are extra named indices (GUI Edge13/17/14), not a
        // fallback used only when vertex stash is empty. Dedup by IsSame.
        // Generated and Modified unique EDGE; Split/N-image rows stay unnamed (I13).
        // Do not sequential-name every EdgeN on the solid.
        for (const auto& rec : fromHist) {
            if ((rec.kind != App::EventKind::Generated && rec.kind != App::EventKind::Modified)
                || !Part::isNamedIndex(rec.toIndex)  // PD32-E2
                || rec.toIndex.type != "Edge") {
                continue;
            }
            TopoDS_Shape s = prism.findShape(TopAbs_EDGE, rec.toIndex.index);
            if (s.IsNull()) {
                continue;
            }
            bool dup = false;
            for (const auto& p : lastPrismGeneratedEdges) {
                if (Part::sameOccShape(p.shape, s)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                lastPrismGeneratedEdges.push_back({rec.fromSeed, s});
            }
        }
        // Fallback: unique EDGE of a Generated side face. Several images → unnamed.
        if (lastPrismGeneratedEdges.empty()) {
            for (const auto& p : lastPrismGenerated) {
                if (p.shape.IsNull() || p.shape.ShapeType() != TopAbs_FACE) {
                    continue;
                }
                TopoDS_Shape unique;
                int n = 0;
                for (TopExp_Explorer ex(p.shape, TopAbs_EDGE); ex.More(); ex.Next()) {
                    ++n;
                    unique = ex.Current();
                }
                if (n == 1 && !unique.IsNull()) {
                    lastPrismGeneratedEdges.push_back({p.fromSeed, unique});
                }
            }
        }
    }
    // UpTo* / taper: BRepFeat_MakePrism and draft makers keep history internal
    // (makeElementPrismUntil). Cannot name Generated faces without inventing
    // FaceN — leave lastPrismGenerated as-is (empty for this side).
    refreshNamedIndices(prism);
}

void FeatureExtrude::captureBooleanHistory(void* occMaker,
                                           const TopoShape& result,
                                           const TopoShape& baseShape,
                                           const TopoShape& tool)
{
    (void)tool;
    if (result.isNull()) {
        return;
    }
    // Prefer fromMaker: BRepAlgoAPI_BooleanOperation::History() is not
    // guaranteed on this OCCT. Generated/Modified/IsDeleted only.
    auto* maker = occMaker ? static_cast<BRepBuilderAPI_MakeShape*>(occMaker) : nullptr;
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (maker) {
        std::vector<PrismFaceSeed> next;
        next.reserve(lastPrismGenerated.size());
        for (const auto& p : lastPrismGenerated) {
            if (p.shape.IsNull()) {
                continue;
            }
            bool any = false;
            for (TopTools_ListIteratorOfListOfShape it(maker->Modified(p.shape)); it.More();
                 it.Next()) {
                if (it.Value().ShapeType() == TopAbs_FACE) {
                    next.push_back({p.fromSeed, it.Value()});
                    any = true;
                }
            }
            if (!any) {
                for (TopTools_ListIteratorOfListOfShape it(maker->Generated(p.shape)); it.More();
                     it.Next()) {
                    if (it.Value().ShapeType() == TopAbs_FACE) {
                        next.push_back({p.fromSeed, it.Value()});
                        any = true;
                    }
                }
            }
            if (!any && result.findShape(p.shape) > 0) {
                next.push_back(p);
            }
            else if (!any) {
                // Refine hole: findShape/IsSame miss. Partner + unique
                // coplanar (indexOnPublishedPartnerCoplanar) still names a unique cap.
                // 0 or N stay dropped (I13).
                const App::ElementIndex idx = Part::indexOnPublishedPartnerCoplanar(result, p.shape);
                if (Part::isNamedIndex(idx) && idx.type == "Face") {  // PD23-E1
                    const TopoDS_Shape located = result.findShape(TopAbs_FACE, idx.index);
                    if (!located.IsNull()) {
                        next.push_back({p.fromSeed, located});
                    }
                }
            }
        }
        lastPrismGenerated.swap(next);

        // Remap prism/tool EDGES (not only faces) onto the published solid.
        // 1 image → keep. Several images → unnamed (I13). Pocket afterExecute
        // then zips lastNamedEdgeIndices with this feature's getID().
        std::vector<PrismFaceSeed> nextEdges;
        nextEdges.reserve(lastPrismGeneratedEdges.size());
        for (const auto& p : lastPrismGeneratedEdges) {
            if (p.shape.IsNull()) {
                continue;
            }
            const std::vector<TopoDS_Shape> images = Part::uniqueModifiedThenGeneratedEdgeImages(maker, p.shape, result);
            if (images.size() == 1) {
                nextEdges.push_back({p.fromSeed, images.front()});
            }
            // size 0 or >1 → leave unnamed (do not invent EdgeN).
        }

        // Body-tip Fillet.Base=(Pocket, EdgeN) picks surviving *base* corners
        // (Automated: vertical outer pad edge on pocket.Shape), not the hole
        // seam. Map unique base Edge Bindings through the same maker.
        if (graph) {
            Part::Feature* baseObj = getBaseObject(true);
            if (baseObj) {
                const App::ObjectId baseId = static_cast<App::ObjectId>(baseObj->getID());
                std::unordered_set<App::SemanticHandle> seenEdge;
                for (const App::SemanticBinding& b : graph->allBindings()) {
                    if (b.feature != baseId || !Part::isNamedIndex(b.index)  // PD32-E3
                        || b.index.type != "Edge"
                        || !b.stid.valid() || !seenEdge.insert(b.stid.handle).second) {
                        continue;
                    }
                    TopoDS_Shape edge = baseShape.findShape(TopAbs_EDGE, b.index.index);
                    if (edge.IsNull()) {
                        continue;
                    }
                    const std::vector<TopoDS_Shape> images =
                        Part::uniqueModifiedThenGeneratedEdgeImages(maker, edge, result);
                    if (images.size() != 1) {
                        continue;
                    }
                    bool dup = false;
                    for (const auto& p : nextEdges) {
                        if (Part::sameOccShape(p.shape, images.front())) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) {
                        nextEdges.push_back({b.stid, images.front()});
                    }
                }
            }
        }
        lastPrismGeneratedEdges.swap(nextEdges);
    }

    if (maker && graph && !lastCutRemnant.valid()) {
        Part::Feature* baseObj = getBaseObject(true);
        if (baseObj) {
            const App::ObjectId baseId = static_cast<App::ObjectId>(baseObj->getID());
            std::deque<TopoDS_Shape> held;
            std::vector<App::SemanticId> cutSeeds;
            std::unordered_set<App::SemanticHandle> cutSeen;
            std::unordered_set<App::SemanticHandle> seen;
            for (const App::SemanticBinding& b : graph->allBindings()) {
                if (b.feature != baseId || !Part::isNamedIndex(b.index)  // PD32-E4
                    || b.index.type != "Face"
                    || !b.stid.valid() || !seen.insert(b.stid.handle).second) {
                    continue;
                }
                TopoDS_Shape face = baseShape.findShape(TopAbs_FACE, b.index.index);
                if (face.IsNull()) {
                    continue;
                }
                held.push_back(face);
                // Remnant is a Face the boolean actually Modified/Split.
                // Unmodified survivors (:U) are EventKind::Modified in
                // fromMaker — counting them would always be >1 (I13).
                if (maker->Modified(held.back()).Extent() < 1) {
                    continue;
                }
                if (cutSeen.insert(b.stid.handle).second) {
                    cutSeeds.push_back(b.stid);
                }
            }
            // Cheap remnant: exactly one named base Face is Modified/Split.
            // Several modified faces → unnamed unless the Profile sketch
            // uniquely names one of those Faces (sketch-on-cap ThroughAll).
            // Never first-Binding-wins. Never sequential FaceN.
            if (cutSeeds.size() == 1) {
                lastCutRemnant = cutSeeds.front();
            }
            else if (cutSeeds.size() > 1) {
                const App::SemanticId namedCap = uniqueNamedProfileFace(Profile.getValue());
                if (namedCap.valid() && cutSeen.count(namedCap.handle)) {
                    for (const App::SemanticId& s : cutSeeds) {
                        if (s.handle == namedCap.handle) {
                            lastCutRemnant = s;
                            break;
                        }
                    }
                }
            }
        }
    }
    refreshNamedIndices(result);
}

void FeatureExtrude::publishSemanticHistory(const TopoShape& published)
{
    // EM14-S1 / PD23-S1: Pad/Pocket Bindings publish via afterExecute only —
    // no stampElementMap dual-write here (Loft/Pipe/Helix only). Do not broaden
    // ElementMap stamp onto Extrude without TESTS re-gate.
    refreshNamedIndices(published);
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph || published.isNull()
        || (lastPrismGenerated.empty() && lastPrismGeneratedEdges.empty())) {
        return;
    }
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    // Keep the Pad/Pocket publisher aligned with afterExecute's 1:1 seed
    // zipping. A two-sided prism can report two images for one curve (and a
    // top edge plus its partner can do the same). Applying those rows as
    // independent Generated events makes the first row win in the binding
    // table, while the remaining rows are invisible to the request zipper.
    // Region seeds intentionally remain 1:N for the two cap faces.
    Part::HistoryTable faceSingles;
    Part::HistoryTable regionRows;
    Part::HistoryTable edgeRows;
    faceSingles.reserve(lastPrismGenerated.size());
    regionRows.reserve(lastPrismGenerated.size());
    edgeRows.reserve(lastPrismGeneratedEdges.size());
    for (const auto& p : lastPrismGenerated) {
        Part::HistoryRecord rec;
        rec.fromSeed = p.fromSeed;
        rec.kind = App::EventKind::Generated;
        rec.outputKind = App::SemanticKind::Face;
        rec.toIndex = Part::indexOnPublishedPartnerCoplanar(published, p.shape);
        if (p.fromSeed.kind == App::SemanticKind::Region) {
            regionRows.push_back(rec);
        }
        else {
            faceSingles.push_back(rec);
        }
    }
    for (const auto& p : lastPrismGeneratedEdges) {
        Part::HistoryRecord rec;
        rec.fromSeed = p.fromSeed;
        rec.kind = App::EventKind::Generated;
        rec.outputKind = App::SemanticKind::Edge;
        rec.toIndex = Part::indexOnPublishedPartnerCoplanar(published, p.shape);
        edgeRows.push_back(rec);
    }

    const Part::HistoryTable uniqueFaces =
        Part::SemanticHistoryAdapter::uniqueOneImageGenerated(faceSingles);
    const Part::HistoryTable uniqueEdges =
        Part::SemanticHistoryAdapter::uniqueOneImageGenerated(edgeRows);
    Part::HistoryTable candidates;
    candidates.reserve(uniqueFaces.size() + regionRows.size() + uniqueEdges.size());
    candidates.insert(candidates.end(), uniqueFaces.begin(), uniqueFaces.end());
    candidates.insert(candidates.end(), regionRows.begin(), regionRows.end());
    candidates.insert(candidates.end(), uniqueEdges.begin(), uniqueEdges.end());

    // A published slot still has exactly one semantic owner, including when
    // a region cap and a side/edge history path converge on it. Same-owner
    // duplicate rows are collapsed; conflicting owners are all refused.
    std::map<std::string, App::SemanticHandle> slotOwner;
    std::unordered_set<std::string> conflictedSlots;
    for (const Part::HistoryRecord& rec : candidates) {
        if (!rec.fromSeed.valid() || !Part::isNamedIndex(rec.toIndex)) {
            continue;
        }
        const std::string slot = rec.toIndex.toString();
        const auto [it, inserted] = slotOwner.emplace(slot, rec.fromSeed.handle);
        if (!inserted && it->second != rec.fromSeed.handle) {
            conflictedSlots.insert(slot);
        }
    }

    Part::HistoryTable table;
    std::unordered_set<std::string> emittedSlots;
    table.reserve(candidates.size());
    for (const Part::HistoryRecord& rec : candidates) {
        if (!rec.fromSeed.valid() || !Part::isNamedIndex(rec.toIndex)) {
            continue;
        }
        const std::string slot = rec.toIndex.toString();
        if (conflictedSlots.count(slot) != 0 || !emittedSlots.insert(slot).second) {
            continue;
        }
        table.push_back(rec);
    }

    // Keep the afterExecute request shape-aligned even when an ambiguous
    // source was refused above. Empty placeholders are intentional: dropping
    // them would shift the next curve/region onto the wrong published slot.
    auto acceptedIndices = [&](App::SemanticHandle handle, const char* type) {
        std::vector<App::ElementIndex> indices;
        for (const Part::HistoryRecord& rec : table) {
            if (rec.fromSeed.handle == handle && rec.toIndex.type == type) {
                indices.push_back(rec.toIndex);
            }
        }
        return indices;
    };
    lastNamedFaceIndices.clear();
    std::unordered_set<App::SemanticHandle> seenFaceSeeds;
    for (const auto& p : lastPrismGenerated) {
        if (!p.fromSeed.valid() || !seenFaceSeeds.insert(p.fromSeed.handle).second) {
            continue;
        }
        const auto indices = acceptedIndices(p.fromSeed.handle, "Face");
        if (p.fromSeed.kind == App::SemanticKind::Region) {
            for (std::size_t cap = 0; cap < 2; ++cap) {
                lastNamedFaceIndices.push_back(cap < indices.size()
                                                   ? indices[cap]
                                                   : App::ElementIndex{});
            }
        }
        else {
            lastNamedFaceIndices.push_back(
                indices.size() == 1 ? indices.front() : App::ElementIndex{}
            );
        }
    }
    lastNamedEdgeIndices.clear();
    std::unordered_set<App::SemanticHandle> seenEdgeSeeds;
    for (const auto& p : lastPrismGeneratedEdges) {
        if (!p.fromSeed.valid() || !seenEdgeSeeds.insert(p.fromSeed.handle).second) {
            continue;
        }
        const auto indices = acceptedIndices(p.fromSeed.handle, "Edge");
        lastNamedEdgeIndices.push_back(
            indices.size() == 1 ? indices.front() : App::ElementIndex{}
        );
    }

    std::vector<App::SemanticId> seeds;
    std::unordered_set<App::SemanticHandle> seenSeeds;
    for (const auto& p : lastPrismGenerated) {
        if (p.fromSeed.valid() && seenSeeds.insert(p.fromSeed.handle).second) {
            seeds.push_back(p.fromSeed);
        }
    }
    for (const auto& p : lastPrismGeneratedEdges) {
        if (p.fromSeed.valid() && seenSeeds.insert(p.fromSeed.handle).second) {
            seeds.push_back(p.fromSeed);
        }
    }
    Part::SemanticHistoryAdapter::applyHistory(
        graph,
        static_cast<App::ObjectId>(getID()),
        eval,
        getAddSubType() == FeatureAddSub::Type::Subtractive ? "Pocket" : "Pad",
        seeds,
        table);
}


short FeatureExtrude::mustExecute() const
{
    if (Placement.isTouched() || SideType.isTouched() || Type.isTouched() || Type2.isTouched()
        || Length.isTouched() || Length2.isTouched() || TaperAngle.isTouched()
        || TaperAngle2.isTouched() || UseCustomVector.isTouched() || Direction.isTouched()
        || ReferenceAxis.isTouched() || AlongSketchNormal.isTouched() || Offset.isTouched()
        || Offset2.isTouched() || StartType.isTouched() || StartOffset.isTouched()
        || StartReference.isTouched() || UpToFace.isTouched() || UpToFace2.isTouched()
        || UpToShape.isTouched() || UpToShape2.isTouched() || UseLegacyTaperDirection.isTouched()) {
        return 1;
    }
    if (const App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
        graph && isValid() && !Shape.getShape().isNull()
        && SemanticEmitter::needsSemanticRepublish(graph, static_cast<App::ObjectId>(getID()))) {
        return 1;
    }
    return ProfileBased::mustExecute();
}

Base::Vector3d FeatureExtrude::computeDirection(const Base::Vector3d& sketchVector, bool inverse)
{
    (void)inverse;
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
                case FeatureAddSub::Type::Additive:
                    extrudeDirection = dir;
                    break;
                case FeatureAddSub::Type::Subtractive:
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
    if (UseCustomVector.getValue()) {
        AlongSketchNormal.setReadOnly(false);
    }

    // explicitly set the Direction so that the dialog shows also the used direction
    // if the sketch's normal vector was used
    Direction.setValue(extrudeDirection);
    return extrudeDirection;
}

bool FeatureExtrude::hasTaperedAngle() const
{
    return fabs(TaperAngle.getValue()) > Base::toRadians(Precision::Angular())
        || fabs(TaperAngle2.getValue()) > Base::toRadians(Precision::Angular());
}

void FeatureExtrude::onChanged(const App::Property* prop)
{
    if (prop == &Midplane && !isRestoring() && !migratingDeprecatedProperties) {
        // Deprecation notice: Midplane property is deprecated and has been replaced by SideType in
        // FreeCAD 1.1 when FeatureExtrude was refactored.
        const char* impliedSideType = Midplane.getValue() ? "Symmetric" : "One side";

        // Scripts routinely assign every property, so only scream when the write actually
        // asks for something SideType is not already saying.
        if (SideType.getValueAsString() != std::string(impliedSideType)) {
            App::DocumentObject* obj = Profile.getValue();
            auto baseName = obj ? obj->getNameInDocument() : "";
            Base::Console().warning(
                "The 'Midplane' property being set for the extrusion of %s is deprecated and has "
                "been replaced by the 'SideType' property in FeatureExtrude; assuming "
                "SideType='%s'. Please update your script, this property will be removed in a"
                " future version.\n",
                baseName,
                impliedSideType
            );
            SideType.setValue(impliedSideType);
        }
    }
    ProfileBased::onChanged(prop);
}

TopoShape FeatureExtrude::makeShellFromUpToShape(TopoShape shape, TopoShape sketchshape, gp_Dir& dir)
{

    // Find nearest/furthest face
    std::vector<Part::cutTopoShapeFaces> cfaces = Part::findAllFacesCutBy(shape, sketchshape, dir);
    if (cfaces.empty()) {
        dir = -dir;
        cfaces = Part::findAllFacesCutBy(shape, sketchshape, dir);
    }

    if (cfaces.empty()) {
        return shape;
    }

    struct Part::cutTopoShapeFaces* nearFace {};
    struct Part::cutTopoShapeFaces* farFace {};
    nearFace = farFace = &cfaces.front();
    for (auto& face : cfaces) {
        if (face.distsq > farFace->distsq) {
            farFace = &face;
        }
        else if (face.distsq < nearFace->distsq) {
            nearFace = &face;
        }
    }

    if (nearFace != farFace) {
        std::vector<TopoShape> faceList;
        for (auto& face : shape.getSubTopoShapes(TopAbs_FACE)) {
            if (!(face == farFace->face)) {
                // don't use the last face so the shell is open
                // and OCC works better
                faceList.push_back(face);
            }
        }
        return shape.makeElementCompound(faceList);
    }
    return shape;
}

void FeatureExtrude::updateProperties()
{
    std::string sideTypeVal = SideType.getValueAsString();
    std::string methodSide1 = Type.getValueAsString();
    std::string methodSide2 = Type2.getValueAsString();

    bool isLength1Enabled = false;
    bool isTaper1Visible = false;
    bool isUpToFace1Enabled = false;
    bool isUpToShape1Enabled = false;
    bool isOffset1Enabled = false;

    bool isType2Enabled = false;
    bool isLength2Enabled = false;
    bool isTaper2Visible = false;
    bool isUpToFace2Enabled = false;
    bool isUpToShape2Enabled = false;
    bool isOffset2Enabled = false;

    bool currentAlongSketchNormalEnabled = false;

    auto configureSideProperties = [&](const std::string& method,
                                       bool& lengthEnabled,
                                       bool& taperVisible,
                                       bool& upToFaceEnabled,
                                       bool& upToShapeEnabled,
                                       bool& localAlongSketchNormal,
                                       bool& localOffset) {
        if (method == "Length") {
            lengthEnabled = true;
            taperVisible = true;
            localAlongSketchNormal = true;
        }
        else if (method == "UpToFace") {
            upToFaceEnabled = true;
            localOffset = true;
        }
        else if (method == "UpToShape") {
            upToShapeEnabled = true;
            localOffset = true;
        }
        else if (method == "UpToLast" || method == "UpToFirst") {
            localOffset = true;
        }
        else if (method == "ThroughAll") {
            taperVisible = true;
        }
    };

    if (sideTypeVal == "One side") {
        bool side1ASN = false;
        configureSideProperties(
            methodSide1,
            isLength1Enabled,
            isTaper1Visible,
            isUpToFace1Enabled,
            isUpToShape1Enabled,
            side1ASN,
            isOffset1Enabled
        );
        currentAlongSketchNormalEnabled = side1ASN;
    }
    else if (sideTypeVal == "Two sides") {
        isType2Enabled = true;

        bool side1ASN = false;
        configureSideProperties(
            methodSide1,
            isLength1Enabled,
            isTaper1Visible,
            isUpToFace1Enabled,
            isUpToShape1Enabled,
            side1ASN,
            isOffset1Enabled
        );

        bool side2ASN = false;
        configureSideProperties(
            methodSide2,
            isLength2Enabled,
            isTaper2Visible,
            isUpToFace2Enabled,
            isUpToShape2Enabled,
            side2ASN,
            isOffset2Enabled
        );

        currentAlongSketchNormalEnabled = side1ASN || side2ASN;  // Enable if either side needs it
    }
    else if (sideTypeVal == "Symmetric") {
        bool symASN = false;
        configureSideProperties(
            methodSide1,
            isLength1Enabled,
            isTaper1Visible,
            isUpToFace1Enabled,
            isUpToShape1Enabled,
            symASN,
            isOffset1Enabled
        );
        currentAlongSketchNormalEnabled = symASN;
    }

    Length.setReadOnly(!isLength1Enabled);
    TaperAngle.setReadOnly(!isTaper1Visible);
    UpToFace.setReadOnly(!isUpToFace1Enabled);
    UpToShape.setReadOnly(!isUpToShape1Enabled);
    Offset.setReadOnly(!isOffset1Enabled);

    Type2.setReadOnly(!isType2Enabled);
    Length2.setReadOnly(!isLength2Enabled);
    TaperAngle2.setReadOnly(!isTaper2Visible);
    UpToFace2.setReadOnly(!isUpToFace2Enabled);
    UpToShape2.setReadOnly(!isUpToShape2Enabled);
    Offset2.setReadOnly(!isOffset2Enabled);

    const bool isStartOffsetEnabled = std::strcmp(StartType.getValueAsString(), "Profile plane") != 0;
    StartOffset.setReadOnly(!isStartOffsetEnabled);
    StartReference.setReadOnly(std::strcmp(StartType.getValueAsString(), "Reference") != 0);

    AlongSketchNormal.setReadOnly(!currentAlongSketchNormalEnabled);
}

void FeatureExtrude::setupObject()
{
    ProfileBased::setupObject();
}

double FeatureExtrude::getStartOffset() const
{
    const char* startType = StartType.getValueAsString();
    if (std::strcmp(startType, "Profile plane") == 0) {
        return 0.0;
    }

    gp_Dir dir = Base::convertTo<gp_Dir>(Direction.getValue());
    if (Reversed.getValue()) {
        dir.Reverse();
    }
    if (std::strcmp(startType, "Offset") == 0) {
        return StartOffset.getValue();
    }

    TopLoc_Location identity;
    return getStartReferenceOffset(
        getTopoShapeVerifiedFace(),
        StartReference,
        dir,
        StartOffset.getValue(),
        identity
    );
}

App::DocumentObjectExecReturn* FeatureExtrude::buildExtrusion(ExtrudeOptions options)
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    clearSemanticCapture();

    bool makeface = options.testFlag(ExtrudeOption::MakeFace);
    bool fuse = options.testFlag(ExtrudeOption::MakeFuse);
    bool inverseDirection = options.testFlag(ExtrudeOption::InverseDirection);

    std::string Sidemethod(SideType.getValueAsString());
    std::string method(Type.getValueAsString());
    std::string method2(Type2.getValueAsString());

    // R1/R2: resolve each named UpToFace before its prism. Dual-write FaceN-only
    // links (no semantic seed) keep the maker (I7). A seeded link must resolve to
    // exactly one same-kind Binding on its linked feature; never run a maker with
    // a stale FaceN fallback or an accepted set where one termination is required.
    const OpcodeRoleId upToFaceRole = addSubType == FeatureAddSub::Type::Subtractive
        ? OpcodeRoleId::PocketUpToFace
        : OpcodeRoleId::PadUpToFace;
    std::string resolvedUpToFace1;
    std::string resolvedUpToFace2;
    const auto upToFaceResolved = [&](const char* side,
                                      const App::PropertyLinkSub& target,
                                      std::string& resolvedSubname) {
        resolvedSubname.clear();
        if (std::strcmp(side, "UpToFace") != 0) {
            return true;
        }
        App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
        if (!graph) {
            return true;
        }
        bool sawSeed = false;
        bool sawFaceSeed = false;
        for (const App::SemanticReference& ref : target.getSemanticRefs()) {
            if (!ref.seed.valid()) {
                continue;
            }
            sawSeed = true;
            if (ref.seed.kind != App::SemanticKind::Face || ref.kind != App::SemanticKind::Face
                || sawFaceSeed) {
                return false;
            }
            sawFaceSeed = true;
            if (!target.getValue()) {
                return false;
            }
            App::ReferenceRequirement requirement = requirementFor(upToFaceRole);
            requirement.acceptedReducers = {ref.reducer};
            const App::ResolutionResult result =
                App::SemanticResolver::resolve(ref, *graph, &requirement);
            if (result.state != App::ResolutionState::Resolved || result.bindings.size() != 1) {
                return false;
            }
            const App::SemanticBinding& binding = result.bindings.front();
            if (binding.feature != static_cast<App::ObjectId>(target.getValue()->getID())
                || binding.stid != ref.seed
                || !Part::isNamedIndex(binding.index)  // PD32-E5
                || binding.index.type != "Face") {
                return false;
            }
            resolvedSubname = binding.index.toString();
        }
        return !sawSeed || sawFaceSeed;
    };
    if (!upToFaceResolved(method.c_str(), UpToFace, resolvedUpToFace1)
        || !upToFaceResolved(method2.c_str(), UpToFace2, resolvedUpToFace2)) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "UpToFace Missing/Ambiguous/Incompatible; maker skipped. No FaceN fallback."
        ));
    }

    // Validate parameters
    double L = method == "ThroughAll" ? getThroughAllLength()
        : method == "Length"          ? Length.getValue()
                                      : 0.0;
    double L2 = Sidemethod == "Two sides" ? method2 == "ThroughAll" ? getThroughAllLength()
            : method2 == "Length"                                   ? Length2.getValue()
                                                                    : 0.0
                                          : 0.0;

    if ((Sidemethod == "One side" && method == "Length")
        || (Sidemethod == "Two sides" && method == "Length" && method2 == "Length")) {

        if (std::abs(L + L2) < Precision::Confusion()) {
            if (addSubType == FeatureAddSub::Type::Additive) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Cannot create a pad with a total length of zero.")
                );
            }
            else {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Cannot create a pocket with a total length of zero.")
                );
            }
        }
    }

    Part::Feature* obj = nullptr;
    TopoShape sketchshape;
    try {
        obj = getVerifiedObject();
        if (makeface) {
            sketchshape = getTopoShapeVerifiedFace();
        }
        else {
            std::vector<TopoShape> shapes;
            bool hasEdges = false;
            auto subs = getProfileSubValuesForMaker();
            if (subs.empty()) {
                subs.emplace_back("");
            }
            bool failed = false;
            for (auto& sub : subs) {
                if (sub.empty() && subs.size() > 1) {
                    continue;
                }
                TopoShape shape = Part::Feature::getTopoShape(
                    obj,
                    Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                        | Part::ShapeOption::Transform,
                    sub.c_str()
                );

                if (shape.isNull()) {
                    FC_ERR(
                        getFullName()
                        << ": failed to get profile shape " << obj->getFullName() << "." << sub
                    );
                    failed = true;
                }
                hasEdges = hasEdges || shape.hasSubShape(TopAbs_EDGE);
                shapes.push_back(shape);
            }
            if (failed) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Failed to obtain profile shape")
                );
            }
            if (hasEdges) {
                sketchshape.makeElementWires(shapes);
            }
            else {
                sketchshape.makeElementCompound(
                    shapes,
                    nullptr,
                    TopoShape::SingleShapeCompoundCreationPolicy::returnShape
                );
            }
        }
    }
    catch (const Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    // if the Base property has a valid shape, fuse the prism into it
    TopoShape base = getBaseTopoShape(true);

    // get the normal vector of the sketch
    Base::Vector3d SketchVector = getProfileNormal();

    try {
        this->positionByPrevious();
        auto invObjLoc = getLocation().Inverted();

        auto invTrsf = invObjLoc.Transformation();

        base.move(invObjLoc);

        Base::Vector3d paddingDirection = computeDirection(SketchVector, inverseDirection);

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
        if (factor < Precision::Confusion()) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Creation failed because direction is orthogonal to sketch's normal vector"
            ));
        }

        // perform the length correction if not along custom vector
        if (AlongSketchNormal.getValue()) {
            L = L / factor;
            L2 = L2 / factor;
        }

        // explicitly set the Direction so that the dialog shows also the used direction
        // if the sketch's normal vector was used
        Direction.setValue(paddingDirection);

        dir.Transform(invTrsf);
        if (Reversed.getValue()) {
            dir.Reverse();
        }

        if (sketchshape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Creating a face from sketch failed")
            );
        }
        sketchshape.move(invObjLoc);

        const char* startType = StartType.getValueAsString();
        const double startOffset = std::strcmp(startType, "Profile plane") == 0 ? 0.0
            : std::strcmp(startType, "Offset") == 0
            ? StartOffset.getValue()
            : getStartReferenceOffset(sketchshape, StartReference, dir, StartOffset.getValue(), invObjLoc);
        const bool useLegacyTaperDirection = UseLegacyTaperDirection.getValue();
        TopoShape startSketch
            = moveProfileToStart(sketchshape, dir, startOffset, useLegacyTaperDirection);

        // Preserve the old deep-copy path for restored files because it can affect both generated
        // topology and taper direction. New features reuse the profile for each side.
        auto profileForSide = [useLegacyTaperDirection](const TopoShape& profile) {
            return useLegacyTaperDirection ? profile.makeElementCopy() : profile;
        };
        std::vector<TopoShape> prisms;  // Stores prisms, all in global CS
        double taper1 = TaperAngle.getValue();
        double offset1 = Offset.getValue();

        if (Sidemethod == "One side") {
            TopoShape prism1 = generateSingleExtrusionSide(
                startSketch,
                method,
                L,
                taper1,
                UpToFace,
                resolvedUpToFace1,
                UpToShape,
                dir,
                offset1,
                makeface,
                base,
                invObjLoc
            );
            prisms.push_back(prism1);
        }
        else if (Sidemethod == "Symmetric") {
            // For Length mode, we are not doing a mirror, but we extrude along the same axis
            // in both directions as it is what users expect.
            if (method == "Length") {
                if (std::fabs(taper1) > Precision::Angular()) {
                    // TAPERED case: We must create two separate prisms and fuse them
                    // to ensure the taper originates correctly from the sketch plane in both
                    // directions.
                    L /= 2.0;
                    TopoShape prism1 = generateSingleExtrusionSide(
                        profileForSide(startSketch),
                        method,
                        L,
                        taper1,
                        UpToFace,
                        resolvedUpToFace1,
                        UpToShape,
                        dir,
                        offset1,
                        makeface,
                        base,
                        invObjLoc
                    );
                    if (!prism1.isNull() && !prism1.getShape().IsNull()) {
                        prisms.push_back(prism1);
                    }

                    gp_Dir dir2 = dir;
                    dir2.Reverse();
                    TopoShape prism2 = generateSingleExtrusionSide(
                        profileForSide(startSketch),
                        method,
                        L,
                        taper1,
                        UpToFace,
                        resolvedUpToFace1,
                        UpToShape,
                        dir2,
                        offset1,
                        makeface,
                        base,
                        invObjLoc
                    );
                    if (!prism2.isNull() && !prism2.getShape().IsNull()) {
                        prisms.push_back(prism2);
                    }
                }
                else {
                    // NON-TAPERED case: We can optimize by creating a single prism.
                    // Translate the sketch to the start position (-L/2) and extrude by the full
                    // length L.
                    gp_Trsf start_transform;
                    start_transform.SetTranslation(gp_Vec(dir).Reversed() * (L / 2.0));

                    TopoShape moved_sketch = profileForSide(startSketch);
                    moved_sketch.move(start_transform);

                    TopoShape prism1 = generateSingleExtrusionSide(
                        moved_sketch,
                        method,
                        L,
                        taper1,
                        UpToFace,
                        resolvedUpToFace1,
                        UpToShape,
                        dir,
                        offset1,
                        makeface,
                        base,
                        invObjLoc
                    );
                    if (!prism1.isNull() && !prism1.getShape().IsNull()) {
                        prisms.push_back(prism1);
                    }
                }
            }
            else {
                // For "UpToFace", "UpToShape", etc., mirror the result.
                TopoShape prism1 = generateSingleExtrusionSide(
                    startSketch,
                    method,
                    L,
                    taper1,
                    UpToFace,
                    resolvedUpToFace1,
                    UpToShape,
                    dir,
                    offset1,
                    makeface,
                    base,
                    invObjLoc
                );
                prisms.push_back(prism1);

                // Prism 2: Mirror prism1 across the sketch plane.
                // The mirror plane's normal must be the sketch normal, not the extrusion direction.
                gp_Dir sketchNormalDir(SketchVector.x, SketchVector.y, SketchVector.z);
                sketchNormalDir.Transform(invTrsf);  // Transform to global CS, like 'dir' was.

                Base::Vector3d sketchCenter = startSketch.getBoundBox().GetCenter();
                gp_Ax2 mirrorPlane(
                    gp_Pnt(sketchCenter.x, sketchCenter.y, sketchCenter.z),
                    sketchNormalDir
                );
                TopoShape prism2 = prism1.makeElementMirror(mirrorPlane);
                prisms.push_back(prism2);
            }
        }
        else if (Sidemethod == "Two sides") {
            double taper2 = TaperAngle2.getValue();
            double offset2 = Offset2.getValue();
            gp_Dir dir2 = dir;
            dir2.Reverse();
            bool noTaper = std::fabs(taper1) < Precision::Angular()
                && std::fabs(taper2) < Precision::Angular();
            bool method1LengthBased = method == "Length" || method == "ThroughAll";
            bool method2LengthBased = method2 == "Length" || method2 == "ThroughAll";
            bool hasStartOffset = std::fabs(startOffset) > Precision::Confusion();

            if (!hasStartOffset && method1LengthBased && method2 != "UpToFirst" && noTaper) {
                gp_Trsf start_transform;
                start_transform.SetTranslation(gp_Vec(dir) * L);

                TopoShape moved_sketch = sketchshape.makeElementCopy();
                moved_sketch.move(start_transform);
                TopoShape prism = generateSingleExtrusionSide(
                    moved_sketch,
                    method2,
                    L + L2,
                    0.0,
                    UpToFace2,
                    resolvedUpToFace2,
                    UpToShape2,
                    dir2,
                    offset2,
                    makeface,
                    base,
                    invObjLoc
                );
                if (!prism.isNull() && !prism.getShape().IsNull()) {
                    prisms.push_back(prism);
                }
            }
            else if (!hasStartOffset && method2LengthBased && method != "UpToFirst" && noTaper) {
                gp_Trsf start_transform;
                start_transform.SetTranslation(gp_Vec(dir).Reversed() * L2);

                TopoShape moved_sketch = sketchshape.makeElementCopy();
                moved_sketch.move(start_transform);
                TopoShape prism = generateSingleExtrusionSide(
                    moved_sketch,
                    method,
                    L + L2,
                    0.0,
                    UpToFace,
                    resolvedUpToFace1,
                    UpToShape,
                    dir,
                    offset1,
                    makeface,
                    base,
                    invObjLoc
                );
                if (!prism.isNull() && !prism.getShape().IsNull()) {
                    prisms.push_back(prism);
                }
            }
            else {
                TopoShape prism1 = generateSingleExtrusionSide(
                    profileForSide(startSketch),
                    method,
                    L,
                    taper1,
                    UpToFace,
                    resolvedUpToFace1,
                    UpToShape,
                    dir,
                    offset1,
                    makeface,
                    base,
                    invObjLoc
                );
                if (!prism1.isNull() && !prism1.getShape().IsNull()) {
                    prisms.push_back(prism1);
                }

                // Side 2
                TopoShape prism2 = generateSingleExtrusionSide(
                    profileForSide(startSketch),
                    method2,
                    L2,
                    taper2,
                    UpToFace2,
                    resolvedUpToFace2,
                    UpToShape2,
                    dir2,
                    offset2,
                    makeface,
                    base,
                    invObjLoc
                );
                if (!prism2.isNull() && !prism2.getShape().IsNull()) {
                    prisms.push_back(prism2);
                }
            }
        }

        // --- Combine generated prisms (all in global CS) ---
        TopoShape prism(0, getDocument()->getStringHasher());
        if (prisms.empty()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "No extrusion geometry was generated.")
            );
        }
        else if (prisms.size() == 1) {
            prism = prisms[0];
        }
        else {
            try {
                prism.makeElementXor(prisms, Part::OpCodes::Extrude);
            }
            catch (const Standard_Failure& e) {
                return new App::DocumentObjectExecReturn(
                    std::string("Failed to xor extrusion sides (OCC): ") + e.GetMessageString()
                );
            }
            catch (const Base::Exception& e) {
                return new App::DocumentObjectExecReturn(
                    std::string("Failed to xor extrusion sides: ") + e.what()
                );
            }
        }

        if (prism.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting fused extrusion is null.")
            );
        }

        // store shape before refinement
        this->rawShape = prism;
        prism = refineShapeIfActive(prism);
        // set the additive shape property for later usage in e.g. pattern
        this->AddSubShape.setValue(prism);
        refreshNamedIndices(prism);

        if (base.shapeType(true) <= TopAbs_SOLID && fuse) {
            prism.Tag = -this->getID();

            // Let's call algorithm computing a fuse operation:
            TopoShape result(0, getDocument()->getStringHasher());
            try {
                const char* maker;
                switch (getAddSubType()) {
                    case FeatureAddSub::Type::Subtractive:
                        maker = Part::OpCodes::Cut;
                        break;
                    default:
                        maker = Part::OpCodes::Fuse;
                }
                std::unique_ptr<BRepAlgoAPI_BooleanOperation> mk;
                if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
                    mk.reset(new FCBRepAlgoAPI_Cut);
                }
                else {
                    mk.reset(new FCBRepAlgoAPI_Fuse);
                }
                TopTools_ListOfShape shapeArguments;
                TopTools_ListOfShape shapeTools;
                shapeArguments.Append(base.getShape());
                shapeTools.Append(prism.getShape());
                mk->SetRunParallel(Standard_True);
                mk->SetArguments(shapeArguments);
                mk->SetTools(shapeTools);
                const double fuzzy = FuzzyTolerance.getValue();
                if (fuzzy > 0.0) {
                    mk->SetFuzzyValue(fuzzy);
                }
                mk->Build();
                result.makeElementShape(*mk, {base, prism}, maker);
                result.makeElementShell(true);
                captureBooleanHistory(mk.get(), result, base, prism);
            }
            catch (Standard_Failure&) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Fusion with base feature failed")
                );
            }
            // we have to get the solids (fuse sometimes creates compounds)
            auto solRes = this->getSolid(result);
            // lets check if the result is a solid
            if (solRes.isNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid")
                );
            }

            // store shape before refinement
            this->rawShape = result;
            solRes = refineShapeIfActive(result);

            if (!isSingleSolidRuleSatisfied(solRes.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            TopoShape published = getSolid(solRes);
            this->Shape.setValue(published);
            publishSemanticHistory(published);
        }
        else if (prism.hasSubShape(TopAbs_SOLID)) {
            if (prism.countSubShapes(TopAbs_SOLID) > 1) {
                prism.makeElementFuse(prism.getSubTopoShapes(TopAbs_SOLID));
            }

            // store shape before refinement
            this->rawShape = prism;
            prism = refineShapeIfActive(prism);
            if (!isSingleSolidRuleSatisfied(prism.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            prism = getSolid(prism);
            this->Shape.setValue(prism);
            publishSemanticHistory(prism);
        }
        else {
            // store shape before refinement
            this->rawShape = prism;
            prism = refineShapeIfActive(prism);
            if (!isSingleSolidRuleSatisfied(prism.getShape())) {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Result has multiple solids: enable 'Allow Compound' in the active body."
                ));
            }
            this->Shape.setValue(prism);
            publishSemanticHistory(prism);
        }

        // eventually disable some settings that are not valid for the current method
        updateProperties();

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        if (std::string(e.GetMessageString()) == "TopoDS::Face") {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple faces in a sketch are not allowed."
            ));
        }
        else {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
}

TopoShape FeatureExtrude::generateSingleExtrusionSide(
    const TopoShape& sketchshape,
    const std::string& method,
    double length,
    double taperAngleDeg,
    App::PropertyLinkSub& upToFacePropHandle,
    const std::string& resolvedUpToFaceSubname,
    App::PropertyLinkSubList& upToShapePropHandle,
    gp_Dir dir,
    double offsetVal,
    bool makeFace,
    const TopoShape& base,
    TopLoc_Location& invObjLoc
)
{
    TopoShape prism(0, getDocument()->getStringHasher());

    if (method == "UpToFirst" || method == "UpToLast" || method == "UpToFace"
        || method == "UpToShape") {
        // Note: This will return an unlimited planar face if support is a datum plane
        TopoShape supportface = getTopoShapeSupportFace();
        supportface.move(invObjLoc);

        if (!supportface.hasSubShape(TopAbs_WIRE)) {
            supportface = TopoShape();
        }

        TopoShape upToShape;
        int faceCount = 1;
        // Find a valid shape, face or datum plane to extrude up to
        if (method == "UpToFace") {
            getUpToFaceFromLinkSub(
                upToShape,
                upToFacePropHandle,
                resolvedUpToFaceSubname.empty() ? nullptr : &resolvedUpToFaceSubname
            );
            upToShape.move(invObjLoc);
        }
        else if (method == "UpToShape") {
            faceCount = getUpToShapeFromLinkSubList(upToShape, upToShapePropHandle);
            upToShape.move(invObjLoc);
            if (faceCount == 0) {
                // No shape selected, use the base
                upToShape = base;
            }
        }

        if (faceCount == 1) {
            getUpToFace(upToShape, base, sketchshape, method, dir);
            addOffsetToFace(upToShape, dir, offsetVal);
        }
        else {
            if (fabs(offsetVal) > Precision::Confusion()) {
                throw Base::RuntimeError("Extrude: Can only offset one face");
            }
            // open the shell by removing the furthest face
            upToShape = makeShellFromUpToShape(upToShape, sketchshape, dir);
        }

        try {
            TopoShape _base;
            if (addSubType != FeatureAddSub::Type::Subtractive) {
                _base = base;  // avoid issue #16690
            }
            // Main TopoShape::makeElementPrismUntil has no maker out-arg.
            // BRepFeat_MakePrism keeps UpTo* history internal inside that helper;
            // do not invent a 9th arg or FaceN (I10/I13). Length path still
            // captures via BRepPrimAPI_MakePrism + capturePrismMaker.
            prism.makeElementPrismUntil(
                _base,
                sketchshape,
                supportface,
                upToShape,
                dir,
                TopoShape::PrismMode::None,
                true /*CheckUpToFaceLimits.getValue()*/
            );
        }
        catch (Base::Exception&) {
            if (method == "UpToShape" && faceCount > 1) {
                throw Base::RuntimeError(
                    "Extrude: Unable to reach the selected shape, please select faces"
                );
            }
        }
    }
    else if (method == "Length" || method == "ThroughAll") {
        using std::numbers::pi;

        Part::ExtrusionParameters params;
        params.taperAngleFwd = Base::toRadians(taperAngleDeg);
        params.innerWireTaper = Part::InnerWireTaper::SameAsOuter;

        if (std::fabs(params.taperAngleFwd) >= Precision::Angular()
            || std::fabs(params.taperAngleRev) >= Precision::Angular()) {
            if (fabs(params.taperAngleFwd) > pi * 0.5 - Precision::Angular()
                || fabs(params.taperAngleRev) > pi * 0.5 - Precision::Angular()) {
                return prism;
            }
            params.dir = dir;
            params.solid = makeFace;
            params.lengthFwd = length;

            std::vector<TopoShape> drafts;
            Part::ExtrusionHelper::makeElementDraft(
                params,
                sketchshape,
                drafts,
                getDocument()->getStringHasher()
            );
            if (drafts.empty()) {
                return prism;
            }
            prism.makeElementCompound(
                drafts,
                nullptr,
                TopoShape::SingleShapeCompoundCreationPolicy::returnShape
            );
        }
        else {
            // Without taper angle we create a prism because its shells are in every case no
            // B-splines and can therefore be use as support for further features like Pads,
            // Lofts etc. B-spline shells can break certain features, see e.g.
            // https://forum.freecad.org/viewtopic.php?p=560785#p560785 It is better not to use
            // BRepFeat_MakePrism here even if we have a support because the resulting shape
            // creates problems with Pocket
            try {
                BRepPrimAPI_MakePrism mkPrism(sketchshape.getShape(), length * gp_Vec(dir));
                prism.makeElementShape(mkPrism, sketchshape, Part::OpCodes::Extrude);
                capturePrismMaker(&mkPrism, prism, sketchshape);
            }
            catch (Standard_Failure&) {
                throw Base::RuntimeError("FeatureExtrusion: Length: Could not extrude the sketch!");
            }
        }
    }

    return prism;
}

void FeatureExtrude::Restore(Base::XMLReader& reader)
{
    // SideType was introduced in 1.1. If the property exists in the file, restoring the remaining
    // properties below overrides this version-based default.
    UseLegacyTaperDirection.setValue(Base::getVersion(reader.ProgramVersion) >= Base::Version::v1_1);
    ProfileBased::Restore(reader);
}

void FeatureExtrude::onDocumentRestored()
{
    Base::StateLocker migrating(migratingDeprecatedProperties);

    // property Type no longer has TwoLengths.
    if (strcmp(Type.getValueAsString(), "?TwoLengths") == 0) {
        // TwoLengths predates SideType and used the original profile for both taper directions.
        UseLegacyTaperDirection.setValue(false);
        Type.setValue("Length");
        Type2.setValue("Length");
        SideType.setValue("Two sides");

        // The old TwoLengths code path (generatePrism) always extruded in +dir:
        //   offset = -L2 * dir  (Reversed=false) or -L * dir (Reversed=true)
        //   extrude = (L+L2) * dir
        // The new "Two sides" code extrudes Side 1 in dir, Side 2 in -dir,
        // with Reversed toggling the sign of dir. To preserve the same OCC
        // topology (face/edge ordering), we toggle Reversed so the effective
        // extrusion direction matches the old +dir, and swap Length/Length2
        // to keep the correct offset for each side.
        Reversed.setValue(!Reversed.getValue());
        double origL = Length.getValue();
        double origL2 = Length2.getValue();
        Length.setValue(origL2);
        Length2.setValue(origL);

        App::ObjectIdentifier lengthPath(Length);
        App::ObjectIdentifier length2Path(Length2);

        // Rename Length <-> Length2 in all expressions before swapping which expression lives on
        // which property. This avoids cyclic dependencies and fixes cross-object references.
        std::map<App::ObjectIdentifier, App::ObjectIdentifier> renames;
        renames[lengthPath] = length2Path;
        renames[length2Path] = lengthPath;
        getDocument()->renameObjectIdentifiers(renames);

        auto exprL = getExpression(lengthPath);
        auto exprL2 = getExpression(length2Path);
        if (exprL.expression || exprL2.expression) {
            clearExpression(lengthPath);
            clearExpression(length2Path);
            if (exprL.expression) {
                setExpression(length2Path, exprL.expression);
            }
            if (exprL2.expression) {
                setExpression(lengthPath, exprL2.expression);
            }
        }
    }
    else if (Midplane.getValue()) {
        // A missing SideType restores as One side. This distinguishes old Midplane documents from
        // newer scripts that saved both Midplane and the corresponding SideType.
        if (strcmp(SideType.getValueAsString(), "One side") == 0) {
            UseLegacyTaperDirection.setValue(false);
        }
        Midplane.setValue(false);
        SideType.setValue("Symmetric");
    }

    ProfileBased::onDocumentRestored();
}
