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

#include "SemanticHistoryAdapter.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <string>

#ifndef SEMANTIC_TOPOLOGY_STANDALONE
#ifndef FC_USE_OCC
#define FC_USE_OCC 1
#endif
#endif

#ifdef FC_USE_OCC
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepTools_History.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS_Shape.hxx>
#endif

namespace Part
{
namespace
{

std::string g_lastApplyNote;

void bindNamed(App::SemanticGraph* graph,
               const App::SemanticId& id,
               App::ObjectId feature,
               App::EvalSerial eval,
               const App::ElementIndex& index)
{
    if (!graph || !id.valid() || !isNamedIndex(index)) {
        return;
    }
    App::SemanticBinding row;
    row.stid = id;
    row.feature = feature;
    row.eval = eval;
    row.kind = id.kind;
    row.index = index;
    row.occ = nullptr;
    graph->bind(row);
}

bool seedListed(const std::vector<App::SemanticId>& inputSeeds, App::SemanticHandle h)
{
    if (inputSeeds.empty()) {
        return true;
    }
    for (const App::SemanticId& s : inputSeeds) {
        if (s.handle == h) {
            return true;
        }
    }
    return false;
}

}  // namespace

const std::string& SemanticHistoryAdapter::lastApplyNote()
{
    return g_lastApplyNote;
}

ApplyResult SemanticHistoryAdapter::applyHistory(App::SemanticGraph* graph,
                                                 App::ObjectId feature,
                                                 App::EvalSerial eval,
                                                 const std::string& opcode,
                                                 const std::vector<App::SemanticId>& inputSeeds,
                                                 const HistoryTable& table)
{
    ApplyResult out;
    g_lastApplyNote.clear();
    if (!graph) {
        out.note = "skip applyHistory: no graph";
        g_lastApplyNote = out.note;
        return out;
    }
    if (table.empty()) {
        out.halfMap = true;
        out.note = "skip applyHistory: empty history (no sequential FaceN)";
        g_lastApplyNote = out.note;
        return out;
    }

    // Group Split rows by fromSeed so 1→N is one event (S1).
    std::map<App::SemanticHandle, std::vector<std::size_t>> splitGroups;
    std::map<std::size_t, std::vector<std::size_t>> seedlessFaces;
    for (std::size_t i = 0; i < table.size(); ++i) {
        const HistoryRecord& rec = table[i];
        if (rec.seedless && (rec.kind == App::EventKind::Generated
                             || rec.kind == App::EventKind::Modified)
            && isNamedIndex(rec.toIndex) && rec.toIndex.type == "Face") {
            seedlessFaces[rec.sourceGroup].push_back(i);
        }
    }

    for (std::size_t i = 0; i < table.size(); ++i) {
        const HistoryRecord& rec = table[i];
        const bool seedlessOutput = rec.seedless && !rec.fromSeed.valid()
            && (rec.kind == App::EventKind::Generated
                || rec.kind == App::EventKind::Modified)
            && isNamedIndex(rec.toIndex) && rec.toIndex.type == "Face";
        if (!rec.fromSeed.valid() && !seedlessOutput) {
            ++out.skippedUnnamed;
            continue;
        }
        if (seedlessOutput) {
            const auto group = seedlessFaces.find(rec.sourceGroup);
            if (group == seedlessFaces.end() || group->second.size() != 1
                || group->second.front() != i) {
                continue;
            }
            std::vector<App::SemanticId> seeds;
            for (const App::SemanticId& extra : rec.extraSeeds) {
                if (extra.valid()) {
                    seeds.push_back(extra);
                }
            }
            const App::SemanticId child = graph->recordGeneratedFrom(
                seeds, rec.outputKind, opcode, feature, eval, App::SemanticRole::None);
            bindNamed(graph, child, feature, eval, rec.toIndex);
            ++out.namedCount;
            ++out.boundCount;
            out.outputs.push_back(child);
            continue;
        }
        if (!seedListed(inputSeeds, rec.fromSeed.handle) && !inputSeeds.empty()) {
            // I10 / I2: do not apply history to a seed the caller did not name.
            continue;
        }

        if (rec.kind == App::EventKind::Deleted) {
            graph->recordDeleted(rec.fromSeed, opcode, feature, eval, rec.fromSeed.allocatedRole);
            continue;
        }

        if (rec.kind == App::EventKind::Modified) {
            graph->recordModified(rec.fromSeed, opcode, feature, eval, rec.fromSeed.allocatedRole);
            if (isNamedIndex(rec.toIndex)) {
                bindNamed(graph, rec.fromSeed, feature, eval, rec.toIndex);
                ++out.namedCount;
                ++out.boundCount;
                out.outputs.push_back(rec.fromSeed);
            }
            continue;
        }

        if (rec.kind == App::EventKind::Split) {
            if (!isNamedIndex(rec.toIndex)) {
                ++out.skippedUnnamed;
                continue;
            }
            splitGroups[rec.fromSeed.handle].push_back(i);
            continue;
        }

        // Generated / Intersection: require a named index. S4: do not invent FaceN.
        if (rec.kind == App::EventKind::Generated || rec.kind == App::EventKind::Intersection) {
            if (!isNamedIndex(rec.toIndex)) {
                ++out.skippedUnnamed;
                continue;
            }
            std::vector<App::SemanticId> seeds;
            seeds.push_back(rec.fromSeed);
            for (const App::SemanticId& extra : rec.extraSeeds) {
                if (extra.valid()) {
                    seeds.push_back(extra);
                }
            }
            const App::SemanticId child = graph->recordGeneratedFrom(
                seeds, rec.outputKind, opcode, feature, eval, App::SemanticRole::None);
            bindNamed(graph, child, feature, eval, rec.toIndex);
            ++out.namedCount;
            ++out.boundCount;
            out.outputs.push_back(child);
        }
    }

    for (const auto& group : splitGroups) {
        if (group.second.empty()) {
            continue;
        }
        const HistoryRecord& first = table[group.second.front()];
        std::vector<App::ElementIndex> indices;
        indices.reserve(group.second.size());
        for (std::size_t idx : group.second) {
            indices.push_back(table[idx].toIndex);
        }
        graph->unbind(first.fromSeed.handle);
        const auto kids = graph->recordSplit(first.fromSeed,
                                             group.second.size(),
                                             opcode,
                                             feature,
                                             eval,
                                             App::SemanticRole::None);
        for (std::size_t k = 0; k < kids.size(); ++k) {
            if (k < indices.size()) {
                bindNamed(graph, kids[k], feature, eval, indices[k]);
                ++out.namedCount;
                ++out.boundCount;
            }
            out.outputs.push_back(kids[k]);
        }
    }

    if (out.boundCount == 0 && out.skippedUnnamed > 0) {
        out.halfMap = true;
        out.note = "skip applyHistory: history rows unnamed (half-map, no FaceN)";
    }
    else {
        out.note = "applyHistory bound=" + std::to_string(out.boundCount)
            + " named=" + std::to_string(out.namedCount)
            + " unnamed=" + std::to_string(out.skippedUnnamed);
    }
    g_lastApplyNote = out.note;
    return out;
}

FilletPreflight SemanticHistoryAdapter::preflightFillet(
    App::SemanticGraph* graph,
    const std::vector<App::SemanticId>& edges)
{
    FilletPreflight out;
    if (!graph) {
        out.makerSkipped = false;
        out.note = "no graph; FaceN dual-write path";
        return out;
    }
    if (edges.empty()) {
        out.makerSkipped = false;
        out.note = "no named FilletEdge seeds; FaceN dual-write path";
        return out;
    }

    out.state = App::ResolutionState::Resolved;
    bool anySkip = false;
    for (const App::SemanticId& edge : edges) {
        App::SemanticReference ref;
        ref.seed = edge;
        ref.kind = App::SemanticKind::Edge;
        ref.role = App::SemanticRole::DressUpEdge;
        App::applyRoleDefaults(ref);
        App::ReferenceRequirement req;
        req.expectedKind = App::SemanticKind::Edge;
        req.acceptedCardinality = App::AcceptedCardinality::OneOrMore;
        req.acceptedReducers = {App::CardinalityReducer::AcceptAll,
                                App::CardinalityReducer::RequireOne};
        const App::ResolutionResult r = App::SemanticResolver::resolve(ref, *graph, &req);
        out.perEdge.push_back(r);
        if (r.state == App::ResolutionState::Missing
            || r.state == App::ResolutionState::Incompatible
            || r.state == App::ResolutionState::Ambiguous) {
            anySkip = true;
            out.state = r.state;
        }
    }
    out.makerSkipped = anySkip;
    if (anySkip) {
        out.note = "FilletEdge Missing/Incompatible/Ambiguous; maker skipped (R2); "
                   "no neighbour (I10)";
    }
    else {
        out.note = "FilletEdge resolved; run maker";
    }
    return out;
}

bool SemanticHistoryAdapter::canUseCachedGeometry(const FilletPreflight& pre,
                                                  bool semanticRepublish)
{
    // C1/D2: republish only a valid cached result; it must not mint a seed from
    // a stale FaceN/EdgeN.  A normal execute may use the cached Base geometry
    // only when every rejected seed is Missing (not Incompatible/Ambiguous).
    if (semanticRepublish) {
        return true;
    }
    if (!pre.makerSkipped || pre.perEdge.empty()) {
        return false;
    }
    return std::all_of(pre.perEdge.begin(),
                       pre.perEdge.end(),
                       [](const App::ResolutionResult& result) {
                           return result.state == App::ResolutionState::Missing;
                       });
}

FilletPreflight SemanticHistoryAdapter::preflightNamedSeeds(
    App::SemanticGraph* graph,
    const std::vector<App::SemanticId>& seeds,
    App::SemanticKind kind)
{
    std::vector<App::SemanticReference> references;
    references.reserve(seeds.size());
    for (const App::SemanticId& seed : seeds) {
        App::SemanticReference ref;
        ref.seed = seed;
        ref.kind = kind;
        ref.filter.flags = App::FilterFlag::DescendantsOfSeed | App::FilterFlag::SameKind;
        ref.reducer = App::CardinalityReducer::AcceptAll;
        references.push_back(ref);
    }
    return preflightNamedReferences(graph, references, kind);
}

FilletPreflight SemanticHistoryAdapter::preflightNamedReferences(
    App::SemanticGraph* graph,
    const std::vector<App::SemanticReference>& references,
    App::SemanticKind kind)
{
    FilletPreflight out;
    if (!graph) {
        out.makerSkipped = false;
        out.note = "no graph; FaceN dual-write path";
        return out;
    }
    if (references.empty()) {
        out.makerSkipped = false;
        out.note = "no named seeds; FaceN dual-write path";
        return out;
    }

    out.state = App::ResolutionState::Resolved;
    bool anySkip = false;
    for (const App::SemanticReference& reference : references) {
        App::ResolutionResult r;
        if (!reference.seed.valid() || reference.seed.kind != kind || reference.kind != kind) {
            r.state = App::ResolutionState::Incompatible;
        }
        else {
            App::ReferenceRequirement req;
            req.expectedKind = kind;
            req.acceptedCardinality = App::AcceptedCardinality::OneOrMore;
            req.acceptedReducers = {reference.reducer};
            r = App::SemanticResolver::resolve(reference, *graph, &req);
        }
        out.perEdge.push_back(r);
        if (r.state == App::ResolutionState::Missing
            || r.state == App::ResolutionState::Incompatible
            || r.state == App::ResolutionState::Ambiguous) {
            anySkip = true;
            out.state = r.state;
        }
    }
    out.makerSkipped = anySkip;
    if (anySkip) {
        out.note = "named seed Missing/Incompatible/Ambiguous; maker skipped (R2); "
                   "no neighbour (I10)";
    }
    else {
        out.note = "named seeds resolved; run maker";
    }
    return out;
}

#ifdef FC_USE_OCC
App::SemanticKind kindFromIndex(const App::ElementIndex& idx, App::SemanticKind fallback)
{
    if (idx.type == "Edge") {
        return App::SemanticKind::Edge;
    }
    if (idx.type == "Face") {
        return App::SemanticKind::Face;
    }
    if (idx.type == "Vertex") {
        return App::SemanticKind::Vertex;
    }
    return fallback;
}

void appendOccLists(HistoryTable& table,
                    const App::SemanticId& seed,
                    const TopoDS_Shape& shape,
                    const TopTools_ListOfShape& generated,
                    const TopTools_ListOfShape& modified,
                    bool removed,
                    const std::function<App::ElementIndex(const void* occShape)>& indexOf,
                    std::size_t sourceGroup)
{
    // Edge seed → Face is the pad-side heuristic when indexOf cannot name the
    // image. When indexOf names an EDGE (Generated(vertex) unique vertical),
    // outputKind must be Edge so fromMaker Edge rows are actually edges.
    const App::SemanticKind genKind =
        seed.kind == App::SemanticKind::Edge ? App::SemanticKind::Face : seed.kind;
    for (TopTools_ListIteratorOfListOfShape it(generated); it.More(); it.Next()) {
        HistoryRecord rec;
        rec.fromSeed = seed;
        rec.kind = App::EventKind::Generated;
        if (indexOf) {
            const TopoDS_Shape& outShape = it.Value();
            rec.toIndex = indexOf(static_cast<const void*>(&outShape));
        }
        if (!seed.valid() && rec.toIndex.type != "Face") {
            continue;
        }
        rec.outputKind = kindFromIndex(rec.toIndex, genKind);
        rec.seedless = !seed.valid();
        rec.sourceGroup = sourceGroup * 2;
        table.push_back(rec);
    }

    // 1 image → Modified (continuity). N images → Split. For a seedless
    // maker input, a unique Face is normalized to a 0-to-1 Generated output;
    // ambiguous history remains unnamed (I13). Do not invent FaceN.
    const int nMod = modified.Extent();
    const App::EventKind modKind =
        nMod > 1 ? App::EventKind::Split : App::EventKind::Modified;
    for (TopTools_ListIteratorOfListOfShape it(modified); it.More(); it.Next()) {
        HistoryRecord rec;
        rec.fromSeed = seed;
        rec.kind = modKind;
        if (indexOf) {
            const TopoDS_Shape& outShape = it.Value();
            rec.toIndex = indexOf(static_cast<const void*>(&outShape));
        }
        rec.outputKind = kindFromIndex(rec.toIndex, seed.kind);
        rec.seedless = !seed.valid();
        rec.sourceGroup = sourceGroup * 2 + 1;
        table.push_back(rec);
    }

    // Deleted only when OCCT reports removal AND there is no image (S4).
    if (removed && generated.IsEmpty() && modified.IsEmpty()) {
        HistoryRecord rec;
        rec.fromSeed = seed;
        rec.kind = App::EventKind::Deleted;
        table.push_back(rec);
        return;
    }
    // Unmodified: not deleted, no Generated/Modified list, input still named
    // on the result (GUI :U;XTR Pad Edge3). Identity continues (Modified).
    if (!removed && generated.IsEmpty() && modified.IsEmpty() && indexOf) {
        SemanticHistoryAdapter::appendUnmodifiedSurvivor(
            table, seed, indexOf(static_cast<const void*>(&shape)));
    }
}
#endif

bool SemanticHistoryAdapter::appendUnmodifiedSurvivor(
    HistoryTable& table,
    const App::SemanticId& seed,
    const App::ElementIndex& toIndex)
{
    if (!seed.valid() || !isNamedIndex(toIndex)) {
        return false;
    }
    HistoryRecord rec;
    rec.fromSeed = seed;
    rec.kind = App::EventKind::Modified;
    rec.toIndex = toIndex;
    if (toIndex.type == "Edge") {
        rec.outputKind = App::SemanticKind::Edge;
    }
    else if (toIndex.type == "Face") {
        rec.outputKind = App::SemanticKind::Face;
    }
    else if (toIndex.type == "Vertex") {
        rec.outputKind = App::SemanticKind::Vertex;
    }
    else {
        rec.outputKind = seed.kind;
    }
    table.push_back(rec);
    return true;
}

HistoryTable SemanticHistoryAdapter::uniqueOneImageGenerated(const HistoryTable& table)
{
    // Count unique published slots — not raw fromMaker rows. MakePrism /
    // MakeRevol often list the same FaceN/EdgeN twice; raw-row counting emptied
    // Part Extrusion/Revolution emit (Windows part_*_seed; Pass-32 class).
    // Face and Edge stay independent per source (PD Revolution capture parity).
    std::map<std::pair<App::SemanticHandle, std::string>, std::set<std::string>> sourceSlots;
    std::map<std::string, std::set<App::SemanticHandle>> slotOwners;
    std::map<std::pair<App::SemanticHandle, std::string>, std::size_t> firstRow;
    for (std::size_t i = 0; i < table.size(); ++i) {
        const HistoryRecord& rec = table[i];
        if (!rec.fromSeed.valid() || rec.kind == App::EventKind::Deleted) {
            continue;
        }
        if (!isNamedIndex(rec.toIndex)) {
            continue;
        }
        if (rec.toIndex.type != "Face" && rec.toIndex.type != "Edge") {
            continue;
        }
        const std::string slot = rec.toIndex.toString();
        const auto key = std::make_pair(rec.fromSeed.handle, rec.toIndex.type);
        sourceSlots[key].insert(slot);
        slotOwners[slot].insert(rec.fromSeed.handle);
        firstRow.emplace(key, i);
    }
    // A published topology slot must have one semantic owner as well as one
    // output per source+kind. Different source rows can otherwise converge on
    // the same FaceN/EdgeN and applyHistory would silently make the first row win.
    HistoryTable out;
    for (const auto& group : sourceSlots) {
        if (group.second.size() != 1) {
            continue;
        }
        const std::string& slot = *group.second.begin();
        if (slotOwners[slot].size() != 1) {
            continue;
        }
        const auto it = firstRow.find(group.first);
        if (it == firstRow.end()) {
            continue;
        }
        HistoryRecord rec = table[it->second];
        rec.kind = App::EventKind::Generated;
        if (rec.toIndex.type == "Edge") {
            rec.outputKind = App::SemanticKind::Edge;
        }
        else if (rec.toIndex.type == "Face") {
            rec.outputKind = App::SemanticKind::Face;
        }
        else {
            rec.outputKind = rec.fromSeed.kind;
        }
        out.push_back(rec);
    }
    return out;
}

HistoryTable SemanticHistoryAdapter::supplementLocatedInputs(
    const HistoryTable& unique,
    const std::vector<std::pair<App::SemanticId, const void*>>& inputs,
    const std::function<App::ElementIndex(const void* occShape)>& indexOf)
{
    std::set<std::string> claimed;
    for (const HistoryRecord& rec : unique) {
        if (!isNamedIndex(rec.toIndex)) {
            continue;
        }
        if (rec.toIndex.type != "Face" && rec.toIndex.type != "Edge") {
            continue;
        }
        claimed.insert(rec.toIndex.toString());
    }
    HistoryTable locate;
    for (const auto& pair : inputs) {
        if (!pair.first.valid() || !pair.second || !indexOf) {
            continue;
        }
        HistoryRecord rec;
        rec.fromSeed = pair.first;
        rec.kind = App::EventKind::Generated;
        rec.toIndex = indexOf(pair.second);
        if (!isNamedIndex(rec.toIndex)) {
            continue;
        }
        if (rec.toIndex.type == "Edge") {
            rec.outputKind = App::SemanticKind::Edge;
        }
        else if (rec.toIndex.type == "Face") {
            rec.outputKind = App::SemanticKind::Face;
        }
        else {
            continue;
        }
        if (claimed.count(rec.toIndex.toString())) {
            continue;
        }
        locate.push_back(rec);
    }
    locate = uniqueOneImageGenerated(locate);
    HistoryTable out = unique;
    for (const HistoryRecord& rec : locate) {
        out.push_back(rec);
    }
    return out;
}

HistoryTable SemanticHistoryAdapter::fromOcctHistory(
    const void* occHistory,
    const std::vector<std::pair<App::SemanticId, const void*>>& inputShapes,
    const std::function<App::ElementIndex(const void* occShape)>& indexOf)
{
    HistoryTable table;
#ifdef FC_USE_OCC
    if (!occHistory) {
        return table;
    }
    const auto* hist = static_cast<const BRepTools_History*>(occHistory);
    for (std::size_t sourceGroup = 0; sourceGroup < inputShapes.size(); ++sourceGroup) {
        const auto& pair = inputShapes[sourceGroup];
        const App::SemanticId& seed = pair.first;
        const void* shapePtr = pair.second;
        if (!seed.valid() || !shapePtr) {
            continue;
        }
        const auto& shape = *static_cast<const TopoDS_Shape*>(shapePtr);
        appendOccLists(table,
                       seed,
                       shape,
                       hist->Generated(shape),
                       hist->Modified(shape),
                       hist->IsRemoved(shape),
                       indexOf,
                       0);
    }
#else
    (void)occHistory;
    (void)inputShapes;
    (void)indexOf;
#endif
    return table;
}

HistoryTable SemanticHistoryAdapter::fromMaker(
    const void* occMaker,
    const std::vector<std::pair<App::SemanticId, const void*>>& inputShapes,
    const std::function<App::ElementIndex(const void* occShape)>& indexOf)
{
    HistoryTable table;
#ifdef FC_USE_OCC
    if (!occMaker) {
        return table;
    }
    // OCCT BRepBuilderAPI_MakeShape history queries are non-const (MSVC).
    auto* maker = static_cast<BRepBuilderAPI_MakeShape*>(const_cast<void*>(occMaker));
    for (std::size_t sourceGroup = 0; sourceGroup < inputShapes.size(); ++sourceGroup) {
        const auto& pair = inputShapes[sourceGroup];
        const App::SemanticId& seed = pair.first;
        const void* shapePtr = pair.second;
        if (!shapePtr) {
            continue;
        }
        const auto& shape = *static_cast<const TopoDS_Shape*>(shapePtr);
        appendOccLists(table,
                       seed,
                       shape,
                       maker->Generated(shape),
                       maker->Modified(shape),
                       maker->IsDeleted(shape),
                       indexOf,
                       sourceGroup);
    }
#else
    (void)occMaker;
    (void)inputShapes;
    (void)indexOf;
#endif
    return table;
}

}  // namespace Part
