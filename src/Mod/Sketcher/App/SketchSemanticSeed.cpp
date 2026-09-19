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

#include "SketchSemanticSeed.h"

#include <algorithm>
#include <cstdint>
#include <charconv>
#include <cmath>
#include <limits>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

namespace Sketcher
{
namespace
{

void bindSketch(App::SemanticGraph& graph,
                const App::SemanticId& id,
                App::ObjectId sketch,
                App::EvalSerial eval,
                SketchEntityHandle handle)
{
    if (!id.valid()) {
        return;
    }
    App::SemanticBinding row;
    row.stid = id;
    row.feature = sketch;
    row.eval = eval;
    row.kind = id.kind;
    row.index.type = "g";
    row.index.index = static_cast<int>(handle);
    row.occ = nullptr;
    graph.bind(row);
}

/// Binding type "Vertex" (never "g") so I13 uniqueness does not collide
/// with curve rows (type "g", index=curveHandle). Pack handle+pos so
/// start/end of one curve do not share an index.
int vertexBindingIndex(const SketchVertexKey& key)
{
    constexpr long kMax = static_cast<long>(std::numeric_limits<int>::max() / 4) - 1;
    long handle = key.handle;
    if (handle < 0) {
        handle = 0;
    }
    if (handle > kMax) {
        handle %= (kMax + 1);
    }
    const long packed = handle * 4 + static_cast<long>(key.pos);
    return static_cast<int>(packed);
}

void bindVertex(App::SemanticGraph& graph,
                const App::SemanticId& id,
                App::ObjectId sketch,
                App::EvalSerial eval,
                const SketchVertexKey& key)
{
    if (!id.valid() || !key.valid()) {
        return;
    }
    App::SemanticBinding row;
    row.stid = id;
    row.feature = sketch;
    row.eval = eval;
    row.kind = App::SemanticKind::Vertex;
    row.index.type = "Vertex";
    row.index.index = vertexBindingIndex(key);
    row.occ = nullptr;
    graph.bind(row);
}

/// Stable positive Binding index for a regionKey. Identity remains the
/// Generated note (`Sketch.` + key); Binding is live-only. Always-1 would
/// collide when one sketch publishes Interior + Hole (I13 slot uniqueness
/// for type "Region"), unlike Vertex which packs handle+pos.
int regionBindingIndex(std::string_view regionKey)
{
    // FNV-1a 32-bit → (1 .. INT_MAX). Empty key is not used by ensureRegionSeed.
    std::uint32_t h = 2166136261u;
    for (unsigned char c : regionKey) {
        h ^= c;
        h *= 16777619u;
    }
    const int idx = static_cast<int>(h & 0x7fffffffu);
    return idx == 0 ? 1 : idx;
}

bool isSketchEntitySeed(const App::SemanticId& id)
{
    return id.valid() && id.kind == App::SemanticKind::Edge;
}

bool parseEntityHandle(std::string_view text, SketchEntityHandle& handle)
{
    if (text.empty()) {
        return false;
    }
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    SketchEntityHandle parsed = SketchEntityIdMap::Invalid;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed, 10);
    if (result.ec != std::errc() || result.ptr != text.data() + text.size()
        || parsed == SketchEntityIdMap::Invalid) {
        return false;
    }
    handle = parsed;
    return true;
}

std::vector<App::SemanticId> outputsOf(const App::SemanticGraph& graph, App::EventId ev)
{
    std::vector<App::SemanticId> out;
    for (const App::EventOutput& o : graph.outputs()) {
        if (o.event == ev) {
            out.push_back(o.semantic);
        }
    }
    return out;
}

bool hasGeneratedNote(const App::SemanticGraph& graph,
                      App::ObjectId sketch,
                      std::string_view note)
{
    for (const App::Event& ev : graph.events()) {
        if (ev.feature == sketch && ev.kind == App::EventKind::Generated && ev.op == note) {
            return true;
        }
    }
    return false;
}

App::SemanticId uniqueGeneratedSeedByNote(const App::SemanticGraph& graph,
                                          App::ObjectId sketch,
                                          App::SemanticKind kind,
                                          std::string_view note)
{
    App::SemanticId found;
    for (const App::Event& ev : graph.events()) {
        if (ev.feature != sketch || ev.kind != App::EventKind::Generated || ev.op != note) {
            continue;
        }
        const auto outs = outputsOf(graph, ev.id);
        if (outs.size() != 1 || !outs.front().valid() || outs.front().kind != kind) {
            return {};
        }
        const App::SemanticId id = outs.front();
        if (found.valid() && found.handle != id.handle) {
            return {};
        }
        found = id;
    }
    return found;
}

bool parseSplitNote(const std::string& op,
                    SketchEntityHandle& parent,
                    std::vector<SketchEntityHandle>& children)
{
    const std::string prefix = SketchSemanticSeeds::SplitPrefix;
    if (op.size() < prefix.size() || op.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::string_view rest(op.data() + prefix.size(), op.size() - prefix.size());
    const auto arrow = rest.find("->");
    if (arrow == std::string_view::npos || arrow == 0 || arrow + 2 >= rest.size()
        || rest.find("->", arrow + 2) != std::string_view::npos) {
        return false;
    }
    if (!parseEntityHandle(rest.substr(0, arrow), parent)) {
        return false;
    }
    std::size_t start = arrow + 2;
    while (start < rest.size()) {
        const auto comma = rest.find(',', start);
        const auto token = rest.substr(start, comma == std::string_view::npos
                                                ? std::string_view::npos
                                                : comma - start);
        SketchEntityHandle child = SketchEntityIdMap::Invalid;
        if (!parseEntityHandle(token, child)
            || child == parent
            || std::find(children.begin(), children.end(), child) != children.end()) {
            return false;
        }
        children.push_back(child);
        if (comma == std::string_view::npos) {
            break;
        }
        if (comma + 1 >= rest.size()) {
            return false;
        }
        start = comma + 1;
    }
    return !children.empty();
}

bool isSplitParentHandle(const App::SemanticGraph& graph,
                          App::ObjectId sketch,
                          SketchEntityHandle handle)
{
    if (handle == SketchEntityIdMap::Invalid) {
        return false;
    }
    for (const App::Event& ev : graph.events()) {
        if (ev.feature != sketch || ev.kind != App::EventKind::Split) {
            continue;
        }
        SketchEntityHandle parent = 0;
        std::vector<SketchEntityHandle> children;
        if (parseSplitNote(ev.op, parent, children) && parent == handle) {
            return true;
        }
    }
    return false;
}

}  // namespace

std::string SketchSemanticSeeds::entityNote(SketchEntityHandle handle)
{
    return std::string(NotePrefix) + "g" + std::to_string(handle);
}

std::string SketchSemanticSeeds::vertexNote(SketchEntityHandle handle, int pos)
{
    return std::string(NotePrefix) + "v" + std::to_string(handle) + ":" + std::to_string(pos);
}

std::string SketchSemanticSeeds::vertexNote(const SketchVertexKey& key)
{
    return vertexNote(key.handle, key.pos);
}

std::string SketchSemanticSeeds::regionNote(std::string_view regionKey)
{
    return std::string(NotePrefix) + std::string(regionKey);
}

App::SemanticId SketchSemanticSeeds::findSeed(const App::SemanticGraph& graph,
                                              App::ObjectId sketch,
                                              SketchEntityHandle handle)
{
    if (handle == SketchEntityIdMap::Invalid) {
        return {};
    }
    const std::string note = entityNote(handle);
    App::SemanticId found;
    for (const App::Event& ev : graph.events()) {
        if (ev.feature != sketch) {
            continue;
        }
        if (ev.kind == App::EventKind::Generated && ev.op == note) {
            const auto outs = outputsOf(graph, ev.id);
            if (outs.size() != 1 || !isSketchEntitySeed(outs.front())) {
                return {};
            }
            const App::SemanticId id = outs.front();
            // A duplicate entity note is ambiguous; never let event order
            // select which durable seed becomes the live sketch entity.
            if (found.valid() && found.handle != id.handle) {
                return {};
            }
            found = id;
            continue;
        }
        SketchEntityHandle parent = 0;
        std::vector<SketchEntityHandle> children;
        if (ev.kind == App::EventKind::Split && parseSplitNote(ev.op, parent, children)) {
            for (std::size_t i = 0; i < children.size(); ++i) {
                if (children[i] != handle) {
                    continue;
                }
                const auto outs = outputsOf(graph, ev.id);
                if (outs.size() != children.size() || !isSketchEntitySeed(outs[i])) {
                    return {};
                }
                if (found.valid() && found.handle != outs[i].handle) {
                    return {};
                }
                found = outs[i];
            }
        }
    }
    return found;
}

App::SemanticId SketchSemanticSeeds::findRegionSeed(const App::SemanticGraph& graph,
                                                    App::ObjectId sketch,
                                                    std::string_view regionKey)
{
    return uniqueGeneratedSeedByNote(graph, sketch, App::SemanticKind::Region, regionNote(regionKey));
}

App::SemanticId SketchSemanticSeeds::findSeedByKindNote(const App::SemanticGraph& graph,
                                                        App::ObjectId sketch,
                                                        App::SemanticKind kind,
                                                        std::string_view note)
{
    if (note.empty()) {
        return {};
    }
    return uniqueGeneratedSeedByNote(graph, sketch, kind, note);
}

App::SemanticId SketchSemanticSeeds::findSeedForVertex(const App::SemanticGraph& graph,
                                                       App::ObjectId sketch,
                                                       const SketchVertexKey& key)
{
    if (!key.valid()) {
        return {};
    }
    return findSeedByKindNote(graph, sketch, App::SemanticKind::Vertex, vertexNote(key));
}

App::SemanticId SketchSemanticSeeds::ensureSeedForEntity(App::SemanticGraph& graph,
                                                         App::ObjectId sketch,
                                                         App::EvalSerial eval,
                                                         SketchEntityHandle handle,
                                                         App::SemanticKind kind)
{
    if (handle == SketchEntityIdMap::Invalid || handle < 0) {
        // Axes (−1/−2) are never issued as sketch handles and are not seeds.
        return {};
    }
    if (kind != App::SemanticKind::Edge) {
        // Sketch entity notes and type "g" are reserved for curve seeds.
        // Vertex corners use ensureSeedForVertex(); regions use their own key.
        return {};
    }
    const App::SemanticId existing = findSeed(graph, sketch, handle);
    if (existing.valid()) {
        return existing;
    }
    // A malformed or convergent durable note is an occupied semantic slot.
    // Do not mint a replacement identity: that would hide the ambiguity and
    // leave the original event/identity orphaned (I13).
    if (hasGeneratedNote(graph, sketch, entityNote(handle))) {
        return {};
    }
    const App::SemanticId id =
        graph.recordGenerated(kind, entityNote(handle), sketch, eval, App::SemanticRole::User);
    bindSketch(graph, id, sketch, eval, handle);
    return id;
}

App::SemanticId SketchSemanticSeeds::ensureSeedForVertex(App::SemanticGraph& graph,
                                                         App::ObjectId sketch,
                                                         App::EvalSerial eval,
                                                         const SketchVertexKey& key)
{
    if (!key.valid()) {
        return {};
    }
    const App::SemanticId existing = findSeedForVertex(graph, sketch, key);
    if (existing.valid()) {
        return existing;  // C1: never overwrite a valid seed
    }
    if (hasGeneratedNote(graph, sketch, vertexNote(key))) {
        return {};
    }
    const App::SemanticId id = graph.recordGenerated(App::SemanticKind::Vertex,
                                                     vertexNote(key),
                                                     sketch,
                                                     eval,
                                                     App::SemanticRole::User);
    bindVertex(graph, id, sketch, eval, key);
    return id;
}

std::vector<SketchVertexKey> SketchSemanticSeeds::uniqueProfileCorners(
    const std::vector<SketchVertexKey>& endpoints,
    const std::vector<std::pair<SketchVertexKey, SketchVertexKey>>& coincidences,
    const std::vector<SketchVertexPoint>& points,
    double confusion)
{
    struct Row
    {
        SketchVertexKey key;
        SketchVertexPoint pt {};
        bool hasPt = false;
    };
    std::vector<Row> rows;
    rows.reserve(endpoints.size());
    for (std::size_t i = 0; i < endpoints.size(); ++i) {
        if (!endpoints[i].valid()) {
            continue;
        }
        Row row;
        row.key = endpoints[i];
        if (i < points.size()) {
            row.pt = points[i];
            row.hasPt = true;
        }
        rows.push_back(row);
    }
    const int n = static_cast<int>(rows.size());
    if (n == 0) {
        return {};
    }
    std::vector<int> parent(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        parent[static_cast<std::size_t>(i)] = i;
    }
    auto find = [&](int i) {
        int r = i;
        while (parent[static_cast<std::size_t>(r)] != r) {
            r = parent[static_cast<std::size_t>(r)];
        }
        int x = i;
        while (x != r) {
            const int p = parent[static_cast<std::size_t>(x)];
            parent[static_cast<std::size_t>(x)] = r;
            x = p;
        }
        return r;
    };
    auto unite = [&](int a, int b) {
        const int ra = find(a);
        const int rb = find(b);
        if (ra != rb) {
            parent[static_cast<std::size_t>(rb)] = ra;
        }
    };
    auto indexOf = [&](const SketchVertexKey& k) -> int {
        for (int i = 0; i < n; ++i) {
            if (rows[static_cast<std::size_t>(i)].key == k) {
                return i;
            }
        }
        return -1;
    };
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (rows[static_cast<std::size_t>(i)].key == rows[static_cast<std::size_t>(j)].key) {
                unite(i, j);
            }
        }
    }
    for (const auto& pair : coincidences) {
        const int a = indexOf(pair.first);
        const int b = indexOf(pair.second);
        if (a >= 0 && b >= 0) {
            unite(a, b);
        }
    }
    if (confusion < 0) {
        confusion = 0;
    }
    const double tol2 = confusion * confusion;
    for (int i = 0; i < n; ++i) {
        if (!rows[static_cast<std::size_t>(i)].hasPt) {
            continue;
        }
        for (int j = i + 1; j < n; ++j) {
            if (!rows[static_cast<std::size_t>(j)].hasPt) {
                continue;
            }
            const auto& a = rows[static_cast<std::size_t>(i)].pt;
            const auto& b = rows[static_cast<std::size_t>(j)].pt;
            const double dx = a.x - b.x;
            const double dy = a.y - b.y;
            const double dz = a.z - b.z;
            if (dx * dx + dy * dy + dz * dz <= tol2) {
                unite(i, j);
            }
        }
    }
    std::vector<SketchVertexKey> out;
    std::vector<char> emitted(static_cast<std::size_t>(n), 0);
    for (int i = 0; i < n; ++i) {
        const int root = find(i);
        if (emitted[static_cast<std::size_t>(root)]) {
            continue;
        }
        emitted[static_cast<std::size_t>(root)] = 1;
        SketchVertexKey canon = rows[static_cast<std::size_t>(root)].key;
        for (int j = 0; j < n; ++j) {
            if (find(j) != root) {
                continue;
            }
            if (rows[static_cast<std::size_t>(j)].key < canon) {
                canon = rows[static_cast<std::size_t>(j)].key;
            }
        }
        out.push_back(canon);
    }
    return out;
}

App::SemanticId SketchSemanticSeeds::ensureRegionSeed(App::SemanticGraph& graph,
                                                      App::ObjectId sketch,
                                                      App::EvalSerial eval,
                                                      std::string_view regionKey)
{
    if (regionKey.empty()) {
        return {};
    }
    const App::SemanticId existing = findRegionSeed(graph, sketch, regionKey);
    if (existing.valid()) {
        return existing;
    }
    if (hasGeneratedNote(graph, sketch, regionNote(regionKey))) {
        return {};
    }
    const App::SemanticId id = graph.recordGenerated(App::SemanticKind::Region,
                                                     regionNote(regionKey),
                                                     sketch,
                                                     eval,
                                                     App::SemanticRole::User);
    App::SemanticBinding row;
    row.stid = id;
    row.feature = sketch;
    row.eval = eval;
    row.kind = App::SemanticKind::Region;
    row.index.type = "Region";
    row.index.index = regionBindingIndex(regionKey);
    row.occ = nullptr;
    graph.bind(row);
    return id;
}

SketchProfileSeeds SketchSemanticSeeds::seedsForProfile(
    App::SemanticGraph& graph,
    App::ObjectId sketch,
    App::EvalSerial eval,
    const std::vector<SketchEntityHandle>& liveCurves,
    const std::vector<std::string>& regionKeys,
    const std::vector<SketchVertexKey>& uniqueCorners)
{
    SketchProfileSeeds out;
    out.curveHandles = liveCurves;
    out.regionKeys = regionKeys;
    out.vertexKeys = uniqueCorners;
    out.curves.reserve(liveCurves.size());
    for (const SketchEntityHandle h : liveCurves) {
        out.curves.push_back(ensureSeedForEntity(graph, sketch, eval, h, App::SemanticKind::Edge));
    }
    out.regions.reserve(regionKeys.size());
    for (const std::string& key : regionKeys) {
        out.regions.push_back(ensureRegionSeed(graph, sketch, eval, key));
    }
    out.vertices.reserve(uniqueCorners.size());
    for (const SketchVertexKey& k : uniqueCorners) {
        out.vertices.push_back(ensureSeedForVertex(graph, sketch, eval, k));
    }
    return out;
}

void SketchSemanticSeeds::recordRetired(App::SemanticGraph& graph,
                                        App::ObjectId sketch,
                                        App::EvalSerial eval,
                                        SketchEntityHandle handle)
{
    const App::SemanticId id = findSeed(graph, sketch, handle);
    if (!id.valid()) {
        return;
    }
    if (graph.hasDeletedEvent(id.handle)) {
        return;
    }
    // S4-S1: Split parents are historical via Split (unbind already done in
    // splitEntity). Do not emit Deleted on top of Split.
    if (isSplitParentHandle(graph, sketch, handle)) {
        return;
    }
    graph.unbind(id.handle);
    graph.recordDeleted(id, "Sketch.delGeometry", sketch, eval, App::SemanticRole::None);
}

std::vector<App::SemanticId> SketchSemanticSeeds::splitEntity(
    App::SemanticGraph& graph,
    App::ObjectId sketch,
    App::EvalSerial eval,
    SketchEntityHandle parent,
    const std::vector<SketchEntityHandle>& children)
{
    if (parent == SketchEntityIdMap::Invalid || parent < 0 || children.empty()) {
        return {};
    }
    for (const SketchEntityHandle child : children) {
        if (child == SketchEntityIdMap::Invalid || child < 0 || child == parent
            || std::count(children.begin(), children.end(), child) != 1) {
            return {};
        }
    }
    App::SemanticId parentId = findSeed(graph, sketch, parent);
    if (!parentId.valid()) {
        parentId = ensureSeedForEntity(graph, sketch, eval, parent, App::SemanticKind::Edge);
    }
    std::ostringstream op;
    op << SplitPrefix << parent << "->";
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (i != 0) {
            op << ',';
        }
        op << children[i];
    }
    graph.unbind(parentId.handle);
    const auto kids =
        graph.recordSplit(parentId, children.size(), op.str(), sketch, eval, App::SemanticRole::User);
    for (std::size_t i = 0; i < kids.size() && i < children.size(); ++i) {
        bindSketch(graph, kids[i], sketch, eval, children[i]);
    }
    return kids;
}

}  // namespace Sketcher
