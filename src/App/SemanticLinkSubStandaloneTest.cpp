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
// Standalone Phase 3 dual-write tests. No Qt / OCCT / FreeCADApp.
// Metric: seed presence, I13 refusal, FaceN cache via dualWriteSubName.
//
//   g++ -std=c++17 -O2 -DSEMANTIC_TOPOLOGY_STANDALONE
//       -I src -I src/App
//       src/App/SemanticId.cpp src/App/SemanticTopology.cpp
//       src/App/SemanticReference.cpp src/App/SemanticLinkSub.cpp
//       src/App/SemanticLinkSubStandaloneTest.cpp
//       -o /tmp/semantic-topology-phase3
//   /tmp/semantic-topology-phase3

#include "SemanticId.h"
#include "SemanticLinkSub.h"
#include "SemanticReference.h"
#include "SemanticTopology.h"

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

void testParseSubName()
{
    CHECK(elementIndexFromSubName("Face6") == ElementIndex::fromString("Face6"));
    CHECK(elementIndexFromSubName("Pad001.Face6") == ElementIndex::fromString("Face6"));
    CHECK(elementIndexFromSubName("Pad001.Edge12") == ElementIndex::fromString("Edge12"));
    CHECK(elementIndexFromSubName("Pad001").type.empty());
    CHECK(elementIndexFromSubName("").type.empty());
    CHECK(elementIndexFromSubName("Body.Pad001").type.empty());
    // GUI mapped blob from Tests_Manual_runb Pad Edge3 :U;XTR.
    CHECK(elementIndexFromSubName(";#9:1;:U;XTR;:H1324:7,E.Edge3")
          == ElementIndex::fromString("Edge3"));
    CHECK(elementIndexFromSubName(";#9:1;:U;XTR;:H1324:7").type.empty());

    const SemanticId tok = semanticIdFromSubName("Pad001.;:ST2a:F.Face6");
    CHECK(tok.valid());
    CHECK(tok.handle == 0x2a);
    CHECK(tok.kind == SemanticKind::Face);
    CHECK(!semanticIdFromSubName("Face6").valid());
}

void testRefuseRawFaceNWithoutGraph()
{
    const SemanticHandle before = 0;
    (void)before;
    const SemanticReference ref = makeSemanticRefForSubName("Face6", nullptr);
    CHECK(!ref.seed.valid());
    CHECK(ref.state == ResolutionState::Incompatible);
    CHECK(ref.fallback == ElementIndex::fromString("Face6"));
    CHECK(dualWriteSubName(ref) == "Face6");
}

void testRefuseRawFaceNEmptyGraph()
{
    SemanticGraph g;
    const SemanticHandle water = g.allocator.highWaterMark();
    const SemanticReference ref = makeSemanticRefForSubName("Pad001.Face6", &g);
    CHECK(!ref.seed.valid());
    CHECK(ref.state == ResolutionState::Incompatible);
    CHECK(g.allocator.highWaterMark() == water);  // I13: did not mint
}

void testPromoteOnlyWhenBindingUniquelyIdentifies()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(a, 1, 1, "Face6"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const SemanticReference ref = makeSemanticRefForSubName("Face6", &g, SemanticRole::SupportFace);
    CHECK(ref.seed.valid());
    CHECK(ref.seed.handle == a.handle);
    CHECK(ref.kind == SemanticKind::Face);
    CHECK(ref.role == SemanticRole::SupportFace);
    CHECK(ref.reducer == CardinalityReducer::RequireOne);
    CHECK(ref.fallback == ElementIndex::fromString("Face6"));
    CHECK(dualWriteSubName(ref) == "Face6");
    CHECK(g.allocator.highWaterMark() == water);  // attached, did not allocate
}

void testRefuseAmbiguousBinding()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const SemanticId b = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face6"));
    g.bind(makeBinding(b, 1, 1, "Face6"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const SemanticReference ref = makeSemanticRefForSubName("Face6", &g);
    CHECK(!ref.seed.valid());
    CHECK(ref.state == ResolutionState::Incompatible);
    CHECK(g.allocator.highWaterMark() == water);
}

void testDecodeMappedTokenWithoutGraph()
{
    const std::string sub = std::string("Pad001.") + SemanticId{0x2a, SemanticKind::Face}.toMappedToken()
        + ".Face6";
    const SemanticReference ref = makeSemanticRefForSubName(sub, nullptr);
    CHECK(ref.seed.valid());
    CHECK(ref.seed.handle == 0x2a);
    CHECK(ref.kind == SemanticKind::Face);
    CHECK(ref.fallback == ElementIndex::fromString("Face6"));
    CHECK(dualWriteSubName(ref) == "Face6");
}

void testSizeSync()
{
    std::vector<SemanticReference> refs;
    syncSemanticRefsSize(refs, 3);
    CHECK(refs.size() == 3);
    CHECK(!refs[0].seed.valid());
    refs.emplace_back();
    syncSemanticRefsSize(refs, 2);
    CHECK(refs.size() == 2);
}

void testXmlRoundTrip()
{
    SemanticReference ref;
    ref.seed.handle = 0x2a;
    ref.seed.kind = SemanticKind::Face;
    ref.kind = SemanticKind::Face;
    ref.role = SemanticRole::SupportFace;
    applyRoleDefaults(ref);
    ref.fallback = ElementIndex::fromString("Face6");

    const std::string attrs = semanticRefXmlAttributes(ref);
    CHECK(attrs.find("stSeed=\"2a\"") != std::string::npos);
    CHECK(attrs.find("stKind=\"F\"") != std::string::npos);
    CHECK(attrs.find("stFallback=\"Face6\"") != std::string::npos);
    CHECK(attrs.find(dualWriteSubName(ref)) != std::string::npos);

    SemanticReference empty;
    empty.fallback = ElementIndex::fromString("Face6");
    CHECK(semanticRefXmlAttributes(empty).empty());  // no seed → no XML (pre-migration)

    SemanticRefXmlFields f;
    f.seed = "2a";
    f.kind = "F";
    f.role = "1";
    f.filter = "3";
    f.reducer = "0";
    f.fallback = "Face6";
    const SemanticReference back = semanticRefFromXmlFields(f);
    CHECK(back.seed.handle == 0x2a);
    CHECK(back.kind == SemanticKind::Face);
    CHECK(back.role == SemanticRole::SupportFace);
    CHECK(back.fallback == ElementIndex::fromString("Face6"));
    CHECK(dualWriteSubName(back) == "Face6");
}

void testXmlRejectsMalformedSeed()
{
    SemanticRefXmlFields trailing;
    trailing.seed = "2a-garbage";
    trailing.kind = "F";
    trailing.fallback = "Face6";
    CHECK(!semanticRefFromXmlFields(trailing).seed.valid());

    SemanticRefXmlFields zero;
    zero.seed = "0";
    zero.kind = "F";
    CHECK(!semanticRefFromXmlFields(zero).seed.valid());

    SemanticRefXmlFields signedValue;
    signedValue.seed = "+2a";
    signedValue.kind = "F";
    CHECK(!semanticRefFromXmlFields(signedValue).seed.valid());

    // Kind must be exactly one mapped letter. Garbage must not become Face.
    SemanticRefXmlFields badKind;
    badKind.seed = "2a";
    badKind.kind = "X";
    badKind.fallback = "Face6";
    CHECK(!semanticRefFromXmlFields(badKind).seed.valid());
    CHECK(semanticRefFromXmlFields(badKind).fallback == ElementIndex::fromString("Face6"));

    SemanticRefXmlFields multi;
    multi.seed = "2a";
    multi.kind = "FX";
    multi.fallback = "Face6";
    CHECK(!semanticRefFromXmlFields(multi).seed.valid());

    SemanticRefXmlFields missingKind;
    missingKind.seed = "2a";
    missingKind.fallback = "Face6";
    CHECK(!semanticRefFromXmlFields(missingKind).seed.valid());

    SemanticRefXmlFields edgeOk;
    edgeOk.seed = "2a";
    edgeOk.kind = "E";
    edgeOk.fallback = "Edge3";
    const SemanticReference edgeBack = semanticRefFromXmlFields(edgeOk);
    CHECK(edgeBack.seed.valid());
    CHECK(edgeBack.seed.kind == SemanticKind::Edge);
    CHECK(edgeBack.kind == SemanticKind::Edge);
}

void testRewriteFallbackFromBinding()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face9"));
    g.commitEvaluate();

    SemanticReference ref;
    ref.seed = a;
    ref.kind = SemanticKind::Face;
    ref.reducer = CardinalityReducer::RequireOne;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    ref.fallback = ElementIndex::fromString("Face6");

    CHECK(rewriteFallbackFromBinding(ref, g));
    CHECK(ref.fallback == ElementIndex::fromString("Face9"));
    CHECK(ref.state == ResolutionState::Resolved);
    CHECK(dualWriteSubName(ref) == "Face9");
}

void testRewriteDoesNotCoerceAmbiguous()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    const auto kids = g.recordSplit(a, 2, "Split", 1, 1, SemanticRole::None);
    g.bind(makeBinding(kids[0], 1, 1, "Face1"));
    g.bind(makeBinding(kids[1], 1, 1, "Face2"));
    g.commitEvaluate();

    SemanticReference ref;
    ref.seed = a;
    ref.kind = SemanticKind::Face;
    ref.reducer = CardinalityReducer::RequireOne;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    ref.fallback = ElementIndex::fromString("Face6");

    CHECK(!rewriteFallbackFromBinding(ref, g));
    CHECK(ref.state == ResolutionState::Ambiguous);
    CHECK(ref.fallback == ElementIndex::fromString("Face6"));  // I9: not coerced
}

void testWholeObjectNoFaceN()
{
    const SemanticReference ref = makeSemanticRefForSubName("", nullptr);
    CHECK(!ref.seed.valid());
    CHECK(ref.state == ResolutionState::Missing);
    CHECK(ref.fallback.type.empty());
}

void testPromoteDoesNotOverwriteRestoredSeed()
{
    // C1: restore has stSeed, STG1 graph has no SemanticBinding. promoteWithGraph must
    // not replace the seed by re-parsing FaceN against empty Bindings.
    SemanticReference restored;
    restored.seed.handle = 0x2a;
    restored.seed.kind = SemanticKind::Face;
    restored.kind = SemanticKind::Face;
    restored.state = ResolutionState::Resolved;
    restored.fallback = ElementIndex::fromString("Face6");

    SemanticGraph empty;
    std::vector<SemanticReference> refs = {restored};
    promoteRefsWithGraph(refs, {"Face6"}, empty);
    CHECK(refs[0].seed.handle == 0x2a);
    CHECK(refs[0].state == ResolutionState::Resolved);
    CHECK(refs[0].fallback == ElementIndex::fromString("Face6"));

    // Even a leftover SemanticBinding on Face6 for a *different* identity must not
    // overwrite the restored seed (silent cross-lineage).
    SemanticGraph leftover;
    leftover.beginEvaluate(1);
    const SemanticId other =
        leftover.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    leftover.bind(makeBinding(other, 1, 1, "Face6"));
    leftover.commitEvaluate();
    CHECK(other.handle != 0x2a);
    promoteRefsWithGraph(refs, {"Face6"}, leftover);
    CHECK(refs[0].seed.handle == 0x2a);
    CHECK(refs[0].state == ResolutionState::Resolved);
}

void testPromoteFillsEmptySlotFromBinding()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a =
        g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(a, 1, 1, "Face6"));
    g.commitEvaluate();

    std::vector<SemanticReference> refs;
    promoteRefsWithGraph(refs, {"Face6"}, g);
    CHECK(refs.size() == 1);
    CHECK(refs[0].seed.handle == a.handle);
    CHECK(refs[0].state == ResolutionState::Resolved);
}

void testD2NoOpWithoutBinding()
{
    SemanticGraph g;
    SemanticReference ref;
    ref.seed.handle = 0x2a;
    ref.seed.kind = SemanticKind::Face;
    ref.kind = SemanticKind::Face;
    ref.reducer = CardinalityReducer::RequireOne;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    ref.state = ResolutionState::Resolved;
    ref.fallback = ElementIndex::fromString("Face6");

    CHECK(!g.hasBindings());
    CHECK(!rewriteFallbackFromBinding(ref, g));
    CHECK(ref.state == ResolutionState::Resolved);
    CHECK(ref.fallback == ElementIndex::fromString("Face6"));
}

void testPromoteEdge8BindingWritesStSeed()
{
    // Step 2: unique live SemanticBinding for Edge8 attaches; XML dual-writes stSeed.
    // Does not mint from raw EdgeN (I13). Face8 is a different index.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(edge, 20, 1, "Edge8"));
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::None);
    g.bind(makeBinding(face, 20, 1, "Face8"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const auto refs = makeSemanticRefsForSubNames({"Edge8"}, &g, SemanticRole::DressUpEdge);
    CHECK(refs.size() == 1);
    CHECK(refs[0].seed.valid());
    CHECK(refs[0].seed.handle == edge.handle);
    CHECK(refs[0].kind == SemanticKind::Edge);
    CHECK(refs[0].fallback == ElementIndex::fromString("Edge8"));
    CHECK(refs[0].fallback.index == 8);
    CHECK(refs[0].fallback.index != 1);
    const std::string attrs = semanticRefXmlAttributes(refs[0]);
    CHECK(attrs.find("stSeed=") != std::string::npos);
    CHECK(attrs.find("stKind=\"E\"") != std::string::npos);
    CHECK(attrs.find("stFallback=\"Edge8\"") != std::string::npos);
    CHECK(g.allocator.highWaterMark() == water);

    // Raw Edge8 with empty graph: cache only, no mint.
    SemanticGraph empty;
    const SemanticHandle water2 = empty.allocator.highWaterMark();
    const SemanticReference raw = makeSemanticRefForSubName("Edge8", &empty);
    CHECK(!raw.seed.valid());
    CHECK(raw.state == ResolutionState::Incompatible);
    CHECK(semanticRefXmlAttributes(raw).empty());
    CHECK(empty.allocator.highWaterMark() == water2);

    // Face8 binding must not attach to an Edge8 subname.
    SemanticGraph facesOnly;
    facesOnly.beginEvaluate(1);
    const SemanticId f =
        facesOnly.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::None);
    facesOnly.bind(makeBinding(f, 20, 1, "Face8"));
    facesOnly.commitEvaluate();
    const SemanticReference wrong = makeSemanticRefForSubName("Edge8", &facesOnly);
    CHECK(!wrong.seed.valid());
}

void testVectorHelper()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 1, 1, SemanticRole::None);
    g.bind(makeBinding(a, 1, 1, "Face1"));
    g.commitEvaluate();

    const auto refs = makeSemanticRefsForSubNames({"Face1", "Face99"}, &g);
    CHECK(refs.size() == 2);
    CHECK(refs[0].seed.handle == a.handle);
    CHECK(!refs[1].seed.valid());
    CHECK(refs[1].state == ResolutionState::Incompatible);
}

void testFeatureScopedEdge15Promote()
{
    // Tests_Manual: Pad and Pad001 both publish Edge15. Document-wide unique
    // Binding refuses (I13). Scoped to the linked feature attaches that seed.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId seedA =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    const SemanticId seedB =
        g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(seedA, 10, 1, "Edge15"));
    g.bind(makeBinding(seedB, 20, 1, "Edge15"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const ElementIndex edge15 = ElementIndex::fromString("Edge15");
    CHECK(!canPromoteIndexedNameToSeed(g, edge15));          // document-wide
    CHECK(!canPromoteIndexedNameToSeed(g, edge15, 0));
    CHECK(canPromoteIndexedNameToSeed(g, edge15, 10));
    CHECK(canPromoteIndexedNameToSeed(g, edge15, 20));
    CHECK(!canPromoteIndexedNameToSeed(g, edge15, 99));      // 0 matches for feature

    const SemanticReference docWide = makeSemanticRefForSubName("Edge15", &g);
    CHECK(!docWide.seed.valid());
    CHECK(docWide.state == ResolutionState::Incompatible);
    CHECK(g.allocator.highWaterMark() == water);  // I13: did not mint

    const SemanticReference forA =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 10);
    CHECK(forA.seed.valid());
    CHECK(forA.seed.handle == seedA.handle);
    CHECK(forA.kind == SemanticKind::Edge);
    CHECK(forA.fallback == edge15);

    const SemanticReference forB =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 20);
    CHECK(forB.seed.valid());
    CHECK(forB.seed.handle == seedB.handle);
    CHECK(forB.seed.handle != seedA.handle);

    const auto refsA = makeSemanticRefsForSubNames({"Edge15"}, &g, SemanticRole::DressUpEdge, 10);
    CHECK(refsA.size() == 1);
    CHECK(refsA[0].seed.handle == seedA.handle);

    const auto refsB = makeSemanticRefsForSubNames({"Edge15"}, &g, SemanticRole::DressUpEdge, 20);
    CHECK(refsB[0].seed.handle == seedB.handle);

    CHECK(g.allocator.highWaterMark() == water);
}

void testPromotePreservesPolicyAndFallbackCache()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Pad", 10, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(face, 10, 1, "Face6"));
    g.commitEvaluate();

    // A malformed/absent seed can still carry restored policy fields. When
    // Sub is a mapped name, stFallback is the only indexed cache available
    // for the same verified C1 promotion.
    SemanticReference restored;
    restored.role = SemanticRole::SupportFace;
    restored.filter.flags = FilterFlag::UserPredicate;
    restored.filter.sameGenerator = 77;
    restored.filter.sameRole = SemanticRole::User;
    restored.filter.sameInstance = 9;
    restored.reducer = CardinalityReducer::UniqueClosest;
    restored.anchor = "datum-anchor";
    restored.fallback = ElementIndex::fromString("Face6");
    restored.state = ResolutionState::Incompatible;

    std::vector<SemanticReference> refs = {restored};
    promoteRefsWithGraph(refs, {";:mapped"}, g, 10);
    CHECK(refs[0].seed.handle == face.handle);
    CHECK(refs[0].fallback == ElementIndex::fromString("Face6"));
    CHECK(refs[0].role == SemanticRole::SupportFace);
    CHECK(refs[0].filter.flags == FilterFlag::UserPredicate);
    CHECK(refs[0].filter.sameGenerator == 77);
    CHECK(refs[0].filter.sameRole == SemanticRole::User);
    CHECK(refs[0].filter.sameInstance == 9);
    CHECK(refs[0].reducer == CardinalityReducer::UniqueClosest);
    CHECK(refs[0].anchor && *refs[0].anchor == "datum-anchor");
}

void testPromoteEmptySlotThenC1()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(a, 10, 1, "Edge15"));
    g.commitEvaluate();

    // Empty slot + Binding appears → promoteRefsWithGraph fills seed.
    std::vector<SemanticReference> refs;
    promoteRefsWithGraph(refs, {"Edge15"}, g, 10);
    CHECK(refs.size() == 1);
    CHECK(refs[0].seed.handle == a.handle);
    CHECK(refs[0].state == ResolutionState::Resolved);

    // C1: a later Binding for a different identity must not overwrite.
    SemanticGraph other;
    other.beginEvaluate(1);
    (void)other.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    const SemanticId b =
        other.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    other.bind(makeBinding(b, 10, 1, "Edge15"));
    other.commitEvaluate();
    CHECK(b.handle != a.handle);
    promoteRefsWithGraph(refs, {"Edge15"}, other, 10);
    CHECK(refs[0].seed.handle == a.handle);
    CHECK(refs[0].state == ResolutionState::Resolved);

    // Same-size caller vector of empty refs (GUI setValue path) fills from graph.
    SemanticReference emptySlot;
    emptySlot.state = ResolutionState::Incompatible;
    emptySlot.fallback = ElementIndex::fromString("Edge15");
    std::vector<SemanticReference> passed = {emptySlot};
    promoteRefsWithGraph(passed, {"Edge15"}, g, 10);
    CHECK(passed[0].seed.handle == a.handle);
}

void testUniqueEdgeBindingOnLinkedFeature()
{
    // Consume uniqueness (I13): unique Edge Binding on the linked feature
    // yields that index. Two features publishing the same EdgeN: leftover
    // does not count for the Pad seed (refuse consume on the other). 0
    // matches → no consume and no mint. Never first-Binding-wins.
    SemanticGraph g;
    const ObjectId pad = 20;
    const ObjectId leftover = 30;
    g.beginEvaluate(1);
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::DressUpEdge);
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    CHECK(!uniqueBindingOnFeature(nullptr, edge, pad, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, SemanticId{}, pad, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, edge, 0, "Edge").has_value());
    CHECK(!uniqueBindingOnFeature(&g, edge, pad, "Edge").has_value());  // 0 matches
    CHECK(!uniqueBindingOnFeature(&g, edge, leftover, "Edge").has_value());
    CHECK(g.allocator.highWaterMark() == water);  // I13: did not mint

    g.beginEvaluate(2);
    g.bind(makeBinding(edge, pad, 2, "Edge5"));
    g.commitEvaluate();
    const auto one = uniqueBindingOnFeature(&g, edge, pad, "Edge");
    CHECK(one.has_value());
    CHECK(one->feature == pad);
    CHECK(one->index.toString() == std::string("Edge5"));
    CHECK(one->index.index == 5);
    CHECK(!uniqueBindingOnFeature(&g, edge, leftover, "Edge").has_value());  // 0 on leftover
    CHECK(g.allocator.highWaterMark() == water);

    // Two features same EdgeN: leftover owns a different seed at Edge5.
    // Pad consume stays unique. Document-wide promote refuses (I13).
    g.beginEvaluate(3);
    const SemanticId other =
        g.recordGenerated(SemanticKind::Edge, "Pad", leftover, 3, SemanticRole::DressUpEdge);
    g.bind(makeBinding(other, leftover, 3, "Edge5"));
    g.commitEvaluate();
    const SemanticHandle water2 = g.allocator.highWaterMark();
    const auto still = uniqueBindingOnFeature(&g, edge, pad, "Edge");
    CHECK(still.has_value());
    CHECK(still->feature == pad);
    CHECK(still->index.toString() == std::string("Edge5"));
    CHECK(!uniqueBindingOnFeature(&g, edge, leftover, "Edge").has_value());  // refuse for the other
    CHECK(uniqueBindingOnFeature(&g, other, leftover, "Edge").has_value());
    CHECK(!canPromoteIndexedNameToSeed(g, ElementIndex::fromString("Edge5")));  // document-wide
    CHECK(canPromoteIndexedNameToSeed(g, ElementIndex::fromString("Edge5"), pad));
    CHECK(g.allocator.highWaterMark() == water2);  // no mint

    g.beginEvaluate(4);
    g.bind(makeBinding(edge, pad, 4, "Edge7"));
    g.commitEvaluate();
    CHECK(!uniqueBindingOnFeature(&g, edge, pad, "Edge").has_value());  // >1 on Pad
    CHECK(g.allocator.highWaterMark() == water2);
}

void testZeroMatchesForFeatureNoMint()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(a, 10, 1, "Edge15"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    // Linked feature 20 has no Edge15 Binding → refuse, do not mint, do not
    // take feature 10's seed (would be first-Binding-wins across features).
    CHECK(!canPromoteIndexedNameToSeed(g, ElementIndex::fromString("Edge15"), 20));
    const SemanticReference ref =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 20);
    CHECK(!ref.seed.valid());
    CHECK(ref.state == ResolutionState::Incompatible);
    CHECK(ref.fallback == ElementIndex::fromString("Edge15"));
    CHECK(g.allocator.highWaterMark() == water);

    std::vector<SemanticReference> refs;
    promoteRefsWithGraph(refs, {"Edge15"}, g, 20);
    CHECK(refs.size() == 1);
    CHECK(!refs[0].seed.valid());
}

void testPocketTipEdge2AndPadEdge13Promote()
{
    // Tests_Automated: Fillet.Base=(Pocket, Edge2). Binding on Pocket → stSeed.
    // Binding only on Pad + linked Pocket → refuse (no first-Binding-wins).
    // Tests_Manual: unique Pad Edge13 (GUI XTR pick) → stSeed, not Edge1.
    const ObjectId pad = 20;
    const ObjectId pocket = 30;
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId pocketEdge =
        g.recordGenerated(SemanticKind::Edge, "Pocket", pocket, 1, SemanticRole::DressUpEdge);
    const SemanticId padEdge13 =
        g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::DressUpEdge);
    const SemanticId padEdge3 =
        g.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(pocketEdge, pocket, 1, "Edge2"));
    g.bind(makeBinding(padEdge13, pad, 1, "Edge13"));
    g.bind(makeBinding(padEdge3, pad, 1, "Edge3"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const ElementIndex edge2 = ElementIndex::fromString("Edge2");
    const ElementIndex edge13 = ElementIndex::fromString("Edge13");

    CHECK(canPromoteIndexedNameToSeed(g, edge2, pocket));
    const SemanticReference tip =
        makeSemanticRefForSubName("Edge2", &g, SemanticRole::DressUpEdge, pocket);
    CHECK(tip.seed.valid());
    CHECK(tip.seed.handle == pocketEdge.handle);
    CHECK(tip.kind == SemanticKind::Edge);
    CHECK(tip.fallback == edge2);
    const std::string xml = semanticRefXmlAttributes(tip);
    CHECK(xml.find("stSeed=") != std::string::npos);

    // Pad owns a different Edge2? none. Linked Pocket must not take Pad's
    // Edge13 or any other Pad row. Pad Binding.feature + linked Pocket → refuse.
    CHECK(!canPromoteIndexedNameToSeed(g, edge2, pad));
    SemanticGraph padOnly;
    padOnly.beginEvaluate(1);
    const SemanticId padOwns2 =
        padOnly.recordGenerated(SemanticKind::Edge, "Pad", pad, 1, SemanticRole::DressUpEdge);
    padOnly.bind(makeBinding(padOwns2, pad, 1, "Edge2"));
    padOnly.commitEvaluate();
    const SemanticHandle padWater = padOnly.allocator.highWaterMark();
    CHECK(!canPromoteIndexedNameToSeed(padOnly, edge2, pocket));
    const SemanticReference refused =
        makeSemanticRefForSubName("Edge2", &padOnly, SemanticRole::DressUpEdge, pocket);
    CHECK(!refused.seed.valid());
    CHECK(refused.state == ResolutionState::Incompatible);
    CHECK(refused.fallback == edge2);
    CHECK(padOnly.allocator.highWaterMark() == padWater);  // I13: did not mint
    CHECK(semanticRefXmlAttributes(refused).find("stSeed=") == std::string::npos);

    CHECK(canPromoteIndexedNameToSeed(g, edge13, pad));
    const SemanticReference gui13 =
        makeSemanticRefForSubName("Edge13", &g, SemanticRole::DressUpEdge, pad);
    CHECK(gui13.seed.valid());
    CHECK(gui13.seed.handle == padEdge13.handle);
    CHECK(gui13.fallback == edge13);
    CHECK(gui13.fallback != ElementIndex::fromString("Edge1"));
    CHECK(semanticRefXmlAttributes(gui13).find("stSeed=") != std::string::npos);

    // GUI mapped :U;XTR blob still parses as Edge3 and promotes (Manual runb).
    const SemanticReference guiU = makeSemanticRefForSubName(
        ";#9:1;:U;XTR;:H1324:7,E.Edge3", &g, SemanticRole::DressUpEdge, pad);
    CHECK(guiU.seed.valid());
    CHECK(guiU.seed.handle == padEdge3.handle);
    CHECK(guiU.fallback == ElementIndex::fromString("Edge3"));
    CHECK(semanticRefXmlAttributes(guiU).find("stSeed=") != std::string::npos);

    CHECK(g.allocator.highWaterMark() == water);
}

void testAmbiguousOnSameFeatureStillRefuses()
{
    // Two published Edge15 Bindings on the *same* feature: scoped uniqueness
    // is still 2 → I13 refuse (multi-image). Not first-Binding-wins.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    const SemanticId b =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(a, 10, 1, "Edge15"));
    g.bind(makeBinding(b, 10, 1, "Edge15"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    CHECK(!canPromoteIndexedNameToSeed(g, ElementIndex::fromString("Edge15"), 10));
    const SemanticReference ref =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 10);
    CHECK(!ref.seed.valid());
    CHECK(g.allocator.highWaterMark() == water);
}


void testKeepValidSeedsForSameIndex()
{
    // TaskDlg accept() rebuilds refs from subnames. Keep Edge7 seed; do not
    // copy it onto Edge8; two previous Edge7 seeds -> I13 no keep.
    SemanticReference seeded;
    seeded.seed.handle = 0x3f;
    seeded.seed.kind = SemanticKind::Edge;
    seeded.kind = SemanticKind::Edge;
    seeded.fallback = ElementIndex::fromString("Edge7");
    seeded.state = ResolutionState::Resolved;

    SemanticReference empty;
    empty.state = ResolutionState::Incompatible;

    std::vector<SemanticReference> dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Edge7"}, {seeded}, {"Edge7"});
    CHECK(dest[0].seed.handle == 0x3f);
    CHECK(dest[0].fallback.toString() == "Edge7");

    dest = {empty};
    keepValidSeedsForSameIndex(
        dest, {";#f:4;:U;XTR;:H302:7,E.Edge7"}, {seeded}, {"Edge7"});
    CHECK(dest[0].seed.handle == 0x3f);

    dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Edge8"}, {seeded}, {"Edge7"});
    CHECK(!dest[0].seed.valid());

    SemanticReference other = seeded;
    other.seed.handle = 0x40;
    dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Edge7"}, {seeded, other}, {"Edge7", "Edge7"});
    CHECK(!dest[0].seed.valid());

    dest = {seeded};
    keepValidSeedsForSameIndex(dest, {"Edge7"}, {other}, {"Edge7"});
    CHECK(dest[0].seed.handle == 0x3f);
}

void testLinkSubListFace6Policy()
{
    // Phase B: PropertyLinkSubList / AttachmentSupport-style (Pad, Face6).
    // Unique Binding on the linked feature attaches; document-wide Face6
    // with two features refuses; per-slot linkedFeature uses THAT slot's id.
    const ObjectId pad = 20;
    const ObjectId pad001 = 30;
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId seedPad =
        g.recordGenerated(SemanticKind::Face, "Pad", pad, 1, SemanticRole::SupportFace);
    const SemanticId seedPad001 =
        g.recordGenerated(SemanticKind::Face, "Pad", pad001, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(seedPad, pad, 1, "Face6"));
    g.bind(makeBinding(seedPad001, pad001, 1, "Face6"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const ElementIndex face6 = ElementIndex::fromString("Face6");

    // Two features Face6 document-wide: linkedFeature 0 refuses (I13).
    CHECK(!canPromoteIndexedNameToSeed(g, face6));
    CHECK(!canPromoteIndexedNameToSeed(g, face6, 0));
    const SemanticReference docWide = makeSemanticRefForSubName("Face6", &g);
    CHECK(!docWide.seed.valid());
    CHECK(docWide.state == ResolutionState::Incompatible);
    CHECK(g.allocator.highWaterMark() == water);  // I13: did not mint

    // AttachmentSupport-style (Pad, Face6): scoped to Pad attaches.
    CHECK(canPromoteIndexedNameToSeed(g, face6, pad));
    const SemanticReference forPad =
        makeSemanticRefForSubName("Face6", &g, SemanticRole::SupportFace, pad);
    CHECK(forPad.seed.valid());
    CHECK(forPad.seed.handle == seedPad.handle);
    CHECK(forPad.kind == SemanticKind::Face);
    CHECK(forPad.fallback == face6);
    CHECK(semanticRefXmlAttributes(forPad).find("stSeed=") != std::string::npos);

    CHECK(canPromoteIndexedNameToSeed(g, face6, pad001));
    const SemanticReference forPad001 =
        makeSemanticRefForSubName("Face6", &g, SemanticRole::SupportFace, pad001);
    CHECK(forPad001.seed.valid());
    CHECK(forPad001.seed.handle == seedPad001.handle);
    CHECK(forPad001.seed.handle != seedPad.handle);

    // Per-slot list: (Pad, Face6) + (Pad001, Face6) each use THAT slot's object id.
    std::vector<SemanticReference> refs;
    syncSemanticRefsSize(refs, 2);
    {
        std::vector<SemanticReference> slot0 = {refs[0]};
        promoteRefsWithGraph(slot0, {"Face6"}, g, pad);
        refs[0] = std::move(slot0[0]);
        std::vector<SemanticReference> slot1 = {refs[1]};
        promoteRefsWithGraph(slot1, {"Face6"}, g, pad001);
        refs[1] = std::move(slot1[0]);
    }
    CHECK(refs[0].seed.handle == seedPad.handle);
    CHECK(refs[1].seed.handle == seedPad001.handle);

    // 0 matches for that feature → no mint (not first-Binding-wins).
    CHECK(!canPromoteIndexedNameToSeed(g, face6, 99));
    const SemanticReference none =
        makeSemanticRefForSubName("Face6", &g, SemanticRole::SupportFace, 99);
    CHECK(!none.seed.valid());
    CHECK(none.state == ResolutionState::Incompatible);
    CHECK(none.fallback == face6);
    CHECK(g.allocator.highWaterMark() == water);

    // keepValidSeeds same FaceN; different FaceN does not keep.
    SemanticReference empty;
    empty.state = ResolutionState::Incompatible;
    std::vector<SemanticReference> dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Face6"}, {forPad}, {"Face6"});
    CHECK(dest[0].seed.handle == seedPad.handle);
    CHECK(dest[0].fallback == face6);

    dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Face7"}, {forPad}, {"Face6"});
    CHECK(!dest[0].seed.valid());
}

void testXLinkSubEdge15Policy()
{
    // Phase E leftover: PropertyXLink / PropertyXLinkSub clone LinkSub policy.
    // Two features Binding Edge15: document-wide refuses; XLink scoped to A
    // attaches A's seed; scoped to B a different seed; 0 matches no mint;
    // C1 keep a valid seed. Does not mint (I13).
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId seedA =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    const SemanticId seedB =
        g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(seedA, 10, 1, "Edge15"));
    g.bind(makeBinding(seedB, 20, 1, "Edge15"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const ElementIndex edge15 = ElementIndex::fromString("Edge15");
    CHECK(!canPromoteIndexedNameToSeed(g, edge15));
    CHECK(!canPromoteIndexedNameToSeed(g, edge15, 0));
    CHECK(canPromoteIndexedNameToSeed(g, edge15, 10));
    CHECK(canPromoteIndexedNameToSeed(g, edge15, 20));
    CHECK(!canPromoteIndexedNameToSeed(g, edge15, 99));

    const SemanticReference docWide = makeSemanticRefForSubName("Edge15", &g);
    CHECK(!docWide.seed.valid());
    CHECK(docWide.state == ResolutionState::Incompatible);
    CHECK(g.allocator.highWaterMark() == water);

    const SemanticReference forA =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 10);
    CHECK(forA.seed.valid());
    CHECK(forA.seed.handle == seedA.handle);
    CHECK(forA.kind == SemanticKind::Edge);
    CHECK(forA.fallback == edge15);
    CHECK(semanticRefXmlAttributes(forA).find("stSeed=") != std::string::npos);

    const SemanticReference forB =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 20);
    CHECK(forB.seed.valid());
    CHECK(forB.seed.handle == seedB.handle);
    CHECK(forB.seed.handle != seedA.handle);

    const auto refsA = makeSemanticRefsForSubNames({"Edge15"}, &g, SemanticRole::DressUpEdge, 10);
    CHECK(refsA.size() == 1);
    CHECK(refsA[0].seed.handle == seedA.handle);
    const auto refsB = makeSemanticRefsForSubNames({"Edge15"}, &g, SemanticRole::DressUpEdge, 20);
    CHECK(refsB[0].seed.handle == seedB.handle);

    CHECK(!canPromoteIndexedNameToSeed(g, edge15, 99));
    const SemanticReference none =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 99);
    CHECK(!none.seed.valid());
    CHECK(none.state == ResolutionState::Incompatible);
    CHECK(none.fallback == edge15);
    CHECK(g.allocator.highWaterMark() == water);

    std::vector<SemanticReference> refs;
    promoteRefsWithGraph(refs, {"Edge15"}, g, 99);
    CHECK(refs.size() == 1);
    CHECK(!refs[0].seed.valid());

    SemanticReference empty;
    empty.state = ResolutionState::Incompatible;
    std::vector<SemanticReference> dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Edge15"}, {forA}, {"Edge15"});
    CHECK(dest[0].seed.handle == seedA.handle);
    CHECK(dest[0].fallback == edge15);

    dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Edge16"}, {forA}, {"Edge15"});
    CHECK(!dest[0].seed.valid());

    dest = {forA};
    promoteRefsWithGraph(dest, {"Edge15"}, g, 20);
    CHECK(dest[0].seed.handle == seedA.handle);
}

void testIdentifierFace6Policy()
{
    // Phase E leftover: Spreadsheet / ObjectIdentifier dual-write.
    // Two features Binding Face6 -> identifier scoped to A attaches A's seed;
    // scoped to B a different seed; 0 matches no mint; C1 keep a valid seed.
    // Does not mint (I13). Not document-wide Face6 (not first-Binding-wins).
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId seedA =
        g.recordGenerated(SemanticKind::Face, "Pad", 10, 1, SemanticRole::SupportFace);
    const SemanticId seedB =
        g.recordGenerated(SemanticKind::Face, "Pad", 20, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(seedA, 10, 1, "Face6"));
    g.bind(makeBinding(seedB, 20, 1, "Face6"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const ElementIndex face6 = ElementIndex::fromString("Face6");
    CHECK(!canPromoteIndexedNameToSeed(g, face6));
    CHECK(!canPromoteIndexedNameToSeed(g, face6, 0));
    const SemanticReference docWide = makeSemanticRefForSubName("Face6", &g);
    CHECK(!docWide.seed.valid());
    CHECK(docWide.state == ResolutionState::Incompatible);
    CHECK(g.allocator.highWaterMark() == water);

    const SemanticReference forA =
        makeSemanticRefForSubName("Face6", &g, SemanticRole::None, 10);
    CHECK(forA.seed.valid());
    CHECK(forA.seed.handle == seedA.handle);
    CHECK(forA.kind == SemanticKind::Face);
    CHECK(forA.fallback == face6);
    CHECK(semanticRefXmlAttributes(forA).find("stSeed=") != std::string::npos);

    const SemanticReference forB =
        makeSemanticRefForSubName("Face6", &g, SemanticRole::None, 20);
    CHECK(forB.seed.valid());
    CHECK(forB.seed.handle == seedB.handle);
    CHECK(forB.seed.handle != seedA.handle);

    CHECK(!canPromoteIndexedNameToSeed(g, face6, 99));
    const SemanticReference none = makeSemanticRefForSubName("Face6", &g, SemanticRole::None, 99);
    CHECK(!none.seed.valid());
    CHECK(none.state == ResolutionState::Incompatible);
    CHECK(none.fallback == face6);
    CHECK(g.allocator.highWaterMark() == water);

    // Identifier-style promote: empty slot fills; C1 keeps a valid seed.
    std::vector<SemanticReference> refs;
    promoteRefsWithGraph(refs, {"Face6"}, g, 10);
    CHECK(refs.size() == 1);
    CHECK(refs[0].seed.handle == seedA.handle);

    SemanticReference empty;
    empty.state = ResolutionState::Incompatible;
    std::vector<SemanticReference> dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Face6"}, {forA}, {"Face6"});
    CHECK(dest[0].seed.handle == seedA.handle);
    CHECK(dest[0].fallback == face6);

    dest = {empty};
    keepValidSeedsForSameIndex(dest, {"Face7"}, {forA}, {"Face6"});
    CHECK(!dest[0].seed.valid());

    dest = {forA};
    promoteRefsWithGraph(dest, {"Face6"}, g, 20);
    CHECK(dest[0].seed.handle == seedA.handle);  // C1: not overwritten

    // D2: unique Binding rewrites FaceN cache; seed unchanged.
    SemanticReference jumped = forA;
    jumped.fallback = ElementIndex::fromString("Face2");
    CHECK(rewriteFallbackFromBinding(jumped, g));
    CHECK(jumped.seed.handle == seedA.handle);
    CHECK(jumped.fallback == face6);
}

void testUniquePadEdge7PromoteAndMaxEval()
{
    // Unique Pad Edge7 (GUI :U;XTR) still promotes. Two bind() on the same
    // feature+Edge7 at the same eval → I13 refuse (no first-Binding-wins).
    // Pile-up across evals: only the highest eval counts → promote latest.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(edge, 20, 1, "Edge7"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    CHECK(canPromoteIndexedNameToSeed(g, ElementIndex::fromString("Edge7"), 20));
    const SemanticReference ref = makeSemanticRefForSubName(
        ";#f:4;:U;XTR;:H302:7,E.Edge7", &g, SemanticRole::DressUpEdge, 20);
    CHECK(ref.seed.valid());
    CHECK(ref.seed.handle == edge.handle);
    CHECK(ref.kind == SemanticKind::Edge);
    CHECK(ref.fallback == ElementIndex::fromString("Edge7"));
    CHECK(semanticRefXmlAttributes(ref).find("stSeed=") != std::string::npos);
    CHECK(g.allocator.highWaterMark() == water);

    SemanticGraph amb;
    amb.beginEvaluate(1);
    const SemanticId a =
        amb.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    const SemanticId b =
        amb.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    amb.bind(makeBinding(a, 20, 1, "Edge7"));
    amb.bind(makeBinding(b, 20, 1, "Edge7"));
    amb.commitEvaluate();
    CHECK(!canPromoteIndexedNameToSeed(amb, ElementIndex::fromString("Edge7"), 20));
    const SemanticReference refused =
        makeSemanticRefForSubName("Edge7", &amb, SemanticRole::DressUpEdge, 20);
    CHECK(!refused.seed.valid());
    CHECK(refused.state == ResolutionState::Incompatible);

    SemanticGraph piled;
    piled.beginEvaluate(1);
    const SemanticId oldId =
        piled.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    piled.bind(makeBinding(oldId, 20, 1, "Edge7"));
    piled.commitEvaluate();
    piled.beginEvaluate(2);
    const SemanticId fresh =
        piled.recordGenerated(SemanticKind::Edge, "Pad", 20, 2, SemanticRole::DressUpEdge);
    piled.bind(makeBinding(fresh, 20, 2, "Edge7"));
    piled.commitEvaluate();
    CHECK(canPromoteIndexedNameToSeed(piled, ElementIndex::fromString("Edge7"), 20));
    const SemanticReference latest =
        makeSemanticRefForSubName("Edge7", &piled, SemanticRole::DressUpEdge, 20);
    CHECK(latest.seed.valid());
    CHECK(latest.seed.handle == fresh.handle);
    CHECK(latest.seed.handle != oldId.handle);
}


void testReadPolicyRewritesEdgeCacheFromUniqueBinding()
{
    // Phase D: cache Edge2 + unique Binding Edge7 + valid seed -> after
    // read policy fallback/sub is Edge7, seed unchanged. 0 or 2 Bindings ->
    // no rewrite (I13). Never mint from raw EdgeN.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(edge, 20, 1, "Edge7"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    SemanticReference ref;
    ref.seed = edge;
    ref.kind = SemanticKind::Edge;
    ref.reducer = CardinalityReducer::RequireOne;
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    ref.fallback = ElementIndex::fromString("Edge2");
    ref.state = ResolutionState::Resolved;
    const SemanticHandle seedHandle = ref.seed.handle;
    std::string sub = "Edge2";

    CHECK(rewriteFallbackFromBinding(ref, g));
    CHECK(ref.seed.handle == seedHandle);
    CHECK(ref.fallback == ElementIndex::fromString("Edge7"));
    CHECK(dualWriteSubName(ref) == "Edge7");
    {
        const std::string cache = dualWriteSubName(ref);
        if (sub.find(';') == std::string::npos && sub.find('.') == std::string::npos
            && !elementIndexFromSubName(sub).type.empty()) {
            sub = cache;
        }
    }
    CHECK(sub == "Edge7");
    CHECK(g.allocator.highWaterMark() == water);  // I13: did not mint

    // 0 Bindings -> no rewrite
    SemanticGraph empty;
    SemanticReference noBind;
    noBind.seed = edge;
    noBind.kind = SemanticKind::Edge;
    noBind.reducer = CardinalityReducer::RequireOne;
    noBind.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    noBind.fallback = ElementIndex::fromString("Edge2");
    noBind.state = ResolutionState::Resolved;
    CHECK(!empty.hasBindings());
    CHECK(!rewriteFallbackFromBinding(noBind, empty));
    CHECK(noBind.seed.handle == seedHandle);
    CHECK(noBind.fallback == ElementIndex::fromString("Edge2"));

    // 2 Bindings (split images of the seed) -> no rewrite (I13)
    SemanticGraph amb;
    amb.beginEvaluate(1);
    const SemanticId parent =
        amb.recordGenerated(SemanticKind::Edge, "Pad", 20, 1, SemanticRole::DressUpEdge);
    const auto kids = amb.recordSplit(parent, 2, "Split", 20, 1, SemanticRole::None);
    amb.bind(makeBinding(kids[0], 20, 1, "Edge7"));
    amb.bind(makeBinding(kids[1], 20, 1, "Edge8"));
    amb.commitEvaluate();
    SemanticReference two;
    two.seed = parent;
    two.kind = SemanticKind::Edge;
    two.reducer = CardinalityReducer::RequireOne;
    two.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    two.fallback = ElementIndex::fromString("Edge2");
    two.state = ResolutionState::Resolved;
    CHECK(!rewriteFallbackFromBinding(two, amb));
    CHECK(two.seed.handle == parent.handle);
    CHECK(two.fallback == ElementIndex::fromString("Edge2"));
}

void testResolveSubNameFromSeedFeatureScope()
{
    // Loop 1 consume helper: unique Face Binding on A rewrites the FaceN
    // cache. Scoped to B (different feature) → no rewrite. 0 matches → no
    // rewrite. C1: valid seed is not overwritten. Never first-Binding-wins.
    SemanticGraph g;
    const ObjectId featA = 10;
    const ObjectId featB = 20;
    g.beginEvaluate(1);
    const SemanticId face =
        g.recordGenerated(SemanticKind::Face, "Pad", featA, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(face, featA, 1, "Face6"));
    const SemanticId edge =
        g.recordGenerated(SemanticKind::Edge, "Pad", featA, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(edge, featA, 1, "Edge9"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    CHECK(resolveSubNameFromSeed(&g, face, featA, "Face2") == std::string("Face6"));
    CHECK(resolveSubNameFromSeed(&g, face, featA, "Pad.Face2") == std::string("Pad.Face6"));
    // A stale cross-kind cache must not redirect a Face seed to Edge bindings.
    CHECK(resolveSubNameFromSeed(&g, face, featA, "Edge2") == std::string("Face6"));
    CHECK(resolveSubNameFromSeed(&g, edge, featA, "Face2") == std::string("Edge9"));
    CHECK(resolveSubNameFromSeed(&g, face, featB, "Face2") == std::string("Face2"));
    CHECK(resolveSubNameFromSeed(nullptr, face, featA, "Face2") == std::string("Face2"));
    CHECK(resolveSubNameFromSeed(&g, SemanticId{}, featA, "Face2") == std::string("Face2"));
    CHECK(resolveSubNameFromSeed(&g, face, 0, "Face2") == std::string("Face2"));
    CHECK(g.allocator.highWaterMark() == water);

    SemanticReference restored;
    restored.seed = face;
    restored.kind = SemanticKind::Face;
    restored.state = ResolutionState::Resolved;
    restored.fallback = ElementIndex::fromString("Face2");
    std::vector<SemanticReference> refs = {restored};
    promoteRefsWithGraph(refs, {"Face2"}, g, featA);
    CHECK(refs[0].seed.handle == face.handle);  // C1: not overwritten
    CHECK(resolveSubNameFromSeed(&g, refs[0].seed, featA, "Face2") == std::string("Face6"));
    CHECK(refs[0].seed.handle == face.handle);
    CHECK(g.allocator.highWaterMark() == water);

    g.beginEvaluate(2);
    g.bind(makeBinding(face, featA, 2, "Face9"));
    g.commitEvaluate();
    CHECK(resolveSubNameFromSeed(&g, face, featA, "Face2") == std::string("Face2"));  // >1
    CHECK(g.allocator.highWaterMark() == water);
}


void testSemanticSubNameMatchesKind()
{
    CHECK(semanticSubNameMatchesKind("Edge13", "Edge"));
    CHECK(semanticSubNameMatchesKind("Face6", "Face"));
    CHECK(semanticSubNameMatchesKind("Pad001.Edge13", "Edge"));
    CHECK(semanticSubNameMatchesKind("Pad001.Face6", "Face"));
    CHECK(!semanticSubNameMatchesKind("Face6", "Edge"));
    CHECK(!semanticSubNameMatchesKind("Edge13", "Face"));
    CHECK(semanticSubNameMatchesKind(";#9:1;:U;XTR;:H1324:7,E.Edge3", "Edge"));
    const std::string mappedFace =
        std::string("Pad001.") + SemanticId{0x2a, SemanticKind::Face}.toMappedToken() + ".Face6";
    CHECK(semanticSubNameMatchesKind(mappedFace, "Face"));
    CHECK(!semanticSubNameMatchesKind(mappedFace, "Edge"));
    const std::string tokenOnly = SemanticId{0x2a, SemanticKind::Edge}.toMappedToken();
    CHECK(semanticSubNameMatchesKind(tokenOnly, "Edge"));
    CHECK(!semanticSubNameMatchesKind(tokenOnly, "Face"));
    CHECK(semanticSubNameMatchesKind("Edge13", ""));
}

void testResolveSemanticSelectionUniqueAndRefuse()
{
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId a = g.recordGenerated(SemanticKind::Face, "Pad", 7, 1, SemanticRole::SupportFace);
    g.bind(makeBinding(a, 7, 1, "Face6"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    CandidateFilter filter;
    const ResolutionResult ok =
        resolveSemanticSelection(&g, 7, "Face6", filter, CardinalityReducer::RequireOne);
    CHECK(ok.state == ResolutionState::Resolved);
    CHECK(ok.identities.size() == 1);
    CHECK(ok.identities.front().handle == a.handle);
    CHECK(g.allocator.highWaterMark() == water);  // I13: did not mint

    const ResolutionResult wrongFeature =
        resolveSemanticSelection(&g, 99, "Face6", filter, CardinalityReducer::RequireOne);
    CHECK(wrongFeature.state != ResolutionState::Resolved);
    CHECK(wrongFeature.identities.empty());

    SemanticGraph empty;
    const ResolutionResult noBind =
        resolveSemanticSelection(&empty, 7, "Face6", filter, CardinalityReducer::RequireOne);
    CHECK(noBind.state != ResolutionState::Resolved);
    CHECK(!makeSemanticRefForSubName("Face6", &empty, SemanticRole::None, 7).seed.valid());

    const ResolutionResult noGraph =
        resolveSemanticSelection(nullptr, 7, "Face6", filter, CardinalityReducer::RequireOne);
    CHECK(noGraph.state == ResolutionState::Missing);

    // Body-tip lookup uses the Tip feature id (7), not the Body id (50).
    // Projection itself lives on DocumentObject; App resolve stays I13-scoped.
    const ResolutionResult asTip =
        resolveSemanticSelection(&g, 7, "Body.Face6", filter, CardinalityReducer::RequireOne);
    CHECK(asTip.state == ResolutionState::Resolved);
    CHECK(asTip.identities.front().handle == a.handle);
    const ResolutionResult asBody =
        resolveSemanticSelection(&g, 50, "Body.Face6", filter, CardinalityReducer::RequireOne);
    CHECK(asBody.state != ResolutionState::Resolved);
    CHECK(g.allocator.highWaterMark() == water);
}

void testPatternFeatureScopedEdge15()
{
    // Loop 2: two originals Binding Edge15 (Pad leftover 10, LinearPattern 40).
    // Scoped to pattern A attaches; scoped to Pad leftover when the link is
    // the pattern refuses; 0 matches no mint; C1 keep.
    SemanticGraph g;
    g.beginEvaluate(1);
    const SemanticId padSeed =
        g.recordGenerated(SemanticKind::Edge, "Pad", 10, 1, SemanticRole::DressUpEdge);
    const SemanticId patternSeed =
        g.recordGenerated(SemanticKind::Edge, "LinearPattern", 40, 1, SemanticRole::DressUpEdge);
    g.bind(makeBinding(padSeed, 10, 1, "Edge15"));
    g.bind(makeBinding(patternSeed, 40, 1, "Edge15"));
    g.commitEvaluate();
    const SemanticHandle water = g.allocator.highWaterMark();

    const ElementIndex edge15 = ElementIndex::fromString("Edge15");
    CHECK(!canPromoteIndexedNameToSeed(g, edge15));          // document-wide
    CHECK(canPromoteIndexedNameToSeed(g, edge15, 40));       // pattern
    CHECK(canPromoteIndexedNameToSeed(g, edge15, 10));       // Pad leftover exists
    CHECK(!canPromoteIndexedNameToSeed(g, edge15, 99));      // 0 matches

    const SemanticReference forPattern =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 40);
    CHECK(forPattern.seed.valid());
    CHECK(forPattern.seed.handle == patternSeed.handle);
    CHECK(forPattern.seed.handle != padSeed.handle);

    // Linked feature is the pattern: Pad leftover Edge15 does not attach.
    CHECK(forPattern.seed.handle != padSeed.handle);
    const SemanticReference forMissing =
        makeSemanticRefForSubName("Edge15", &g, SemanticRole::DressUpEdge, 99);
    CHECK(!forMissing.seed.valid());
    CHECK(g.allocator.highWaterMark() == water);

    std::vector<SemanticReference> keep = {forPattern};
    promoteRefsWithGraph(keep, {"Edge15"}, g, 40);
    CHECK(keep[0].seed.handle == patternSeed.handle);  // C1

    std::vector<SemanticReference> emptySlot(1);
    emptySlot[0].fallback = edge15;
    promoteRefsWithGraph(emptySlot, {"Edge15"}, g, 40);
    CHECK(emptySlot[0].seed.handle == patternSeed.handle);

    std::vector<SemanticReference> refusePad(1);
    refusePad[0].fallback = edge15;
    promoteRefsWithGraph(refusePad, {"Edge15"}, g, 99);
    CHECK(!refusePad[0].seed.valid());
    CHECK(g.allocator.highWaterMark() == water);
}

}  // namespace

int main()
{
    testParseSubName();
    testRefuseRawFaceNWithoutGraph();
    testRefuseRawFaceNEmptyGraph();
    testPromoteOnlyWhenBindingUniquelyIdentifies();
    testRefuseAmbiguousBinding();
    testDecodeMappedTokenWithoutGraph();
    testSizeSync();
    testXmlRoundTrip();
    testXmlRejectsMalformedSeed();
    testRewriteFallbackFromBinding();
    testRewriteDoesNotCoerceAmbiguous();
    testWholeObjectNoFaceN();
    testPromoteEdge8BindingWritesStSeed();
    testVectorHelper();
    testPromoteDoesNotOverwriteRestoredSeed();
    testPromoteFillsEmptySlotFromBinding();
    testD2NoOpWithoutBinding();
    testFeatureScopedEdge15Promote();
    testPromotePreservesPolicyAndFallbackCache();
    testPromoteEmptySlotThenC1();
    testUniqueEdgeBindingOnLinkedFeature();
    testZeroMatchesForFeatureNoMint();
    testPocketTipEdge2AndPadEdge13Promote();
    testAmbiguousOnSameFeatureStillRefuses();
    testKeepValidSeedsForSameIndex();
    testLinkSubListFace6Policy();
    testXLinkSubEdge15Policy();
    testIdentifierFace6Policy();
    testUniquePadEdge7PromoteAndMaxEval();
    testReadPolicyRewritesEdgeCacheFromUniqueBinding();
    testResolveSubNameFromSeedFeatureScope();
    testSemanticSubNameMatchesKind();
    testResolveSemanticSelectionUniqueAndRefuse();
    testPatternFeatureScopedEdge15();

    std::cout << "Semantic topology Phase 3: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
