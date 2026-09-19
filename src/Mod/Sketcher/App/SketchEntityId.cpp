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

#include "SketchEntityId.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace Sketcher
{

SketchEntityHandle SketchEntityIdMap::allocate()
{
    const SketchEntityHandle id = nextHandle++;
    issuedSet.insert(id);
    return id;
}

SketchEntityHandle SketchEntityIdMap::insert(int slot)
{
    const SketchEntityHandle id = allocate();
    if (slot < 0) {
        slot = 0;
    }
    if (slot > static_cast<int>(liveIds.size())) {
        slot = static_cast<int>(liveIds.size());
    }
    liveIds.insert(liveIds.begin() + slot, id);
    rebuildIndex();
    return id;
}

SketchEntityHandle SketchEntityIdMap::append()
{
    return insert(static_cast<int>(liveIds.size()));
}

SketchEntityHandle SketchEntityIdMap::eraseSlot(int slot)
{
    if (slot < 0 || slot >= static_cast<int>(liveIds.size())) {
        return Invalid;
    }
    const SketchEntityHandle id = liveIds[static_cast<std::size_t>(slot)];
    liveIds.erase(liveIds.begin() + slot);
    retireHandle(id, slot);
    rebuildIndex();
    return id;
}

bool SketchEntityIdMap::erase(SketchEntityHandle id)
{
    const int slot = slotOf(id);
    if (slot == InvalidSlot) {
        return false;
    }
    eraseSlot(slot);
    return true;
}

int SketchEntityIdMap::slotOf(SketchEntityHandle id) const
{
    const auto it = idToSlot.find(id);
    if (it == idToSlot.end()) {
        return InvalidSlot;
    }
    return it->second;
}

SketchEntityHandle SketchEntityIdMap::idAt(int slot) const
{
    if (slot < 0 || slot >= static_cast<int>(liveIds.size())) {
        return Invalid;
    }
    return liveIds[static_cast<std::size_t>(slot)];
}

bool SketchEntityIdMap::isLive(SketchEntityHandle id) const
{
    return idToSlot.find(id) != idToSlot.end();
}

bool SketchEntityIdMap::isRetired(SketchEntityHandle id) const
{
    return retiredSet.find(id) != retiredSet.end();
}

bool SketchEntityIdMap::isIssued(SketchEntityHandle id) const
{
    return issuedSet.find(id) != issuedSet.end();
}

void SketchEntityIdMap::replaceLive(const std::vector<SketchEntityHandle>& ids)
{
    std::unordered_set<SketchEntityHandle> incoming(ids.begin(), ids.end());

    for (int slot = 0; slot < static_cast<int>(liveIds.size()); ++slot) {
        const SketchEntityHandle id = liveIds[static_cast<std::size_t>(slot)];
        if (incoming.find(id) == incoming.end()) {
            retireHandle(id, slot);
        }
    }

    for (const SketchEntityHandle id : ids) {
        if (id <= Invalid) {
            continue;
        }
        issuedSet.insert(id);
        if (id >= nextHandle) {
            nextHandle = id + 1;
        }
        // Undo of a Deleted entity: the same handle becomes live again.
        // This is restore, not reuse for a new entity.
        retiredSet.erase(id);
    }

    liveIds = ids;
    rebuildIndex();
}

void SketchEntityIdMap::retireAllLive()
{
    for (int slot = 0; slot < static_cast<int>(liveIds.size()); ++slot) {
        retireHandle(liveIds[static_cast<std::size_t>(slot)], slot);
    }
    liveIds.clear();
    idToSlot.clear();
}

void SketchEntityIdMap::noteIssued(SketchEntityHandle id)
{
    if (id <= Invalid) {
        return;
    }
    issuedSet.insert(id);
    if (id >= nextHandle) {
        nextHandle = id + 1;
    }
}

int SketchEntityIdMap::remapSlotAfterInsert(int geoId, int insertAt)
{
    if (geoId < 0) {
        return geoId;
    }
    if (geoId >= insertAt) {
        return geoId + 1;
    }
    return geoId;
}

int SketchEntityIdMap::remapSlotAfterErase(int geoId, int erasedAt)
{
    if (geoId < 0) {
        return geoId;
    }
    if (geoId == erasedAt) {
        return InvalidSlot;
    }
    if (geoId > erasedAt) {
        return geoId - 1;
    }
    return geoId;
}

std::string SketchEntityIdMap::regionKey(std::vector<SketchEntityHandle> wireIds,
                                         std::string_view role)
{
    std::sort(wireIds.begin(), wireIds.end());
    wireIds.erase(std::unique(wireIds.begin(), wireIds.end()), wireIds.end());
    wireIds.erase(std::remove(wireIds.begin(), wireIds.end(), Invalid), wireIds.end());

    std::ostringstream os;
    os << 'W';
    for (std::size_t i = 0; i < wireIds.size(); ++i) {
        if (i != 0) {
            os << ',';
        }
        os << wireIds[i];
    }
    os << ':' << role;
    return os.str();
}

void SketchEntityIdMap::rebuildIndex()
{
    idToSlot.clear();
    idToSlot.reserve(liveIds.size());
    for (int slot = 0; slot < static_cast<int>(liveIds.size()); ++slot) {
        idToSlot[liveIds[static_cast<std::size_t>(slot)]] = slot;
    }
}

void SketchEntityIdMap::retireHandle(SketchEntityHandle id, int lastSlot)
{
    if (id <= Invalid) {
        return;
    }
    if (retiredSet.insert(id).second) {
        retiredLog.push_back(RetiredSketchEntity {id, lastSlot});
    }
}

namespace
{

std::vector<SketchEntityHandle> normalizeWireIds(std::vector<SketchEntityHandle> ids)
{
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    ids.erase(std::remove(ids.begin(), ids.end(), SketchEntityIdMap::Invalid), ids.end());
    return ids;
}

}  // namespace

SketchEntityHandle SketchEntityIdMap::entityIdFromMappedName(std::string_view mapped)
{
    for (std::size_t i = 0; i < mapped.size(); ++i) {
        const char c = mapped[i];
        if (c != 'g' && c != 'e') {
            continue;
        }
        if (i > 0) {
            const unsigned char prev = static_cast<unsigned char>(mapped[i - 1]);
            if (std::isalnum(prev)) {
                continue;
            }
        }
        if (i + 1 >= mapped.size() || !std::isdigit(static_cast<unsigned char>(mapped[i + 1]))) {
            continue;
        }
        std::size_t j = i + 1;
        while (j < mapped.size() && std::isdigit(static_cast<unsigned char>(mapped[j]))) {
            ++j;
        }
        try {
            const long id = std::stol(std::string(mapped.substr(i + 1, j - i - 1)));
            if (id > 0) {
                return id;
            }
        }
        catch (...) {
            return Invalid;
        }
    }
    return Invalid;
}

std::vector<SketchEntityHandle> SketchEntityIdMap::entityIdsFromMappedNames(
    const std::vector<std::string>& mappedNames)
{
    std::vector<SketchEntityHandle> ids;
    ids.reserve(mappedNames.size());
    for (const std::string& name : mappedNames) {
        const SketchEntityHandle id = entityIdFromMappedName(name);
        if (id != Invalid) {
            ids.push_back(id);
        }
    }
    return normalizeWireIds(std::move(ids));
}

bool SketchEntityIdMap::isRegionKey(std::string_view key)
{
    if (key.size() < 3 || key[0] != 'W') {
        return false;
    }
    const auto colon = key.rfind(':');
    if (colon == std::string_view::npos || colon < 1) {
        return false;
    }
    const std::string_view role = key.substr(colon + 1);
    return role == RoleInterior || role == RoleHole || role == RoleCap;
}

std::vector<SketchEntityIdMap::RegionStamp> SketchEntityIdMap::regionStampsForFaces(
    const std::vector<FaceWires>& faces)
{
    std::vector<std::vector<SketchEntityHandle>> innerSets;
    for (const FaceWires& face : faces) {
        for (const auto& inner : face.inners) {
            auto ids = normalizeWireIds(inner);
            if (!ids.empty()) {
                innerSets.push_back(std::move(ids));
            }
        }
    }

    auto outerIsInnerOfAnother = [&](const std::vector<SketchEntityHandle>& outer) {
        const auto n = normalizeWireIds(outer);
        for (const auto& inner : innerSets) {
            if (inner == n) {
                return true;
            }
        }
        return false;
    };

    std::vector<RegionStamp> out;
    std::unordered_set<std::string> seen;
    int index = 0;
    for (const FaceWires& face : faces) {
        ++index;
        const auto outer = normalizeWireIds(face.outer);
        if (outer.empty()) {
            continue;
        }
        const bool hole = outerIsInnerOfAnother(face.outer);
        const char* role = hole ? RoleHole : RoleInterior;
        RegionStamp stamp;
        stamp.faceIndex = index;
        stamp.role = role;
        stamp.key = regionKey(outer, role);
        seen.insert(stamp.key);
        out.push_back(std::move(stamp));
    }

    // Inner wires that FaceMaker did not emit as their own face still publish
    // Hole seeds. Same key as a Hole face is not duplicated.
    for (const FaceWires& face : faces) {
        for (const auto& inner : face.inners) {
            const auto ids = normalizeWireIds(inner);
            if (ids.empty()) {
                continue;
            }
            std::string key = regionKey(ids, RoleHole);
            if (seen.insert(key).second) {
                RegionStamp stamp;
                stamp.faceIndex = 0;
                stamp.role = RoleHole;
                stamp.key = std::move(key);
                out.push_back(std::move(stamp));
            }
        }
    }
    return out;
}

std::vector<std::string> SketchEntityIdMap::regionKeysForFaces(const std::vector<FaceWires>& faces)
{
    std::vector<std::string> keys;
    std::unordered_set<std::string> seen;
    for (const RegionStamp& stamp : regionStampsForFaces(faces)) {
        if (seen.insert(stamp.key).second) {
            keys.push_back(stamp.key);
        }
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

}  // namespace Sketcher
