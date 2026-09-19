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
// Standalone tests for App::SemanticDocumentState (no Qt / OCCT / FreeCADApp).
// Metric is ResolutionState + handle identity, not FaceN equality.

#include "SemanticDocumentState.h"
#include "SemanticId.h"
#include "SemanticReference.h"
#include "SemanticTopology.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace App;

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

SemanticId generateFace(SemanticGraph& g, ObjectId feature, EvalSerial eval)
{
    return g.recordGenerated(SemanticKind::Face, "Test", feature, eval, SemanticRole::None);
}

void testOwnsGraphAndGraphFor()
{
    SemanticDocumentState state;
    int fakeDoc = 1;
    int fakeFeature = 2;
    int other = 3;

    CHECK(SemanticDocumentState::graphFor(nullptr) == nullptr);
    CHECK(SemanticDocumentState::graphFor(&fakeDoc) == nullptr);
    CHECK(SemanticDocumentState::stateFor(&fakeDoc) == nullptr);

    state.bindOwner(&fakeDoc);
    CHECK(SemanticDocumentState::graphFor(&fakeDoc) == &state.graph());
    CHECK(SemanticDocumentState::stateFor(&fakeDoc) == &state);
    CHECK(SemanticDocumentState::graphFor(&fakeFeature) == nullptr);

    state.bindAlias(&fakeFeature);
    CHECK(SemanticDocumentState::graphFor(&fakeFeature) == &state.graph());
    CHECK(SemanticDocumentState::graphFor(&other) == nullptr);

    state.unbindAlias(&fakeFeature);
    CHECK(SemanticDocumentState::graphFor(&fakeFeature) == nullptr);
    CHECK(SemanticDocumentState::graphFor(&fakeDoc) == &state.graph());

    state.unbindOwner();
    CHECK(SemanticDocumentState::graphFor(&fakeDoc) == nullptr);
}

void testTwoDocumentsTwoGraphs()
{
    SemanticDocumentState a;
    SemanticDocumentState b;
    int docA = 1;
    int docB = 2;
    a.bindOwner(&docA);
    b.bindOwner(&docB);
    CHECK(SemanticDocumentState::graphFor(&docA) == &a.graph());
    CHECK(SemanticDocumentState::graphFor(&docB) == &b.graph());
    CHECK(SemanticDocumentState::graphFor(&docA) != SemanticDocumentState::graphFor(&docB));
}

void testBeginCommitEvaluate()
{
    SemanticDocumentState state;
    CHECK(!state.isEvaluating());
    CHECK(state.currentEval() == 0);
    CHECK(state.nextEvalSerial() == 1);

    const EvalSerial e1 = state.beginEvaluate();
    CHECK(e1 == 1);
    CHECK(state.isEvaluating());
    CHECK(state.currentEval() == 1);
    const SemanticId id = generateFace(state.graph(), 10, e1);
    CHECK(id.valid());
    CHECK(!state.graph().allocator.isPublished(id.handle));
    state.commitEvaluate();
    CHECK(!state.isEvaluating());
    CHECK(state.graph().allocator.isPublished(id.handle));
    CHECK(state.nextEvalSerial() == 2);
}

void testAbortBurnsHandlesI5()
{
    SemanticDocumentState state;
    state.beginEvaluate();
    const SemanticId kept = generateFace(state.graph(), 10, state.currentEval());
    state.commitEvaluate();
    const SemanticHandle waterAfterKeep = state.graph().allocator.highWaterMark();

    state.beginEvaluate();
    const SemanticId burned = generateFace(state.graph(), 10, state.currentEval());
    CHECK(burned.handle != kept.handle);
    const SemanticHandle waterAfterBurn = state.graph().allocator.highWaterMark();
    CHECK(waterAfterBurn > waterAfterKeep);
    state.abortEvaluate();

    CHECK(state.graph().allocator.isPublished(kept.handle));
    CHECK(state.graph().allocator.isBurned(burned.handle));
    CHECK(!state.graph().allocator.isPublished(burned.handle));
    CHECK(state.graph().allocator.highWaterMark() == waterAfterBurn);

    state.beginEvaluate();
    const SemanticId next = generateFace(state.graph(), 10, state.currentEval());
    state.commitEvaluate();
    CHECK(next.handle != burned.handle);
    CHECK(next.handle >= waterAfterBurn);
    CHECK(state.graph().allocator.highWaterMark() > burned.handle);
}

void testEvaluateScopeAbortOnUnwind()
{
    SemanticDocumentState state;
    SemanticHandle burned = 0;
    SemanticHandle water = 0;
    bool threw = false;
    try {
        SemanticDocumentState::EvaluateScope scope(state);
        const SemanticId id = generateFace(state.graph(), 11, scope.eval());
        burned = id.handle;
        water = state.graph().allocator.highWaterMark();
        throw std::runtime_error("failed evaluate");
        scope.commit();
    }
    catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(!state.isEvaluating());
    CHECK(state.graph().allocator.isBurned(burned));
    CHECK(state.graph().allocator.highWaterMark() == water);
}

void testEvaluateScopeCommit()
{
    SemanticDocumentState state;
    SemanticId id;
    {
        SemanticDocumentState::EvaluateScope scope(state);
        id = generateFace(state.graph(), 12, scope.eval());
        scope.commit();
    }
    CHECK(!state.isEvaluating());
    CHECK(state.graph().allocator.isPublished(id.handle));
    CHECK(!state.graph().allocator.isBurned(id.handle));
}

void testUndoRestoresPublishedKeepsHighWater()
{
    SemanticDocumentState state;
    state.beginEvaluate();
    const SemanticId first = generateFace(state.graph(), 20, state.currentEval());
    state.commitEvaluate();

    state.markUndoPoint();
    const SemanticHandle waterAtMark = state.graph().allocator.highWaterMark();

    state.beginEvaluate();
    const SemanticId second = generateFace(state.graph(), 20, state.currentEval());
    state.commitEvaluate();
    CHECK(state.graph().allocator.isPublished(second.handle));
    CHECK(state.graph().identity(second.handle) != nullptr);

    const SemanticHandle waterAfter = state.graph().allocator.highWaterMark();
    CHECK(waterAfter > waterAtMark);

    state.undoGraph();
    CHECK(state.graph().identity(first.handle) != nullptr);
    CHECK(state.graph().allocator.isPublished(first.handle));
    // Second is no longer in the published graph; high-water is not rewound.
    CHECK(state.graph().allocator.highWaterMark() == waterAfter);
    CHECK(state.graph().allocator.highWaterMark() > second.handle ||
          state.graph().allocator.highWaterMark() > first.handle);

    state.beginEvaluate();
    const SemanticId third = generateFace(state.graph(), 20, state.currentEval());
    state.commitEvaluate();
    CHECK(third.handle != second.handle);
    CHECK(third.handle >= waterAfter);
}

void testRedoAndAbortGraph()
{
    SemanticDocumentState state;
    state.beginEvaluate();
    const SemanticId a = generateFace(state.graph(), 30, state.currentEval());
    state.commitEvaluate();
    state.markUndoPoint();

    state.beginEvaluate();
    const SemanticId b = generateFace(state.graph(), 30, state.currentEval());
    state.commitEvaluate();
    CHECK(state.graph().identity(b.handle) != nullptr);

    state.undoGraph();
    CHECK(state.graph().identity(a.handle) != nullptr);
    CHECK(state.graph().events().size() == 1);

    state.redoGraph();
    CHECK(state.graph().identity(b.handle) != nullptr);
    CHECK(state.graph().events().size() == 2);

    state.markUndoPoint();
    state.beginEvaluate();
    const SemanticId c = generateFace(state.graph(), 30, state.currentEval());
    state.commitEvaluate();
    CHECK(state.graph().identity(c.handle) != nullptr);
    const SemanticHandle water = state.graph().allocator.highWaterMark();
    state.abortGraph();
    CHECK(state.graph().identity(b.handle) != nullptr);
    CHECK(state.graph().allocator.highWaterMark() == water);
}

void testResetIsNewDocumentNotUndo()
{
    SemanticDocumentState state;
    int doc = 7;
    state.bindOwner(&doc);
    state.beginEvaluate();
    (void)generateFace(state.graph(), 1, state.currentEval());
    state.commitEvaluate();
    CHECK(state.graph().events().size() == 1);
    CHECK(SemanticDocumentState::graphFor(&doc) == &state.graph());

    state.reset();
    CHECK(state.graph().events().empty());
    CHECK(state.nextEvalSerial() == 1);
    CHECK(state.currentEval() == 0);
    // Owner binding survives reset so graphFor still finds this document.
    CHECK(SemanticDocumentState::graphFor(&doc) == &state.graph());
}

void testDestructorUnbinds()
{
    int doc = 9;
    {
        SemanticDocumentState state;
        state.bindOwner(&doc);
        CHECK(SemanticDocumentState::graphFor(&doc) != nullptr);
    }
    CHECK(SemanticDocumentState::graphFor(&doc) == nullptr);
}

void testSerializeDeserializeKeepsHandles()
{
    SemanticDocumentState a;
    a.beginEvaluate();
    const SemanticId id = generateFace(a.graph(), 10, a.currentEval());
    a.graph().recordSplit(id, 2, "Split", 10, a.currentEval(), SemanticRole::None);
    const SemanticHandle water = a.graph().allocator.highWaterMark();
    a.commitEvaluate();

    const std::string s = a.serialize();
    CHECK(s.find("STD1") == 0);
    const std::string hex = SemanticDocumentState::hexEncode(s);
    CHECK(SemanticDocumentState::hexDecode(hex) == s);

    SemanticDocumentState b;
    CHECK(b.deserialize(s));
    CHECK(b.graph().allocator.highWaterMark() >= water);
    CHECK(b.graph().identity(id.handle) != nullptr);
    const auto kids = b.graph().descendants(id.handle);
    CHECK(kids.size() >= 2);

    b.beginEvaluate();
    const SemanticId next = generateFace(b.graph(), 11, b.currentEval());
    b.commitEvaluate();
    CHECK(next.handle >= water);
    CHECK(next.handle != id.handle);
}

void testNestedBeginAbortsInFlight()
{
    SemanticDocumentState state;
    state.beginEvaluate();
    const SemanticId inflight = generateFace(state.graph(), 40, state.currentEval());
    const SemanticHandle water = state.graph().allocator.highWaterMark();
    const EvalSerial e2 = state.beginEvaluate();
    CHECK(e2 == 2);
    CHECK(state.graph().allocator.isBurned(inflight.handle));
    CHECK(state.graph().allocator.highWaterMark() == water);
    const SemanticId ok = generateFace(state.graph(), 40, e2);
    state.commitEvaluate();
    CHECK(state.graph().allocator.isPublished(ok.handle));
    CHECK(ok.handle != inflight.handle);
}

void testI5HighWaterMatrix()
{
    // DESIGN §9 / I5: undo + abort never rewind allocator high-water.
    SemanticDocumentState state;
    state.beginEvaluate();
    const SemanticId first = generateFace(state.graph(), 40, state.currentEval());
    state.commitEvaluate();
    state.markUndoPoint();
    const SemanticHandle waterMark = state.graph().allocator.highWaterMark();

    state.beginEvaluate();
    const SemanticId second = generateFace(state.graph(), 40, state.currentEval());
    state.commitEvaluate();
    const SemanticHandle waterAfter = state.graph().allocator.highWaterMark();
    CHECK(waterAfter > waterMark);
    CHECK(second.handle > first.handle);

    state.undoGraph();
    CHECK(state.graph().allocator.highWaterMark() == waterAfter);
    CHECK(state.graph().identity(first.handle) != nullptr);
    CHECK(state.graph().identity(second.handle) == nullptr);

    state.beginEvaluate();
    const SemanticId inflight = generateFace(state.graph(), 40, state.currentEval());
    const SemanticHandle waterInflight = state.graph().allocator.highWaterMark();
    state.abortEvaluate();
    CHECK(state.graph().allocator.isBurned(inflight.handle));
    CHECK(state.graph().allocator.highWaterMark() == waterInflight);
    CHECK(inflight.handle != first.handle);
    CHECK(inflight.handle != second.handle);
}

void testMissingSectionIsNoOp()
{
    // Pre-migration FCStd: no SemanticGraph attribute / child.
    SemanticDocumentState fresh;
    CHECK(fresh.restoreOptionalHexPayload(false, ""));
    CHECK(fresh.graph().events().empty());
    CHECK(fresh.graph().allIdentities().empty());
    CHECK(fresh.graph().allBindings().empty());
    CHECK(fresh.graph().allocator.highWaterMark() == 1);

    // A FaceN string is not a seed (I13): absent section never mints.
    CHECK(fresh.restoreOptionalHexPayload(false, "Face6"));
    CHECK(fresh.graph().events().empty());
    CHECK(fresh.graph().allIdentities().empty());

    SemanticDocumentState seeded;
    seeded.beginEvaluate();
    const SemanticId kept = generateFace(seeded.graph(), 7, seeded.currentEval());
    seeded.commitEvaluate();
    const SemanticHandle water = seeded.graph().allocator.highWaterMark();
    const std::size_t nEvents = seeded.graph().events().size();

    CHECK(seeded.restoreOptionalHexPayload(false, seeded.hexPayload()));
    CHECK(seeded.graph().identity(kept.handle) != nullptr);
    CHECK(seeded.graph().events().size() == nEvents);
    CHECK(seeded.graph().allocator.highWaterMark() == water);

    seeded.beginEvaluate();
    const SemanticId next = generateFace(seeded.graph(), 7, seeded.currentEval());
    seeded.commitEvaluate();
    CHECK(next.handle >= water);
    CHECK(next.handle != kept.handle);
}

void testResetDropsAliasesKeepsOwner()
{
    SemanticDocumentState state;
    int doc = 7;
    int feat = 8;
    state.bindOwner(&doc);
    state.bindAlias(&feat);
    CHECK(SemanticDocumentState::graphFor(&feat) == &state.graph());
    CHECK(SemanticDocumentState::graphFor(&doc) == &state.graph());

    state.beginEvaluate();
    const SemanticId kept = generateFace(state.graph(), 1, state.currentEval());
    SemanticBinding row;
    row.stid = kept;
    row.feature = 1;
    row.eval = state.currentEval();
    row.kind = SemanticKind::Face;
    row.index = ElementIndex::fromString("Face6");
    state.graph().bind(row);
    state.commitEvaluate();
    CHECK(!state.graph().allBindings().empty());
    CHECK(state.graph().events().size() == 1);

    state.reset();
    CHECK(state.graph().events().empty());
    CHECK(state.graph().allIdentities().empty());
    CHECK(state.graph().allBindings().empty());
    CHECK(state.nextEvalSerial() == 1);
    CHECK(state.currentEval() == 0);
    // G3: aliases dropped, owner kept.
    CHECK(SemanticDocumentState::graphFor(&doc) == &state.graph());
    CHECK(SemanticDocumentState::graphFor(&feat) == nullptr);
}

void testRestoreWithoutStg1DoesNotKeepPreviousGraph()
{
    // C3/I2: Document::restore resets before load. Absent STG1 must not keep
    // leftover events/identities/Bindings from the previous document.
    SemanticDocumentState state;
    int doc = 3;
    int feat = 4;
    state.bindOwner(&doc);
    state.bindAlias(&feat);
    state.beginEvaluate();
    const SemanticId old = generateFace(state.graph(), 10, state.currentEval());
    SemanticBinding faceN;
    faceN.stid = old;
    faceN.feature = 10;
    faceN.eval = state.currentEval();
    faceN.kind = SemanticKind::Face;
    faceN.index = ElementIndex::fromString("Face6");
    state.graph().bind(faceN);
    state.commitEvaluate();
    CHECK(state.graph().identity(old.handle) != nullptr);
    CHECK(!state.graph().allBindings().empty());

    state.reset();
    CHECK(state.restoreOptionalHexPayload(false, ""));
    CHECK(state.graph().identity(old.handle) == nullptr);
    CHECK(state.graph().events().empty());
    CHECK(state.graph().allIdentities().empty());
    CHECK(state.graph().allBindings().empty());
    CHECK(SemanticDocumentState::graphFor(&feat) == nullptr);
    CHECK(SemanticDocumentState::graphFor(&doc) == &state.graph());
    CHECK(!canPromoteIndexedNameToSeed(state.graph(), ElementIndex::fromString("Face6")));
}

void testPresentSectionRestoresEventsAndHandles()
{
    SemanticDocumentState a;
    a.beginEvaluate();
    const SemanticId id = generateFace(a.graph(), 10, a.currentEval());
    a.graph().recordSplit(id, 2, "Split", 10, a.currentEval(), SemanticRole::None);
    SemanticBinding faceN;
    faceN.stid = id;
    faceN.feature = 10;
    faceN.eval = a.currentEval();
    faceN.kind = SemanticKind::Face;
    faceN.index = ElementIndex::fromString("Face6");
    a.graph().bind(faceN);
    CHECK(!a.graph().allBindings().empty());
    const SemanticHandle water = a.graph().allocator.highWaterMark();
    a.commitEvaluate();
    const std::size_t nEvents = a.graph().events().size();
    const std::size_t nIds = a.graph().allIdentities().size();

    // Same path Document::Save writes: hexEncode(serialize()).
    const std::string hex = SemanticDocumentState::hexEncode(a.serialize());
    CHECK(hex == a.hexPayload());
    CHECK(!hex.empty());

    SemanticDocumentState b;
    CHECK(b.restoreOptionalHexPayload(true, hex));
    CHECK(b.graph().identity(id.handle) != nullptr);
    CHECK(b.graph().events().size() == nEvents);
    CHECK(b.graph().allIdentities().size() == nIds);
    const auto kids = b.graph().descendants(id.handle);
    CHECK(kids.size() >= 2);
    // Bindings are transient (not in STG1).
    CHECK(b.graph().allBindings().empty());
    // I5: high-water not rewound.
    CHECK(b.graph().allocator.highWaterMark() >= water);
    // I13: restore did not mint a seed from Face6.
    for (const SemanticId& sid : b.graph().allIdentities()) {
        CHECK(sid.handle != 0);
    }

    b.beginEvaluate();
    const SemanticId next = generateFace(b.graph(), 11, b.currentEval());
    b.commitEvaluate();
    CHECK(next.handle >= water);
    CHECK(next.handle != id.handle);

    // Present but not STD1 / not hex: fail, still no FaceN mint.
    SemanticDocumentState c;
    CHECK(!c.restoreOptionalHexPayload(true, "Face6"));
    CHECK(c.graph().events().empty());
    CHECK(c.graph().allIdentities().empty());
    CHECK(c.restoreOptionalHexPayload(true, ""));
    CHECK(c.graph().events().empty());
}

void testRestoreIsAtomicAndAbortsInFlight()
{
    SemanticDocumentState source;
    source.beginEvaluate();
    const SemanticId sourceId = generateFace(source.graph(), 50, source.currentEval());
    source.commitEvaluate();
    const std::string payload = source.hexPayload();

    SemanticDocumentState target;
    // Publish two local identities so at least one restored-absent handle sits
    // above the source high-water; the in-flight handle is a third.
    target.beginEvaluate();
    const SemanticId localA = generateFace(target.graph(), 60, target.currentEval());
    target.commitEvaluate();
    target.beginEvaluate();
    const SemanticId localB = generateFace(target.graph(), 61, target.currentEval());
    target.commitEvaluate();
    target.beginEvaluate();
    const SemanticId inFlight = generateFace(target.graph(), 51, target.currentEval());
    CHECK(localA.handle == sourceId.handle);  // colliding republish case
    CHECK(localB.handle != sourceId.handle);
    CHECK(inFlight.handle != sourceId.handle);
    CHECK(inFlight.handle != localB.handle);
    const std::string malformed = SemanticDocumentState::hexEncode(
        "STD1\nnextEval 9\nGRAPH\nSTG1\nunknown\n");

    // A failed restore must not discard the current in-flight graph.
    CHECK(!target.restoreOptionalHexPayload(true, malformed));
    CHECK(target.isEvaluating());
    CHECK(target.graph().identity(inFlight.handle) != nullptr);
    CHECK(target.graph().identity(localB.handle) != nullptr);

    // A successful restore is a C1 boundary: replace with the payload, burn
    // abandoned live/in-flight handles that are absent from it, and leave the
    // document non-evaluating. A handle that collides with a restored published
    // identity is correctly republished as that restored identity.
    CHECK(target.restoreOptionalHexPayload(true, payload));
    CHECK(!target.isEvaluating());
    CHECK(target.graph().identity(sourceId.handle) != nullptr);
    CHECK(target.graph().allocator.isPublished(sourceId.handle));
    CHECK(target.graph().identity(inFlight.handle) == nullptr);
    CHECK(target.graph().allocator.isBurned(inFlight.handle));
    CHECK(!target.graph().allocator.isPublished(inFlight.handle));
    CHECK(target.graph().identity(localB.handle) == nullptr);
    CHECK(target.graph().allocator.isBurned(localB.handle));
    CHECK(!target.graph().allocator.isPublished(localB.handle));
    CHECK(target.graph().allocator.highWaterMark()
          >= std::max(source.graph().allocator.highWaterMark(), inFlight.handle + 1));
}

void testRestoreRejectsSerialMismatch()
{
    SemanticDocumentState source;
    source.beginEvaluate();
    (void)generateFace(source.graph(), 10, source.currentEval());
    source.commitEvaluate();
    const std::string payload = source.serialize();

    std::string mismatchedEvent = payload;
    const std::string eventPrefix = "event 1 1 Test 10 1 ";
    const std::size_t eventPos = mismatchedEvent.find(eventPrefix);
    CHECK(eventPos != std::string::npos);
    if (eventPos != std::string::npos) {
        mismatchedEvent.replace(eventPos, eventPrefix.size(), "event 1 1 Test 10 2 ");
        SemanticDocumentState target;
        CHECK(!target.deserialize(mismatchedEvent));
        CHECK(target.graph().events().empty());
    }

    std::string zeroNext = payload;
    const std::size_t nextPos = zeroNext.find("nextEval 2");
    CHECK(nextPos != std::string::npos);
    if (nextPos != std::string::npos) {
        zeroNext.replace(nextPos, 10, "nextEval 0");
        CHECK(!SemanticDocumentState().deserialize(zeroNext));
    }
}

}  // namespace

int main()
{
    testOwnsGraphAndGraphFor();
    testTwoDocumentsTwoGraphs();
    testBeginCommitEvaluate();
    testAbortBurnsHandlesI5();
    testEvaluateScopeAbortOnUnwind();
    testEvaluateScopeCommit();
    testUndoRestoresPublishedKeepsHighWater();
    testRedoAndAbortGraph();
    testResetIsNewDocumentNotUndo();
    testDestructorUnbinds();
    testSerializeDeserializeKeepsHandles();
    testNestedBeginAbortsInFlight();
    testI5HighWaterMatrix();
    testMissingSectionIsNoOp();
    testPresentSectionRestoresEventsAndHandles();
    testResetDropsAliasesKeepsOwner();
    testRestoreWithoutStg1DoesNotKeepPreviousGraph();
    testRestoreIsAtomicAndAbortsInFlight();
    testRestoreRejectsSerialMismatch();

    std::cout << "Semantic document state: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
