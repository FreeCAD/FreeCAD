// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *   Copyright (c) 2026 Sauli Kiviranta                                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                         *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                          *
 ***************************************************************************/

#pragma once

/// Rev 3.1 Phase 1: sketch-scoped entity IDs that are not array indices.
/// Insertion does not change existing IDs. Deletion retires an ID; it is
/// never reused for a new entity in the same sketch lifetime (I5). This
/// map sits next to the geometry list and issues SketchGeometryExtension::Id
/// — it is not a second competing identity heap.
///
/// GeoId remains the solver/array slot until Constraint stores EntityId.
/// Use Handle for durable identity; use slot only to index Geometry[].

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef SKETCH_ENTITY_ID_STANDALONE
# ifndef SketcherExport
#  define SketcherExport
# endif
#else
# include <Mod/Sketcher/SketcherGlobal.h>
#endif

namespace Sketcher
{

/// Durable sketch entity handle. 0 is invalid. Axes stay GeoEnum HAxis/VAxis
/// (-1/-2) and are never issued here.
using SketchEntityHandle = long;

/// Tombstone for a deleted sketch entity. Equivalent of a Deleted event
/// until App::SemanticGraph is attached to the document (Phase 2 follow-up).
struct SketcherExport RetiredSketchEntity
{
    SketchEntityHandle id = 0;
    int lastSlot = -1;  ///< Geometry[] index at the moment of deletion
};

/// Sketch-scoped never-recycle allocator + live slot list.
///
/// Live handles are stored in slot order, parallel to Geometry[]. Inserting
/// at a slot shifts later *slots* but not later *handles*. Erasing a slot
/// retires that handle. High-water never decreases (undo rebinds, it does
/// not rewind).
class SketcherExport SketchEntityIdMap
{
public:
    static constexpr SketchEntityHandle Invalid = 0;
    /// Same numeric value as GeoEnum::GeoUndef; kept numeric so this header
    /// does not pull GeoEnum.h into the standalone test.
    static constexpr int InvalidSlot = -2000;

    SketchEntityIdMap() = default;

    /// Issue the next handle. Not bound to a slot. Never recycled, including
    /// if the caller discards the geometry (I5 burn-on-failure).
    SketchEntityHandle allocate();

    /// Allocate and bind at `slot`, shifting later live slots up.
    /// `slot` is clamped to [0, liveCount()].
    SketchEntityHandle insert(int slot);

    /// Allocate and bind at the end of the live list.
    SketchEntityHandle append();

    /// Retire the handle at `slot` and shift later slots down.
    /// Returns the retired handle, or Invalid if the slot is out of range.
    SketchEntityHandle eraseSlot(int slot);

    /// Retire `id` if live. Returns false if it was not live.
    bool erase(SketchEntityHandle id);

    /// Current Geometry[] index of `id`, or InvalidSlot if not live.
    int slotOf(SketchEntityHandle id) const;

    /// Handle currently bound at `slot`, or Invalid if out of range.
    SketchEntityHandle idAt(int slot) const;

    bool isLive(SketchEntityHandle id) const;
    bool isRetired(SketchEntityHandle id) const;
    /// True if the handle has ever been issued (live, retired, or burned).
    bool isIssued(SketchEntityHandle id) const;

    int liveCount() const
    {
        return static_cast<int>(liveIds.size());
    }

    /// Next handle that allocate() will issue. Never decreases.
    SketchEntityHandle highWater() const
    {
        return nextHandle;
    }

    const std::vector<SketchEntityHandle>& live() const
    {
        return liveIds;
    }

    const std::vector<RetiredSketchEntity>& retired() const
    {
        return retiredLog;
    }

    /// Rebuild the live list from Geometry[] IDs (restore, undo, onChanged).
    /// Handles that were live and are absent are retired. Handles that
    /// reappear after retirement are un-retired (undo of the same entity),
    /// not allocated as new identities. High-water is never rewound.
    void replaceLive(const std::vector<SketchEntityHandle>& ids);

    /// Drop every live binding and retire them. High-water is kept.
    void retireAllLive();

    /// Record an already-persisted handle (external geometry, restore) so the
    /// high-water stays above it. Does not bind a slot.
    void noteIssued(SketchEntityHandle id);

    /// Index-based constraint compatibility until Constraint stores EntityId.
    /// Negative GeoIds (axes, external, GeoUndef) are left unchanged.
    static int remapSlotAfterInsert(int geoId, int insertAt);
    /// Returns InvalidSlot if `geoId` was the erased slot (constraint must drop).
    static int remapSlotAfterErase(int geoId, int erasedAt);

    /// Internal-face name from generating wire entity IDs plus a region role.
    /// Not FaceN, not "largest face of this evaluate." Wire IDs are sorted
    /// unique so the key is independent of list order.
    /// Format: `W<id1>,<id2>,...:<Role>`  e.g. `W3,7,12:Interior`.
    static std::string regionKey(std::vector<SketchEntityHandle> wireIds, std::string_view role);

    static constexpr const char* RoleInterior = "Interior";
    static constexpr const char* RoleHole = "Hole";
    static constexpr const char* RoleCap = "Cap";

    /// Parse `g{id}` / `e{id}` from a mapped element name (`g12`, `g12;SKT`,
    /// `;g4.eEdge1`). Returns Invalid for FaceN / unparseable / "largest".
    static SketchEntityHandle entityIdFromMappedName(std::string_view mapped);

    /// Unique generating entity IDs parsed from edge mapped names.
    static std::vector<SketchEntityHandle> entityIdsFromMappedNames(
        const std::vector<std::string>& mappedNames
    );

    /// True if `key` is `W...:Interior|Hole|Cap`, not FaceN.
    static bool isRegionKey(std::string_view key);

    /// One FaceMaker face described by generating-wire entity IDs.
    /// Outer wire → Interior unless that outer is some other face's inner
    /// (then Hole). Inner wires that are not themselves FaceMaker faces still
    /// produce Hole region seeds.
    struct FaceWires
    {
        std::vector<SketchEntityHandle> outer;
        std::vector<std::vector<SketchEntityHandle>> inners;
    };

    /// ElementMap stamp + seed key for one region. `faceIndex` is the
    /// 1-based IndexedName FaceN (Binding only). 0 = seed-only (no FaceMaker
    /// face for this hole wire).
    struct RegionStamp
    {
        int faceIndex = 0;
        std::string key;
        const char* role = RoleInterior;
    };

    /// Stamp plan: FaceN is Binding/OCCT order; keys come from wire IDs +
    /// role and do not change if the face list is reordered.
    static std::vector<RegionStamp> regionStampsForFaces(const std::vector<FaceWires>& faces);

    /// Unique region keys, sorted. For SketchSemanticSeeds::seedsForProfile.
    static std::vector<std::string> regionKeysForFaces(const std::vector<FaceWires>& faces);

private:
    void rebuildIndex();
    void retireHandle(SketchEntityHandle id, int lastSlot);

    SketchEntityHandle nextHandle = 1;  ///< never decreases
    std::vector<SketchEntityHandle> liveIds;
    std::unordered_map<SketchEntityHandle, int> idToSlot;
    std::unordered_set<SketchEntityHandle> retiredSet;
    std::unordered_set<SketchEntityHandle> issuedSet;
    std::vector<RetiredSketchEntity> retiredLog;
};

/// A constraint reference that survives insert because it stores the handle,
/// not the slot. Next pass: store `entity` on Constraint instead of GeoId.
struct SketcherExport SketchConstraintRef
{
    SketchEntityHandle entity = SketchEntityIdMap::Invalid;
    int slot = SketchEntityIdMap::InvalidSlot;  ///< GeoId-as-index compatibility
};

}  // namespace Sketcher
