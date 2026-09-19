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
// Standalone tests: region keys published via SketchSemanticSeeds are stable
// across wire reorder / insert. Same generating wires → same regionKey →
// same SemanticId. Not FaceN / "largest face".
// Vertex: rectangle 8 endpoints → 4 unique corners; Vertex find ≠ Edge;
// curve Binding type "g" stays unique (Vertex uses type "Vertex").
//
//   src/Mod/Sketcher/App/compile_sketch_semantic_seed_test.sh /tmp/sketch-semantic-seed-phase1
//   /tmp/sketch-semantic-seed-phase1

#include "SketchSemanticSeed.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

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

void testRegionSeedsStableAcrossReorderAndInsert()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();
    const auto d = ids.append();

    SketchEntityIdMap::FaceWires rect;
    rect.outer = {d, b, a, c};  // unordered generating edges
    const auto keys = SketchEntityIdMap::regionKeysForFaces({rect});
    CHECK(keys.size() == 1);
    CHECK(keys[0] == SketchEntityIdMap::regionKey({a, b, c, d}, SketchEntityIdMap::RoleInterior));
    CHECK(keys[0].find("Face") == std::string::npos);

    App::SemanticGraph graph;
    const App::ObjectId sketch = 42;
    const App::EvalSerial eval = 1;
    const auto live = ids.live();
    const auto first = SketchSemanticSeeds::seedsForProfile(graph, sketch, eval, live, keys);

    CHECK(first.regions.size() == 1);
    CHECK(first.regions[0].valid());
    CHECK(first.regions[0].kind == App::SemanticKind::Region);
    CHECK(first.regionKeys == keys);

    // Reorder of the same generating wires: same key, same seed (no second heap).
    SketchEntityIdMap::FaceWires shuffled;
    shuffled.outer = {c, a, d, b};
    const auto keys2 = SketchEntityIdMap::regionKeysForFaces({shuffled});
    CHECK(keys2 == keys);
    const auto again = SketchSemanticSeeds::seedsForProfile(graph, sketch, eval, live, keys2);
    CHECK(again.regions.size() == 1);
    CHECK(again.regions[0].handle == first.regions[0].handle);

    // Insert an unrelated entity: Interior key unchanged, seed unchanged.
    ids.insert(0);
    SketchEntityIdMap::FaceWires still;
    still.outer = {a, b, c, d};
    const auto keys3 = SketchEntityIdMap::regionKeysForFaces({still});
    CHECK(keys3 == keys);
    const auto afterInsert = SketchSemanticSeeds::seedsForProfile(graph, sketch, eval, ids.live(), keys3);
    CHECK(afterInsert.regions[0].handle == first.regions[0].handle);
    CHECK(afterInsert.curves.size() == ids.live().size());

    // Adding a hole publishes a new Hole seed; Interior seed is not recycled.
    const auto hole = ids.append();
    SketchEntityIdMap::FaceWires ring;
    ring.outer = {a, b, c, d};
    ring.inners = {{hole}};
    const auto keysHole = SketchEntityIdMap::regionKeysForFaces({ring});
    CHECK(keysHole.size() == 2);
    const auto withHole = SketchSemanticSeeds::seedsForProfile(graph, sketch, eval, ids.live(), keysHole);
    CHECK(withHole.regions.size() == 2);
    bool keptInterior = false;
    bool newHole = false;
    for (const auto& r : withHole.regions) {
        if (r.handle == first.regions[0].handle) {
            keptInterior = true;
        }
        else {
            newHole = true;
            CHECK(r.handle != first.regions[0].handle);
            CHECK(r.kind == App::SemanticKind::Region);
        }
    }
    CHECK(keptInterior);
    CHECK(newHole);

    // Region Bindings must not share index=1 (Interior + Hole on one sketch).
    std::vector<int> regionBindIndex;
    for (const App::SemanticBinding& row : graph.allBindings()) {
        if (row.feature != sketch || row.index.type != "Region") {
            continue;
        }
        CHECK(row.kind == App::SemanticKind::Region);
        CHECK(row.index.index > 0);
        regionBindIndex.push_back(row.index.index);
    }
    CHECK(regionBindIndex.size() == 2);
    CHECK(regionBindIndex[0] != regionBindIndex[1]);

    const auto found = SketchSemanticSeeds::findRegionSeed(graph, sketch, keys[0]);
    CHECK(found.handle == first.regions[0].handle);
    CHECK(SketchSemanticSeeds::regionNote(keys[0]).find("Face") == std::string::npos);
}

void testRegionSeedNotFaceN()
{
    App::SemanticGraph graph;
    const std::string key = SketchEntityIdMap::regionKey({3, 7, 12}, SketchEntityIdMap::RoleInterior);
    const App::SemanticId id = SketchSemanticSeeds::ensureRegionSeed(graph, 1, 1, key);
    CHECK(id.valid());
    CHECK(id.kind == App::SemanticKind::Region);
    CHECK(SketchSemanticSeeds::findRegionSeed(graph, 1, "Face6").valid() == false);
    CHECK(SketchSemanticSeeds::ensureRegionSeed(graph, 1, 1, key).handle == id.handle);
    int regionRows = 0;
    for (const App::SemanticBinding& row : graph.allBindings()) {
        if (row.stid.handle != id.handle) {
            continue;
        }
        ++regionRows;
        CHECK(row.index.type == "Region");
        CHECK(row.index.index > 0);
        CHECK(row.kind == App::SemanticKind::Region);
    }
    CHECK(regionRows == 1);
}

}  // namespace

void testRectangleFourVertexSeedsNotEight()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();
    const auto d = ids.append();

    const std::vector<SketchVertexKey> endpoints = {
        {a, 1}, {a, 2}, {b, 1}, {b, 2}, {c, 1}, {c, 2}, {d, 1}, {d, 2},
    };
    const std::vector<std::pair<SketchVertexKey, SketchVertexKey>> coins = {
        {{a, 2}, {b, 1}},
        {{b, 2}, {c, 1}},
        {{c, 2}, {d, 1}},
        {{d, 2}, {a, 1}},
    };
    const auto unique = SketchSemanticSeeds::uniqueProfileCorners(endpoints, coins);
    CHECK(unique.size() == 4);

    App::SemanticGraph graph;
    const App::ObjectId sketch = 7;
    const App::EvalSerial eval = 1;
    const auto profile = SketchSemanticSeeds::seedsForProfile(
        graph, sketch, eval, {a, b, c, d}, {}, unique);
    CHECK(profile.curves.size() == 4);
    CHECK(profile.vertices.size() == 4);
    CHECK(profile.vertexKeys.size() == 4);
    for (const auto& v : profile.vertices) {
        CHECK(v.valid());
        CHECK(v.kind == App::SemanticKind::Vertex);
    }
    for (const auto& e : profile.curves) {
        CHECK(e.valid());
        CHECK(e.kind == App::SemanticKind::Edge);
    }

    const App::SemanticId edgeA = SketchSemanticSeeds::findSeed(graph, sketch, a);
    CHECK(edgeA.kind == App::SemanticKind::Edge);
    const App::SemanticId vert0 = SketchSemanticSeeds::findSeedForVertex(graph, sketch, unique[0]);
    CHECK(vert0.kind == App::SemanticKind::Vertex);
    CHECK(vert0.handle != edgeA.handle);
    CHECK(SketchSemanticSeeds::findSeedByKindNote(
              graph, sketch, App::SemanticKind::Vertex, SketchSemanticSeeds::entityNote(a))
              .valid()
          == false);

    int gBind = 0;
    int vBind = 0;
    std::vector<int> gIndex;
    std::vector<int> vIndex;
    for (const App::SemanticBinding& row : graph.allBindings()) {
        if (row.feature != sketch) {
            continue;
        }
        if (row.index.type == "g") {
            ++gBind;
            gIndex.push_back(row.index.index);
            CHECK(row.kind == App::SemanticKind::Edge);
        }
        else if (row.index.type == "Vertex") {
            ++vBind;
            vIndex.push_back(row.index.index);
            CHECK(row.kind == App::SemanticKind::Vertex);
            CHECK(row.index.type != "g");
        }
    }
    CHECK(gBind == 4);
    CHECK(vBind == 4);
    std::sort(gIndex.begin(), gIndex.end());
    std::sort(vIndex.begin(), vIndex.end());
    CHECK(std::unique(gIndex.begin(), gIndex.end()) == gIndex.end());
    CHECK(std::unique(vIndex.begin(), vIndex.end()) == vIndex.end());
    // Curve Bindings keep type "g" + curve handle; Vertex never uses "g".
    CHECK(gIndex[0] == static_cast<int>(a));
    CHECK(gIndex[3] == static_cast<int>(d));
    // Re-publish: C1 keeps the same Vertex handles.
    const auto again = SketchSemanticSeeds::seedsForProfile(
        graph, sketch, eval, {a, b, c, d}, {}, unique);
    CHECK(again.vertices.size() == 4);
    for (std::size_t i = 0; i < 4; ++i) {
        CHECK(again.vertices[i].handle == profile.vertices[i].handle);
    }
}

void testVertexFindDoesNotReturnEdge()
{
    App::SemanticGraph graph;
    const App::ObjectId sketch = 3;
    const App::EvalSerial eval = 1;
    const SketchEntityHandle h = 1;
    const App::SemanticId edge =
        SketchSemanticSeeds::ensureSeedForEntity(graph, sketch, eval, h, App::SemanticKind::Edge);
    const SketchVertexKey key{h, 1};
    const App::SemanticId vert = SketchSemanticSeeds::ensureSeedForVertex(graph, sketch, eval, key);
    CHECK(edge.valid());
    CHECK(vert.valid());
    CHECK(edge.kind == App::SemanticKind::Edge);
    CHECK(vert.kind == App::SemanticKind::Vertex);
    CHECK(edge.handle != vert.handle);
    CHECK(SketchSemanticSeeds::findSeed(graph, sketch, h).handle == edge.handle);
    CHECK(SketchSemanticSeeds::findSeed(graph, sketch, h).kind == App::SemanticKind::Edge);
    CHECK(SketchSemanticSeeds::findSeedForVertex(graph, sketch, key).handle == vert.handle);
    CHECK(SketchSemanticSeeds::ensureSeedForEntity(graph, sketch, eval, h, App::SemanticKind::Vertex)
              .valid()
          == false);
    CHECK(SketchSemanticSeeds::ensureSeedForEntity(graph, sketch, eval, h, App::SemanticKind::Face)
              .valid()
          == false);
    CHECK(SketchSemanticSeeds::vertexNote(key).find("Sketch.v") == 0);
    CHECK(SketchSemanticSeeds::entityNote(h).find("Sketch.g") == 0);
    CHECK(SketchSemanticSeeds::vertexNote(key) != SketchSemanticSeeds::entityNote(h));

    bool vertexUsedG = false;
    bool edgeUsedVertex = false;
    for (const App::SemanticBinding& row : graph.allBindings()) {
        if (row.stid.handle == vert.handle && row.index.type == "g") {
            vertexUsedG = true;
        }
        if (row.stid.handle == edge.handle && row.index.type == "Vertex") {
            edgeUsedVertex = true;
        }
    }
    CHECK(!vertexUsedG);
    CHECK(!edgeUsedVertex);
}

void testEntitySeedEmitIntegrity()
{
    const App::ObjectId sketch = 31;

    App::SemanticGraph valid;
    valid.beginEvaluate(1);
    const App::SemanticId edge = valid.recordGenerated(
        App::SemanticKind::Edge, SketchSemanticSeeds::entityNote(1), sketch, 1, App::SemanticRole::User);
    const auto children = valid.recordSplit(
        edge, 2, "Sketch.split:1->2,3", sketch, 1, App::SemanticRole::User);
    valid.commitEvaluate();
    CHECK(SketchSemanticSeeds::findSeed(valid, sketch, 1).handle == edge.handle);
    CHECK(SketchSemanticSeeds::findSeed(valid, sketch, 2).handle == children[0].handle);
    CHECK(SketchSemanticSeeds::findSeed(valid, sketch, 3).handle == children[1].handle);

    App::SemanticGraph wrongKind;
    wrongKind.beginEvaluate(1);
    wrongKind.recordGenerated(
        App::SemanticKind::Edge, SketchSemanticSeeds::entityNote(4), sketch, 1, App::SemanticRole::User);
    wrongKind.recordGenerated(
        App::SemanticKind::Face, SketchSemanticSeeds::entityNote(4), sketch, 1, App::SemanticRole::User);
    wrongKind.commitEvaluate();
    CHECK(!SketchSemanticSeeds::findSeed(wrongKind, sketch, 4).valid());

    App::SemanticGraph duplicate;
    duplicate.beginEvaluate(1);
    duplicate.recordGenerated(
        App::SemanticKind::Edge, SketchSemanticSeeds::entityNote(5), sketch, 1, App::SemanticRole::User);
    duplicate.recordGenerated(
        App::SemanticKind::Edge, SketchSemanticSeeds::entityNote(5), sketch, 1, App::SemanticRole::User);
    duplicate.commitEvaluate();
    CHECK(!SketchSemanticSeeds::findSeed(duplicate, sketch, 5).valid());
    const auto duplicateEvents = duplicate.events().size();
    CHECK(!SketchSemanticSeeds::ensureSeedForEntity(duplicate, sketch, 2, 5).valid());
    CHECK(duplicate.events().size() == duplicateEvents);

    App::SemanticGraph wrongKindEnsure;
    wrongKindEnsure.beginEvaluate(1);
    wrongKindEnsure.recordGenerated(
        App::SemanticKind::Face, SketchSemanticSeeds::entityNote(7), sketch, 1, App::SemanticRole::User);
    wrongKindEnsure.commitEvaluate();
    const auto wrongKindEvents = wrongKindEnsure.events().size();
    CHECK(!SketchSemanticSeeds::ensureSeedForEntity(wrongKindEnsure, sketch, 2, 7).valid());
    CHECK(wrongKindEnsure.events().size() == wrongKindEvents);

    App::SemanticGraph malformed;
    malformed.beginEvaluate(1);
    const App::SemanticId malformedParent = malformed.recordGenerated(
        App::SemanticKind::Edge, SketchSemanticSeeds::entityNote(6), sketch, 1, App::SemanticRole::User);
    malformed.recordSplit(
        malformedParent, 1, "Sketch.split:6->7,", sketch, 1, App::SemanticRole::User);
    malformed.commitEvaluate();
    CHECK(!SketchSemanticSeeds::findSeed(malformed, sketch, 7).valid());
    App::SemanticGraph duplicateRegion;
    duplicateRegion.beginEvaluate(1);
    const std::string regionNote = SketchSemanticSeeds::regionNote("Interior:1,2,3");
    duplicateRegion.recordGenerated(
        App::SemanticKind::Region, regionNote, sketch, 1, App::SemanticRole::User);
    duplicateRegion.recordGenerated(
        App::SemanticKind::Region, regionNote, sketch, 1, App::SemanticRole::User);
    duplicateRegion.commitEvaluate();
    CHECK(!SketchSemanticSeeds::findRegionSeed(duplicateRegion, sketch, "Interior:1,2,3").valid());
    const auto duplicateRegionEvents = duplicateRegion.events().size();
    CHECK(!SketchSemanticSeeds::ensureRegionSeed(duplicateRegion, sketch, 2, "Interior:1,2,3").valid());
    CHECK(duplicateRegion.events().size() == duplicateRegionEvents);
}

void testPointDedupWithoutCoincidence()
{
    const std::vector<SketchVertexKey> endpoints = {
        {1, 1}, {1, 2}, {2, 1}, {2, 2}, {3, 1}, {3, 2}, {4, 1}, {4, 2},
    };
    const std::vector<SketchVertexPoint> pts = {
        {0, 1, 0}, {1, 1, 0}, {1, 1, 0}, {1, 0, 0},
        {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0},
    };
    const auto unique =
        SketchSemanticSeeds::uniqueProfileCorners(endpoints, {}, pts, 1e-7);
    CHECK(unique.size() == 4);
}

int main()
{
    testRegionSeedsStableAcrossReorderAndInsert();
    testRegionSeedNotFaceN();
    testRectangleFourVertexSeedsNotEight();
    testVertexFindDoesNotReturnEdge();
    testEntitySeedEmitIntegrity();
    testPointDedupWithoutCoincidence();

    std::cout << "Sketch semantic seed region+vertex: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
