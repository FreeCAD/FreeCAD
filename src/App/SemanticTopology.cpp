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

#include "SemanticTopology.h"

#include <algorithm>
#include <functional>
#include <queue>
#include <sstream>

namespace App
{

// ---------------------------------------------------------------------------
// AliasClass
// ---------------------------------------------------------------------------

void AliasClass::unite(SemanticHandle a, SemanticHandle b)
{
    if (a == 0 || b == 0) {
        return;
    }
    const SemanticHandle ra = find(a);
    const SemanticHandle rb = find(b);
    if (ra == rb) {
        return;
    }
    const int raRank = rank[ra];
    const int rbRank = rank[rb];
    if (raRank < rbRank) {
        parent[ra] = rb;
    }
    else if (raRank > rbRank) {
        parent[rb] = ra;
    }
    else {
        parent[rb] = ra;
        rank[ra] = raRank + 1;
    }
}

SemanticHandle AliasClass::find(SemanticHandle a) const
{
    if (a == 0) {
        return 0;
    }
    if (parent.find(a) == parent.end()) {
        parent[a] = a;
        rank[a] = 0;
        return a;
    }
    SemanticHandle root = a;
    while (parent[root] != root) {
        root = parent[root];
    }
    SemanticHandle cur = a;
    while (cur != root) {
        const SemanticHandle next = parent[cur];
        parent[cur] = root;
        cur = next;
    }
    return root;
}

bool AliasClass::same(SemanticHandle a, SemanticHandle b) const
{
    if (a == 0 || b == 0) {
        return false;
    }
    return find(a) == find(b);
}

std::vector<SemanticHandle> AliasClass::members(SemanticHandle a) const
{
    std::vector<SemanticHandle> out;
    if (a == 0) {
        return out;
    }
    const SemanticHandle root = find(a);
    for (const auto& kv : parent) {
        if (find(kv.first) == root) {
            out.push_back(kv.first);
        }
    }
    if (out.empty()) {
        out.push_back(a);
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::unordered_set<SemanticHandle> AliasClass::close(
    const std::unordered_set<SemanticHandle>& ids) const
{
    std::unordered_set<SemanticHandle> out;
    for (const SemanticHandle h : ids) {
        for (const SemanticHandle m : members(h)) {
            out.insert(m);
        }
        out.insert(h);
    }
    return out;
}

void AliasClass::clear()
{
    parent.clear();
    rank.clear();
}

std::size_t AliasClass::classCount() const
{
    std::unordered_set<SemanticHandle> roots;
    for (const auto& kv : parent) {
        roots.insert(find(kv.first));
    }
    return roots.size();
}

AliasClass::Snapshot AliasClass::snapshot() const
{
    Snapshot s;
    s.parent = parent;
    s.rank = rank;
    return s;
}

void AliasClass::restore(const Snapshot& snap)
{
    parent = snap.parent;
    rank = snap.rank;
}

// ---------------------------------------------------------------------------
// SemanticAllocator
// ---------------------------------------------------------------------------

SemanticId SemanticAllocator::generatorMarker(ObjectId feature, SemanticRole role)
{
    SemanticId id;
    id.handle = 0;
    id.kind = SemanticKind::Region;
    id.allocatedBy = feature;
    id.allocatedAtEval = 0;
    id.allocatedRole = role == SemanticRole::None ? SemanticRole::Generator : role;
    return id;
}

SemanticId SemanticAllocator::allocate(SemanticKind kind,
                                       ObjectId allocatedBy,
                                       EvalSerial allocatedAtEval,
                                       SemanticRole allocatedRole)
{
    SemanticId id;
    id.handle = highWater;
    ++highWater;  // never rewind
    id.kind = kind;
    id.allocatedBy = allocatedBy;
    id.allocatedAtEval = allocatedAtEval;
    id.allocatedRole = allocatedRole;
    if (evaluating) {
        inFlight.insert(id.handle);
    }
    else {
        published.insert(id.handle);
    }
    return id;
}

void SemanticAllocator::beginEvaluate(EvalSerial eval)
{
    currentEval = eval;
    evaluating = true;
    inFlight.clear();
}

void SemanticAllocator::commitEvaluate()
{
    for (const SemanticHandle h : inFlight) {
        published.insert(h);
    }
    inFlight.clear();
    evaluating = false;
}

void SemanticAllocator::abortEvaluate()
{
    for (const SemanticHandle h : inFlight) {
        burned.insert(h);
        // not published; highWater is not rewound
    }
    inFlight.clear();
    evaluating = false;
}

void SemanticAllocator::noteUndoOrDiscard()
{
    // High-water stays. In-flight handles, if any, are burned.
    if (!inFlight.empty()) {
        abortEvaluate();
    }
}

void SemanticAllocator::adoptRestored(SemanticHandle restoredHighWater,
                                      const std::unordered_set<SemanticHandle>& publishedHandles,
                                      const std::unordered_set<SemanticHandle>& burnedHandles)
{
    // I5: never rewind high-water across restore.
    if (restoredHighWater > highWater) {
        highWater = restoredHighWater;
    }

    // C1 restore replaces the published set rather than merging it. Handles that
    // were live/in-flight here but absent from the restored published set stay
    // burned so they cannot be recycled, while restored published handles win.
    std::unordered_set<SemanticHandle> abandoned = inFlight;
    for (const SemanticHandle h : published) {
        if (publishedHandles.find(h) == publishedHandles.end()) {
            abandoned.insert(h);
        }
    }
    std::unordered_set<SemanticHandle> keepBurned = burned;

    published = publishedHandles;
    burned.clear();
    for (const SemanticHandle h : burnedHandles) {
        if (published.find(h) == published.end()) {
            burned.insert(h);
        }
    }
    for (const SemanticHandle h : keepBurned) {
        if (published.find(h) == published.end()) {
            burned.insert(h);
        }
    }
    for (const SemanticHandle h : abandoned) {
        if (published.find(h) == published.end()) {
            burned.insert(h);
        }
    }
    for (const SemanticHandle h : published) {
        burned.erase(h);
    }
    inFlight.clear();
    evaluating = false;
}

bool SemanticAllocator::isPublished(SemanticHandle h) const
{
    return published.find(h) != published.end();
}

bool SemanticAllocator::isBurned(SemanticHandle h) const
{
    return burned.find(h) != burned.end();
}

bool SemanticAllocator::isIssued(SemanticHandle h) const
{
    return h != 0 && h < highWater;
}

// ---------------------------------------------------------------------------
// SemanticGraph
// ---------------------------------------------------------------------------

SemanticId SemanticGraph::allocate(SemanticKind kind,
                                   ObjectId allocatedBy,
                                   EvalSerial allocatedAtEval,
                                   SemanticRole allocatedRole)
{
    const SemanticId id = allocator.allocate(kind, allocatedBy, allocatedAtEval, allocatedRole);
    identities[id.handle] = id;
    return id;
}

void SemanticGraph::beginEvaluate(EvalSerial eval)
{
    // Nested begin: abort the in-flight eval first (burns those handles, I5).
    if (allocator.isEvaluating()) {
        abortEvaluate();
    }
    // C2 / I5 / §8 R4: snapshot published graph so abort is atomic.
    evalSnapshot = snapshotPublished();
    hasEvalSnapshot = true;
    allocator.beginEvaluate(eval);
}

void SemanticGraph::commitEvaluate()
{
    for (Event& ev : eventList) {
        ev.published = true;
    }
    allocator.commitEvaluate();
    hasEvalSnapshot = false;
    evalSnapshot = Snapshot();
}

void SemanticGraph::applyPublishedSnapshot(const Snapshot& snap)
{
    eventList = snap.eventList;
    inputList = snap.inputList;
    outputList = snap.outputList;
    aliases.restore(snap.aliases);
    bindings = snap.bindings;
    identities = snap.identities;
    nextEventId = snap.nextEventId;
    modifiedCreatedAlias = snap.modifiedCreatedAlias;
    pruneInvalidBindings();
}

bool SemanticGraph::bindingMatchesIdentity(const SemanticBinding& row) const
{
    if (!row.stid.valid() || row.feature == 0 || row.eval == 0) {
        return false;
    }
    const auto it = identities.find(row.stid.handle);
    if (it == identities.end()) {
        return false;
    }
    const SemanticId& id = it->second;
    return id.handle == row.stid.handle && id.kind == row.stid.kind
        && id.allocatedBy == row.stid.allocatedBy
        && id.allocatedAtEval == row.stid.allocatedAtEval
        && id.allocatedRole == row.stid.allocatedRole;
}

void SemanticGraph::pruneInvalidBindings()
{
    bindings.erase(std::remove_if(bindings.begin(),
                                  bindings.end(),
                                  [&](const SemanticBinding& row) {
                                      return !bindingMatchesIdentity(row);
                                  }),
                   bindings.end());
}


void SemanticGraph::dropBurnedIdentitiesAndBindings()
{
    // C2: do not keep burned identities just because a SemanticBinding pointed at them.
    for (auto it = identities.begin(); it != identities.end();) {
        if (allocator.isBurned(it->first)) {
            it = identities.erase(it);
        }
        else {
            ++it;
        }
    }
    bindings.erase(std::remove_if(bindings.begin(),
                                  bindings.end(),
                                  [&](const SemanticBinding& b) {
                                      return allocator.isBurned(b.stid.handle);
                                  }),
                   bindings.end());
}

void SemanticGraph::abortEvaluate()
{
    // Failed evaluate leaves the published graph unchanged except burned
    // handles / high-water (I5 / §8 R4). Snapshot covers bind/unbind/merge alias.
    if (hasEvalSnapshot) {
        applyPublishedSnapshot(evalSnapshot);
        hasEvalSnapshot = false;
        evalSnapshot = Snapshot();
    }
    else {
        // No beginEvaluate snapshot: drop unpublished events only.
        std::unordered_set<EventId> drop;
        for (const Event& ev : eventList) {
            if (!ev.published) {
                drop.insert(ev.id);
            }
        }
        eventList.erase(std::remove_if(eventList.begin(),
                                       eventList.end(),
                                       [&](const Event& ev) {
                                           return !ev.published;
                                       }),
                        eventList.end());
        inputList.erase(std::remove_if(inputList.begin(),
                                       inputList.end(),
                                       [&](const EventInput& in) {
                                           return drop.count(in.event) != 0;
                                       }),
                        inputList.end());
        outputList.erase(std::remove_if(outputList.begin(),
                                        outputList.end(),
                                        [&](const EventOutput& out) {
                                            return drop.count(out.event) != 0;
                                        }),
                         outputList.end());
    }

    allocator.abortEvaluate();
    dropBurnedIdentitiesAndBindings();
}

EventId SemanticGraph::addEvent(EventKind kind,
                                const std::string& op,
                                ObjectId feature,
                                EvalSerial eval,
                                SemanticRole role)
{
    Event ev;
    ev.id = nextEventId++;
    ev.kind = kind;
    ev.op = op;
    ev.feature = feature;
    ev.eval = eval;
    ev.role = role;
    ev.published = false;
    eventList.push_back(ev);
    return ev.id;
}

void SemanticGraph::addInput(EventId ev, const SemanticId& id)
{
    EventInput in;
    in.event = ev;
    in.semantic = id;
    inputList.push_back(in);
}

bool SemanticGraph::wouldCreateEventCycle(SemanticHandle from, SemanticHandle to) const
{
    if (from == 0 || to == 0 || from == to) {
        return false;  // Modified self-loop is continuity, not a provenance cycle
    }
    // Walk descendants of `to`. If we reach `from`, adding from->to cycles.
    const auto desc = descendants(to);
    return desc.find(from) != desc.end();
}

bool SemanticGraph::addOutput(EventId ev, const SemanticId& id)
{
    for (const EventInput& in : inputList) {
        if (in.event == ev && wouldCreateEventCycle(in.semantic.handle, id.handle)) {
            return false;
        }
    }
    EventOutput out;
    out.event = ev;
    out.semantic = id;
    outputList.push_back(out);
    identities[id.handle] = id;
    return true;
}

EventId SemanticGraph::recordModified(const SemanticId& id,
                                      const std::string& op,
                                      ObjectId feature,
                                      EvalSerial eval,
                                      SemanticRole role)
{
    // Continuity: same handle. Alias is forbidden here (I / §5.1).
    const EventId ev = addEvent(EventKind::Modified, op, feature, eval, role);
    addInput(ev, id);
    addOutput(ev, id);
    identities[id.handle] = id;
    return ev;
}

std::vector<SemanticId> SemanticGraph::recordSplit(const SemanticId& input,
                                                   std::size_t count,
                                                   const std::string& op,
                                                   ObjectId feature,
                                                   EvalSerial eval,
                                                   SemanticRole role)
{
    const EventId ev = addEvent(EventKind::Split, op, feature, eval, role);
    addInput(ev, input);
    std::vector<SemanticId> outs;
    outs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const SemanticId child = allocate(input.kind, feature, eval, role);
        addOutput(ev, child);
        outs.push_back(child);
    }
    return outs;
}

SemanticId SemanticGraph::recordMerge(const std::vector<SemanticId>& inputs,
                                      const std::string& op,
                                      ObjectId feature,
                                      EvalSerial eval,
                                      SemanticRole role)
{
    SemanticKind kind = inputs.empty() ? SemanticKind::Face : inputs.front().kind;
    const SemanticId child = allocate(kind, feature, eval, role);
    const EventId ev = addEvent(EventKind::Merge, op, feature, eval, role);
    for (const SemanticId& in : inputs) {
        addInput(ev, in);
    }
    addOutput(ev, child);
    for (const SemanticId& in : inputs) {
        aliases.unite(in.handle, child.handle);
    }
    return child;
}

EventId SemanticGraph::recordDeleted(const SemanticId& input,
                                     const std::string& op,
                                     ObjectId feature,
                                     EvalSerial eval,
                                     SemanticRole role)
{
    const EventId ev = addEvent(EventKind::Deleted, op, feature, eval, role);
    addInput(ev, input);
    return ev;
}

SemanticId SemanticGraph::recordGenerated(SemanticKind kind,
                                          const std::string& op,
                                          ObjectId feature,
                                          EvalSerial eval,
                                          SemanticRole role)
{
    const SemanticId child = allocate(kind, feature, eval, role);
    const EventId ev = addEvent(EventKind::Generated, op, feature, eval, role);
    addInput(ev, SemanticAllocator::generatorMarker(feature, role));
    addOutput(ev, child);
    return child;
}

SemanticId SemanticGraph::recordGeneratedFrom(const std::vector<SemanticId>& seeds,
                                              SemanticKind kind,
                                              const std::string& op,
                                              ObjectId feature,
                                              EvalSerial eval,
                                              SemanticRole role)
{
    const SemanticId child = allocate(kind, feature, eval, role);
    const EventId ev = addEvent(EventKind::Generated, op, feature, eval, role);
    if (seeds.empty()) {
        addInput(ev, SemanticAllocator::generatorMarker(feature, role));
    }
    else {
        for (const SemanticId& in : seeds) {
            addInput(ev, in);
        }
    }
    addOutput(ev, child);
    return child;
}

std::vector<SemanticId> SemanticGraph::recordIntersection(const std::vector<SemanticId>& inputs,
                                                          std::size_t outCount,
                                                          const std::string& op,
                                                          ObjectId feature,
                                                          EvalSerial eval,
                                                          SemanticRole role)
{
    SemanticKind kind = inputs.empty() ? SemanticKind::Face : inputs.front().kind;
    const EventId ev = addEvent(EventKind::Intersection, op, feature, eval, role);
    for (const SemanticId& in : inputs) {
        addInput(ev, in);
    }
    std::vector<SemanticId> outs;
    outs.reserve(outCount);
    for (std::size_t i = 0; i < outCount; ++i) {
        const SemanticId child = allocate(kind, feature, eval, role);
        addOutput(ev, child);
        outs.push_back(child);
    }
    return outs;
}

bool SemanticGraph::aliasWouldCycleQuotient(SemanticHandle a, SemanticHandle b) const
{
    if (a == 0 || b == 0 || a == b) {
        return false;
    }
    // Build event edges between handles, then contract a and b (and their classes).
    // A cycle of length >= 2 in the quotient is I11.
    const SemanticHandle ra = aliases.find(a);
    const SemanticHandle rb = aliases.find(b);
    if (ra == rb) {
        return false;
    }

    auto cls = [&](SemanticHandle h) -> SemanticHandle {
        const SemanticHandle r = aliases.find(h);
        if (r == ra || r == rb || h == a || h == b) {
            return ra;  // contracted
        }
        return r;
    };

    std::unordered_map<SemanticHandle, std::vector<SemanticHandle>> adj;
    for (const EventOutput& out : outputList) {
        for (const EventInput& in : inputList) {
            if (in.event != out.event) {
                continue;
            }
            const SemanticHandle u = cls(in.semantic.handle);
            const SemanticHandle v = cls(out.semantic.handle);
            if (u == 0 || v == 0 || u == v) {
                continue;  // self-loop after contraction is merge, allowed
            }
            adj[u].push_back(v);
        }
    }

    // DFS cycle detect
    enum Color
    {
        White,
        Grey,
        Black
    };
    std::unordered_map<SemanticHandle, Color> color;
    std::function<bool(SemanticHandle)> dfs = [&](SemanticHandle n) -> bool {
        color[n] = Grey;
        for (const SemanticHandle m : adj[n]) {
            const Color c = color.count(m) ? color[m] : White;
            if (c == Grey) {
                return true;
            }
            if (c == White && dfs(m)) {
                return true;
            }
        }
        color[n] = Black;
        return false;
    };
    for (const auto& kv : adj) {
        if ((color.count(kv.first) ? color[kv.first] : White) == White) {
            if (dfs(kv.first)) {
                return true;
            }
        }
    }
    return false;
}

bool SemanticGraph::alias(const SemanticId& a, const SemanticId& b)
{
    if (!a.valid() || !b.valid()) {
        return false;
    }
    if (aliasWouldCycleQuotient(a.handle, b.handle)) {
        return false;
    }
    aliases.unite(a.handle, b.handle);
    return true;
}

void SemanticGraph::bind(const SemanticBinding& row)
{
    // C1: a transient FaceN/EdgeN row is usable only while its durable
    // identity still exists and its provenance metadata is unchanged.
    if (bindingMatchesIdentity(row)) {
        bindings.push_back(row);
    }
}

void SemanticGraph::clearBindings(ObjectId feature)
{
    // Publishers clear per-feature Bindings before rebinding a new eval so
    // uniqueBindingOnFeature's all-eval count stays a singleton (AG13-E1).
    bindings.erase(std::remove_if(bindings.begin(),
                                  bindings.end(),
                                  [&](const SemanticBinding& b) {
                                      return b.feature == feature;
                                  }),
                   bindings.end());
}

void SemanticGraph::unbind(SemanticHandle h)
{
    bindings.erase(std::remove_if(bindings.begin(),
                                  bindings.end(),
                                  [&](const SemanticBinding& b) {
                                      return b.stid.handle == h;
                                  }),
                   bindings.end());
}

void SemanticGraph::clearAllBindings()
{
    bindings.clear();
}

bool SemanticGraph::hasFeatureData(ObjectId feature) const
{
    if (feature == 0) {
        return false;
    }
    for (const Event& event : eventList) {
        if (event.feature == feature && event.published) {
            return true;
        }
    }
    for (const auto& [handle, id] : identities) {
        (void)handle;
        if (id.allocatedBy == feature && allocator.isPublished(id.handle)) {
            return true;
        }
    }
    return false;
}

std::vector<SemanticBinding> SemanticGraph::bindingsOf(SemanticHandle h) const
{
    std::vector<SemanticBinding> out;
    for (const SemanticBinding& b : bindings) {
        if (b.stid.handle == h) {
            out.push_back(b);
        }
    }
    return out;
}

std::unordered_set<SemanticHandle> SemanticGraph::descendants(SemanticHandle seed) const
{
    // Event walk only. Alias is ignored (I11 / §4.1).
    std::unordered_set<SemanticHandle> seen;
    std::queue<SemanticHandle> q;
    if (seed != 0) {
        q.push(seed);
        seen.insert(seed);
    }
    std::unordered_set<EventId> walkedEvents;
    while (!q.empty()) {
        const SemanticHandle cur = q.front();
        q.pop();
        for (const EventInput& in : inputList) {
            if (in.semantic.handle != cur) {
                continue;
            }
            if (!walkedEvents.insert(in.event).second) {
                continue;
            }
            for (const EventOutput& out : outputList) {
                if (out.event != in.event) {
                    continue;
                }
                if (seen.insert(out.semantic.handle).second) {
                    q.push(out.semantic.handle);
                }
            }
        }
    }
    return seen;
}

std::unordered_set<SemanticHandle> SemanticGraph::ancestors(SemanticHandle id) const
{
    std::unordered_set<SemanticHandle> seen;
    std::queue<SemanticHandle> q;
    if (id != 0) {
        q.push(id);
        seen.insert(id);
    }
    std::unordered_set<EventId> walkedEvents;
    while (!q.empty()) {
        const SemanticHandle cur = q.front();
        q.pop();
        for (const EventOutput& out : outputList) {
            if (out.semantic.handle != cur) {
                continue;
            }
            if (!walkedEvents.insert(out.event).second) {
                continue;
            }
            for (const EventInput& in : inputList) {
                if (in.event != out.event) {
                    continue;
                }
                if (in.semantic.handle == 0) {
                    continue;  // generator marker
                }
                if (seen.insert(in.semantic.handle).second) {
                    q.push(in.semantic.handle);
                }
            }
        }
    }
    return seen;
}

bool SemanticGraph::isAncestor(SemanticHandle ancestor, SemanticHandle descendant) const
{
    if (ancestor == 0 || descendant == 0) {
        return false;
    }
    const auto desc = descendants(ancestor);
    return desc.find(descendant) != desc.end();
}

std::unordered_set<SemanticHandle> SemanticGraph::closeUnderAlias(
    const std::unordered_set<SemanticHandle>& ids) const
{
    return aliases.close(ids);
}

bool SemanticGraph::hasDeletedEvent(SemanticHandle h) const
{
    for (const EventInput& in : inputList) {
        if (in.semantic.handle != h) {
            continue;
        }
        const Event* ev = eventById(in.event);
        if (ev && ev->kind == EventKind::Deleted) {
            return true;
        }
    }
    return false;
}

bool SemanticGraph::isHistorical(SemanticHandle h) const
{
    // Historical: appears as EventInput of an event that has outputs and this
    // handle is not among those outputs (split / merge / intersection).
    for (const EventInput& in : inputList) {
        if (in.semantic.handle != h) {
            continue;
        }
        bool hasOut = false;
        bool isOut = false;
        for (const EventOutput& out : outputList) {
            if (out.event != in.event) {
                continue;
            }
            hasOut = true;
            if (out.semantic.handle == h) {
                isOut = true;
            }
        }
        if (hasOut && !isOut) {
            return true;
        }
    }
    return false;
}

const Event* SemanticGraph::eventById(EventId id) const
{
    for (const Event& ev : eventList) {
        if (ev.id == id) {
            return &ev;
        }
    }
    return nullptr;
}

const SemanticId* SemanticGraph::identity(SemanticHandle h) const
{
    const auto it = identities.find(h);
    if (it == identities.end()) {
        return nullptr;
    }
    return &it->second;
}

bool SemanticGraph::eventGraphIsDag() const
{
    std::unordered_map<SemanticHandle, std::vector<SemanticHandle>> adj;
    for (const EventOutput& out : outputList) {
        for (const EventInput& in : inputList) {
            if (in.event != out.event) {
                continue;
            }
            if (in.semantic.handle == 0 || in.semantic.handle == out.semantic.handle) {
                continue;
            }
            adj[in.semantic.handle].push_back(out.semantic.handle);
        }
    }
    enum Color
    {
        White,
        Grey,
        Black
    };
    std::unordered_map<SemanticHandle, Color> color;
    std::function<bool(SemanticHandle)> dfs = [&](SemanticHandle n) -> bool {
        color[n] = Grey;
        for (const SemanticHandle m : adj[n]) {
            const Color c = color.count(m) ? color[m] : White;
            if (c == Grey) {
                return true;
            }
            if (c == White && dfs(m)) {
                return true;
            }
        }
        color[n] = Black;
        return false;
    };
    for (const auto& kv : adj) {
        if ((color.count(kv.first) ? color[kv.first] : White) == White) {
            if (dfs(kv.first)) {
                return false;
            }
        }
    }
    return true;
}

bool SemanticGraph::eventGraphIsWellFormed() const
{
    std::unordered_map<EventId, const Event*> byId;
    for (const Event& ev : eventList) {
        if (ev.id == 0 || ev.feature == 0 || ev.eval == 0
            || !byId.emplace(ev.id, &ev).second) {
            return false;
        }
    }

    auto identityMatches = [&](const SemanticId& id) {
        if (!id.valid()) {
            return false;
        }
        const auto it = identities.find(id.handle);
        if (it == identities.end()) {
            return false;
        }
        return it->second.handle == id.handle && it->second.kind == id.kind
            && it->second.allocatedBy == id.allocatedBy
            && it->second.allocatedAtEval == id.allocatedAtEval
            && it->second.allocatedRole == id.allocatedRole;
    };

    std::unordered_map<EventId, bool> inputHasGenerator;
    std::unordered_map<EventId, bool> inputHasEntity;
    std::unordered_map<EventId, std::unordered_set<SemanticHandle>> inputHandles;
    for (const EventInput& in : inputList) {
        const auto evIt = byId.find(in.event);
        if (evIt == byId.end()) {
            return false;
        }
        auto& handles = inputHandles[in.event];
        if (!handles.insert(in.semantic.handle).second) {
            return false;
        }
        if (in.semantic.handle == 0) {
            const Event& ev = *evIt->second;
            const SemanticId marker = SemanticAllocator::generatorMarker(ev.feature, ev.role);
            if (ev.kind != EventKind::Generated || in.semantic.kind != marker.kind
                || in.semantic.allocatedBy != marker.allocatedBy
                || in.semantic.allocatedAtEval != marker.allocatedAtEval
                || in.semantic.allocatedRole != marker.allocatedRole) {
                return false;
            }
            inputHasGenerator[in.event] = true;
        }
        else {
            if (!identityMatches(in.semantic)) {
                return false;
            }
            inputHasEntity[in.event] = true;
        }
    }
    for (const auto& entry : inputHasGenerator) {
        if (entry.second && inputHasEntity[entry.first]) {
            return false;
        }
    }

    std::unordered_map<EventId, std::unordered_set<SemanticHandle>> outputHandles;
    for (const EventOutput& out : outputList) {
        const auto evIt = byId.find(out.event);
        if (evIt == byId.end() || !identityMatches(out.semantic)) {
            return false;
        }
        const Event& ev = *evIt->second;
        // Generated topology is allocated by the event feature in its serial.
        // Modified output deliberately reuses an older identity.
        const bool allocatesOutput = ev.kind == EventKind::Generated
            || ev.kind == EventKind::Split || ev.kind == EventKind::Merge
            || ev.kind == EventKind::Intersection;
        if (allocatesOutput
            && (out.semantic.allocatedBy != ev.feature
                || out.semantic.allocatedAtEval != ev.eval)) {
            return false;
        }
        auto& handles = outputHandles[out.event];
        if (!handles.insert(out.semantic.handle).second) {
            return false;
        }
    }
    return true;
}

SemanticGraph::Snapshot SemanticGraph::snapshotPublished() const
{
    Snapshot s;
    for (const Event& ev : eventList) {
        if (ev.published) {
            s.eventList.push_back(ev);
        }
    }
    std::unordered_set<EventId> keep;
    for (const Event& ev : s.eventList) {
        keep.insert(ev.id);
    }
    for (const EventInput& in : inputList) {
        if (keep.count(in.event) != 0) {
            s.inputList.push_back(in);
        }
    }
    for (const EventOutput& out : outputList) {
        if (keep.count(out.event) != 0) {
            s.outputList.push_back(out);
        }
    }
    s.aliases = aliases.snapshot();
    s.bindings = bindings;
    // Published identities only. In-flight / burned handles are not in the snapshot.
    s.identities.clear();
    for (const auto& kv : identities) {
        if (allocator.isPublished(kv.first)) {
            s.identities[kv.first] = kv.second;
        }
    }
    s.nextEventId = nextEventId;
    s.modifiedCreatedAlias = modifiedCreatedAlias;
    return s;
}

void SemanticGraph::restorePublished(const Snapshot& snap)
{
    // Undo restores published graph. Allocator high-water is untouched (I5).
    applyPublishedSnapshot(snap);
    allocator.noteUndoOrDiscard();
    dropBurnedIdentitiesAndBindings();
}


std::vector<SemanticId> SemanticGraph::allIdentities() const
{
    std::vector<SemanticId> out;
    out.reserve(identities.size());
    for (const auto& kv : identities) {
        out.push_back(kv.second);
    }
    return out;
}

namespace
{

std::string encodeField(const std::string& s)
{
    std::string o;
    o.reserve(s.size());
    for (const char c : s) {
        if (c == '\\') {
            o += "\\\\";
        }
        else if (c == '\n') {
            o += "\\n";
        }
        else if (c == '\t') {
            o += "\\t";
        }
        else {
            o += c;
        }
    }
    return o;
}

std::string decodeField(const std::string& s)
{
    std::string o;
    o.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            const char n = s[i + 1];
            if (n == '\\') {
                o += '\\';
                ++i;
            }
            else if (n == 'n') {
                o += '\n';
                ++i;
            }
            else if (n == 't') {
                o += '\t';
                ++i;
            }
            else {
                o += s[i];
            }
        }
        else {
            o += s[i];
        }
    }
    return o;
}

void writeId(std::ostringstream& ss, const SemanticId& id)
{
    ss << id.handle << ' ' << SemanticId::kindChar(id.kind) << ' ' << id.allocatedBy << ' '
       << id.allocatedAtEval << ' ' << static_cast<unsigned>(id.allocatedRole);
}

bool readId(std::istringstream& ls, SemanticId& id)
{
    char kind = 'F';
    unsigned role = 0;
    if (!(ls >> id.handle >> kind >> id.allocatedBy >> id.allocatedAtEval >> role)) {
        return false;
    }
    // Reject unknown kind letters before live-state replacement. kindFromChar
    // would invent Face for garbage and let malformed STG1 look Face-named.
    const auto parsedKind = SemanticId::tryKindFromChar(kind);
    if (!parsedKind) {
        return false;
    }
    id.kind = *parsedKind;
    id.allocatedRole = static_cast<SemanticRole>(role);
    return true;
}

}  // namespace

std::string SemanticGraph::serialize() const
{
    std::ostringstream ss;
    ss << "STG1\n";
    ss << "highWater " << allocator.highWaterMark() << "\n";
    ss << "nextEventId " << nextEventId << "\n";
    ss << "modifiedAlias " << (modifiedCreatedAlias ? 1 : 0) << "\n";
    for (const auto& kv : identities) {
        ss << "identity ";
        writeId(ss, kv.second);
        ss << "\n";
    }
    for (SemanticHandle h = 1; h < allocator.highWaterMark(); ++h) {
        if (allocator.isBurned(h)) {
            ss << "burned " << h << "\n";
        }
    }
    for (const Event& ev : eventList) {
        ss << "event " << ev.id << ' ' << static_cast<unsigned>(ev.kind) << ' '
           << encodeField(ev.op) << ' ' << ev.feature << ' ' << ev.eval << ' '
           << static_cast<unsigned>(ev.role) << ' ' << (ev.published ? 1 : 0) << "\n";
    }
    for (const EventInput& in : inputList) {
        ss << "input " << in.event << ' ';
        writeId(ss, in.semantic);
        ss << "\n";
    }
    for (const EventOutput& out : outputList) {
        ss << "output " << out.event << ' ';
        writeId(ss, out.semantic);
        ss << "\n";
    }
    const AliasClass::Snapshot snap = aliases.snapshot();
    for (const auto& kv : snap.parent) {
        ss << "aliasParent " << kv.first << ' ' << kv.second << "\n";
    }
    for (const auto& kv : snap.rank) {
        ss << "aliasRank " << kv.first << ' ' << kv.second << "\n";
    }
    return ss.str();
}

bool SemanticGraph::deserialize(std::string_view text)
{
    std::istringstream in{std::string(text)};
    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }
    if (line != "STG1") {
        return false;
    }

    SemanticHandle restoredHigh = 1;
    EventId restoredNextEvent = 1;
    bool restoredAliasFlag = false;
    std::unordered_map<SemanticHandle, SemanticId> newIdentities;
    std::unordered_set<SemanticHandle> burned;
    std::vector<Event> newEvents;
    std::vector<EventInput> newInputs;
    std::vector<EventOutput> newOutputs;
    AliasClass::Snapshot aliasSnap;

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "highWater") {
            ls >> restoredHigh;
        }
        else if (tag == "nextEventId") {
            ls >> restoredNextEvent;
        }
        else if (tag == "modifiedAlias") {
            int v = 0;
            ls >> v;
            restoredAliasFlag = v != 0;
        }
        else if (tag == "identity") {
            SemanticId id;
            if (!readId(ls, id)) {
                return false;
            }
            newIdentities[id.handle] = id;
        }
        else if (tag == "burned") {
            SemanticHandle h = 0;
            ls >> h;
            if (h != 0) {
                burned.insert(h);
            }
        }
        else if (tag == "event") {
            Event ev;
            unsigned kind = 0;
            unsigned role = 0;
            int published = 0;
            std::string op;
            ls >> ev.id >> kind >> op >> ev.feature >> ev.eval >> role >> published;
            ev.kind = static_cast<EventKind>(kind);
            ev.op = decodeField(op);
            ev.role = static_cast<SemanticRole>(role);
            ev.published = published != 0;
            newEvents.push_back(ev);
        }
        else if (tag == "input") {
            EventInput ei;
            ls >> ei.event;
            if (!readId(ls, ei.semantic)) {
                return false;
            }
            newInputs.push_back(ei);
        }
        else if (tag == "output") {
            EventOutput eo;
            ls >> eo.event;
            if (!readId(ls, eo.semantic)) {
                return false;
            }
            newOutputs.push_back(eo);
        }
        else if (tag == "aliasParent") {
            SemanticHandle a = 0;
            SemanticHandle b = 0;
            ls >> a >> b;
            aliasSnap.parent[a] = b;
        }
        else if (tag == "aliasRank") {
            SemanticHandle a = 0;
            int r = 0;
            ls >> a >> r;
            aliasSnap.rank[a] = r;
        }
        else {
            return false;
        }
    }

    // Validate connector references before replacing the live graph.
    SemanticGraph candidate = *this;
    candidate.eventList = newEvents;
    candidate.inputList = newInputs;
    candidate.outputList = newOutputs;
    candidate.identities = newIdentities;
    if (!candidate.eventGraphIsWellFormed() || !candidate.eventGraphIsDag()) {
        return false;
    }

    eventList = std::move(newEvents);
    inputList = std::move(newInputs);
    outputList = std::move(newOutputs);
    identities = std::move(newIdentities);
    aliases.restore(aliasSnap);
    nextEventId = restoredNextEvent;
    modifiedCreatedAlias = restoredAliasFlag;
    bindings.clear();
    hasEvalSnapshot = false;
    evalSnapshot = Snapshot();

    std::unordered_set<SemanticHandle> published;
    for (const auto& kv : identities) {
        if (burned.find(kv.first) == burned.end()) {
            published.insert(kv.first);
        }
    }
    allocator.adoptRestored(restoredHigh, published, burned);
    return true;
}

}  // namespace App
