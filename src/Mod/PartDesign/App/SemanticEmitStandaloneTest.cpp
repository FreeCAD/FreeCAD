// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *   Copyright (c) 2026 Sauli Kiviranta                                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/
//
// Standalone Rev 3.1 emit scenario: graphFor + EvaluateScope + sketch seed
// ensure + pad/pocket/fillet afterExecute. No Qt / OCCT / FreeCADApp.
// Metric is ResolutionState + provenance family, not FaceN equality.

#include "SemanticOpcode.h"
#include "SketchSemanticSeed.h"

#include <App/SemanticDocumentState.h>
#include <App/SemanticId.h>
#include <App/SemanticLinkSub.h>
#include <App/SemanticReference.h>
#include <App/SemanticTopology.h>
#include <Mod/Sketcher/App/SketchEntityId.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace App;
using namespace PartDesign;
using namespace Sketcher;

namespace
{

int g_failed = 0;
int g_passed = 0;


void check(bool cond, const char* expr, const char* file, int line)
{
    if (cond) {
        ++g_passed;
        return;
    }
    ++g_failed;
    std::cerr << "FAIL " << file << ":" << line << "  " << expr << "\n";
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

SemanticReference makeRef(const SemanticId& seed, CardinalityReducer reducer, SemanticKind kind)
{
    SemanticReference ref;
    ref.seed = seed;
    ref.kind = kind;
    ref.reducer = reducer;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    return ref;
}

void testSketchSeedsAreDocumentHandles()
{
    SemanticDocumentState state;
    int doc = 1;
    int sketchPtr = 10;
    state.bindOwner(&doc);
    state.bindAlias(&sketchPtr);
    CHECK(SemanticEmitter::graphFor(&sketchPtr) == &state.graph());

    SketchEntityIdMap map;
    const auto a = map.append();
    const auto b = map.append();
    const auto c = map.append();
    const auto d = map.append();
    CHECK(a != 0 && b != a);

    SemanticGraph* g = SemanticEmitter::graphFor(&sketchPtr);
    const ObjectId sketch = 10;
    SemanticDocumentState::EvaluateScope scope(state);
    const EvalSerial eval = scope.eval();
    CHECK(eval != 0);

    const SemanticId sA
        = SketchSemanticSeeds::ensureSeedForEntity(*g, sketch, eval, a, SemanticKind::Edge);
    const SemanticId sA2
        = SketchSemanticSeeds::ensureSeedForEntity(*g, sketch, eval, a, SemanticKind::Edge);
    CHECK(sA.valid());
    CHECK(sA.handle == sA2.handle);  // reuse, not a second heap
    CHECK(sA.kind == SemanticKind::Edge);
    CHECK(sA.allocatedBy == sketch);
    CHECK(sA.allocatedRole == SemanticRole::User);

    const SemanticId sB
        = SketchSemanticSeeds::ensureSeedForEntity(*g, sketch, eval, b, SemanticKind::Edge);
    CHECK(sB.handle != sA.handle);

    // Axes / invalid never become seeds.
    CHECK(!SketchSemanticSeeds::ensureSeedForEntity(*g, sketch, eval, 0).valid());
    CHECK(!SketchSemanticSeeds::ensureSeedForEntity(*g, sketch, eval, -1).valid());

    const std::string key = SketchEntityIdMap::regionKey({a, b, c, d}, SketchEntityIdMap::RoleInterior);
    const SemanticId region = SketchSemanticSeeds::ensureRegionSeed(*g, sketch, eval, key);
    CHECK(region.valid());
    CHECK(region.kind == SemanticKind::Region);
    const SemanticId region2 = SketchSemanticSeeds::ensureRegionSeed(*g, sketch, eval, key);
    CHECK(region.handle == region2.handle);

    // Retired handle: Deleted, not recycled into a new seed.
    map.erase(b);
    SketchSemanticSeeds::recordRetired(*g, sketch, eval, b);
    CHECK(g->hasDeletedEvent(sB.handle));
    const SemanticId stillB = SketchSemanticSeeds::findSeed(*g, sketch, b);
    CHECK(stillB.handle == sB.handle);
    const auto next = map.append();
    CHECK(next != b);  // I5 sketch handle
    const SemanticId sNext
        = SketchSemanticSeeds::ensureSeedForEntity(*g, sketch, eval, next, SemanticKind::Edge);
    CHECK(sNext.handle != sB.handle);  // I5 semantic handle

    scope.commit();
}

void testClickableEmitScenario()
{
    SemanticDocumentState state;
    int doc = 1;
    int sketchPtr = 10;
    int padPtr = 20;
    int pocketPtr = 30;
    int filletPtr = 40;
    state.bindOwner(&doc);
    state.bindAlias(&sketchPtr);
    state.bindAlias(&padPtr);
    state.bindAlias(&pocketPtr);
    state.bindAlias(&filletPtr);

    CHECK(SemanticEmitter::graphFor(&padPtr) == &state.graph());
    CHECK(SemanticEmitter::graphFor(&doc) == &state.graph());

    SketchEntityIdMap map;
    const auto h1 = map.append();
    const auto h2 = map.append();
    const auto h3 = map.append();
    const auto h4 = map.append();

    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    const ObjectId pocket = 30;
    const ObjectId fillet = 40;

    SemanticId c1, c2, c3, c4, region;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        SemanticGraph* g = SemanticEmitter::graphFor(&sketchPtr);
        CHECK(scope.eval() != 0);
        auto profile = SketchSemanticSeeds::seedsForProfile(
            *g,
            sketch,
            scope.eval(),
            {h1, h2, h3, h4},
            {SketchEntityIdMap::regionKey({h1, h2, h3, h4}, SketchEntityIdMap::RoleInterior)}
        );
        CHECK(profile.curves.size() == 4);
        CHECK(profile.regions.size() == 1);
        c1 = profile.curves[0];
        c2 = profile.curves[1];
        c3 = profile.curves[2];
        c4 = profile.curves[3];
        region = profile.regions[0];
        scope.commit();
    }

    SemanticId side1, side2, side3, side4, cap0, cap1, padEdge;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {c1, c2, c3, c4};
        req.regionSeeds = {region};
        req.allowSequentialFaceN = true;  // test fallback; product leaves this false
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&padPtr),
            Opcode::Pad,
            pad,
            scope.eval(),
            req
        );
        CHECK(scope.eval() != 0);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
        auto s1 = SemanticEmitter::generatedFrom(state.graph(), c1.handle);
        auto s2 = SemanticEmitter::generatedFrom(state.graph(), c2.handle);
        auto s3 = SemanticEmitter::generatedFrom(state.graph(), c3.handle);
        auto s4 = SemanticEmitter::generatedFrom(state.graph(), c4.handle);
        auto caps = SemanticEmitter::generatedFrom(state.graph(), region.handle);
        CHECK(s1.size() == 1 && s2.size() == 1 && s3.size() == 1 && s4.size() == 1);
        CHECK(caps.size() == 2);
        side1 = s1.front();
        side2 = s2.front();
        side3 = s3.front();
        side4 = s4.front();
        cap0 = caps[0];
        cap1 = caps[1];
        padEdge = SemanticEmitter::emitGeneratedFrom(
            SemanticEmitter::graphFor(&padPtr),
            {c1},
            SemanticKind::Edge,
            "Pad",
            pad,
            scope.eval(),
            SemanticRole::None,
            ElementIndex::fromString("Edge1")
        );
        CHECK(padEdge.valid());
        CHECK(padEdge.kind == SemanticKind::Edge);
        scope.commit();
    }

    // I2: cap seed does not resolve to another curve's side.
    {
        const auto capAll = SemanticResolver::resolve(
            makeRef(cap0, CardinalityReducer::AcceptAll, SemanticKind::Face),
            state.graph()
        );
        CHECK(capAll.state == ResolutionState::Resolved);
        CHECK(capAll.identities.front().handle == cap0.handle);
        CHECK(capAll.identities.front().handle != side1.handle);
        CHECK(capAll.identities.front().handle != side2.handle);
        const auto sideA = SemanticResolver::resolve(
            makeRef(side1, CardinalityReducer::RequireOne, SemanticKind::Face),
            state.graph()
        );
        CHECK(sideA.state == ResolutionState::Resolved);
        CHECK(sideA.identities.front().handle == side1.handle);
        CHECK(sideA.identities.front().handle != side2.handle);
    }

    // Pocket: Type ThroughAll would Split; without a named cap we emit Generated
    // sides/caps only (decision point). With a named cap + ThroughCut, Split.
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {};  // pocket sketch would have its own; use pad cap as target
        req.regionSeeds = {};
        req.pocketTarget = cap0;
        req.pocketMode = AfterExecuteRequest::PocketMode::ThroughCut;
        req.allowSequentialFaceN = true;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&pocketPtr),
            Opcode::Pocket,
            pocket,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("ThroughCut") != std::string::npos);
        scope.commit();
    }
    {
        const auto reqOne = SemanticResolver::resolve(
            makeRef(cap0, CardinalityReducer::RequireOne, SemanticKind::Face),
            state.graph()
        );
        CHECK(reqOne.state == ResolutionState::Ambiguous);
        const auto accAll = SemanticResolver::resolve(
            makeRef(cap0, CardinalityReducer::AcceptAll, SemanticKind::Face),
            state.graph()
        );
        CHECK(accAll.state == ResolutionState::ResolvedSet);
        CHECK(accAll.identities.size() == 2);
        for (const SemanticBinding& b : accAll.bindings) {
            CHECK(b.stid.handle != side1.handle);
            CHECK(b.stid.handle != side2.handle);
            CHECK(b.stid.handle != cap1.handle);
        }
    }

    // Fillet on the named pad edge + adjacent faces.
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.filletEdges = {padEdge};
        req.filletAdjacentFaces = {side1, cap0};
        req.allowSequentialFaceN = true;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&filletPtr),
            Opcode::Fillet,
            fillet,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted fillet") == 0);
        scope.commit();
    }

    // Insert-in-sketch: split curve 1 → corresponding pad side splits.
    {
        SemanticDocumentState::EvaluateScope scope(state);
        const auto childA = map.append();
        const auto childB = map.append();
        CHECK(childA != h1 && childB != h1);
        SemanticGraph* g = SemanticEmitter::graphFor(&sketchPtr);
        const auto kids
            = SketchSemanticSeeds::splitEntity(*g, sketch, scope.eval(), h1, {childA, childB});
        CHECK(kids.size() == 2);
        CHECK(g->isHistorical(c1.handle));
        const auto prop = SemanticEmitter::propagateSourceSplit(g, c1, 2, "Pad", pad, scope.eval(), 40);
        CHECK(prop.size() >= 2);
        scope.commit();

        const auto sideAfter = SemanticResolver::resolve(
            makeRef(side1, CardinalityReducer::RequireOne, SemanticKind::Face),
            state.graph()
        );
        CHECK(sideAfter.state == ResolutionState::Ambiguous);
        const auto sideAll = SemanticResolver::resolve(
            makeRef(side1, CardinalityReducer::AcceptAll, SemanticKind::Face),
            state.graph()
        );
        CHECK(sideAll.state == ResolutionState::ResolvedSet);
        CHECK(sideAll.identities.size() >= 2);
        int remnants = 0;
        for (const SemanticId& id : sideAll.identities) {
            for (const SemanticId& k : prop) {
                if (k.kind == SemanticKind::Face && id.handle == k.handle) {
                    ++remnants;
                }
            }
        }
        CHECK(remnants == 2);
        // No cross-lineage: other sides stay themselves.
        const auto other = SemanticResolver::resolve(
            makeRef(side2, CardinalityReducer::RequireOne, SemanticKind::Face),
            state.graph()
        );
        CHECK(other.state == ResolutionState::Resolved);
        CHECK(other.identities.front().handle == side2.handle);
        for (const SemanticBinding& b : sideAll.bindings) {
            CHECK(b.stid.handle != side2.handle);
            CHECK(b.stid.handle != side3.handle);
            CHECK(b.stid.handle != cap1.handle);
        }
    }

    // Fillet Missing on a deleted edge that has no live remnants. Neighbour
    // faces stay; they must not be invented as the dress-up (I2/I10).
    SemanticId doomed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        doomed = SemanticEmitter::emitGeneratedFrom(
            SemanticEmitter::graphFor(&padPtr),
            {c2},
            SemanticKind::Edge,
            "Pad",
            pad,
            scope.eval(),
            SemanticRole::None,
            ElementIndex::fromString("Edge2")
        );
        CHECK(doomed.valid());
        SemanticEmitter::emitDeleted(
            SemanticEmitter::graphFor(&padPtr),
            doomed,
            "DeleteEdge",
            pad,
            scope.eval(),
            SemanticRole::DressUpEdge
        );
        SemanticEmitter::graphFor(&padPtr)->unbind(doomed.handle);
        scope.commit();
    }
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.filletEdges = {doomed};
        req.filletAdjacentFaces = {side2, cap1};
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&filletPtr),
            Opcode::Fillet,
            fillet,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("Missing") != std::string::npos);
        const ResolutionResult fil = SemanticEmitter::emitFillet(
            SemanticEmitter::graphFor(&filletPtr),
            doomed,
            {side2, cap1},
            fillet,
            scope.eval(),
            ElementIndex::fromString("Face99")
        );
        CHECK(fil.state == ResolutionState::Missing);
        CHECK(fil.identities.empty());
        const auto neighbour = SemanticResolver::resolve(
            makeRef(doomed, CardinalityReducer::AcceptAll, SemanticKind::Edge),
            state.graph()
        );
        CHECK(neighbour.state == ResolutionState::Missing);
        CHECK(neighbour.bindings.empty());
        scope.commit();
    }

    CHECK(state.graph().eventGraphIsDag());
}

void testFilletConsumesAttachedEdge8()
{
    // Step 3: Binding Edge8 → makeSemanticRefsForSubNames → filletEdges non-empty
    // (same filter as FeatureFillet::execute over Base.getSemanticRefs()).
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int filletPtr = 40;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&filletPtr);

    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId curve, padEdge;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        curve = g->recordGenerated(SemanticKind::Edge, "Sketch", 10, scope.eval(), SemanticRole::None);
        AfterExecuteRequest req;
        req.curveSeeds = {curve};
        req.namedFaceIndices = {ElementIndex::fromString("Face7")};
        req.namedEdgeIndices = {ElementIndex::fromString("Edge8")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
        const auto kids = SemanticEmitter::generatedFrom(*g, curve.handle);
        CHECK(kids.size() == 2);  // Face7 + Edge8 from the same curve seed
        for (const SemanticId& id : kids) {
            if (id.kind == SemanticKind::Edge) {
                padEdge = id;
            }
        }
        CHECK(padEdge.valid());
        CHECK(padEdge.kind == SemanticKind::Edge);
        const auto rows = g->bindingsOf(padEdge.handle);
        CHECK(rows.size() == 1);
        CHECK(rows.front().index.toString() == "Edge8");
        scope.commit();
    }

    const auto refs = makeSemanticRefsForSubNames({"Edge8"}, g, SemanticRole::DressUpEdge, 20);
    CHECK(refs.size() == 1);
    CHECK(refs[0].seed.valid());
    CHECK(refs[0].seed.handle == padEdge.handle);
    CHECK(semanticRefXmlAttributes(refs[0]).find("stSeed=") != std::string::npos);

    AfterExecuteRequest fil;
    for (const SemanticReference& ref : refs) {
        if (!ref.seed.valid()) {
            continue;
        }
        if (ref.seed.kind == SemanticKind::Edge || ref.kind == SemanticKind::Edge) {
            fil.filletEdges.push_back(ref.seed);
        }
    }
    CHECK(!fil.filletEdges.empty());
    fil.allowSequentialFaceN = true;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&filletPtr),
            Opcode::Fillet,
            40,
            scope.eval(),
            fil
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted fillet") == 0);
        scope.commit();
    }
}

void testDressUpAdjacentFaceUniqueness()
{
    SemanticGraph graph;
    const ObjectId pad = 20;
    const ObjectId fillet = 40;
    graph.beginEvaluate(1);
    const SemanticId edge
        = graph.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    const SemanticId face
        = graph.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    SemanticEmitter::bind(&graph, edge, pad, 1, ElementIndex::fromString("Edge1"));
    graph.commitEvaluate();

    graph.beginEvaluate(2);
    const ResolutionResult result = SemanticEmitter::emitFillet(
        &graph,
        edge,
        {face, face},
        fillet,
        2,
        ElementIndex::fromString("Face7")
    );
    CHECK(result.state == ResolutionState::Resolved);
    const auto events = graph.events();
    CHECK(!events.empty());
    const EventId event = events.back().id;
    int inputs = 0;
    for (const EventInput& input : graph.inputs()) {
        if (input.event == event) {
            ++inputs;
        }
    }
    CHECK(inputs == 2);  // edge + one unique adjacent face
    graph.commitEvaluate();
}

void testFilletPocketTipEdge2AndPadEdge13()
{
    // Phase A follow-on: Fillet.Base on Body tip (Pocket Edge2) attaches when
    // Binding.feature==Pocket. Pad-owned Edge2 + linked Pocket refuses.
    // Pad unique Edge13 attaches (GUI XTR pick), not sequential Edge1.
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int pocketPtr = 30;
    int filletPtr = 40;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&pocketPtr);
    state.bindAlias(&filletPtr);

    SemanticGraph* g = SemanticEmitter::graphFor(&pocketPtr);
    SemanticId curve, pocketEdge, padEdge13;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        curve = g->recordGenerated(SemanticKind::Edge, "Sketch", 11, scope.eval(), SemanticRole::None);
        AfterExecuteRequest preq;
        preq.curveSeeds = {curve};
        preq.namedEdgeIndices = {ElementIndex::fromString("Edge2")};
        preq.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pocket, 30, scope.eval(), preq);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pocket") == 0);
        const auto kids = SemanticEmitter::generatedFrom(*g, curve.handle);
        CHECK(kids.size() == 1);
        CHECK(kids.front().kind == SemanticKind::Edge);
        pocketEdge = kids.front();
        CHECK(g->bindingsOf(pocketEdge.handle).front().feature == 30);
        CHECK(g->bindingsOf(pocketEdge.handle).front().index.toString() == "Edge2");

        const SemanticId v13
            = g->recordGenerated(SemanticKind::Vertex, "Sketch", 10, scope.eval(), SemanticRole::None);
        AfterExecuteRequest padReq;
        padReq.vertexSeeds = {v13};
        padReq.namedEdgeIndices = {ElementIndex::fromString("Edge13")};
        padReq.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), padReq);
        const auto padKids = SemanticEmitter::generatedFrom(*g, v13.handle);
        CHECK(padKids.size() == 1);
        padEdge13 = padKids.front();
        CHECK(padEdge13.kind == SemanticKind::Edge);
        CHECK(g->bindingsOf(padEdge13.handle).front().index.toString() == "Edge13");
        CHECK(g->bindingsOf(padEdge13.handle).front().feature == 20);
        scope.commit();
    }

    const auto tipRefs = makeSemanticRefsForSubNames({"Edge2"}, g, SemanticRole::DressUpEdge, 30);
    CHECK(tipRefs.size() == 1);
    CHECK(tipRefs[0].seed.valid());
    CHECK(tipRefs[0].seed.handle == pocketEdge.handle);
    CHECK(semanticRefXmlAttributes(tipRefs[0]).find("stSeed=") != std::string::npos);

    const auto pad13Refs = makeSemanticRefsForSubNames({"Edge13"}, g, SemanticRole::DressUpEdge, 20);
    CHECK(pad13Refs[0].seed.handle == padEdge13.handle);
    CHECK(pad13Refs[0].fallback == ElementIndex::fromString("Edge13"));
    CHECK(semanticRefXmlAttributes(pad13Refs[0]).find("stSeed=") != std::string::npos);

    // Binding.feature=Pad (Edge13) + linked Pocket → refuse Edge13 on Pocket.
    const auto scopedRefuse = makeSemanticRefsForSubNames({"Edge13"}, g, SemanticRole::DressUpEdge, 30);
    CHECK(scopedRefuse.size() == 1);
    CHECK(!scopedRefuse[0].seed.valid());

    AfterExecuteRequest fil;
    fil.filletEdges.push_back(tipRefs[0].seed);
    fil.allowSequentialFaceN = true;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&filletPtr),
            Opcode::Fillet,
            40,
            scope.eval(),
            fil
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted fillet") == 0);
        scope.commit();
    }
}

void testNamedHistoryPadCapsBindFace5AndFace6()
{
    // Product Pad: allowSequentialFaceN=false. namedFaceIndices is 4 side
    // FaceNs then 2 cap FaceNs (capture order: Generated sides, then First/Last).
    // Caps must bind the named cap indices, not sequential Face1/Face2.
    SemanticDocumentState state;
    int doc = 1;
    int sketchPtr = 10;
    int padPtr = 20;
    state.bindOwner(&doc);
    state.bindAlias(&sketchPtr);
    state.bindAlias(&padPtr);

    SketchEntityIdMap map;
    const auto h1 = map.append();
    const auto h2 = map.append();
    const auto h3 = map.append();
    const auto h4 = map.append();

    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId c1, c2, c3, c4, region;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        auto profile = SketchSemanticSeeds::seedsForProfile(
            *g,
            10,
            scope.eval(),
            {h1, h2, h3, h4},
            {SketchEntityIdMap::regionKey({h1, h2, h3, h4}, SketchEntityIdMap::RoleInterior)}
        );
        CHECK(profile.curves.size() == 4);
        CHECK(profile.regions.size() == 1);
        c1 = profile.curves[0];
        c2 = profile.curves[1];
        c3 = profile.curves[2];
        c4 = profile.curves[3];
        region = profile.regions[0];
        scope.commit();
    }
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {c1, c2, c3, c4};
        req.regionSeeds = {region};
        req.allowSequentialFaceN = false;
        req.namedFaceIndices = {
            ElementIndex::fromString("Face10"),
            ElementIndex::fromString("Face11"),
            ElementIndex::fromString("Face12"),
            ElementIndex::fromString("Face13"),
            ElementIndex::fromString("Face5"),
            ElementIndex::fromString("Face6")
        };
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("caps=2") != std::string::npos);

        const auto sides1 = SemanticEmitter::generatedFrom(*g, c1.handle);
        const auto sides2 = SemanticEmitter::generatedFrom(*g, c2.handle);
        const auto sides3 = SemanticEmitter::generatedFrom(*g, c3.handle);
        const auto sides4 = SemanticEmitter::generatedFrom(*g, c4.handle);
        CHECK(sides1.size() == 1 && sides2.size() == 1 && sides3.size() == 1 && sides4.size() == 1);
        CHECK(g->bindingsOf(sides1.front().handle).front().index.toString() == "Face10");
        CHECK(g->bindingsOf(sides2.front().handle).front().index.toString() == "Face11");
        CHECK(g->bindingsOf(sides3.front().handle).front().index.toString() == "Face12");
        CHECK(g->bindingsOf(sides4.front().handle).front().index.toString() == "Face13");

        const auto caps = SemanticEmitter::generatedFrom(*g, region.handle);
        CHECK(caps.size() == 2);
        std::vector<std::string> capNames;
        for (const SemanticId& cap : caps) {
            CHECK(cap.kind == SemanticKind::Face);
            const auto rows = g->bindingsOf(cap.handle);
            CHECK(rows.size() == 1);
            capNames.push_back(rows.front().index.toString());
            CHECK(rows.front().index.toString() != "Face1");
            CHECK(rows.front().index.toString() != "Face2");
            CHECK(rows.front().feature == 20);
        }
        std::sort(capNames.begin(), capNames.end());
        CHECK(capNames.size() == 2);
        CHECK(capNames[0] == "Face5");
        CHECK(capNames[1] == "Face6");
        scope.commit();
    }
}

void testNoHalfMapWithoutSeeds()
{
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    SemanticDocumentState::EvaluateScope scope(state);
    SemanticEmitter::afterExecute(SemanticEmitter::graphFor(&padPtr), Opcode::Pad, 20, scope.eval(), {});
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("skip emit") == 0);
    // No identities minted from FaceN.
    CHECK(state.graph().allIdentities().empty());
    scope.commit();
}

void testGraphPersistenceRoundTrip()
{
    SemanticDocumentState a;
    int doc = 7;
    a.bindOwner(&doc);
    SemanticDocumentState::EvaluateScope scope(a);
    SemanticGraph* g = &a.graph();
    SketchEntityIdMap map;
    const auto h = map.append();
    const SemanticId seed
        = SketchSemanticSeeds::ensureSeedForEntity(*g, 10, scope.eval(), h, SemanticKind::Edge);
    AfterExecuteRequest req;
    req.curveSeeds = {seed};
    req.allowSequentialFaceN = true;
    SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
    const auto sides = SemanticEmitter::generatedFrom(*g, seed.handle);
    CHECK(sides.size() == 1);
    const SemanticHandle water = g->allocator.highWaterMark();
    const EvalSerial nextEval = a.nextEvalSerial();
    scope.commit();

    const std::string payload = a.serialize();
    CHECK(payload.find("STD1") == 0);
    CHECK(payload.find("STG1") != std::string::npos);
    const std::string hex = SemanticDocumentState::hexEncode(payload);
    CHECK(SemanticDocumentState::hexDecode(hex) == payload);

    SemanticDocumentState b;
    CHECK(b.deserialize(payload));
    CHECK(b.graph().allocator.highWaterMark() == water);
    CHECK(b.nextEvalSerial() >= nextEval);
    const SemanticId* restored = b.graph().identity(seed.handle);
    CHECK(restored != nullptr);
    CHECK(restored->handle == seed.handle);
    CHECK(restored->kind == SemanticKind::Edge);
    const auto sides2 = SemanticEmitter::generatedFrom(b.graph(), seed.handle);
    CHECK(sides2.size() == 1);
    CHECK(sides2.front().handle == sides.front().handle);

    // I5: next allocate does not reuse.
    b.beginEvaluate();
    const SemanticId extra = b.graph().recordGenerated(
        SemanticKind::Face,
        "AfterRestore",
        99,
        b.currentEval(),
        SemanticRole::None
    );
    b.commitEvaluate();
    CHECK(extra.handle >= water);
    CHECK(extra.handle != seed.handle);
    CHECK(extra.handle != sides.front().handle);

    // Merge + alias round-trip.
    SemanticDocumentState c;
    c.beginEvaluate();
    const SemanticId p1 = c.graph().recordGenerated(SemanticKind::Face, "A", 1, 1, SemanticRole::None);
    const SemanticId p2 = c.graph().recordGenerated(SemanticKind::Face, "A", 1, 1, SemanticRole::None);
    const SemanticId m = c.graph().recordMerge({p1, p2}, "Merge", 1, 1, SemanticRole::None);
    CHECK(c.graph().aliasClass().same(p1.handle, m.handle));
    c.commitEvaluate();
    SemanticDocumentState d;
    CHECK(d.deserialize(c.serialize()));
    CHECK(d.graph().aliasClass().same(p1.handle, m.handle));
    CHECK(d.graph().identity(m.handle) != nullptr);
}

void testUnknownOpcodeAndNullGraph()
{
    SemanticEmitter::afterExecute(nullptr, Opcode::Pad, 1, 1, {});
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("no graph") != std::string::npos);
}

void testChamferAfterExecuteNamedFace()
{
    // Named edge seed + namedFaceIndex -> Chamfer events (opcode token Chamfer), not Fillet.
    // Missing edge -> skip, no neighbour invented (I10).
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int chamferPtr = 50;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&chamferPtr);

    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId padEdge;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        const SemanticId curve
            = g->recordGenerated(SemanticKind::Edge, "Sketch", 10, scope.eval(), SemanticRole::None);
        AfterExecuteRequest req;
        req.curveSeeds = {curve};
        req.namedEdgeIndices = {ElementIndex::fromString("Edge8")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        const auto kids = SemanticEmitter::generatedFrom(*g, curve.handle);
        for (const SemanticId& id : kids) {
            if (id.kind == SemanticKind::Edge) {
                padEdge = id;
            }
        }
        CHECK(padEdge.valid());
        CHECK(padEdge.kind == SemanticKind::Edge);
        scope.commit();
    }

    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.filletEdges = {padEdge};  // Chamfer reuses dress-up vectors
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&chamferPtr),
            Opcode::Chamfer,
            50,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted chamfer") == 0);
        bool sawChamfer = false;
        bool sawFilletOnChamfer = false;
        for (const Event& ev : g->events()) {
            if (ev.feature != 50) {
                continue;
            }
            if (ev.op == "Chamfer") {
                sawChamfer = true;
            }
            if (ev.op == "Fillet") {
                sawFilletOnChamfer = true;
            }
        }
        CHECK(sawChamfer);
        CHECK(!sawFilletOnChamfer);
        const auto kids = SemanticEmitter::generatedFrom(*g, padEdge.handle);
        bool namedFace = false;
        for (const SemanticId& id : kids) {
            if (id.kind != SemanticKind::Face) {
                continue;
            }
            const auto rows = g->bindingsOf(id.handle);
            if (!rows.empty() && rows.front().index.toString() == "Face9"
                && rows.front().feature == 50) {
                namedFace = true;
            }
        }
        CHECK(namedFace);
        scope.commit();
    }

    SemanticId doomed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        doomed = SemanticEmitter::emitGeneratedFrom(
            g,
            {padEdge},
            SemanticKind::Edge,
            "Pad",
            20,
            scope.eval(),
            SemanticRole::None,
            ElementIndex::fromString("Edge2")
        );
        CHECK(doomed.valid());
        SemanticEmitter::emitDeleted(g, doomed, "DeleteEdge", 20, scope.eval(), SemanticRole::DressUpEdge);
        g->unbind(doomed.handle);
        scope.commit();
    }
    const std::size_t nIdentities = g->allIdentities().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.filletEdges = {doomed};
        req.namedFaceIndices = {ElementIndex::fromString("Face99")};
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&chamferPtr),
            Opcode::Chamfer,
            50,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("Missing") != std::string::npos);
        const ResolutionResult r = SemanticEmitter::emitChamfer(
            g,
            doomed,
            {},
            50,
            scope.eval(),
            ElementIndex::fromString("Face99")
        );
        CHECK(r.state == ResolutionState::Missing);
        CHECK(r.identities.empty());
        const auto neighbour = SemanticResolver::resolve(
            makeRef(doomed, CardinalityReducer::AcceptAll, SemanticKind::Edge),
            *g
        );
        CHECK(neighbour.state == ResolutionState::Missing);
        CHECK(neighbour.bindings.empty());
        CHECK(g->allIdentities().size() == nIdentities);
        scope.commit();
    }
}

void testDraftAfterExecuteNamedFace()
{
    // Named Face seed + namedFaceIndex -> Draft events (opcode token Draft).
    // Missing face -> skip, no neighbour invented (I10).
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int draftPtr = 60;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&draftPtr);

    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId padFace;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        const SemanticId region
            = g->recordGenerated(SemanticKind::Region, "Sketch", 10, scope.eval(), SemanticRole::None);
        AfterExecuteRequest req;
        req.regionSeeds = {region};
        req.namedFaceIndices = {ElementIndex::fromString("Face6"), ElementIndex::fromString("Face5")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        const auto kids = SemanticEmitter::generatedFrom(*g, region.handle);
        CHECK(kids.size() == 2);
        padFace = kids.front();
        CHECK(padFace.valid());
        CHECK(padFace.kind == SemanticKind::Face);
        scope.commit();
    }

    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.draftFaces = {padFace};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&draftPtr),
            Opcode::Draft,
            60,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted draft") == 0);
        bool sawDraft = false;
        bool sawFilletOnDraft = false;
        bool sawChamferOnDraft = false;
        for (const Event& ev : g->events()) {
            if (ev.feature != 60) {
                continue;
            }
            if (ev.op == "Draft") {
                sawDraft = true;
            }
            if (ev.op == "Fillet") {
                sawFilletOnDraft = true;
            }
            if (ev.op == "Chamfer") {
                sawChamferOnDraft = true;
            }
        }
        CHECK(sawDraft);
        CHECK(!sawFilletOnDraft);
        CHECK(!sawChamferOnDraft);
        const auto kids = SemanticEmitter::generatedFrom(*g, padFace.handle);
        bool namedFace = false;
        for (const SemanticId& id : kids) {
            if (id.kind != SemanticKind::Face) {
                continue;
            }
            const auto rows = g->bindingsOf(id.handle);
            if (!rows.empty() && rows.front().index.toString() == "Face9"
                && rows.front().feature == 60) {
                namedFace = true;
            }
        }
        CHECK(namedFace);
        scope.commit();
    }

    SemanticId doomed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        doomed = SemanticEmitter::emitGeneratedFrom(
            g,
            {padFace},
            SemanticKind::Face,
            "Pad",
            20,
            scope.eval(),
            SemanticRole::None,
            ElementIndex::fromString("Face2")
        );
        CHECK(doomed.valid());
        SemanticEmitter::emitDeleted(g, doomed, "DeleteFace", 20, scope.eval(), SemanticRole::None);
        g->unbind(doomed.handle);
        scope.commit();
    }
    const std::size_t nIdentities = g->allIdentities().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.draftFaces = {doomed};
        req.namedFaceIndices = {ElementIndex::fromString("Face99")};
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&draftPtr),
            Opcode::Draft,
            60,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("Missing") != std::string::npos);
        const ResolutionResult r = SemanticEmitter::emitDraft(
            g,
            doomed,
            60,
            scope.eval(),
            ElementIndex::fromString("Face99")
        );
        CHECK(r.state == ResolutionState::Missing);
        CHECK(r.identities.empty());
        const auto neighbour = SemanticResolver::resolve(
            makeRef(doomed, CardinalityReducer::AcceptAll, SemanticKind::Face),
            *g
        );
        CHECK(neighbour.state == ResolutionState::Missing);
        CHECK(neighbour.bindings.empty());
        CHECK(g->allIdentities().size() == nIdentities);
        scope.commit();
    }
}


void testThicknessAfterExecuteNamedFace()
{
    // Named Face seed + namedFaceIndex -> Thickness events (opcode token Thickness).
    // Missing face -> skip, no neighbour invented (I10).
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int thicknessPtr = 70;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&thicknessPtr);

    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId padFace;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        const SemanticId region
            = g->recordGenerated(SemanticKind::Region, "Sketch", 10, scope.eval(), SemanticRole::None);
        AfterExecuteRequest req;
        req.regionSeeds = {region};
        req.namedFaceIndices = {ElementIndex::fromString("Face6"), ElementIndex::fromString("Face5")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        const auto kids = SemanticEmitter::generatedFrom(*g, region.handle);
        CHECK(kids.size() == 2);
        padFace = kids.front();
        CHECK(padFace.valid());
        CHECK(padFace.kind == SemanticKind::Face);
        scope.commit();
    }

    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.thicknessFaces = {padFace};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&thicknessPtr),
            Opcode::Thickness,
            70,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted thickness") == 0);
        bool sawThickness = false;
        bool sawDraftOnThickness = false;
        for (const Event& ev : g->events()) {
            if (ev.feature != 70) {
                continue;
            }
            if (ev.op == "Thickness") {
                sawThickness = true;
            }
            if (ev.op == "Draft") {
                sawDraftOnThickness = true;
            }
        }
        CHECK(sawThickness);
        CHECK(!sawDraftOnThickness);
        const auto kids = SemanticEmitter::generatedFrom(*g, padFace.handle);
        bool namedFace = false;
        for (const SemanticId& id : kids) {
            if (id.kind != SemanticKind::Face) {
                continue;
            }
            const auto rows = g->bindingsOf(id.handle);
            if (!rows.empty() && rows.front().index.toString() == "Face9"
                && rows.front().feature == 70) {
                namedFace = true;
            }
        }
        CHECK(namedFace);
        scope.commit();
    }

    SemanticId doomed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        doomed = SemanticEmitter::emitGeneratedFrom(
            g,
            {padFace},
            SemanticKind::Face,
            "Pad",
            20,
            scope.eval(),
            SemanticRole::None,
            ElementIndex::fromString("Face2")
        );
        CHECK(doomed.valid());
        SemanticEmitter::emitDeleted(g, doomed, "DeleteFace", 20, scope.eval(), SemanticRole::None);
        g->unbind(doomed.handle);
        scope.commit();
    }
    const std::size_t nIdentities = g->allIdentities().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.thicknessFaces = {doomed};
        req.namedFaceIndices = {ElementIndex::fromString("Face99")};
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&thicknessPtr),
            Opcode::Thickness,
            70,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("Missing") != std::string::npos);
        const ResolutionResult r = SemanticEmitter::emitThickness(
            g,
            doomed,
            70,
            scope.eval(),
            ElementIndex::fromString("Face99")
        );
        CHECK(r.state == ResolutionState::Missing);
        CHECK(r.identities.empty());
        const auto neighbour = SemanticResolver::resolve(
            makeRef(doomed, CardinalityReducer::AcceptAll, SemanticKind::Face),
            *g
        );
        CHECK(neighbour.state == ResolutionState::Missing);
        CHECK(neighbour.bindings.empty());
        CHECK(g->allIdentities().size() == nIdentities);
        scope.commit();
    }
}


void testHoleAfterExecuteNamedFace()
{
    // Named StartReference Face seed + namedFaceIndex -> Hole events.
    // Empty namedFaceIndices -> skip history mint (I10). Missing -> skip,
    // no neighbour invented.
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int holePtr = 80;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&holePtr);

    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId padFace;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        const SemanticId region
            = g->recordGenerated(SemanticKind::Region, "Sketch", 10, scope.eval(), SemanticRole::None);
        AfterExecuteRequest req;
        req.regionSeeds = {region};
        req.namedFaceIndices = {ElementIndex::fromString("Face6"), ElementIndex::fromString("Face5")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        const auto kids = SemanticEmitter::generatedFrom(*g, region.handle);
        CHECK(kids.size() == 2);
        padFace = kids.front();
        CHECK(padFace.valid());
        CHECK(padFace.kind == SemanticKind::Face);
        scope.commit();
    }

    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.holeFaces = {padFace};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&holePtr),
            Opcode::Hole,
            80,
            scope.eval(),
            req
        );
        CHECK(
            SemanticEmitter::lastAfterExecuteNote().find("no named hole history") != std::string::npos
        );
        bool sawHole = false;
        for (const Event& ev : g->events()) {
            if (ev.feature == 80 && ev.op == "Hole") {
                sawHole = true;
            }
        }
        CHECK(!sawHole);
        scope.commit();
    }

    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.holeFaces = {padFace};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&holePtr),
            Opcode::Hole,
            80,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted hole") == 0);
        bool sawHole = false;
        bool sawDraftOnHole = false;
        for (const Event& ev : g->events()) {
            if (ev.feature != 80) {
                continue;
            }
            if (ev.op == "Hole") {
                sawHole = true;
            }
            if (ev.op == "Draft") {
                sawDraftOnHole = true;
            }
        }
        CHECK(sawHole);
        CHECK(!sawDraftOnHole);
        const auto kids = SemanticEmitter::generatedFrom(*g, padFace.handle);
        bool namedFace = false;
        for (const SemanticId& id : kids) {
            if (id.kind != SemanticKind::Face) {
                continue;
            }
            const auto rows = g->bindingsOf(id.handle);
            if (!rows.empty() && rows.front().index.toString() == "Face9"
                && rows.front().feature == 80) {
                namedFace = true;
            }
        }
        CHECK(namedFace);
        scope.commit();
    }

    SemanticId doomed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        doomed = SemanticEmitter::emitGeneratedFrom(
            g,
            {padFace},
            SemanticKind::Face,
            "Pad",
            20,
            scope.eval(),
            SemanticRole::None,
            ElementIndex::fromString("Face2")
        );
        CHECK(doomed.valid());
        SemanticEmitter::emitDeleted(g, doomed, "DeleteFace", 20, scope.eval(), SemanticRole::None);
        g->unbind(doomed.handle);
        scope.commit();
    }
    const std::size_t nIdentities = g->allIdentities().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.holeFaces = {doomed};
        req.namedFaceIndices = {ElementIndex::fromString("Face99")};
        SemanticEmitter::afterExecute(
            SemanticEmitter::graphFor(&holePtr),
            Opcode::Hole,
            80,
            scope.eval(),
            req
        );
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("Missing") != std::string::npos);
        const ResolutionResult r
            = SemanticEmitter::emitHole(g, doomed, 80, scope.eval(), ElementIndex::fromString("Face99"));
        CHECK(r.state == ResolutionState::Missing);
        CHECK(r.identities.empty());
        const auto neighbour = SemanticResolver::resolve(
            makeRef(doomed, CardinalityReducer::AcceptAll, SemanticKind::Face),
            *g
        );
        CHECK(neighbour.state == ResolutionState::Missing);
        CHECK(neighbour.bindings.empty());
        CHECK(g->allIdentities().size() == nIdentities);
        scope.commit();
    }
}

void testHoleOutputOwnershipConflict()
{
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int holePtr = 80;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&holePtr);
    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);

    SemanticId source;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        source = g->recordGenerated(SemanticKind::Face, "Pad", 20, scope.eval(), SemanticRole::None);
        SemanticEmitter::bind(g, source, 20, scope.eval(), ElementIndex::fromString("Face4"));
        scope.commit();
    }

    {
        SemanticDocumentState::EvaluateScope scope(state);
        const SemanticId first
            = g->recordGenerated(SemanticKind::Face, "Hole", 80, scope.eval(), SemanticRole::None);
        const SemanticId second
            = g->recordGenerated(SemanticKind::Face, "Hole", 80, scope.eval(), SemanticRole::None);
        const ElementIndex output = ElementIndex::fromString("Face9");
        SemanticEmitter::bind(g, first, 80, scope.eval(), output);
        SemanticEmitter::bind(g, second, 80, scope.eval(), output);
        scope.commit();
    }

    const std::size_t identities = g->allIdentities().size();
    const std::size_t events = g->events().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.holeFaces = {source};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        req.allowSequentialFaceN = false;
        SemanticEmitter::afterExecute(g, Opcode::Hole, 80, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("HoleStart Ambiguous") == 0);
        CHECK(g->allIdentities().size() == identities);
        CHECK(g->events().size() == events);
        scope.commit();
    }
}

void testPadPocketOutputOwnershipConflict()
{
    SemanticDocumentState state;
    int doc = 1;
    int padPtr = 20;
    int pocketPtr = 30;
    state.bindOwner(&doc);
    state.bindAlias(&padPtr);
    state.bindAlias(&pocketPtr);
    SemanticGraph* g = SemanticEmitter::graphFor(&padPtr);
    SemanticId padSeed;
    SemanticId pocketSeed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        padSeed = g->recordGenerated(SemanticKind::Edge, "Sketch", 10, scope.eval(), SemanticRole::None);
        pocketSeed
            = g->recordGenerated(SemanticKind::Edge, "Sketch", 10, scope.eval(), SemanticRole::None);
        for (const auto feature : {20, 30}) {
            const SemanticId first = g->recordGenerated(
                SemanticKind::Face,
                feature == 20 ? "Pad" : "Pocket",
                feature,
                scope.eval(),
                SemanticRole::None
            );
            const SemanticId second = g->recordGenerated(
                SemanticKind::Face,
                feature == 20 ? "Pad" : "Pocket",
                feature,
                scope.eval(),
                SemanticRole::None
            );
            SemanticEmitter::bind(g, first, feature, scope.eval(), ElementIndex::fromString("Face9"));
            SemanticEmitter::bind(g, second, feature, scope.eval(), ElementIndex::fromString("Face9"));
        }
        scope.commit();
    }
    const std::size_t identities = g->allIdentities().size();
    const std::size_t events = g->events().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {padSeed};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        SemanticEmitter::afterExecute(g, Opcode::Pad, 20, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("sides=0") != std::string::npos);
        CHECK(g->allIdentities().size() == identities);
        CHECK(g->events().size() == events);
        scope.commit();
    }
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {pocketSeed};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        SemanticEmitter::afterExecute(g, Opcode::Pocket, 30, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("sides=0") != std::string::npos);
        CHECK(g->allIdentities().size() == identities);
        CHECK(g->events().size() == events);
        scope.commit();
    }
}

void testRevolutionGrooveOutputOwnershipConflict()
{
    SemanticDocumentState state;
    int doc = 1;
    int revolutionPtr = 40;
    int groovePtr = 50;
    state.bindOwner(&doc);
    state.bindAlias(&revolutionPtr);
    state.bindAlias(&groovePtr);
    SemanticGraph* g = SemanticEmitter::graphFor(&revolutionPtr);
    SemanticId revolutionSeed;
    SemanticId grooveSeed;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        revolutionSeed
            = g->recordGenerated(SemanticKind::Edge, "Sketch", 10, scope.eval(), SemanticRole::None);
        grooveSeed
            = g->recordGenerated(SemanticKind::Edge, "Sketch", 10, scope.eval(), SemanticRole::None);
        for (const auto feature : {40, 50}) {
            const char* op = feature == 40 ? "Revolution" : "Groove";
            const SemanticKind outputKind = feature == 40 ? SemanticKind::Face : SemanticKind::Edge;
            const ElementIndex output = feature == 40 ? ElementIndex::fromString("Face9")
                                                      : ElementIndex::fromString("Edge9");
            const SemanticId first
                = g->recordGenerated(outputKind, op, feature, scope.eval(), SemanticRole::None);
            const SemanticId second
                = g->recordGenerated(outputKind, op, feature, scope.eval(), SemanticRole::None);
            SemanticEmitter::bind(g, first, feature, scope.eval(), output);
            SemanticEmitter::bind(g, second, feature, scope.eval(), output);
        }
        scope.commit();
    }
    const std::size_t identities = g->allIdentities().size();
    const std::size_t events = g->events().size();
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {revolutionSeed};
        req.namedFaceIndices = {ElementIndex::fromString("Face9")};
        SemanticEmitter::afterExecute(g, Opcode::Revolution, 40, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("sides=0") != std::string::npos);
        CHECK(g->allIdentities().size() == identities);
        CHECK(g->events().size() == events);
        scope.commit();
    }
    {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {grooveSeed};
        req.namedEdgeIndices = {ElementIndex::fromString("Edge9")};
        SemanticEmitter::afterExecute(g, Opcode::Groove, 50, scope.eval(), req);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("edges=0") != std::string::npos);
        CHECK(g->allIdentities().size() == identities);
        CHECK(g->events().size() == events);
        scope.commit();
    }
}

void testDirectHistoryGuardCountsValidSlots()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const std::vector<Opcode> direct = {
        Opcode::Boolean,
        Opcode::AdditiveBox,
        Opcode::AdditiveCylinder,
        Opcode::AdditiveSphere,
        Opcode::AdditiveCone,
        Opcode::AdditiveTorus,
        Opcode::AdditivePrism,
        Opcode::AdditiveWedge,
        Opcode::AdditiveEllipsoid,
        Opcode::SubtractiveBox,
        Opcode::SubtractiveCylinder,
        Opcode::SubtractiveSphere,
        Opcode::SubtractiveCone,
        Opcode::SubtractiveTorus,
        Opcode::SubtractivePrism,
        Opcode::SubtractiveWedge,
        Opcode::SubtractiveEllipsoid,
    };
    for (const Opcode opcode : direct) {
        AfterExecuteRequest invalid;
        invalid.namedFaceIndices.push_back({});
        invalid.namedEdgeIndices.push_back({});
        SemanticEmitter::afterExecute(&g, opcode, 1, 1, invalid);
        CHECK(SemanticEmitter::lastAfterExecuteNote().find("half-map") != std::string::npos);
    }

    AfterExecuteRequest mixed;
    mixed.namedFaceIndices.push_back({});
    mixed.namedFaceIndices.push_back(ElementIndex::fromString("Face4"));
    mixed.namedEdgeIndices.push_back({});
    SemanticEmitter::afterExecute(&g, Opcode::SubtractiveBox, 1, 1, mixed);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("faces=1") != std::string::npos);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("edges=0") != std::string::npos);
    g.commitEvaluate();
}

}  // namespace

void testProfileSweepOutputOwnershipConflict();

int main()
{
    testSketchSeedsAreDocumentHandles();
    testClickableEmitScenario();
    testFilletConsumesAttachedEdge8();
    testDressUpAdjacentFaceUniqueness();
    testFilletPocketTipEdge2AndPadEdge13();
    testNamedHistoryPadCapsBindFace5AndFace6();
    testNoHalfMapWithoutSeeds();
    testGraphPersistenceRoundTrip();
    testUnknownOpcodeAndNullGraph();
    testChamferAfterExecuteNamedFace();
    testDraftAfterExecuteNamedFace();
    testThicknessAfterExecuteNamedFace();
    testHoleAfterExecuteNamedFace();
    testHoleOutputOwnershipConflict();
    testPadPocketOutputOwnershipConflict();
    testRevolutionGrooveOutputOwnershipConflict();
    testProfileSweepOutputOwnershipConflict();
    testDirectHistoryGuardCountsValidSlots();

    std::cout << "Semantic emit scenario: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}

void testProfileSweepOutputOwnershipConflict()
{
    SemanticDocumentState state;
    int doc = 1;
    int featureIds[] = {60, 70, 80, 90, 100, 110};
    state.bindOwner(&doc);
    for (int& feature : featureIds) {
        state.bindAlias(&feature);
    }
    SemanticGraph* g = SemanticEmitter::graphFor(&featureIds[0]);
    const std::vector<Opcode> opcodes = {
        Opcode::Loft,
        Opcode::Pipe,
        Opcode::Helix,
        Opcode::SubtractiveLoft,
        Opcode::SubtractivePipe,
        Opcode::SubtractiveHelix,
    };
    const std::vector<ElementIndex> outputs = {
        ElementIndex::fromString("Face9"),
        ElementIndex::fromString("Edge9"),
        ElementIndex::fromString("Face9"),
        ElementIndex::fromString("Edge9"),
        ElementIndex::fromString("Face9"),
        ElementIndex::fromString("Edge9"),
    };
    std::vector<SemanticId> seeds;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        for (std::size_t i = 0; i < opcodes.size(); ++i) {
            const int feature = featureIds[i];
            const char* op = opcodeName(opcodes[i]);
            const SemanticKind outputKind = outputs[i].type == "Face" ? SemanticKind::Face
                                                                      : SemanticKind::Edge;
            seeds.push_back(g->recordGenerated(
                SemanticKind::Edge,
                "Sketch",
                20 + static_cast<int>(i),
                scope.eval(),
                SemanticRole::None
            ));
            const SemanticId first
                = g->recordGenerated(outputKind, op, feature, scope.eval(), SemanticRole::None);
            const SemanticId second
                = g->recordGenerated(outputKind, op, feature, scope.eval(), SemanticRole::None);
            SemanticEmitter::bind(g, first, feature, scope.eval(), outputs[i]);
            SemanticEmitter::bind(g, second, feature, scope.eval(), outputs[i]);
        }
        scope.commit();
    }
    const std::size_t identities = g->allIdentities().size();
    const std::size_t events = g->events().size();
    for (std::size_t i = 0; i < opcodes.size(); ++i) {
        SemanticDocumentState::EvaluateScope scope(state);
        AfterExecuteRequest req;
        req.curveSeeds = {seeds[i]};
        if (outputs[i].type == "Face") {
            req.namedFaceIndices = {outputs[i]};
        }
        else {
            req.namedEdgeIndices = {outputs[i]};
        }
        SemanticEmitter::afterExecute(g, opcodes[i], featureIds[i], scope.eval(), req);
        const char* expected = outputs[i].type == "Face" ? "sides=0" : "edges=0";
        CHECK(SemanticEmitter::lastAfterExecuteNote().find(expected) != std::string::npos);
        CHECK(g->allIdentities().size() == identities);
        CHECK(g->events().size() == events);
        scope.commit();
    }
}
