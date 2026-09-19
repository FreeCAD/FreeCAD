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
// Standalone Phase 2 tests. Metric is ResolutionState + provenance family,
// not FaceN equality. Compile without Qt / OCCT / FreeCADApp:
//
//   g++ -std=c++17 -O2 -DSEMANTIC_TOPOLOGY_STANDALONE
//       -I src -I src/App
//       src/App/SemanticId.cpp src/App/SemanticTopology.cpp
//       src/App/SemanticReference.cpp
//       src/App/SemanticTopologyStandaloneTest.cpp
//       -o /tmp/semantic-topology-phase2
//   /tmp/semantic-topology-phase2

#include "SemanticId.h"
#include "SemanticReference.h"
#include "SemanticTopology.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

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

SemanticBinding makeBinding(const SemanticId& id, ObjectId feature, EvalSerial eval, const char* face)
{
    SemanticBinding b;
    b.stid = id;
    b.feature = feature;
    b.eval = eval;
    b.kind = id.kind;
    b.index = ElementIndex::fromString(face);
    b.occ = nullptr;
    return b;
}

SemanticReference makeRef(const SemanticId& seed,
                          CardinalityReducer reducer,
                          SemanticKind kind = SemanticKind::Face)
{
    SemanticReference ref;
    ref.seed = seed;
    ref.kind = kind;
    ref.reducer = reducer;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    ref.fallback = ElementIndex::fromString("Face1");
    return ref;
}

void testMappedToken()
{
    SemanticId id;
    id.handle = 0x2a;
    id.kind = SemanticKind::Face;
    const std::string tok = id.toMappedToken();
    // E2 / EM14-T1: encode prefix must match mappedTokenPrefix (not a second literal).
    CHECK(tok.find(SemanticId::mappedTokenPrefix()) == 0);
    const SemanticId back = SemanticId::fromMappedToken(tok);
    CHECK(back.handle == 0x2a);
    CHECK(back.kind == SemanticKind::Face);
    CHECK(!SemanticId::fromMappedToken("Face6").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST2a").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST2a:X").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST2aZZ:F").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST0:F").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST 2a:F").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST+2a:F").valid());
    CHECK(!SemanticId::fromMappedToken(";:ST0x2a:F").valid());
    CHECK(SemanticId::isMappedKindChar('F'));
    CHECK(SemanticId::isMappedKindChar('e'));
    CHECK(!SemanticId::isMappedKindChar('X'));
    CHECK(SemanticId::tryKindFromChar('E') == SemanticKind::Edge);
    CHECK(!SemanticId::tryKindFromChar('X').has_value());
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Face, "Face"));
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Region, "Face"));
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Edge, "Edge"));
    CHECK(!SemanticId::kindMatchesElementType(SemanticKind::Edge, "Face"));
    CHECK(!SemanticId::kindMatchesElementType(SemanticKind::Face, "Edge"));
    // AG13-K1 / EM14-T1: exact Wire/Shell/Solid/Region for ;:ST kind-match gates.
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Wire, "Wire"));
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Shell, "Shell"));
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Solid, "Solid"));
    CHECK(SemanticId::kindMatchesElementType(SemanticKind::Region, "Region"));
    CHECK(!SemanticId::kindMatchesElementType(SemanticKind::Wire, "Face"));
    CHECK(!SemanticId::kindMatchesElementType(SemanticKind::Face, "Wire"));
}

void testModifiedKeepsHandle()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face6"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.clearBindings(1);
    const EventId ev = g.recordModified(a, "Pad", 1, 2, SemanticRole::None);
    g.bind(makeBinding(a, 1, 2, "Face9"));  // index may change
    g.commitEvaluate();

    CHECK(g.eventById(ev)->kind == EventKind::Modified);
    CHECK(g.aliasClass().members(a.handle).size() <= 1);
    CHECK(!g.aliasUsedForModified());
    CHECK(g.eventGraphIsDag());

    const auto r = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.size() == 1);
    CHECK(r.identities.front().handle == a.handle);
    CHECK(r.bindings.front().index.toString() == "Face9");
}

void testSplitRequireOneAmbiguousAcceptAllSet()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.clearBindings(1);
    const auto kids = g.recordSplit(a, 2, "Pocket", 1, 2, SemanticRole::None);
    CHECK(kids.size() == 2);
    CHECK(kids[0].handle != a.handle);
    CHECK(kids[1].handle != a.handle);
    CHECK(kids[0].handle != kids[1].handle);
    g.bind(makeBinding(kids[0], 1, 2, "Face2"));
    g.bind(makeBinding(kids[1], 1, 2, "Face3"));
    g.commitEvaluate();

    const auto amb = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(amb.state == ResolutionState::Ambiguous);
    CHECK(amb.identities.empty());  // I9: do not coerce a singleton

    const auto all = SemanticResolver::resolve(makeRef(a, CardinalityReducer::AcceptAll), g);
    CHECK(all.state == ResolutionState::ResolvedSet);
    CHECK(all.identities.size() == 2);
    CHECK((all.identities[0].handle == kids[0].handle && all.identities[1].handle == kids[1].handle)
          || (all.identities[0].handle == kids[1].handle
              && all.identities[1].handle == kids[0].handle));
}

void testMergeNewChildAliasOneBinding()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.bind(makeBinding(b, 1, 1, "Face2"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.clearBindings(1);
    const SemanticId d = g.recordMerge({a, b}, "Boolean", 1, 2, SemanticRole::None);
    CHECK(d.handle != a.handle);
    CHECK(d.handle != b.handle);
    CHECK(g.aliasClass().same(a.handle, d.handle));
    CHECK(g.aliasClass().same(b.handle, d.handle));
    g.bind(makeBinding(d, 1, 2, "Face4"));  // one SemanticBinding row
    g.commitEvaluate();

    CHECK(g.allBindings().size() == 1);

    const auto ra = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(ra.state == ResolutionState::Resolved);
    CHECK(ra.identities.size() == 1);
    CHECK(ra.identities.front().handle == d.handle);

    const auto rb = SemanticResolver::resolve(makeRef(b, CardinalityReducer::RequireOne), g);
    CHECK(rb.state == ResolutionState::Resolved);
    CHECK(rb.identities.front().handle == d.handle);
}

void testDeleteMissingNoNeighbour()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId neighbour =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.bind(makeBinding(neighbour, 1, 1, "Face2"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.clearBindings(1);
    g.recordDeleted(a, "DeleteEdge", 1, 2, SemanticRole::None);
    g.bind(makeBinding(neighbour, 1, 2, "Face2"));  // neighbour still live
    g.commitEvaluate();

    const auto r = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(r.state == ResolutionState::Missing);
    CHECK(r.identities.empty());
    for (const SemanticId& id : r.identities) {
        CHECK(id.handle != neighbour.handle);
    }

    // AcceptAll must not pick the neighbour either (I2).
    const auto all = SemanticResolver::resolve(makeRef(a, CardinalityReducer::AcceptAll), g);
    CHECK(all.state == ResolutionState::Missing);
}

void testFailedEvaluateBurnsHandles()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId published =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(published, 1, 1, "Face1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    const SemanticId burned1 =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 2, SemanticRole::None);
    const SemanticId burned2 =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 2, SemanticRole::None);
    CHECK(burned1.handle != published.handle);
    CHECK(burned2.handle != burned1.handle);
    g.abortEvaluate();

    CHECK(g.allocator.isBurned(burned1.handle));
    CHECK(g.allocator.isBurned(burned2.handle));
    CHECK(!g.allocator.isPublished(burned1.handle));
    CHECK(g.identity(burned1.handle) == nullptr);

    g.beginEvaluate(3);
    const SemanticId ok = g.recordGenerated(SemanticKind::Face, "Pad", 1, 3, SemanticRole::None);
    g.bind(makeBinding(ok, 1, 3, "Face1"));
    g.commitEvaluate();

    CHECK(ok.handle != burned1.handle);
    CHECK(ok.handle != burned2.handle);
    CHECK(ok.handle > burned2.handle);
    CHECK(g.allocator.isPublished(ok.handle));
    CHECK(g.allocator.isBurned(burned1.handle));
}

void testNestedSplitMergeSplit()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();

    // A → {B, C}
    g.beginEvaluate(2);
    g.clearBindings(1);
    const auto bc = g.recordSplit(a, 2, "Split1", 1, 2, SemanticRole::None);
    const SemanticId b = bc[0];
    const SemanticId c = bc[1];
    g.bind(makeBinding(b, 1, 2, "Face2"));
    g.bind(makeBinding(c, 1, 2, "Face3"));
    g.commitEvaluate();

    auto r = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(r.state == ResolutionState::Ambiguous);

    // {B, C} → D
    g.beginEvaluate(3);
    g.clearBindings(1);
    const SemanticId d = g.recordMerge({b, c}, "Merge1", 1, 3, SemanticRole::None);
    g.bind(makeBinding(d, 1, 3, "Face4"));
    g.commitEvaluate();

    r = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.front().handle == d.handle);

    // D → {E, F}
    g.beginEvaluate(4);
    g.clearBindings(1);
    const auto ef = g.recordSplit(d, 2, "Split2", 1, 4, SemanticRole::None);
    g.bind(makeBinding(ef[0], 1, 4, "Face5"));
    g.bind(makeBinding(ef[1], 1, 4, "Face6"));
    g.commitEvaluate();

    r = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(r.state == ResolutionState::Ambiguous);
}

void testI10GeometryDoesNotIntroduce()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "PadA", 10, 1, SemanticRole::None);
    const SemanticId x = g.recordGenerated(SemanticKind::Face, "PadX", 20, 1, SemanticRole::None);
    g.bind(makeBinding(a, 10, 1, "Face1"));
    g.bind(makeBinding(x, 20, 1, "Face2"));
    g.commitEvaluate();

    // Identical geometry; X is "closer" and "larger". Must not enter A's family.
    std::vector<GeometryScore> geom = {
        GeometryScore {a.handle, 1.0, 5.0},
        GeometryScore {x.handle, 100.0, 0.0},
    };

    SemanticReference ref = makeRef(a, CardinalityReducer::UniqueClosest);
    auto r = SemanticResolver::resolve(ref, g, nullptr, &geom);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.size() == 1);
    CHECK(r.identities.front().handle == a.handle);

    ref.reducer = CardinalityReducer::UniqueLargest;
    r = SemanticResolver::resolve(ref, g, nullptr, &geom);
    CHECK(r.state == ResolutionState::Resolved);
    CHECK(r.identities.front().handle == a.handle);

    // Coin-flip is forbidden: X is never a candidate for seed A (I2, I10).
    for (const SemanticId& id : r.identities) {
        CHECK(id.handle != x.handle);
    }
}

void testNoHandleReuseAfterUndoDiscard()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();
    const auto snap = g.snapshotPublished();
    const SemanticHandle waterAfterA = g.allocator.highWaterMark();

    g.beginEvaluate(2);
    const SemanticId discarded =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 2, SemanticRole::None);
    g.abortEvaluate();
    CHECK(g.allocator.isBurned(discarded.handle));

    g.restorePublished(snap);
    g.beginEvaluate(3);
    const SemanticId afterUndo =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 3, SemanticRole::None);
    g.commitEvaluate();

    CHECK(afterUndo.handle != discarded.handle);
    CHECK(afterUndo.handle >= waterAfterA);
    CHECK(g.allocator.highWaterMark() > discarded.handle);
    CHECK(g.identity(discarded.handle) == nullptr);
}

void testAliasNotUsedForModified()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.recordModified(a, "Pad", 1, 1, SemanticRole::None);
    g.commitEvaluate();

    const auto mem = g.aliasClass().members(a.handle);
    // find() may materialize a singleton class; that is not an Alias edge.
    CHECK(mem.size() <= 1);
    CHECK(!g.aliasUsedForModified());
}

void testAncestryIgnoresAliasResolveClosesAfter()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId x = g.recordGenerated(SemanticKind::Face, "Other", 2, 1, SemanticRole::None);
    const auto kids = g.recordSplit(a, 2, "Split", 1, 1, SemanticRole::None);
    const SemanticId b = kids[0];
    const SemanticId c = kids[1];
    // Explicit equivalence of a split child with an unrelated identity.
    CHECK(g.alias(b, x));
    g.bind(makeBinding(b, 1, 1, "FaceB"));
    g.bind(makeBinding(c, 1, 1, "FaceC"));
    g.bind(makeBinding(x, 2, 1, "FaceX"));
    g.commitEvaluate();

    const auto desc = g.descendants(a.handle);
    CHECK(desc.count(a.handle) == 1);
    CHECK(desc.count(b.handle) == 1);
    CHECK(desc.count(c.handle) == 1);
    CHECK(desc.count(x.handle) == 0);  // ancestry ignores Alias

    const auto anc = g.ancestors(b.handle);
    CHECK(anc.count(a.handle) == 1);
    CHECK(anc.count(x.handle) == 0);

    // Resolve closes Alias only after the descendant walk, so x joins via b.
    const auto all = SemanticResolver::resolve(makeRef(a, CardinalityReducer::AcceptAll), g);
    CHECK(all.state == ResolutionState::ResolvedSet);
    bool sawX = false;
    bool sawB = false;
    bool sawC = false;
    for (const SemanticId& id : all.identities) {
        if (id.handle == x.handle) {
            sawX = true;
        }
        if (id.handle == b.handle) {
            sawB = true;
        }
        if (id.handle == c.handle) {
            sawC = true;
        }
    }
    CHECK(sawB);
    CHECK(sawC);
    CHECK(sawX);  // closed after descendants — x is in b's class

    // Closing Alias first would also leak x's non-descendant neighbours.
    // Generate y as sibling of x (same feature, no event from A). It must
    // not appear: we never walk from x, we only close the descendant set.
    // (x itself is in b's class; y is not.)
}

void testI11DagAndAliasQuotient()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const auto mid = g.recordSplit(a, 1, "Split1", 1, 1, SemanticRole::None);
    const SemanticId b = mid[0];
    const auto leaf = g.recordSplit(b, 1, "Split2", 1, 1, SemanticRole::None);
    const SemanticId c = leaf[0];
    CHECK(g.eventGraphIsDag());
    CHECK(g.isAncestor(a.handle, c.handle));

    // A → B → C plus Alias(A, C) is a 2-cycle in the quotient. Forbidden.
    CHECK(!g.alias(a, c));
    CHECK(!g.aliasClass().same(a.handle, c.handle));

    // Merge alias of parent+child is a self-loop in the quotient — allowed.
    const SemanticId d = g.recordMerge({b, c}, "Merge", 1, 1, SemanticRole::None);
    CHECK(g.aliasClass().same(b.handle, d.handle));
    CHECK(g.eventGraphIsDag());
    g.commitEvaluate();
}

void testI12ConsumerArity()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const auto kids = g.recordSplit(a, 2, "Split", 1, 1, SemanticRole::None);
    g.bind(makeBinding(kids[0], 1, 1, "Face1"));
    g.bind(makeBinding(kids[1], 1, 1, "Face2"));
    g.commitEvaluate();

    SemanticReference ref = makeRef(a, CardinalityReducer::AcceptAll);
    ReferenceRequirement req;
    req.expectedKind = SemanticKind::Face;
    req.acceptedCardinality = AcceptedCardinality::One;
    req.acceptedReducers = {CardinalityReducer::RequireOne};

    const auto r = SemanticResolver::resolve(ref, g, &req);
    CHECK(r.state == ResolutionState::Incompatible);
    CHECK(r.identities.empty());
}

void testI12RejectsUnacceptedReducerSet()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId seed = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const auto split = g.recordSplit(seed, 2, "Split", 1, 1, SemanticRole::None);
    g.bind(makeBinding(split[0], 1, 1, "Face1"));
    g.bind(makeBinding(split[1], 1, 1, "Face2"));
    g.commitEvaluate();

    SemanticReference ref = makeRef(seed, CardinalityReducer::AcceptAll);
    ReferenceRequirement req;
    req.expectedKind = SemanticKind::Face;
    req.acceptedCardinality = AcceptedCardinality::OneOrMore;
    req.acceptedReducers = {CardinalityReducer::RequireOne};

    const auto result = SemanticResolver::resolve(ref, g, &req);
    CHECK(result.state == ResolutionState::Incompatible);
    CHECK(result.identities.empty());
    CHECK(result.bindings.empty());
}

void testI13NoUnverifiedPromotion()
{
    SemanticGraph g;
    const ElementIndex raw = ElementIndex::fromString("Face6");
    CHECK(!canPromoteIndexedNameToSeed(g, raw));

    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face6"));
    g.commitEvaluate();
    CHECK(canPromoteIndexedNameToSeed(g, raw));
    CHECK(canPromoteIndexedNameToSeed(g, raw, 1));
    CHECK(!canPromoteIndexedNameToSeed(g, raw, 2));  // 0 matches for feature 2

    SemanticReference ref = makeRef(a, CardinalityReducer::RequireOne);
    ref.fallback = raw;
    CHECK(dualWriteSubName(ref) == "Face6");

    // Second feature also Binding Face6: document-wide refuses; scoped still unique.
    g.beginEvaluate(2);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Pad", 2, 2, SemanticRole::None);
    g.bind(makeBinding(b, 2, 2, "Face6"));
    g.commitEvaluate();
    CHECK(!canPromoteIndexedNameToSeed(g, raw));
    CHECK(!canPromoteIndexedNameToSeed(g, raw, 0));
    CHECK(canPromoteIndexedNameToSeed(g, raw, 1));
    CHECK(canPromoteIndexedNameToSeed(g, raw, 2));
}

void testOutputOwnershipConflict()
{
    SemanticGraph g;
    const ElementIndex output = ElementIndex::fromString("Face9");
    CHECK(!hasOutputOwnershipConflict(nullptr, 17, output));
    CHECK(!hasOutputOwnershipConflict(&g, 17, output));
    CHECK(!uniqueIdentityAtIndex(nullptr, 17, output).valid());
    CHECK(!uniqueIdentityAtIndex(&g, 17, output).valid());

    g.beginEvaluate(1);
    const SemanticId first = g.recordGenerated(SemanticKind::Face, "Extrude", 17, 1, SemanticRole::None);
    g.bind(makeBinding(first, 17, 1, "Face9"));
    g.commitEvaluate();
    CHECK(!hasOutputOwnershipConflict(&g, 17, output));
    CHECK(uniqueIdentityAtIndex(&g, 17, output).valid());
    CHECK(uniqueIdentityAtIndex(&g, 17, output).handle == first.handle);

    g.beginEvaluate(2);
    const SemanticId second = g.recordGenerated(SemanticKind::Face, "Extrude", 17, 2, SemanticRole::None);
    const SemanticId third = g.recordGenerated(SemanticKind::Face, "Revolve", 17, 2, SemanticRole::None);
    g.bind(makeBinding(second, 17, 2, "Face9"));
    g.bind(makeBinding(third, 17, 2, "Face9"));
    CHECK(hasOutputOwnershipConflict(&g, 17, output));
    CHECK(!uniqueIdentityAtIndex(&g, 17, output).valid());
    g.commitEvaluate();

    // Unpublished in-flight owner (not yet commitEvaluate) is a conflict: do not
    // mint a second Generated identity over a slot that already has a row.
    SemanticGraph inflight;
    inflight.beginEvaluate(1);
    const SemanticId pending =
        inflight.recordGenerated(SemanticKind::Face, "Primitive", 21, 1, SemanticRole::None);
    inflight.bind(makeBinding(pending, 21, 1, "Face9"));
    CHECK(hasOutputOwnershipConflict(&inflight, 21, output));
    CHECK(!uniqueIdentityAtIndex(&inflight, 21, output).valid());
    inflight.commitEvaluate();
    CHECK(!hasOutputOwnershipConflict(&inflight, 21, output));
    CHECK(uniqueIdentityAtIndex(&inflight, 21, output).valid());
    CHECK(uniqueIdentityAtIndex(&inflight, 21, output).handle == pending.handle);
    // Note: SemanticGraph::bind rejects identity-mismatched / empty-stid rows,
    // so an invalid-stid Binding cannot enter the graph through the normal path.
}

void testAlreadyUniquelyBound()
{
    SemanticGraph g;
    const SemanticId invalid;
    CHECK(!alreadyUniquelyBound(nullptr, invalid, 1, "Face"));
    CHECK(!alreadyUniquelyBound(&g, invalid, 1, "Face"));
    CHECK(!alreadyUniquelyBound(&g, invalid, 0, "Face"));

    g.beginEvaluate(1);
    const SemanticId seed = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.commitEvaluate();

    CHECK(!alreadyUniquelyBound(&g, seed, 0, "Face"));
    CHECK(!alreadyUniquelyBound(&g, seed, 1, ""));
    CHECK(!alreadyUniquelyBound(&g, seed, 1, nullptr));
    CHECK(!alreadyUniquelyBound(&g, seed, 1, "Face"));  // no Binding yet

    g.beginEvaluate(2);
    g.bind(makeBinding(seed, 7, 2, "Face4"));
    g.commitEvaluate();
    CHECK(alreadyUniquelyBound(&g, seed, 7, "Face"));
    CHECK(!alreadyUniquelyBound(&g, seed, 7, "Edge"));
    CHECK(!alreadyUniquelyBound(&g, seed, 8, "Face"));

    // Ambiguous (>1) Face Bindings on the same feature → not uniquely bound.
    g.beginEvaluate(3);
    g.bind(makeBinding(seed, 7, 3, "Face5"));
    g.commitEvaluate();
    CHECK(!alreadyUniquelyBound(&g, seed, 7, "Face"));

    // Descendant unique Binding: Split children carry Face rows; parent seed
    // itself has none on feature 11, but alreadyUniquelyBound walks descendants.
    SemanticGraph split;
    split.beginEvaluate(1);
    const SemanticId parent =
        split.recordGenerated(SemanticKind::Face, "Pad", 11, 1, SemanticRole::None);
    split.bind(makeBinding(parent, 11, 1, "Face1"));
    split.commitEvaluate();
    split.beginEvaluate(2);
    split.clearBindings(11);
    const auto kids = split.recordSplit(parent, 2, "Pocket", 11, 2, SemanticRole::None);
    CHECK(kids.size() == 2);
    split.bind(makeBinding(kids[0], 11, 2, "Face2"));
    split.bind(makeBinding(kids[1], 11, 2, "Face3"));
    split.commitEvaluate();
    CHECK(!uniqueBindingOnFeature(&split, parent, 11, "Face").has_value());
    CHECK(uniqueBindingOnFeature(&split, kids[0], 11, "Face").has_value());
    CHECK(alreadyUniquelyBound(&split, parent, 11, "Face"));
}

void testShouldRefuseGeneratedMint()
{
    SemanticGraph g;
    const ElementIndex face4 = ElementIndex::fromString("Face4");
    const SemanticId invalid;
    CHECK(!shouldRefuseGeneratedMint(nullptr, invalid, 1, face4));
    CHECK(!shouldRefuseGeneratedMint(&g, invalid, 1, face4));  // empty slot, unbound

    g.beginEvaluate(1);
    const SemanticId seed = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId other = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.commitEvaluate();
    CHECK(!shouldRefuseGeneratedMint(&g, seed, 7, face4));  // may mint into empty slot

    g.beginEvaluate(2);
    g.bind(makeBinding(seed, 7, 2, "Face4"));
    g.commitEvaluate();
    // Seed already uniquely bound on feature → refuse remint (C1).
    CHECK(shouldRefuseGeneratedMint(&g, seed, 7, face4));
    // Different seed into uniquely published slot → refuse (I13 unique owner).
    CHECK(shouldRefuseGeneratedMint(&g, other, 7, face4));
    CHECK(uniqueIdentityAtIndex(&g, 7, face4).valid());

    // Ambiguous latest ownership → refuse via conflict gate.
    g.beginEvaluate(3);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Boolean", 7, 3, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Boolean", 7, 3, SemanticRole::None);
    g.bind(makeBinding(a, 7, 3, "Face4"));
    g.bind(makeBinding(b, 7, 3, "Face4"));
    CHECK(hasOutputOwnershipConflict(&g, 7, face4));
    CHECK(shouldRefuseGeneratedMint(&g, other, 7, face4));
    g.commitEvaluate();
}

void testShouldRefuseDressUpMintKind()
{
    SemanticGraph g;
    const ElementIndex unnamed;
    const ElementIndex face4 = ElementIndex::fromString("Face4");

    // Non-minting kinds never refuse via this helper (even with unnamed index).
    CHECK(!shouldRefuseDressUpMintKind(EventKind::Modified, &g, 7, unnamed));
    CHECK(!shouldRefuseDressUpMintKind(EventKind::Merge, &g, 7, unnamed));
    CHECK(!shouldRefuseDressUpMintKind(EventKind::Deleted, &g, 7, unnamed));

    // Generated / Intersection / Split with empty or unnamed toIndex → refuse (I13).
    CHECK(shouldRefuseDressUpMintKind(EventKind::Generated, &g, 7, unnamed));
    CHECK(shouldRefuseDressUpMintKind(EventKind::Intersection, &g, 7, unnamed));
    CHECK(shouldRefuseDressUpMintKind(EventKind::Split, &g, 7, unnamed));
    ElementIndex zeroFace;
    zeroFace.type = "Face";
    zeroFace.index = 0;
    CHECK(shouldRefuseDressUpMintKind(EventKind::Generated, &g, 7, zeroFace));

    // Generated + named empty slot → may mint.
    CHECK(!shouldRefuseDressUpMintKind(EventKind::Generated, &g, 7, face4));
    CHECK(!shouldRefuseDressUpMintKind(EventKind::Intersection, &g, 7, face4));
    CHECK(!shouldRefuseDressUpMintKind(EventKind::Split, &g, 7, face4));

    // Generated + named conflicted slot (two bindings) → refuse.
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Fillet", 7, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Fillet", 7, 1, SemanticRole::None);
    g.bind(makeBinding(a, 7, 1, "Face4"));
    g.bind(makeBinding(b, 7, 1, "Face4"));
    CHECK(hasOutputOwnershipConflict(&g, 7, face4));
    CHECK(shouldRefuseDressUpMintKind(EventKind::Generated, &g, 7, face4));
    CHECK(shouldRefuseDressUpMintKind(EventKind::Intersection, &g, 7, face4));
    CHECK(shouldRefuseDressUpMintKind(EventKind::Split, &g, 7, face4));
    g.commitEvaluate();
}

void testShouldRefuseBoundAt()
{
    SemanticGraph g;
    const ElementIndex face4 = ElementIndex::fromString("Face4");

    // Null / empty graph / empty slot → may mint (locate/bind and
    // FeaturePrimitive locate/considerTool mint paths).
    CHECK(!shouldRefuseBoundAt(nullptr, 7, face4));
    CHECK(!shouldRefuseBoundAt(&g, 7, face4));
    CHECK(!uniqueIdentityAtIndex(&g, 7, face4).valid());

    // Uniquely published owner → refuse remint; locate/tool callers reuse
    // the published identity instead of minting (I13).
    g.beginEvaluate(1);
    const SemanticId owner = g.recordGenerated(SemanticKind::Face, "Extrude", 7, 1, SemanticRole::None);
    g.bind(makeBinding(owner, 7, 1, "Face4"));
    g.commitEvaluate();
    CHECK(uniqueIdentityAtIndex(&g, 7, face4).valid());
    CHECK(uniqueIdentityAtIndex(&g, 7, face4).handle == owner.handle);
    CHECK(shouldRefuseBoundAt(&g, 7, face4));

    // Ambiguous latest ownership → refuse via conflict gate; locate/tool
    // callers see invalid uniqueIdentity and leave the slot unnamed.
    g.beginEvaluate(2);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Extrude", 7, 2, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Extrude", 7, 2, SemanticRole::None);
    g.bind(makeBinding(a, 7, 2, "Face4"));
    g.bind(makeBinding(b, 7, 2, "Face4"));
    CHECK(hasOutputOwnershipConflict(&g, 7, face4));
    CHECK(shouldRefuseBoundAt(&g, 7, face4));
    CHECK(!uniqueIdentityAtIndex(&g, 7, face4).valid());
    g.commitEvaluate();
}

void testShouldRefuseNamedEmitSlot()
{
    SemanticGraph g;
    const ElementIndex face4 = ElementIndex::fromString("Face4");
    const ElementIndex unnamed;
    ElementIndex zeroFace;
    zeroFace.type = "Face";
    zeroFace.index = 0;
    ElementIndex emptyType;
    emptyType.type = "";
    emptyType.index = 4;
    ElementIndex edge4 = ElementIndex::fromString("Edge4");

    // Invalid / empty / wrong-type index → refuse (typed Face/Edge gate).
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, unnamed, "Face"));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, zeroFace, "Face"));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, emptyType, "Face"));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, edge4, "Face"));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, face4, "Edge"));

    // Empty slot + protect (default) → may emit.
    CHECK(!shouldRefuseNamedEmitSlot(&g, 7, face4, "Face"));
    CHECK(!shouldRefuseNamedEmitSlot(&g, 7, face4, "Face", true));
    CHECK(!shouldRefuseNamedEmitSlot(nullptr, 7, face4, "Face"));

    // Conflicted slot + protect → refuse; protect=false → only invalid index refuses.
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 7, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Pad", 7, 1, SemanticRole::None);
    g.bind(makeBinding(a, 7, 1, "Face4"));
    g.bind(makeBinding(b, 7, 1, "Face4"));
    CHECK(hasOutputOwnershipConflict(&g, 7, face4));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, face4, "Face"));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, face4, "Face", true));
    CHECK(!shouldRefuseNamedEmitSlot(&g, 7, face4, "Face", false));
    CHECK(shouldRefuseNamedEmitSlot(&g, 7, unnamed, "Face", false));
    g.commitEvaluate();

    // Uniquely published alone is NOT a refuse reason for this helper
    // (unlike shouldRefuseBoundAt).
    SemanticGraph uniq;
    uniq.beginEvaluate(1);
    const SemanticId owner =
        uniq.recordGenerated(SemanticKind::Face, "Pad", 7, 1, SemanticRole::None);
    uniq.bind(makeBinding(owner, 7, 1, "Face4"));
    uniq.commitEvaluate();
    CHECK(uniqueIdentityAtIndex(&uniq, 7, face4).valid());
    CHECK(shouldRefuseBoundAt(&uniq, 7, face4));
    CHECK(!shouldRefuseNamedEmitSlot(&uniq, 7, face4, "Face"));
}

void testAbortRestoresBindings()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();
    CHECK(g.allBindings().size() == 1);
    const std::size_t nEvents = g.events().size();

    g.beginEvaluate(2);
    g.clearBindings(1);
    g.bind(makeBinding(a, 1, 2, "Face9"));
    CHECK(g.allBindings().size() == 1);
    CHECK(g.allBindings().front().index == ElementIndex::fromString("Face9"));
    g.abortEvaluate();

    CHECK(g.events().size() == nEvents);
    CHECK(g.allBindings().size() == 1);
    CHECK(g.allBindings().front().index == ElementIndex::fromString("Face1"));
    CHECK(g.allocator.isPublished(a.handle));
    CHECK(g.identity(a.handle) != nullptr);
}

void testAbortRestoresUnbind()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();

    g.beginEvaluate(2);
    g.unbind(a.handle);
    CHECK(g.allBindings().empty());
    g.abortEvaluate();
    CHECK(g.allBindings().size() == 1);
    CHECK(g.allBindings().front().stid.handle == a.handle);
}

void testAbortRollsBackMergeAlias()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.commitEvaluate();
    CHECK(!g.aliasClass().same(a.handle, b.handle));
    const std::size_t nEvents = g.events().size();
    const std::size_t nIds = g.allIdentities().size();

    g.beginEvaluate(2);
    const SemanticId merged = g.recordMerge({a, b}, "Merge", 1, 2, SemanticRole::None);
    CHECK(g.aliasClass().same(a.handle, merged.handle));
    g.bind(makeBinding(merged, 1, 2, "Face3"));
    g.abortEvaluate();

    CHECK(g.allocator.isBurned(merged.handle));
    CHECK(!g.allocator.isPublished(merged.handle));
    CHECK(g.identity(merged.handle) == nullptr);
    CHECK(!g.aliasClass().same(a.handle, b.handle));
    CHECK(g.events().size() == nEvents);
    CHECK(g.allIdentities().size() == nIds);
    for (const SemanticBinding& row : g.allBindings()) {
        CHECK(row.stid.handle != merged.handle);
    }
    CHECK(g.allocator.isPublished(a.handle));
    CHECK(g.allocator.isPublished(b.handle));
}

void testAbortDoesNotKeepBurnedIdentityViaBinding()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    g.beginEvaluate(2);
    const SemanticId burned = g.recordGenerated(SemanticKind::Face, "Pad", 1, 2, SemanticRole::None);
    g.bind(makeBinding(burned, 1, 2, "Face2"));
    CHECK(g.identity(burned.handle) != nullptr);
    g.abortEvaluate();

    CHECK(g.allocator.isBurned(burned.handle));
    CHECK(g.identity(burned.handle) == nullptr);
    CHECK(g.allocator.highWaterMark() > water);
    for (const SemanticBinding& row : g.allBindings()) {
        CHECK(row.stid.handle != burned.handle);
    }
    CHECK(g.identity(a.handle) != nullptr);
}

void testIntersectionNM()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "BoolA", 1, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "BoolB", 2, 1, SemanticRole::None);
    const auto outs = g.recordIntersection({a, b}, 2, "Common", 3, 1, SemanticRole::None);
    CHECK(outs.size() == 2);
    g.bind(makeBinding(outs[0], 3, 1, "Face1"));
    g.bind(makeBinding(outs[1], 3, 1, "Face2"));
    g.commitEvaluate();

    const auto r = SemanticResolver::resolve(makeRef(a, CardinalityReducer::AcceptAll), g);
    CHECK(r.state == ResolutionState::ResolvedSet);
    CHECK(r.identities.size() == 2);
}

void testBindingSnapshotIntegrity()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId id = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(id, 1, 1, "Face1"));
    g.commitEvaluate();

    // A snapshot must not be able to resurrect a Binding for an identity that
    // is absent from the restored durable table.
    SemanticGraph::Snapshot snap = g.snapshotPublished();
    SemanticBinding stale = makeBinding(id, 1, 1, "Face9");
    stale.stid.handle = id.handle + 1000;
    snap.bindings.push_back(stale);
    g.restorePublished(snap);
    CHECK(g.allBindings().size() == 1);
    CHECK(g.allBindings().front().index == ElementIndex::fromString("Face1"));

    // Metadata drift is equally stale, even when the handle still exists.
    SemanticBinding mismatched = makeBinding(id, 1, 2, "Face2");
    mismatched.stid.kind = SemanticKind::Edge;
    g.bind(mismatched);
    CHECK(g.allBindings().size() == 1);
}



void testDeserializeRejectsInvalidKindLetter()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId id = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.commitEvaluate();
    const std::string serialized = g.serialize();

    const std::string needle = "identity " + std::to_string(id.handle) + " F ";
    const std::string smashed = "identity " + std::to_string(id.handle) + " X ";
    const auto ip = serialized.find(needle);
    CHECK(ip != std::string::npos);

    std::string bad = serialized;
    bad.replace(ip, needle.size(), smashed);
    SemanticGraph restored;
    CHECK(!restored.deserialize(bad));
    CHECK(restored.events().empty());
}

void testI5I9MissingAmbiguousIncompatiblePolicy()
{
    // DESIGN §9 / I5 / I9 / I13: Missing, Ambiguous, Incompatible are values;
    // allocator high-water never rewinds on abort (silence beats a neighbour).
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.bind(makeBinding(b, 1, 1, "Face2"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    // Missing: deleted seed with neighbour still live → no neighbour pick (I9/I10).
    g.beginEvaluate(2);
    g.clearBindings(1);
    g.recordDeleted(a, "DeleteFace", 1, 2, SemanticRole::None);
    g.bind(makeBinding(b, 1, 2, "Face2"));
    g.commitEvaluate();
    const auto missing = SemanticResolver::resolve(makeRef(a, CardinalityReducer::RequireOne), g);
    CHECK(missing.state == ResolutionState::Missing);
    CHECK(missing.identities.empty());
    for (const SemanticId& id : missing.identities) {
        CHECK(id.handle != b.handle);
    }

    // Ambiguous: RequireOne on a split parent with two live kids.
    g.beginEvaluate(3);
    g.clearBindings(1);
    const auto kids = g.recordSplit(b, 2, "Pocket", 1, 3, SemanticRole::None);
    CHECK(kids.size() == 2);
    g.bind(makeBinding(kids[0], 1, 3, "Face3"));
    g.bind(makeBinding(kids[1], 1, 3, "Face4"));
    g.commitEvaluate();
    const auto amb = SemanticResolver::resolve(makeRef(b, CardinalityReducer::RequireOne), g);
    CHECK(amb.state == ResolutionState::Ambiguous);
    CHECK(amb.identities.empty());

    // Incompatible: AcceptAll reducer rejected by requirement that only allows RequireOne (I12/I9).
    SemanticReference setRef = makeRef(b, CardinalityReducer::AcceptAll);
    ReferenceRequirement req;
    req.expectedKind = SemanticKind::Face;
    req.acceptedCardinality = AcceptedCardinality::One;
    req.acceptedReducers = {CardinalityReducer::RequireOne};
    const auto incompat = SemanticResolver::resolve(setRef, g, &req);
    CHECK(incompat.state == ResolutionState::Incompatible);
    CHECK(incompat.identities.empty());

    // I5: abort burns in-flight; high-water not rewound.
    g.beginEvaluate(4);
    const SemanticId burned =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 4, SemanticRole::None);
    g.abortEvaluate();
    CHECK(g.allocator.isBurned(burned.handle));
    CHECK(g.allocator.highWaterMark() > burned.handle);
    CHECK(g.allocator.highWaterMark() >= water);
    g.beginEvaluate(5);
    const SemanticId next =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 5, SemanticRole::None);
    g.commitEvaluate();
    CHECK(next.handle != burned.handle);
    CHECK(g.allocator.highWaterMark() > next.handle || g.allocator.isPublished(next.handle));
}

void testEventConnectorIntegrity()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId id = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.commitEvaluate();
    CHECK(g.eventGraphIsWellFormed());

    const std::string serialized = g.serialize();
    const std::size_t output = serialized.find("output ");
    CHECK(output != std::string::npos);
    const std::size_t eventId = serialized.find(" ", output + 7);
    CHECK(eventId != std::string::npos);

    std::string danglingEvent = serialized;
    danglingEvent.replace(output + 7, eventId - (output + 7), "999");
    SemanticGraph restored;
    CHECK(!restored.deserialize(danglingEvent));
    CHECK(restored.events().empty());

    std::string danglingIdentity = serialized;
    const std::size_t handle = serialized.find(" ", eventId + 1);
    CHECK(handle != std::string::npos);
    danglingIdentity.replace(eventId + 1, handle - (eventId + 1), "999");
    CHECK(!restored.deserialize(danglingIdentity));
    CHECK(restored.events().empty());

    // A Generated event must use either its generator marker or real inputs,
    // never both.  Mixed EventInput provenance is malformed restore data.
    std::string mixedGeneratorInputs = serialized;
    mixedGeneratorInputs += "input 1 " + std::to_string(id.handle) + " "
        + SemanticId::kindChar(id.kind) + " " + std::to_string(id.allocatedBy) + " "
        + std::to_string(id.allocatedAtEval) + " "
        + std::to_string(static_cast<unsigned>(id.allocatedRole)) + "\n";
    CHECK(!restored.deserialize(mixedGeneratorInputs));
    CHECK(restored.events().empty());
    CHECK(id.handle != 0);
}
}  // namespace

int main()

{
    testMappedToken();
    testModifiedKeepsHandle();
    testSplitRequireOneAmbiguousAcceptAllSet();
    testMergeNewChildAliasOneBinding();
    testDeleteMissingNoNeighbour();
    testFailedEvaluateBurnsHandles();
    testNestedSplitMergeSplit();
    testI10GeometryDoesNotIntroduce();
    testNoHandleReuseAfterUndoDiscard();
    testAliasNotUsedForModified();
    testAncestryIgnoresAliasResolveClosesAfter();
    testI11DagAndAliasQuotient();
    testI12ConsumerArity();
    testI12RejectsUnacceptedReducerSet();
    testI13NoUnverifiedPromotion();
    testOutputOwnershipConflict();
    testAlreadyUniquelyBound();
    testShouldRefuseGeneratedMint();
    testShouldRefuseDressUpMintKind();
    testShouldRefuseBoundAt();
    testShouldRefuseNamedEmitSlot();
    testIntersectionNM();
    testAbortRestoresBindings();
    testAbortRestoresUnbind();
    testAbortRollsBackMergeAlias();
    testAbortDoesNotKeepBurnedIdentityViaBinding();
    testBindingSnapshotIntegrity();
    testI5I9MissingAmbiguousIncompatiblePolicy();
    testEventConnectorIntegrity();
    testDeserializeRejectsInvalidKindLetter();

    std::cout << "Semantic topology Phase 2: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
