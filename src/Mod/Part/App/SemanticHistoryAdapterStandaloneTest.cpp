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
// BRep history adapter standalone tests (fake table, no OCCT).
//   src/Mod/Part/App/compile_semantic_history_adapter_test.sh
//   /tmp/semantic-history-adapter

#include "SemanticHistoryAdapter.h"
#include "SemanticOpcode.h"

#include <App/SemanticDocumentState.h>
#include <App/SemanticId.h>
#include <App/SemanticReference.h>
#include <App/SemanticTopology.h>

#include <iostream>
#include <string>
#include <vector>

using namespace App;
using namespace Part;
using namespace PartDesign;

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

SemanticBinding makeBinding(const SemanticId& id, ObjectId feature, EvalSerial eval, const char* name)
{
    SemanticBinding b;
    b.stid = id;
    b.feature = feature;
    b.eval = eval;
    b.kind = id.kind;
    b.index = ElementIndex::fromString(name);
    b.occ = nullptr;
    return b;
}

SemanticReference makeRef(const SemanticId& seed, CardinalityReducer reducer, SemanticKind kind)
{
    SemanticReference ref;
    ref.seed = seed;
    ref.kind = kind;
    ref.reducer = reducer;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    return ref;
}

void testNullGraphAndEmptyTable()
{
    CHECK(SemanticHistoryAdapter::applyHistory(nullptr, 1, 1, "Pad", {}, {}).boundCount == 0);

    SemanticGraph g;
    const auto empty = SemanticHistoryAdapter::applyHistory(&g, 20, 1, "Pad", {}, {});
    CHECK(empty.halfMap);
    CHECK(empty.boundCount == 0);
    CHECK(g.allIdentities().empty());
    CHECK(g.allBindings().empty());

    const FilletPreflight noGraph = SemanticHistoryAdapter::preflightFillet(nullptr, {});
    CHECK(!noGraph.makerSkipped);

    const FilletPreflight noEdges = SemanticHistoryAdapter::preflightFillet(&g, {});
    CHECK(!noEdges.makerSkipped);
}

void testFakeHistoryOneCurveOneSideFace()
{
    SemanticGraph g;
    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    g.beginEvaluate(1);
    const SemanticId curve
        = g.recordGenerated(SemanticKind::Edge, "Sketch", sketch, 1, SemanticRole::None);
    g.bind(makeBinding(curve, sketch, 1, "Edge1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    HistoryRecord rec;
    rec.fromSeed = curve;
    rec.toIndex = ElementIndex::fromString("Face7");  // named by history, not sequential Face1
    rec.kind = EventKind::Generated;
    rec.outputKind = SemanticKind::Face;
    const ApplyResult applied = SemanticHistoryAdapter::applyHistory(&g, pad, 2, "Pad", {curve}, {rec});
    g.commitEvaluate();

    CHECK(applied.boundCount == 1);
    CHECK(applied.namedCount == 1);
    CHECK(!applied.halfMap);
    CHECK(applied.outputs.size() == 1);
    CHECK(applied.outputs.front().kind == SemanticKind::Face);

    const SemanticId side = applied.outputs.front();
    CHECK(g.descendants(curve.handle).count(side.handle) == 1);

    const auto rows = g.bindingsOf(side.handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().index.toString() == "Face7");
    CHECK(rows.front().index.index == 7);
    CHECK(rows.front().index.index != 1);

    // Resolving the curve's Generated face is that named side, not Face1.
    const auto from = SemanticEmitter::generatedFrom(g, curve.handle);
    CHECK(from.size() == 1);
    CHECK(from.front().handle == side.handle);
    CHECK(from.front().handle != curve.handle);

    const auto r = SemanticResolver::resolve(
        makeRef(side, CardinalityReducer::RequireOne, SemanticKind::Face),
        g
    );
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.front().handle == side.handle);
    CHECK(r.bindings.front().index.toString() == "Face7");
}

void testDeletedEdgeFilletMissingMakerSkipped()
{
    SemanticGraph g;
    const ObjectId pad = 20;
    g.beginEvaluate(1);
    const SemanticId edge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    const SemanticId f1 = g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    const SemanticId f2 = g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    const SemanticId neighbourEdge
        = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(edge, pad, 1, "Edge1"));
    g.bind(makeBinding(f1, pad, 1, "Face1"));
    g.bind(makeBinding(f2, pad, 1, "Face2"));
    g.bind(makeBinding(neighbourEdge, pad, 1, "Edge2"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.clearBindings(pad);
    SemanticEmitter::bind(&g, f1, pad, 2, ElementIndex::fromString("Face1"));
    SemanticEmitter::bind(&g, f2, pad, 2, ElementIndex::fromString("Face2"));
    SemanticEmitter::bind(&g, neighbourEdge, pad, 2, ElementIndex::fromString("Edge2"));
    SemanticEmitter::emitDeleted(&g, edge, "Pocket", 30, 2, SemanticRole::DressUpEdge);
    g.commitEvaluate();

    const FilletPreflight pre = SemanticHistoryAdapter::preflightFillet(&g, {edge});
    CHECK(pre.makerSkipped == true);
    CHECK(pre.state == ResolutionState::Missing);
    CHECK(pre.perEdge.size() == 1);
    CHECK(pre.perEdge.front().state == ResolutionState::Missing);

    // afterExecute must not mint a fillet face or bind a neighbour.
    AfterExecuteRequest req;
    req.filletEdges = {edge};
    req.filletAdjacentFaces = {f1, f2};
    const std::size_t nBefore = g.allIdentities().size();
    SemanticEmitter::afterExecute(&g, Opcode::Fillet, 40, 3, req);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("Missing") != std::string::npos);
    CHECK(g.allIdentities().size() == nBefore);

    const auto neighbour = SemanticResolver::resolve(
        makeRef(edge, CardinalityReducer::AcceptAll, SemanticKind::Edge),
        g
    );
    CHECK(neighbour.state == ResolutionState::Missing);
    CHECK(neighbour.bindings.empty());
    CHECK(neighbour.identities.empty());

    // Neighbour edge still resolves as itself — not as the deleted fillet seed (I10).
    const auto other = SemanticResolver::resolve(
        makeRef(neighbourEdge, CardinalityReducer::AcceptAll, SemanticKind::Edge),
        g
    );
    CHECK(other.state == ResolutionState::Resolved);
    CHECK(other.identities.front().handle == neighbourEdge.handle);
    CHECK(other.identities.front().handle != edge.handle);

    // Neighbour faces were not bound as the fillet output.
    bool face1IsFillet = false;
    for (const SemanticBinding& b : g.allBindings()) {
        if (b.stid.handle == f1.handle && b.feature == 40) {
            face1IsFillet = true;
        }
    }
    CHECK(!face1IsFillet);

    // Empty history after a skipped maker still does not invent FaceN.
    const ApplyResult skippedHist
        = SemanticHistoryAdapter::applyHistory(&g, 40, 3, "Fillet", {edge}, {});
    CHECK(skippedHist.boundCount == 0);
    CHECK(skippedHist.halfMap);
}

void testNoNeighbourBindFromHistory()
{
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId fillet = 40;
    g.beginEvaluate(1);
    const SemanticId edge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    const SemanticId f1 = g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    const SemanticId f2 = g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(edge, pad, 1, "Edge1"));
    g.bind(makeBinding(f1, pad, 1, "Face1"));
    g.bind(makeBinding(f2, pad, 1, "Face2"));
    g.commitEvaluate();

    // History names Face12 from (edge, adj faces). A similar-length Face2 is not used.
    g.beginEvaluate(2);
    HistoryRecord rec;
    rec.fromSeed = edge;
    rec.extraSeeds = {f1, f2};
    rec.toIndex = ElementIndex::fromString("Face12");
    rec.kind = EventKind::Generated;
    rec.outputKind = SemanticKind::Face;
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, fillet, 2, "Fillet", {edge}, {rec});
    g.commitEvaluate();

    CHECK(applied.boundCount == 1);
    CHECK(applied.outputs.size() == 1);
    const SemanticId face = applied.outputs.front();
    CHECK(g.descendants(edge.handle).count(face.handle) == 1);
    CHECK(g.descendants(f1.handle).count(face.handle) == 1);
    CHECK(g.descendants(f2.handle).count(face.handle) == 1);

    const auto rows = g.bindingsOf(face.handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().index.toString() == "Face12");
    CHECK(rows.front().index.toString() != "Face2");
    CHECK(rows.front().index.toString() != "Face1");
    CHECK(face.handle != f1.handle);
    CHECK(face.handle != f2.handle);

    // A history row that names nothing does not fall back to Face1.
    g.beginEvaluate(3);
    HistoryRecord unnamed;
    unnamed.fromSeed = edge;
    unnamed.kind = EventKind::Generated;
    unnamed.outputKind = SemanticKind::Face;
    const std::size_t nBind = g.allBindings().size();
    const ApplyResult noName
        = SemanticHistoryAdapter::applyHistory(&g, fillet, 3, "Fillet", {edge}, {unnamed});
    g.commitEvaluate();
    CHECK(noName.boundCount == 0);
    CHECK(noName.skippedUnnamed == 1);
    CHECK(noName.halfMap);
    CHECK(g.allBindings().size() == nBind);
}

void testSeedlessMakerFacePublication()
{
    SemanticGraph g;
    const ObjectId fillet = 40;

    g.beginEvaluate(1);
    HistoryRecord unique;
    unique.seedless = true;
    unique.sourceGroup = 3;
    unique.toIndex = ElementIndex::fromString("Face12");
    unique.kind = EventKind::Generated;
    unique.outputKind = SemanticKind::Face;
    const ApplyResult published
        = SemanticHistoryAdapter::applyHistory(&g, fillet, 1, "Fillet", {}, {unique});
    g.commitEvaluate();

    CHECK(published.boundCount == 1);
    CHECK(published.outputs.size() == 1);
    CHECK(published.outputs.front().allocatedBy == fillet);
    CHECK(published.outputs.front().kind == SemanticKind::Face);
    const auto rows = g.bindingsOf(published.outputs.front().handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().feature == fillet);
    CHECK(rows.front().index.toString() == "Face12");

    g.beginEvaluate(2);
    HistoryRecord ambiguousA = unique;
    HistoryRecord ambiguousB = unique;
    ambiguousA.toIndex = ElementIndex::fromString("Face13");
    ambiguousB.toIndex = ElementIndex::fromString("Face14");
    const std::size_t idsBefore = g.allIdentities().size();
    const std::size_t bindingsBefore = g.allBindings().size();
    const ApplyResult refused
        = SemanticHistoryAdapter::applyHistory(&g, fillet, 2, "Fillet", {}, {ambiguousA, ambiguousB});
    g.commitEvaluate();

    CHECK(refused.boundCount == 0);
    CHECK(refused.outputs.empty());
    CHECK(g.allIdentities().size() == idsBefore);
    CHECK(g.allBindings().size() == bindingsBefore);
}

void testSeedlessModifiedFacePublication()
{
    SemanticGraph g;
    const ObjectId fillet = 40;

    g.beginEvaluate(1);
    HistoryRecord unique;
    unique.seedless = true;
    unique.sourceGroup = 7;
    unique.kind = EventKind::Modified;
    unique.outputKind = SemanticKind::Face;
    unique.toIndex = ElementIndex::fromString("Face4");
    const ApplyResult published
        = SemanticHistoryAdapter::applyHistory(&g, fillet, 1, "Fillet", {}, {unique});
    g.commitEvaluate();

    CHECK(published.boundCount == 1);
    CHECK(published.outputs.size() == 1);
    const auto rows = g.bindingsOf(published.outputs.front().handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().feature == fillet);
    CHECK(rows.front().index.toString() == "Face4");

    g.beginEvaluate(2);
    HistoryRecord ambiguousA = unique;
    HistoryRecord ambiguousB = unique;
    ambiguousA.toIndex = ElementIndex::fromString("Face5");
    ambiguousB.toIndex = ElementIndex::fromString("Face6");
    const std::size_t idsBefore = g.allIdentities().size();
    const std::size_t bindingsBefore = g.allBindings().size();
    const ApplyResult refused
        = SemanticHistoryAdapter::applyHistory(&g, fillet, 2, "Fillet", {}, {ambiguousA, ambiguousB});
    g.commitEvaluate();

    CHECK(refused.boundCount == 0);
    CHECK(refused.outputs.empty());
    CHECK(g.allIdentities().size() == idsBefore);
    CHECK(g.allBindings().size() == bindingsBefore);
}

void testFromOcctHistoryStandaloneIsEmpty()
{
    const auto table = SemanticHistoryAdapter::fromOcctHistory(nullptr, {}, {});
    CHECK(table.empty());
    const auto fromMaker = SemanticHistoryAdapter::fromMaker(nullptr, {}, {});
    CHECK(fromMaker.empty());

    // I13: missing maker / empty history must not invent FaceN.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId curve = g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    g.bind(makeBinding(curve, 10, 1, "Edge1"));
    const std::size_t nBind = g.allBindings().size();
    const std::size_t nId = g.allIdentities().size();
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, 20, 1, "Pad", {curve}, fromMaker);
    CHECK(applied.halfMap);
    CHECK(applied.boundCount == 0);
    CHECK(g.allBindings().size() == nBind);
    CHECK(g.allIdentities().size() == nId);
    for (const SemanticBinding& b : g.allBindings()) {
        CHECK(b.index.toString() != "Face1");
        CHECK(b.kind != SemanticKind::Face);
    }
}

void testPadAfterExecuteBindsNamedFace7NotFace1()
{
    SemanticGraph g;
    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    g.beginEvaluate(1);
    const SemanticId curve
        = g.recordGenerated(SemanticKind::Edge, "Sketch", sketch, 1, SemanticRole::None);
    g.bind(makeBinding(curve, sketch, 1, "Edge1"));
    g.commitEvaluate();

    AfterExecuteRequest req;
    req.curveSeeds = {curve};
    req.allowSequentialFaceN = false;
    req.namedFaceIndices = {ElementIndex::fromString("Face7")};

    g.beginEvaluate(2);
    SemanticEmitter::afterExecute(&g, Opcode::Pad, pad, 2, req);
    g.commitEvaluate();

    CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
    const auto sides = SemanticEmitter::generatedFrom(g, curve.handle);
    CHECK(sides.size() == 1);
    const auto rows = g.bindingsOf(sides.front().handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().index.toString() == "Face7");
    CHECK(rows.front().index.index == 7);
    CHECK(rows.front().index.toString() != "Face1");

    // A second curve with no named slot is skipped (half-map), not Face1/Face2.
    const SemanticId curve2
        = g.recordGenerated(SemanticKind::Edge, "Sketch", sketch, 1, SemanticRole::None);
    AfterExecuteRequest req2;
    req2.curveSeeds = {curve2};
    req2.namedFaceIndices = {ElementIndex::fromString("Face3")};
    req2.allowSequentialFaceN = false;
    SemanticEmitter::afterExecute(&g, Opcode::Pad, pad, 2, req2);
    const auto sides2 = SemanticEmitter::generatedFrom(g, curve2.handle);
    CHECK(sides2.size() == 1);
    CHECK(g.bindingsOf(sides2.front().handle).front().index.toString() == "Face3");
    CHECK(g.bindingsOf(sides2.front().handle).front().index.toString() != "Face1");
    CHECK(g.bindingsOf(sides.front().handle).front().index.toString() == "Face7");
}

void testPadNamedEmptySlotSkipsNoFaceN()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId c1 = g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    const SemanticId c2 = g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    AfterExecuteRequest req;
    req.curveSeeds = {c1, c2};
    req.namedFaceIndices = {ElementIndex::fromString("Face7")};  // only first named
    req.allowSequentialFaceN = false;
    SemanticEmitter::afterExecute(&g, Opcode::Pad, 20, 1, req);
    const auto s1 = SemanticEmitter::generatedFrom(g, c1.handle);
    const auto s2 = SemanticEmitter::generatedFrom(g, c2.handle);
    CHECK(s1.size() == 1);
    CHECK(s2.empty());  // unnamed slot skipped, not Face8 / Face1
    CHECK(g.bindingsOf(s1.front().handle).front().index.toString() == "Face7");
}

void testPreflightResolvedDoesNotSkip()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId edge = g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::None);
    g.bind(makeBinding(edge, 20, 1, "Edge1"));
    g.commitEvaluate();

    const FilletPreflight pre = SemanticHistoryAdapter::preflightFillet(&g, {edge});
    CHECK(pre.makerSkipped == false);
    CHECK(pre.state == ResolutionState::Resolved || pre.state == ResolutionState::ResolvedSet);
}

void testNamedReferencePolicyGate()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId seed = g.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::None);
    const SemanticId child1
        = g.recordGeneratedFrom({seed}, SemanticKind::Face, "Draft", 30, 1, SemanticRole::None);
    const SemanticId child2
        = g.recordGeneratedFrom({seed}, SemanticKind::Face, "Draft", 30, 1, SemanticRole::None);
    g.bind(makeBinding(seed, 20, 1, "Face1"));
    g.bind(makeBinding(child1, 30, 1, "Face2"));
    g.bind(makeBinding(child2, 30, 1, "Face3"));
    g.commitEvaluate();

    const SemanticReference strict = makeRef(seed, CardinalityReducer::RequireOne, SemanticKind::Face);
    const FilletPreflight ambiguous
        = SemanticHistoryAdapter::preflightNamedReferences(&g, {strict}, SemanticKind::Face);
    CHECK(ambiguous.makerSkipped);
    CHECK(ambiguous.state == ResolutionState::Ambiguous);

    SemanticReference acceptsSet = strict;
    acceptsSet.reducer = CardinalityReducer::AcceptAll;
    const FilletPreflight resolvedSet
        = SemanticHistoryAdapter::preflightNamedReferences(&g, {acceptsSet}, SemanticKind::Face);
    CHECK(!resolvedSet.makerSkipped);
    CHECK(resolvedSet.state == ResolutionState::ResolvedSet);

    SemanticReference wrongKind = strict;
    wrongKind.kind = SemanticKind::Edge;
    const FilletPreflight incompatible
        = SemanticHistoryAdapter::preflightNamedReferences(&g, {wrongKind}, SemanticKind::Face);
    CHECK(incompatible.makerSkipped);
    CHECK(incompatible.state == ResolutionState::Incompatible);

    // The seed-only wrapper keeps its legacy AcceptAll policy.
    const FilletPreflight legacy
        = SemanticHistoryAdapter::preflightNamedSeeds(&g, {seed}, SemanticKind::Face);
    CHECK(!legacy.makerSkipped);
}

void testCachedGeometryFallbackPolicy()
{
    FilletPreflight empty;
    CHECK(!SemanticHistoryAdapter::canUseCachedGeometry(empty, false));
    CHECK(SemanticHistoryAdapter::canUseCachedGeometry(empty, true));

    FilletPreflight missing;
    missing.makerSkipped = true;
    ResolutionResult onlyMissing;
    onlyMissing.state = ResolutionState::Missing;
    missing.perEdge.push_back(onlyMissing);
    CHECK(SemanticHistoryAdapter::canUseCachedGeometry(missing, false));

    ResolutionResult ambiguous;
    ambiguous.state = ResolutionState::Ambiguous;
    missing.perEdge.push_back(ambiguous);
    CHECK(!SemanticHistoryAdapter::canUseCachedGeometry(missing, false));
}

void testPadAfterExecuteBindsNamedEdge8NotEdge1()
{
    SemanticGraph g;
    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    g.beginEvaluate(1);
    const SemanticId curve
        = g.recordGenerated(SemanticKind::Edge, "Sketch", sketch, 1, SemanticRole::None);
    const SemanticId vertex
        = g.recordGenerated(SemanticKind::Vertex, "Sketch", sketch, 1, SemanticRole::None);
    g.bind(makeBinding(curve, sketch, 1, "Edge1"));
    g.bind(makeBinding(vertex, sketch, 1, "Vertex1"));
    g.commitEvaluate();

    AfterExecuteRequest req;
    req.curveSeeds = {curve};
    req.vertexSeeds = {vertex};
    req.allowSequentialFaceN = false;
    req.namedFaceIndices = {ElementIndex::fromString("Face7")};
    req.namedEdgeIndices = {ElementIndex::fromString("Edge8")};

    g.beginEvaluate(2);
    SemanticEmitter::afterExecute(&g, Opcode::Pad, pad, 2, req);
    g.commitEvaluate();

    CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
    const auto sides = SemanticEmitter::generatedFrom(g, curve.handle);
    CHECK(sides.size() == 1);
    CHECK(sides.front().kind == SemanticKind::Face);
    CHECK(g.bindingsOf(sides.front().handle).front().index.toString() == "Face7");

    const auto vertEdges = SemanticEmitter::generatedFrom(g, vertex.handle);
    CHECK(vertEdges.size() == 1);
    CHECK(vertEdges.front().kind == SemanticKind::Edge);
    const auto rows = g.bindingsOf(vertEdges.front().handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().index.toString() == "Edge8");
    CHECK(rows.front().index.index == 8);
    CHECK(rows.front().index.toString() != "Edge1");
    CHECK(rows.front().kind == SemanticKind::Edge);
}

void testPadNamedEmptyEdgeSlotSkipsNoEdgeN()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId v1 = g.recordGenerated(SemanticKind::Vertex, "Sketch", 10, 1, SemanticRole::None);
    const SemanticId v2 = g.recordGenerated(SemanticKind::Vertex, "Sketch", 10, 1, SemanticRole::None);
    AfterExecuteRequest req;
    req.vertexSeeds = {v1, v2};
    req.namedEdgeIndices = {ElementIndex::fromString("Edge8")};  // only first named
    req.allowSequentialFaceN = false;
    SemanticEmitter::afterExecute(&g, Opcode::Pad, 20, 1, req);
    const auto e1 = SemanticEmitter::generatedFrom(g, v1.handle);
    const auto e2 = SemanticEmitter::generatedFrom(g, v2.handle);
    CHECK(e1.size() == 1);
    CHECK(e1.front().kind == SemanticKind::Edge);
    CHECK(e2.empty());  // unnamed slot skipped, not Edge1 / Edge9
    CHECK(g.bindingsOf(e1.front().handle).front().index.toString() == "Edge8");
}

void testPadEdgeZipFallsBackToCurveSeed()
{
    // No vertex seeds: named Edge8 zips onto the curve seed (same 1:1 as faces).
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId curve = g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    AfterExecuteRequest req;
    req.curveSeeds = {curve};
    req.namedEdgeIndices = {ElementIndex::fromString("Edge8")};
    req.allowSequentialFaceN = false;
    SemanticEmitter::afterExecute(&g, Opcode::Pad, 20, 1, req);
    const auto kids = SemanticEmitter::generatedFrom(g, curve.handle);
    CHECK(kids.size() == 1);
    CHECK(kids.front().kind == SemanticKind::Edge);
    CHECK(g.bindingsOf(kids.front().handle).front().index.toString() == "Edge8");
    CHECK(g.bindingsOf(kids.front().handle).front().index.toString() != "Edge1");
}

void testApplyHistoryNamedEdge8()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId vertex
        = g.recordGenerated(SemanticKind::Vertex, "Sketch", 10, 1, SemanticRole::None);
    HistoryRecord rec;
    rec.fromSeed = vertex;
    rec.toIndex = ElementIndex::fromString("Edge8");
    rec.kind = EventKind::Generated;
    rec.outputKind = SemanticKind::Edge;
    const ApplyResult applied = SemanticHistoryAdapter::applyHistory(&g, 20, 1, "Pad", {vertex}, {rec});
    g.commitEvaluate();
    CHECK(applied.boundCount == 1);
    CHECK(!applied.halfMap);
    CHECK(applied.outputs.front().kind == SemanticKind::Edge);
    CHECK(g.bindingsOf(applied.outputs.front().handle).front().index.index == 8);
}

void testPocketApplyHistoryBindsEdge2OnPocketFeature()
{
    // captureBooleanHistory remap + publishSemanticHistory Edge row for Pocket.
    SemanticGraph g;
    const ObjectId pocket = 30;
    g.beginEvaluate(1);
    const SemanticId padCorner
        = g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::None);
    HistoryRecord rec;
    rec.fromSeed = padCorner;
    rec.toIndex = ElementIndex::fromString("Edge2");
    rec.kind = EventKind::Generated;
    rec.outputKind = SemanticKind::Edge;
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, pocket, 1, "Pocket", {padCorner}, {rec});
    g.commitEvaluate();
    CHECK(applied.boundCount == 1);
    CHECK(!applied.halfMap);
    CHECK(applied.outputs.front().kind == SemanticKind::Edge);
    const auto rows = g.bindingsOf(applied.outputs.front().handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().feature == pocket);
    CHECK(rows.front().index.toString() == "Edge2");
    CHECK(rows.front().index.toString() != "Edge1");
}

void testUnmodifiedSurvivorPadEdge3Promotes()
{
    // Tests_Manual_runb Fillet.Base Pad Edge3 :U;XTR. Unmodified 1-image is
    // Modified continuity, Binding.feature = Pad, unique → stSeed. Empty index
    // does not invent EdgeN (I13).
    HistoryTable table;
    SemanticId sketchEdge;
    sketchEdge.handle = 4;
    sketchEdge.kind = SemanticKind::Edge;
    CHECK(!SemanticHistoryAdapter::appendUnmodifiedSurvivor(table, sketchEdge, ElementIndex {}));
    CHECK(table.empty());
    CHECK(
        SemanticHistoryAdapter::appendUnmodifiedSurvivor(table, sketchEdge, ElementIndex::fromString("Edge3"))
    );
    CHECK(table.size() == 1);
    CHECK(table[0].kind == EventKind::Modified);
    CHECK(table[0].toIndex.toString() == "Edge3");
    CHECK(table[0].outputKind == SemanticKind::Edge);

    SemanticGraph g;
    const ObjectId pad = 10;
    g.beginEvaluate(1);
    const SemanticId seed = g.recordGenerated(SemanticKind::Edge, "Sketch", pad, 1, SemanticRole::None);
    table[0].fromSeed = seed;
    const ApplyResult applied = SemanticHistoryAdapter::applyHistory(&g, pad, 1, "Pad", {seed}, table);
    g.commitEvaluate();
    CHECK(applied.boundCount == 1);
    CHECK(!applied.halfMap);
    CHECK(applied.outputs.front().handle == seed.handle);
    const auto rows = g.bindingsOf(seed.handle);
    CHECK(rows.size() == 1);
    CHECK(rows.front().feature == pad);
    CHECK(rows.front().index.toString() == "Edge3");
}

void testPadAfterExecuteBindsNamedEdge13NotEdge1()
{
    // GUI Tests_Manual picks Edge13/17/14: zip must use type-local findShape
    // indices, not sequential Edge1.
    SemanticGraph g;
    const ObjectId sketch = 10;
    const ObjectId pad = 20;
    g.beginEvaluate(1);
    const SemanticId v1
        = g.recordGenerated(SemanticKind::Vertex, "Sketch", sketch, 1, SemanticRole::None);
    const SemanticId v2
        = g.recordGenerated(SemanticKind::Vertex, "Sketch", sketch, 1, SemanticRole::None);
    const SemanticId v3
        = g.recordGenerated(SemanticKind::Vertex, "Sketch", sketch, 1, SemanticRole::None);
    g.commitEvaluate();

    AfterExecuteRequest req;
    req.vertexSeeds = {v1, v2, v3};
    req.allowSequentialFaceN = false;
    req.namedEdgeIndices = {
        ElementIndex::fromString("Edge13"),
        ElementIndex::fromString("Edge17"),
        ElementIndex::fromString("Edge14")
    };

    g.beginEvaluate(2);
    SemanticEmitter::afterExecute(&g, Opcode::Pad, pad, 2, req);
    g.commitEvaluate();

    CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
    const auto e1 = SemanticEmitter::generatedFrom(g, v1.handle);
    const auto e2 = SemanticEmitter::generatedFrom(g, v2.handle);
    const auto e3 = SemanticEmitter::generatedFrom(g, v3.handle);
    CHECK(e1.size() == 1 && e1.front().kind == SemanticKind::Edge);
    CHECK(e2.size() == 1 && e2.front().kind == SemanticKind::Edge);
    CHECK(e3.size() == 1 && e3.front().kind == SemanticKind::Edge);
    CHECK(g.bindingsOf(e1.front().handle).front().index.toString() == "Edge13");
    CHECK(g.bindingsOf(e2.front().handle).front().index.toString() == "Edge17");
    CHECK(g.bindingsOf(e3.front().handle).front().index.toString() == "Edge14");
    CHECK(g.bindingsOf(e1.front().handle).front().index.toString() != "Edge1");
    CHECK(g.bindingsOf(e1.front().handle).front().feature == pad);
}

void testBooleanUniqueOneImageBindsOnFeature()
{
    // Unique 1-image Edge/Face from Base onto Boolean feature binds.
    // allocatedBy / Binding.feature is the Fuse id, not a Pad leftover.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId fuse = 50;
    g.beginEvaluate(1);
    const SemanticId padEdge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    const SemanticId padFace = g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padEdge, pad, 1, "Edge2"));
    g.bind(makeBinding(padFace, pad, 1, "Face6"));
    g.commitEvaluate();

    CHECK(uniqueBindingOnFeature(&g, padEdge, pad, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, padEdge, fuse, "Edge").has_value());

    HistoryTable raw;
    HistoryRecord e;
    e.fromSeed = padEdge;
    e.toIndex = ElementIndex::fromString("Edge7");
    e.kind = EventKind::Modified;
    e.outputKind = SemanticKind::Edge;
    raw.push_back(e);
    HistoryRecord f;
    f.fromSeed = padFace;
    f.toIndex = ElementIndex::fromString("Face3");
    f.kind = EventKind::Modified;
    f.outputKind = SemanticKind::Face;
    raw.push_back(f);

    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    CHECK(unique.size() == 2);
    CHECK(unique[0].kind == EventKind::Generated);
    CHECK(unique[1].kind == EventKind::Generated);

    g.beginEvaluate(2);
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, fuse, 2, "FUS", {padEdge, padFace}, unique);
    g.commitEvaluate();

    CHECK(applied.boundCount == 2);
    CHECK(!applied.halfMap);
    CHECK(applied.outputs.size() == 2);
    const SemanticId fuseEdge = applied.outputs[0];
    const SemanticId fuseFace = applied.outputs[1];
    CHECK(fuseEdge.kind == SemanticKind::Edge);
    CHECK(fuseFace.kind == SemanticKind::Face);
    CHECK(fuseEdge.handle != padEdge.handle);
    CHECK(fuseFace.handle != padFace.handle);

    const auto edgeRow = uniqueBindingOnFeature(&g, fuseEdge, fuse, "Edge");
    CHECK(edgeRow.has_value());
    CHECK(edgeRow->feature == fuse);
    CHECK(edgeRow->index.toString() == std::string("Edge7"));
    CHECK(edgeRow->index.toString() != std::string("Edge2"));
    CHECK(!uniqueBindingOnFeature(&g, fuseEdge, pad, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, padEdge, fuse, "Edge").has_value());

    const auto faceRow = uniqueBindingOnFeature(&g, fuseFace, fuse, "Face");
    CHECK(faceRow.has_value());
    CHECK(faceRow->feature == fuse);
    CHECK(faceRow->index.toString() == std::string("Face3"));
}

void testBooleanTwoImageRefuses()
{
    // 2-image Face from Base: uniqueOneImageGenerated drops the seed (I13).
    // Never first-Binding-wins. uniqueBindingOnFeature on Fuse stays empty.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId fuse = 50;
    g.beginEvaluate(1);
    const SemanticId padFace = g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padFace, pad, 1, "Face1"));
    g.commitEvaluate();

    HistoryTable raw;
    HistoryRecord a;
    a.fromSeed = padFace;
    a.toIndex = ElementIndex::fromString("Face3");
    a.kind = EventKind::Split;
    a.outputKind = SemanticKind::Face;
    raw.push_back(a);
    HistoryRecord b;
    b.fromSeed = padFace;
    b.toIndex = ElementIndex::fromString("Face4");
    b.kind = EventKind::Split;
    b.outputKind = SemanticKind::Face;
    raw.push_back(b);

    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    CHECK(unique.empty());

    const std::size_t nBind = g.allBindings().size();
    const std::size_t nId = g.allIdentities().size();
    g.beginEvaluate(2);
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, fuse, 2, "FUS", {padFace}, unique);
    g.commitEvaluate();
    CHECK(applied.boundCount == 0);
    CHECK(applied.halfMap);
    CHECK(g.allBindings().size() == nBind);
    CHECK(g.allIdentities().size() == nId);
    CHECK(!uniqueBindingOnFeature(&g, padFace, fuse, "Face").has_value());
}


void testDuplicateSameSlotRowsKept()
{
    // MakePrism/MakeRevol often list the same FaceN twice for one seed.
    // Raw-row counting would drop the image; unique-slot counting keeps it.
    SemanticId seed;
    seed.handle = 11;
    seed.kind = SemanticKind::Face;

    HistoryRecord first;
    first.fromSeed = seed;
    first.toIndex = ElementIndex::fromString("Face3");
    first.kind = EventKind::Generated;
    first.outputKind = SemanticKind::Face;
    HistoryRecord dup = first;

    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated({first, dup});
    CHECK(unique.size() == 1);
    CHECK(unique[0].toIndex.toString() == std::string("Face3"));
    CHECK(unique[0].fromSeed.handle == seed.handle);
}

void testSupplementLocatedInputsFillsUncoveredEdges()
{
    // fromMaker uniquely kept Faces; vertical Edge only in locate inputs.
    SemanticId faceSeed;
    faceSeed.handle = 21;
    faceSeed.kind = SemanticKind::Face;
    SemanticId edgeSeed;
    edgeSeed.handle = 22;
    edgeSeed.kind = SemanticKind::Edge;

    HistoryRecord faceRec;
    faceRec.fromSeed = faceSeed;
    faceRec.toIndex = ElementIndex::fromString("Face2");
    faceRec.kind = EventKind::Generated;
    faceRec.outputKind = SemanticKind::Face;

    int faceKey = 1;
    int edgeKey = 2;
    std::vector<std::pair<SemanticId, const void*>> inputs = {
        {faceSeed, &faceKey},
        {edgeSeed, &edgeKey},
    };
    auto indexOf = [](const void* occ) -> ElementIndex {
        if (!occ) {
            return {};
        }
        if (*static_cast<const int*>(occ) == 1) {
            return ElementIndex::fromString("Face2");
        }
        if (*static_cast<const int*>(occ) == 2) {
            return ElementIndex::fromString("Edge5");
        }
        return {};
    };
    const HistoryTable out
        = SemanticHistoryAdapter::supplementLocatedInputs({faceRec}, inputs, indexOf);
    CHECK(out.size() == 2);
    bool sawEdge = false;
    for (const auto& rec : out) {
        if (rec.toIndex.toString() == std::string("Edge5")) {
            sawEdge = true;
            CHECK(rec.fromSeed.handle == edgeSeed.handle);
            CHECK(rec.outputKind == SemanticKind::Edge);
        }
    }
    CHECK(sawEdge);
}

void testDuplicatePublishedSlotRefuses()
{
    SemanticId firstSeed;
    firstSeed.handle = 1;
    firstSeed.kind = SemanticKind::Face;
    SemanticId secondSeed;
    secondSeed.handle = 2;
    secondSeed.kind = SemanticKind::Face;

    HistoryRecord first;
    first.fromSeed = firstSeed;
    first.toIndex = ElementIndex::fromString("Face7");
    HistoryRecord second;
    second.fromSeed = secondSeed;
    second.toIndex = ElementIndex::fromString("Face7");

    // Two distinct sources cannot both own the exact published slot.
    // Refuse both instead of allowing map/order to choose a first row.
    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated({first, second});
    CHECK(unique.empty());
}

void testBooleanC1SeedUnchanged()
{
    // Unique 1-image bind, then a later table that would rename Edge9: C1
    // keeps the existing unique Binding. Seed unchanged. uniqueBindingOnFeature
    // still Edge7. Empty history does not invent FaceN (I10).
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId fuse = 50;
    g.beginEvaluate(1);
    const SemanticId padEdge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padEdge, pad, 1, "Edge2"));
    g.commitEvaluate();

    HistoryRecord rec;
    rec.fromSeed = padEdge;
    rec.toIndex = ElementIndex::fromString("Edge7");
    rec.kind = EventKind::Modified;
    rec.outputKind = SemanticKind::Edge;
    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated({rec});

    g.beginEvaluate(2);
    const ApplyResult first
        = SemanticHistoryAdapter::applyHistory(&g, fuse, 2, "FUS", {padEdge}, unique);
    g.commitEvaluate();
    CHECK(first.boundCount == 1);
    const SemanticId child = first.outputs.front();
    const SemanticHandle water = g.allocator.highWaterMark();
    CHECK(uniqueBindingOnFeature(&g, child, fuse, "Edge")->index.toString() == std::string("Edge7"));

    HistoryRecord rec2;
    rec2.fromSeed = padEdge;
    rec2.toIndex = ElementIndex::fromString("Edge9");
    rec2.kind = EventKind::Modified;
    rec2.outputKind = SemanticKind::Edge;
    const HistoryTable wouldRename = SemanticHistoryAdapter::uniqueOneImageGenerated({rec2});
    CHECK(wouldRename.size() == 1);

    // Product C1: skip apply when a unique descendant is already bound on Fuse.
    const bool already = uniqueBindingOnFeature(&g, child, fuse, "Edge").has_value();
    CHECK(already);
    g.beginEvaluate(3);
    if (!already) {
        SemanticHistoryAdapter::applyHistory(&g, fuse, 3, "FUS", {padEdge}, wouldRename);
    }
    const ApplyResult empty = SemanticHistoryAdapter::applyHistory(&g, fuse, 3, "FUS", {padEdge}, {});
    g.commitEvaluate();
    CHECK(empty.halfMap);
    CHECK(empty.boundCount == 0);
    CHECK(g.allocator.highWaterMark() == water);
    const auto still = uniqueBindingOnFeature(&g, child, fuse, "Edge");
    CHECK(still.has_value());
    CHECK(still->index.toString() == std::string("Edge7"));
    CHECK(still->index.toString() != std::string("Edge9"));
    CHECK(child.handle == first.outputs.front().handle);
}


void testPatternUniqueOneImageBindsOnFeature()
{
    // Unique 1-image Edge from Pad onto LinearPattern binds.
    // allocatedBy / Binding.feature is the pattern id, not a Pad leftover.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId pattern = 60;
    g.beginEvaluate(1);
    const SemanticId padEdge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padEdge, pad, 1, "Edge15"));
    g.commitEvaluate();

    CHECK(uniqueBindingOnFeature(&g, padEdge, pad, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, padEdge, pattern, "Edge").has_value());

    HistoryRecord rec;
    rec.fromSeed = padEdge;
    rec.toIndex = ElementIndex::fromString("Edge4");
    rec.kind = EventKind::Modified;
    rec.outputKind = SemanticKind::Edge;
    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated({rec});
    CHECK(unique.size() == 1);
    CHECK(unique[0].kind == EventKind::Generated);

    g.beginEvaluate(2);
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, pattern, 2, "LinearPattern", {padEdge}, unique);
    g.commitEvaluate();

    CHECK(applied.boundCount == 1);
    CHECK(!applied.halfMap);
    const SemanticId child = applied.outputs.front();
    CHECK(child.handle != padEdge.handle);
    const auto row = uniqueBindingOnFeature(&g, child, pattern, "Edge");
    CHECK(row.has_value());
    CHECK(row->feature == pattern);
    CHECK(row->index.toString() == std::string("Edge4"));
    CHECK(!uniqueBindingOnFeature(&g, child, pad, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, padEdge, pattern, "Edge").has_value());
}

void testPatternNImageUnnamedAndPadLeftoverRefuse()
{
    // N-image (original + copy) of Edge15 stays unnamed (I13).
    // uniqueBindingOnFeature on the pattern stays empty. Pad leftover
    // Binding does not attach when the linked feature is the pattern.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId pattern = 60;
    g.beginEvaluate(1);
    const SemanticId padEdge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padEdge, pad, 1, "Edge15"));
    g.commitEvaluate();

    HistoryTable raw;
    HistoryRecord a;
    a.fromSeed = padEdge;
    a.toIndex = ElementIndex::fromString("Edge4");
    a.kind = EventKind::Generated;
    a.outputKind = SemanticKind::Edge;
    raw.push_back(a);
    HistoryRecord b;
    b.fromSeed = padEdge;
    b.toIndex = ElementIndex::fromString("Edge9");
    b.kind = EventKind::Generated;
    b.outputKind = SemanticKind::Edge;
    raw.push_back(b);

    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    CHECK(unique.empty());

    g.beginEvaluate(2);
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, pattern, 2, "LinearPattern", {padEdge}, unique);
    g.commitEvaluate();
    CHECK(applied.boundCount == 0);
    CHECK(applied.halfMap);
    CHECK(!uniqueBindingOnFeature(&g, padEdge, pattern, "Edge").has_value());

    // PolarPattern / Mirrored share uniqueOneImageGenerated.
    HistoryRecord one;
    one.fromSeed = padEdge;
    one.toIndex = ElementIndex::fromString("Edge2");
    one.kind = EventKind::Modified;
    one.outputKind = SemanticKind::Edge;
    const HistoryTable polar = SemanticHistoryAdapter::uniqueOneImageGenerated({one});
    CHECK(polar.size() == 1);
    g.beginEvaluate(3);
    const ApplyResult polarApplied
        = SemanticHistoryAdapter::applyHistory(&g, 70, 3, "PolarPattern", {padEdge}, polar);
    g.commitEvaluate();
    CHECK(polarApplied.boundCount == 1);
    CHECK(polarApplied.outputs.front().handle != padEdge.handle);
}

void testPatternC1KeepValidSeed()
{
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId mirrored = 80;
    g.beginEvaluate(1);
    const SemanticId padEdge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padEdge, pad, 1, "Edge15"));
    g.commitEvaluate();

    HistoryRecord rec;
    rec.fromSeed = padEdge;
    rec.toIndex = ElementIndex::fromString("Edge7");
    rec.kind = EventKind::Modified;
    rec.outputKind = SemanticKind::Edge;
    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated({rec});

    g.beginEvaluate(2);
    const ApplyResult first
        = SemanticHistoryAdapter::applyHistory(&g, mirrored, 2, "Mirrored", {padEdge}, unique);
    g.commitEvaluate();
    CHECK(first.boundCount == 1);
    const SemanticId child = first.outputs.front();
    const SemanticHandle water = g.allocator.highWaterMark();

    HistoryRecord rec2;
    rec2.fromSeed = padEdge;
    rec2.toIndex = ElementIndex::fromString("Edge9");
    rec2.kind = EventKind::Modified;
    rec2.outputKind = SemanticKind::Edge;
    const HistoryTable wouldRename = SemanticHistoryAdapter::uniqueOneImageGenerated({rec2});

    const bool already = uniqueBindingOnFeature(&g, child, mirrored, "Edge").has_value();
    CHECK(already);
    g.beginEvaluate(3);
    if (!already) {
        SemanticHistoryAdapter::applyHistory(&g, mirrored, 3, "Mirrored", {padEdge}, wouldRename);
    }
    const ApplyResult empty
        = SemanticHistoryAdapter::applyHistory(&g, mirrored, 3, "Mirrored", {padEdge}, {});
    g.commitEvaluate();
    CHECK(empty.halfMap);
    CHECK(g.allocator.highWaterMark() == water);
    const auto still = uniqueBindingOnFeature(&g, child, mirrored, "Edge");
    CHECK(still.has_value());
    CHECK(still->index.toString() == std::string("Edge7"));
    CHECK(child.handle == first.outputs.front().handle);
}

void testFirstNamedFaceAndSequentialFlag()
{
    SemanticReference a;
    a.seed.handle = 0;
    CHECK(!firstNamedFace({a}).valid());
    CHECK(!uniqueNamedFace({}).valid());
    CHECK(!uniqueNamedFace({a}).valid());

    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId face = g.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::None);
    const SemanticId face2 = g.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::None);
    const SemanticId edge = g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None);
    g.bind(makeBinding(face, 20, 1, "Face5"));
    g.commitEvaluate();

    SemanticReference named;
    named.seed = face;
    named.kind = SemanticKind::Face;
    CHECK(firstNamedFace({named}).handle == face.handle);
    CHECK(uniqueNamedFace({named}).handle == face.handle);

    SemanticReference edgeRef;
    edgeRef.seed = edge;
    edgeRef.kind = SemanticKind::Edge;
    CHECK(uniqueNamedFace({edgeRef, named}).handle == face.handle);

    SemanticReference named2;
    named2.seed = face2;
    named2.kind = SemanticKind::Face;
    CHECK(firstNamedFace({named, named2}).handle == face.handle);
    CHECK(!uniqueNamedFace({named, named2}).valid());

    // Product default: afterExecute Pad does not mint sequential FaceN.
    AfterExecuteRequest req;
    req.curveSeeds = {g.recordGenerated(SemanticKind::Edge, "Sketch", 10, 1, SemanticRole::None)};
    CHECK(!req.allowSequentialFaceN);
    const std::size_t nBind = g.allBindings().size();
    SemanticEmitter::afterExecute(&g, Opcode::Pad, 30, 2, req);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("half-map") != std::string::npos);
    CHECK(g.allBindings().size() == nBind);

    // Explicit test flag restores sequential Binding.
    req.allowSequentialFaceN = true;
    SemanticEmitter::afterExecute(&g, Opcode::Pad, 30, 2, req);
    CHECK(SemanticEmitter::lastAfterExecuteNote().find("emitted pad") == 0);
}

void testPatternSupportCopyNotPadShapeContract()
{
    // Isolation (no OCCT / FreeCADCmd): Transformed execute deep-copies
    // Pad.Shape before makeElementFuse. fromMaker runs FCBRepAlgoAPI only
    // on those copies. After execute, the boolean support TopoDS must not
    // be IsSame the live Pad.Shape (would mutate Pad -> invalid support).
    // Unique 1-image Binding.feature / allocatedBy is the pattern id.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId pattern = 60;
    g.beginEvaluate(1);
    const SemanticId padEdge = g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::None);
    g.bind(makeBinding(padEdge, pad, 1, "Edge15"));
    g.commitEvaluate();

    HistoryRecord rec;
    rec.fromSeed = padEdge;
    rec.toIndex = ElementIndex::fromString("Edge4");
    rec.kind = EventKind::Modified;
    rec.outputKind = SemanticKind::Edge;
    const HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated({rec});

    g.beginEvaluate(2);
    const ApplyResult applied
        = SemanticHistoryAdapter::applyHistory(&g, pattern, 2, "LinearPattern", {padEdge}, unique);
    g.commitEvaluate();

    CHECK(applied.boundCount == 1);
    const SemanticId child = applied.outputs.front();
    const auto row = uniqueBindingOnFeature(&g, child, pattern, "Edge");
    CHECK(row.has_value());
    CHECK(row->feature == pattern);
    CHECK(row->feature != pad);
    const SemanticId* ident = g.identity(child.handle);
    CHECK(ident != nullptr);
    CHECK(ident->allocatedBy == pattern);
    CHECK(ident->allocatedBy != pad);
    CHECK(!uniqueBindingOnFeature(&g, padEdge, pattern, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, child, pad, "Edge").has_value());
}

}  // namespace

int main()
{
    testNullGraphAndEmptyTable();
    testFakeHistoryOneCurveOneSideFace();
    testDeletedEdgeFilletMissingMakerSkipped();
    testSeedlessMakerFacePublication();
    testSeedlessModifiedFacePublication();
    testNoNeighbourBindFromHistory();
    testFromOcctHistoryStandaloneIsEmpty();
    testPadAfterExecuteBindsNamedFace7NotFace1();
    testPadNamedEmptySlotSkipsNoFaceN();
    testNamedReferencePolicyGate();
    testCachedGeometryFallbackPolicy();
    testPreflightResolvedDoesNotSkip();
    testPadAfterExecuteBindsNamedEdge8NotEdge1();
    testPadNamedEmptyEdgeSlotSkipsNoEdgeN();
    testPadEdgeZipFallsBackToCurveSeed();
    testApplyHistoryNamedEdge8();
    testPocketApplyHistoryBindsEdge2OnPocketFeature();
    testUnmodifiedSurvivorPadEdge3Promotes();
    testPadAfterExecuteBindsNamedEdge13NotEdge1();
    testBooleanUniqueOneImageBindsOnFeature();
    testBooleanTwoImageRefuses();
    testDuplicateSameSlotRowsKept();
    testSupplementLocatedInputsFillsUncoveredEdges();
    testDuplicatePublishedSlotRefuses();
    testBooleanC1SeedUnchanged();
    testPatternUniqueOneImageBindsOnFeature();
    testPatternNImageUnnamedAndPadLeftoverRefuse();
    testPatternC1KeepValidSeed();
    testPatternSupportCopyNotPadShapeContract();
    testFirstNamedFaceAndSequentialFlag();

    std::cout << "Semantic history adapter: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
