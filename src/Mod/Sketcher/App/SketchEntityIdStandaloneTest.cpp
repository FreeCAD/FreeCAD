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
// Standalone Phase 1 tests for sketch entity IDs. Metric is handle
// stability across insert/erase, never-recycle (I5), and region keys
// from generating wire IDs + role — not FaceN / "largest face".
//
//   src/Mod/Sketcher/App/compile_sketch_entity_id_test.sh /tmp/sketch-entity-id-phase1
//   /tmp/sketch-entity-id-phase1

#include "SketchEntityId.h"

#include <algorithm>
#include <cstdlib>
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

void testAllocateNeverZeroAndMonotone()
{
    SketchEntityIdMap ids;
    const auto a = ids.allocate();
    const auto b = ids.allocate();
    CHECK(a != SketchEntityIdMap::Invalid);
    CHECK(b != SketchEntityIdMap::Invalid);
    CHECK(a != b);
    CHECK(b == a + 1);
    CHECK(ids.highWater() == b + 1);
    CHECK(ids.isIssued(a));
    CHECK(!ids.isLive(a));
    CHECK(!ids.isRetired(a));
}

void testAppendInsertDoesNotRenumber()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();
    CHECK(ids.liveCount() == 3);
    CHECK(ids.idAt(0) == a);
    CHECK(ids.idAt(1) == b);
    CHECK(ids.idAt(2) == c);

    const auto x = ids.insert(0);
    CHECK(x != a && x != b && x != c);
    CHECK(ids.idAt(0) == x);
    CHECK(ids.idAt(1) == a);
    CHECK(ids.idAt(2) == b);
    CHECK(ids.idAt(3) == c);
    CHECK(ids.slotOf(a) == 1);
    CHECK(ids.slotOf(b) == 2);
    CHECK(ids.slotOf(c) == 3);

    const auto y = ids.insert(ids.slotOf(b));
    CHECK(ids.slotOf(a) == 1);
    CHECK(ids.slotOf(y) == 2);
    CHECK(ids.slotOf(b) == 3);
    CHECK(ids.slotOf(c) == 4);
    CHECK(ids.idAt(ids.slotOf(b)) == b);
}

void testEraseRetiresAndDoesNotReuse()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();

    CHECK(ids.erase(b));
    CHECK(ids.isRetired(b));
    CHECK(!ids.isLive(b));
    CHECK(ids.slotOf(b) == SketchEntityIdMap::InvalidSlot);
    CHECK(ids.liveCount() == 2);
    CHECK(ids.idAt(0) == a);
    CHECK(ids.idAt(1) == c);
    CHECK(ids.slotOf(a) == 0);
    CHECK(ids.slotOf(c) == 1);

    const auto d = ids.append();
    CHECK(d != b);
    CHECK(d != a && d != c);
    CHECK(!ids.isLive(b));
    CHECK(ids.isRetired(b));
    CHECK(ids.retired().size() == 1);
    CHECK(ids.retired().front().id == b);
    CHECK(d > b);
}

void testConstraintStoredAsEntitySurvivesInsert()
{
    SketchEntityIdMap ids;
    ids.append();
    const auto b = ids.append();
    ids.append();

    SketchConstraintRef ref;
    ref.entity = b;
    ref.slot = ids.slotOf(b);
    CHECK(ref.slot == 1);

    ids.insert(0);

    CHECK(ref.entity == b);
    CHECK(ids.slotOf(ref.entity) == 2);
    CHECK(ids.idAt(ids.slotOf(ref.entity)) == b);
    CHECK(ids.idAt(0) != b);
}

void testIndexConstraintNeedsRemapUntilMigration()
{
    SketchEntityIdMap ids;
    ids.append();
    const auto b = ids.append();
    ids.append();

    int geoId = ids.slotOf(b);
    CHECK(geoId == 1);

    const int insertAt = 0;
    ids.insert(insertAt);

    CHECK(ids.idAt(geoId) != b);

    geoId = SketchEntityIdMap::remapSlotAfterInsert(geoId, insertAt);
    CHECK(geoId == 2);
    CHECK(ids.idAt(geoId) == b);
}

void testEraseRemapDropsDeletedAndShiftsLater()
{
    CHECK(SketchEntityIdMap::remapSlotAfterErase(0, 1) == 0);
    CHECK(SketchEntityIdMap::remapSlotAfterErase(1, 1) == SketchEntityIdMap::InvalidSlot);
    CHECK(SketchEntityIdMap::remapSlotAfterErase(2, 1) == 1);
    CHECK(SketchEntityIdMap::remapSlotAfterErase(-1, 1) == -1);
    CHECK(SketchEntityIdMap::remapSlotAfterErase(-3, 1) == -3);
    CHECK(SketchEntityIdMap::remapSlotAfterInsert(-1, 0) == -1);
    CHECK(SketchEntityIdMap::remapSlotAfterInsert(0, 0) == 1);
    CHECK(SketchEntityIdMap::remapSlotAfterInsert(2, 0) == 3);
}

void testHighWaterNeverRewinds()
{
    SketchEntityIdMap ids;
    ids.append();
    ids.append();
    const auto highAfterTwo = ids.highWater();
    ids.eraseSlot(0);
    ids.eraseSlot(0);
    CHECK(ids.liveCount() == 0);
    CHECK(ids.highWater() == highAfterTwo);

    const auto next = ids.allocate();
    CHECK(next == highAfterTwo);
    CHECK(ids.highWater() == highAfterTwo + 1);

    const auto burned = ids.allocate();
    const auto afterBurn = ids.append();
    CHECK(afterBurn != burned);
    CHECK(afterBurn == burned + 1);
}

void testUndoRebindsSameHandle()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();
    const auto high = ids.highWater();

    ids.erase(b);
    CHECK(ids.isRetired(b));

    ids.replaceLive({a, b, c});
    CHECK(ids.isLive(b));
    CHECK(!ids.isRetired(b));
    CHECK(ids.slotOf(b) == 1);
    CHECK(ids.highWater() == high);

    const auto d = ids.append();
    CHECK(d != b);
    CHECK(d == high);
}

void testReplaceLiveRetiresVanished()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();

    ids.replaceLive({a, c});
    CHECK(ids.isRetired(b));
    CHECK(!ids.isLive(b));
    CHECK(ids.liveCount() == 2);
    CHECK(ids.slotOf(c) == 1);

    ids.retireAllLive();
    CHECK(ids.liveCount() == 0);
    CHECK(ids.isRetired(a));
    CHECK(ids.isRetired(c));
    const auto next = ids.allocate();
    CHECK(next != a && next != b && next != c);
}

void testRestoreAdoptsPersistedIds()
{
    SketchEntityIdMap ids;
    ids.replaceLive({2, 5, 9});
    CHECK(ids.idAt(0) == 2);
    CHECK(ids.idAt(1) == 5);
    CHECK(ids.idAt(2) == 9);
    CHECK(ids.highWater() == 10);
    const auto next = ids.allocate();
    CHECK(next == 10);
    CHECK(next != 3 && next != 4);
}

void testRegionKeyFromWireIdsAndRole()
{
    const auto k1 = SketchEntityIdMap::regionKey({7, 3, 12}, SketchEntityIdMap::RoleInterior);
    const auto k2 = SketchEntityIdMap::regionKey({12, 3, 7}, SketchEntityIdMap::RoleInterior);
    const auto kHole = SketchEntityIdMap::regionKey({7, 3, 12}, SketchEntityIdMap::RoleHole);
    const auto kDup = SketchEntityIdMap::regionKey({3, 3, 7, 12}, SketchEntityIdMap::RoleInterior);

    CHECK(k1 == "W3,7,12:Interior");
    CHECK(k1 == k2);
    CHECK(k1 == kDup);
    CHECK(kHole == "W3,7,12:Hole");
    CHECK(k1 != kHole);
    CHECK(k1.find("Face") == std::string::npos);
    CHECK(k1.find("largest") == std::string::npos);

    const auto empty = SketchEntityIdMap::regionKey({}, SketchEntityIdMap::RoleCap);
    CHECK(empty == "W:Cap");

    const auto one = SketchEntityIdMap::regionKey({4}, SketchEntityIdMap::RoleInterior);
    CHECK(one == "W4:Interior");
}

void testInsertThenEraseStability()
{
    SketchEntityIdMap ids;
    std::vector<SketchEntityHandle> originals;
    for (int i = 0; i < 5; ++i) {
        originals.push_back(ids.append());
    }

    ids.insert(2);
    for (int i = 0; i < 5; ++i) {
        CHECK(ids.isLive(originals[static_cast<std::size_t>(i)]));
    }

    ids.eraseSlot(2);
    for (int i = 0; i < 5; ++i) {
        CHECK(ids.slotOf(originals[static_cast<std::size_t>(i)]) == i);
        CHECK(ids.idAt(i) == originals[static_cast<std::size_t>(i)]);
    }
}

void testAxesNeverIssued()
{
    SketchEntityIdMap ids;
    const auto a = ids.allocate();
    CHECK(a > 0);
    CHECK(a != -1);
    CHECK(a != -2);
}

void testEntityIdFromMappedName()
{
    CHECK(SketchEntityIdMap::entityIdFromMappedName("g12") == 12);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("e3") == 3);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("g7;SKT") == 7);
    CHECK(SketchEntityIdMap::entityIdFromMappedName(";g4.eEdge1") == 4);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("g2v1") == 2);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("Face6") == SketchEntityIdMap::Invalid);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("InternalFace2") == SketchEntityIdMap::Invalid);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("largest") == SketchEntityIdMap::Invalid);
    CHECK(SketchEntityIdMap::entityIdFromMappedName("") == SketchEntityIdMap::Invalid);

    const auto ids = SketchEntityIdMap::entityIdsFromMappedNames({"g12;SKT", "g3", "g12", "Face1", "e5"});
    CHECK(ids.size() == 3);
    CHECK(ids[0] == 3);
    CHECK(ids[1] == 5);
    CHECK(ids[2] == 12);
}

void testRegionKeysStableAcrossReorderAndInsert()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();
    const auto d = ids.append();

    SketchEntityIdMap::FaceWires rect;
    rect.outer = {a, b, c, d};
    SketchEntityIdMap::FaceWires reordered;
    reordered.outer = {d, a, c, b};

    const auto k1 = SketchEntityIdMap::regionKeysForFaces({rect});
    const auto k2 = SketchEntityIdMap::regionKeysForFaces({reordered});
    CHECK(k1.size() == 1);
    CHECK(k1 == k2);
    CHECK(k1[0] == SketchEntityIdMap::regionKey({a, b, c, d}, SketchEntityIdMap::RoleInterior));
    CHECK(k1[0].find("Face") == std::string::npos);
    CHECK(k1[0].find("largest") == std::string::npos);
    CHECK(SketchEntityIdMap::isRegionKey(k1[0]));

    // Insert an unrelated entity at the front: slots shift, generating wires do not.
    const auto extra = ids.insert(0);
    CHECK(extra != a && extra != b && extra != c && extra != d);
    CHECK(ids.slotOf(a) == 1);
    SketchEntityIdMap::FaceWires afterInsert;
    afterInsert.outer = {a, b, c, d};
    CHECK(SketchEntityIdMap::regionKeysForFaces({afterInsert}) == k1);

    // Reorder Geometry[] (wire edge order) still same key.
    SketchEntityIdMap::FaceWires shuffled;
    shuffled.outer = {c, a, d, b};
    CHECK(SketchEntityIdMap::regionKeysForFaces({shuffled}) == k1);
}

void testRegionKeysHoleInsertDoesNotRenameInterior()
{
    SketchEntityIdMap ids;
    const auto a = ids.append();
    const auto b = ids.append();
    const auto c = ids.append();
    const auto d = ids.append();
    const auto hole = ids.append();

    SketchEntityIdMap::FaceWires rect;
    rect.outer = {a, b, c, d};
    const auto before = SketchEntityIdMap::regionKeysForFaces({rect});
    CHECK(before.size() == 1);

    SketchEntityIdMap::FaceWires ring;
    ring.outer = {a, b, c, d};
    ring.inners = {{hole}};
    const auto after = SketchEntityIdMap::regionKeysForFaces({ring});
    CHECK(std::find(after.begin(), after.end(), before[0]) != after.end());
    const auto holeKey = SketchEntityIdMap::regionKey({hole}, SketchEntityIdMap::RoleHole);
    CHECK(std::find(after.begin(), after.end(), holeKey) != after.end());
    CHECK(after.size() == 2);
    CHECK(before[0] != holeKey);
}

void testRegionStampsIndependentOfFaceNOrder()
{
    SketchEntityIdMap::FaceWires ring;
    ring.outer = {1, 2, 3, 4};
    ring.inners = {{10}};
    SketchEntityIdMap::FaceWires disk;
    disk.outer = {10};

    const auto orderRingFirst = SketchEntityIdMap::regionStampsForFaces({ring, disk});
    const auto orderDiskFirst = SketchEntityIdMap::regionStampsForFaces({disk, ring});

    auto keysOf = [](const std::vector<SketchEntityIdMap::RegionStamp>& stamps) {
        std::vector<std::string> keys;
        for (const auto& s : stamps) {
            keys.push_back(s.key);
        }
        std::sort(keys.begin(), keys.end());
        keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
        return keys;
    };

    CHECK(keysOf(orderRingFirst) == keysOf(orderDiskFirst));
    CHECK(keysOf(orderRingFirst) == SketchEntityIdMap::regionKeysForFaces({ring, disk}));
    CHECK(keysOf(orderRingFirst) == SketchEntityIdMap::regionKeysForFaces({disk, ring}));

    // FaceN (Binding) follows OCCT order; identity does not.
    bool foundRingInterior = false;
    bool foundDiskHole = false;
    for (const auto& s : orderRingFirst) {
        if (s.faceIndex == 1) {
            CHECK(std::string(s.role) == SketchEntityIdMap::RoleInterior);
            CHECK(s.key == "W1,2,3,4:Interior");
            foundRingInterior = true;
        }
        if (s.faceIndex == 2) {
            CHECK(std::string(s.role) == SketchEntityIdMap::RoleHole);
            CHECK(s.key == "W10:Hole");
            foundDiskHole = true;
        }
        CHECK(s.key.find("Face") == std::string::npos);
    }
    CHECK(foundRingInterior);
    CHECK(foundDiskHole);

    foundRingInterior = false;
    foundDiskHole = false;
    for (const auto& s : orderDiskFirst) {
        if (s.faceIndex == 1) {
            CHECK(std::string(s.role) == SketchEntityIdMap::RoleHole);
            CHECK(s.key == "W10:Hole");
            foundDiskHole = true;
        }
        if (s.faceIndex == 2) {
            CHECK(std::string(s.role) == SketchEntityIdMap::RoleInterior);
            CHECK(s.key == "W1,2,3,4:Interior");
            foundRingInterior = true;
        }
    }
    CHECK(foundRingInterior);
    CHECK(foundDiskHole);

    // Two side-by-side interiors: neither is a hole.
    SketchEntityIdMap::FaceWires left;
    left.outer = {1, 2, 3, 4};
    SketchEntityIdMap::FaceWires right;
    right.outer = {5, 6, 7, 8};
    const auto two = SketchEntityIdMap::regionStampsForFaces({right, left});
    CHECK(two.size() == 2);
    CHECK(two[0].faceIndex == 1);
    CHECK(two[0].key == "W5,6,7,8:Interior");
    CHECK(two[1].faceIndex == 2);
    CHECK(two[1].key == "W1,2,3,4:Interior");
}

void testIsRegionKeyRejectsFaceN()
{
    CHECK(SketchEntityIdMap::isRegionKey("W1,2:Interior"));
    CHECK(SketchEntityIdMap::isRegionKey("W10:Hole"));
    CHECK(SketchEntityIdMap::isRegionKey("W:Cap"));
    CHECK(!SketchEntityIdMap::isRegionKey("Face1"));
    CHECK(!SketchEntityIdMap::isRegionKey("InternalFace2"));
    CHECK(!SketchEntityIdMap::isRegionKey("largest"));
    CHECK(!SketchEntityIdMap::isRegionKey("W1,2"));
}

}  // namespace

int main()
{
    testAllocateNeverZeroAndMonotone();
    testAppendInsertDoesNotRenumber();
    testEraseRetiresAndDoesNotReuse();
    testConstraintStoredAsEntitySurvivesInsert();
    testIndexConstraintNeedsRemapUntilMigration();
    testEraseRemapDropsDeletedAndShiftsLater();
    testHighWaterNeverRewinds();
    testUndoRebindsSameHandle();
    testReplaceLiveRetiresVanished();
    testRestoreAdoptsPersistedIds();
    testRegionKeyFromWireIdsAndRole();
    testInsertThenEraseStability();
    testAxesNeverIssued();
    testEntityIdFromMappedName();
    testRegionKeysStableAcrossReorderAndInsert();
    testRegionKeysHoleInsertDoesNotRenameInterior();
    testRegionStampsIndependentOfFaceNOrder();
    testIsRegionKeyRejectsFaceN();

    std::cout << "Sketch entity ID Phase 1: " << g_passed << " checks passed, " << g_failed
              << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
