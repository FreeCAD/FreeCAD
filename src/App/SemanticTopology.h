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

#pragma once

/// Provenance directed hypergraph: Event is a node, EventInput / EventOutput
/// are the connectors. No LineageEdge.via. Alias is equivalence, not ancestry.

#include "SemanticId.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace App
{

enum class EventKind : std::uint8_t
{
    Modified = 0,
    Generated,
    Split,
    Merge,
    Deleted,
    Intersection,
};

struct AppExport Event
{
    EventId id = 0;
    EventKind kind = EventKind::Modified;
    std::string op;
    ObjectId feature = 0;
    EvalSerial eval = 0;
    SemanticRole role = SemanticRole::None;
    bool published = false;
};

struct AppExport EventInput
{
    EventId event = 0;
    SemanticId semantic;
};

struct AppExport EventOutput
{
    EventId event = 0;
    SemanticId semantic;
};

/// Transient join. FaceN lives here and only here.
struct AppExport SemanticBinding
{
    SemanticId stid;
    ObjectId feature = 0;
    EvalSerial eval = 0;
    SemanticKind kind = SemanticKind::Face;
    ElementIndex index;
    const void* occ = nullptr;  ///< opaque TopoDS handle; never persisted
};

/// Union-find. ONLY for N-to-1 merge and explicit equivalence. NOT for 1-to-1 Modified.
class AppExport AliasClass
{
public:
    void unite(SemanticHandle a, SemanticHandle b);
    SemanticHandle find(SemanticHandle a) const;
    bool same(SemanticHandle a, SemanticHandle b) const;
    std::vector<SemanticHandle> members(SemanticHandle a) const;
    std::unordered_set<SemanticHandle> close(const std::unordered_set<SemanticHandle>& ids) const;
    void clear();
    std::size_t classCount() const;

    /// Snapshot / restore for undo of published state.
    struct Snapshot
    {
        std::unordered_map<SemanticHandle, SemanticHandle> parent;
        std::unordered_map<SemanticHandle, int> rank;
    };
    Snapshot snapshot() const;
    void restore(const Snapshot& snap);

private:
    mutable std::unordered_map<SemanticHandle, SemanticHandle> parent;
    mutable std::unordered_map<SemanticHandle, int> rank;
};

/// I5: never recycle, including undo, failed evaluate, discarded transactions.
/// Burn-on-failure. Gaps required. Do not rewind on failed evaluate.
class AppExport SemanticAllocator
{
public:
    SemanticAllocator() = default;

    /// Issue the next handle. Persists in the allocation log before publish.
    SemanticId allocate(SemanticKind kind,
                        ObjectId allocatedBy,
                        EvalSerial allocatedAtEval,
                        SemanticRole allocatedRole);

    /// Begin an evaluation. Issued handles are unpublished until commit.
    void beginEvaluate(EvalSerial eval);
    /// Publish handles issued in this evaluation.
    void commitEvaluate();
    /// Burn handles issued in this evaluation. nextHandle is not rewound (I5).
    void abortEvaluate();

    /// Undo restores published identities; it does not resurrect burned handles
    /// and does not rewind the high-water mark.
    void noteUndoOrDiscard();

    SemanticHandle nextHandle() const
    {
        return highWater;
    }

    bool isPublished(SemanticHandle h) const;
    bool isBurned(SemanticHandle h) const;
    bool isIssued(SemanticHandle h) const;
    bool isEvaluating() const
    {
        return evaluating;
    }

    SemanticHandle highWaterMark() const
    {
        return highWater;
    }

    /// Restore after persistence: replace published set, burn abandoned
    /// live/in-flight handles absent from the payload, never rewind high-water (C1/I5).
    void adoptRestored(SemanticHandle restoredHighWater,
                       const std::unordered_set<SemanticHandle>& publishedHandles,
                       const std::unordered_set<SemanticHandle>& burnedHandles);

    /// Reserved handle 0 is the generator marker, never an allocated entity.
    static SemanticId generatorMarker(ObjectId feature, SemanticRole role);

private:
    SemanticHandle highWater = 1;  ///< next handle to issue; never decreases
    EvalSerial currentEval = 0;
    bool evaluating = false;
    std::unordered_set<SemanticHandle> inFlight;
    std::unordered_set<SemanticHandle> published;
    std::unordered_set<SemanticHandle> burned;
};

/// Durable provenance hypergraph + transient SemanticBinding table.
class AppExport SemanticGraph
{
public:
    SemanticAllocator allocator;

    SemanticId allocate(SemanticKind kind,
                        ObjectId allocatedBy,
                        EvalSerial allocatedAtEval,
                        SemanticRole allocatedRole);

    void beginEvaluate(EvalSerial eval);
    void commitEvaluate();
    /// Failed evaluate: restore the published snapshot taken at beginEvaluate,
    /// then burn in-flight handles. High-water is not rewound (I5 / §8 R4).
    void abortEvaluate();

    /// 1-to-1 Modified: same handle, no Alias.
    EventId recordModified(const SemanticId& id,
                           const std::string& op,
                           ObjectId feature,
                           EvalSerial eval,
                           SemanticRole role);

    /// 1-to-N Split: new output handles; input remains historical.
    std::vector<SemanticId> recordSplit(const SemanticId& input,
                                        std::size_t count,
                                        const std::string& op,
                                        ObjectId feature,
                                        EvalSerial eval,
                                        SemanticRole role);

    /// N-to-1 Merge: new child + Alias(parents, child).
    SemanticId recordMerge(const std::vector<SemanticId>& inputs,
                           const std::string& op,
                           ObjectId feature,
                           EvalSerial eval,
                           SemanticRole role);

    /// 1-to-0 Deleted: input only.
    EventId recordDeleted(const SemanticId& input,
                          const std::string& op,
                          ObjectId feature,
                          EvalSerial eval,
                          SemanticRole role);

    /// 0-to-1 Generated: generator marker in, new handle out.
    SemanticId recordGenerated(SemanticKind kind,
                               const std::string& op,
                               ObjectId feature,
                               EvalSerial eval,
                               SemanticRole role);

    /// Generated from existing seeds (Rev 3.1 §10): pad side from curve,
    /// cap from region, fillet face from (edge, adjacent faces).
    /// EventInput = seeds; empty seeds fall back to the generator marker.
    /// Additive Phase 5 API — does not change recordGenerated.
    SemanticId recordGeneratedFrom(const std::vector<SemanticId>& seeds,
                                   SemanticKind kind,
                                   const std::string& op,
                                   ObjectId feature,
                                   EvalSerial eval,
                                   SemanticRole role);

    /// N-to-M Intersection.
    std::vector<SemanticId> recordIntersection(const std::vector<SemanticId>& inputs,
                                               std::size_t outCount,
                                               const std::string& op,
                                               ObjectId feature,
                                               EvalSerial eval,
                                               SemanticRole role);

    /// Explicit user / binder equivalence. Rejects I11 quotient cycles.
    bool alias(const SemanticId& a, const SemanticId& b);

    void bind(const SemanticBinding& row);
    void clearBindings(ObjectId feature);
    void unbind(SemanticHandle h);  ///< drop transient rows for one handle
    void clearAllBindings();
    std::vector<SemanticBinding> bindingsOf(SemanticHandle h) const;
    std::vector<SemanticBinding> allBindings() const
    {
        return bindings;
    }
    bool hasBindings() const
    {
        return !bindings.empty();
    }

    /// True when the durable graph has at least one event or identity owned by
    /// this feature. Bindings are intentionally excluded: they are transient
    /// and are not present in STG1.
    bool hasFeatureData(ObjectId feature) const;

    /// Ancestry: EventOutput ← Event ← EventInput. Ignores Alias.
    std::unordered_set<SemanticHandle> descendants(SemanticHandle seed) const;
    std::unordered_set<SemanticHandle> ancestors(SemanticHandle id) const;
    bool isAncestor(SemanticHandle ancestor, SemanticHandle descendant) const;

    /// Close under Alias of the given set. Order: descendants first, then this.
    std::unordered_set<SemanticHandle> closeUnderAlias(
        const std::unordered_set<SemanticHandle>& ids) const;

    bool hasDeletedEvent(SemanticHandle h) const;
    bool isHistorical(SemanticHandle h) const;

    const Event* eventById(EventId id) const;
    std::vector<Event> events() const
    {
        return eventList;
    }
    std::vector<EventInput> inputs() const
    {
        return inputList;
    }
    std::vector<EventOutput> outputs() const
    {
        return outputList;
    }

    const SemanticId* identity(SemanticHandle h) const;
    std::vector<SemanticId> allIdentities() const;

    /// Line-oriented STG1 payload (no Qt). Bindings are transient and omitted.
    std::string serialize() const;
    bool deserialize(std::string_view text);

    bool aliasUsedForModified() const
    {
        return modifiedCreatedAlias;
    }

    /// I11: event hypergraph (ignoring Alias, ignoring Modified self-loops) is a DAG.
    bool eventGraphIsDag() const;

    /// Structural Event/EventInput/EventOutput integrity. Every connector must
    /// reference an existing Event and every non-marker identity must match the
    /// graph identity table; outputs reject generator markers and duplicate slots.
    bool eventGraphIsWellFormed() const;

    /// Published-graph snapshot for undo. Allocator high-water is not in the snapshot.
    struct Snapshot
    {
        std::vector<Event> eventList;
        std::vector<EventInput> inputList;
        std::vector<EventOutput> outputList;
        AliasClass::Snapshot aliases;
        std::vector<SemanticBinding> bindings;
        std::unordered_map<SemanticHandle, SemanticId> identities;
        EventId nextEventId = 1;
        bool modifiedCreatedAlias = false;
    };
    Snapshot snapshotPublished() const;
    void restorePublished(const Snapshot& snap);

    const AliasClass& aliasClass() const
    {
        return aliases;
    }

private:
    EventId nextEventId = 1;
    std::vector<Event> eventList;
    std::vector<EventInput> inputList;
    std::vector<EventOutput> outputList;
    AliasClass aliases;
    std::vector<SemanticBinding> bindings;
    std::unordered_map<SemanticHandle, SemanticId> identities;
    bool modifiedCreatedAlias = false;

    EventId addEvent(EventKind kind,
                     const std::string& op,
                     ObjectId feature,
                     EvalSerial eval,
                     SemanticRole role);
    void addInput(EventId ev, const SemanticId& id);
    bool addOutput(EventId ev, const SemanticId& id);
    bool wouldCreateEventCycle(SemanticHandle from, SemanticHandle to) const;
    bool aliasWouldCycleQuotient(SemanticHandle a, SemanticHandle b) const;

    /// Published graph at beginEvaluate. Restored by abortEvaluate (C2).
    Snapshot evalSnapshot;
    bool hasEvalSnapshot = false;

    void applyPublishedSnapshot(const Snapshot& snap);
    bool bindingMatchesIdentity(const SemanticBinding& row) const;
    void pruneInvalidBindings();
    void dropBurnedIdentitiesAndBindings();
};

}  // namespace App
