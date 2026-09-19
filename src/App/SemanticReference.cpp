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

#include "SemanticReference.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace App
{

const char* resolutionStateName(ResolutionState state)
{
    switch (state) {
        case ResolutionState::Resolved:
            return "Resolved";
        case ResolutionState::ResolvedSet:
            return "ResolvedSet";
        case ResolutionState::Missing:
            return "Missing";
        case ResolutionState::Ambiguous:
            return "Ambiguous";
        case ResolutionState::Incompatible:
            return "Incompatible";
    }
    return "Unknown";
}

std::string dualWriteSubName(const SemanticReference& ref)
{
    // FaceN is a cache. The seed is the identity.
    if (!ref.fallback.type.empty()) {
        return ref.fallback.toString();
    }
    return {};
}

std::optional<SemanticBinding> uniquePublishedBinding(const SemanticGraph& graph,
                                                      const ElementIndex& index,
                                                      ObjectId linkedFeature)
{
    // I13 / D3b: a raw FaceN is not a seed. Promotion is allowed only when
    // SemanticBinding plus lineage identify exactly one published entity.
    // Scope to the linked feature when linkedFeature != 0 (Fillet.Base → Pad,
    // not every Edge15 in the document). linkedFeature == 0 keeps the
    // document-wide rule: ambiguity still refuses (no first-Binding-wins).
    // Highest eval is per feature so Generated children piled across evals
    // do not I13-refuse a fresh Fillet.Base on that linked object. Document-
    // wide (linkedFeature 0) still refuses if two features own the index
    // (I13; no first-Binding-wins), even when their evals differ. Two rows
    // at the same (feature, eval) still 0 or >1 → refuse.
    std::unordered_map<ObjectId, EvalSerial> maxEvalByFeature;
    for (const SemanticBinding& b : graph.allBindings()) {
        if (b.index != index) {
            continue;
        }
        if (linkedFeature != 0 && b.feature != linkedFeature) {
            continue;
        }
        auto it = maxEvalByFeature.find(b.feature);
        if (it == maxEvalByFeature.end() || b.eval > it->second) {
            maxEvalByFeature[b.feature] = b.eval;
        }
    }
    std::optional<SemanticBinding> found;
    int matches = 0;
    for (const SemanticBinding& b : graph.allBindings()) {
        if (b.index != index) {
            continue;
        }
        if (linkedFeature != 0 && b.feature != linkedFeature) {
            continue;
        }
        const auto it = maxEvalByFeature.find(b.feature);
        if (it == maxEvalByFeature.end() || b.eval != it->second) {
            continue;
        }
        ++matches;
        found = b;
        if (!b.stid.valid() || !graph.allocator.isPublished(b.stid.handle)) {
            return std::nullopt;
        }
    }
    if (matches != 1 || !found || !found->stid.valid()) {
        return std::nullopt;
    }
    return found;
}

bool hasOutputOwnershipConflict(const SemanticGraph* graph,
                                ObjectId feature,
                                const ElementIndex& index)
{
    if (!graph || feature == 0 || index.type.empty() || index.index <= 0) {
        return false;
    }

    for (const SemanticBinding& binding : graph->allBindings()) {
        if (binding.feature == feature && binding.index == index) {
            return !uniquePublishedBinding(*graph, index, feature).has_value();
        }
    }
    return false;
}

SemanticId uniqueIdentityAtIndex(const SemanticGraph* graph,
                                 ObjectId feature,
                                 const ElementIndex& index)
{
    SemanticId out;
    if (!graph || feature == 0 || index.type.empty() || index.index <= 0) {
        return out;
    }
    const std::optional<SemanticBinding> b =
        uniquePublishedBinding(*graph, index, feature);
    if (b.has_value() && b->stid.valid()) {
        return b->stid;
    }
    return out;
}

bool canPromoteIndexedNameToSeed(const SemanticGraph& graph,
                                 const ElementIndex& index,
                                 ObjectId linkedFeature)
{
    return uniquePublishedBinding(graph, index, linkedFeature).has_value();
}

std::optional<SemanticBinding> uniqueBindingOnFeature(const SemanticGraph* graph,
                                                      const SemanticId& seed,
                                                      ObjectId linkedFeature,
                                                      const char* indexType)
{
    // I13 consume: 0 or >1 Bindings of this type with index>0 on the linked
    // feature → no consume. Bindings on other features do not count.
    // All live evals count (no max-eval filter — unlike uniquePublishedBinding).
    // Fail-closed if stale multi-eval rows linger; publishers clearBindings
    // first (AG13-E1 watchlist — do not align max-eval without TESTS re-gate).
    // Never first-Binding-wins. Does not mint.
    if (!graph || !seed.valid() || linkedFeature == 0 || !indexType
        || indexType[0] == '\0') {
        return std::nullopt;
    }
    std::optional<SemanticBinding> found;
    int matches = 0;
    for (const SemanticBinding& b : graph->bindingsOf(seed.handle)) {
        if (b.feature != linkedFeature) {
            continue;
        }
        if (b.index.type != indexType || b.index.index <= 0) {
            continue;
        }
        ++matches;
        found = b;
        if (matches > 1) {
            return std::nullopt;
        }
    }
    if (matches != 1) {
        return std::nullopt;
    }
    return found;
}

bool alreadyUniquelyBound(const SemanticGraph* graph,
                          const SemanticId& seed,
                          ObjectId feature,
                          const char* indexType)
{
    // I13 / C1 reuse: seed or a unique descendant already uniquely bound on
    // this feature. Does not mint.
    if (!graph || !seed.valid() || feature == 0 || !indexType || indexType[0] == '\0') {
        return false;
    }
    if (uniqueBindingOnFeature(graph, seed, feature, indexType).has_value()) {
        return true;
    }
    for (SemanticHandle h : graph->descendants(seed.handle)) {
        SemanticId child;
        child.handle = h;
        child.kind = seed.kind;
        if (uniqueBindingOnFeature(graph, child, feature, indexType).has_value()) {
            return true;
        }
    }
    return false;
}

bool shouldRefuseGeneratedMint(const SemanticGraph* graph,
                               const SemanticId& seed,
                               ObjectId feature,
                               const ElementIndex& toIndex)
{
    // I13 leave-unnamed / C1 reuse: refuse Generated mint when the seed is
    // already uniquely bound on this feature, the output slot's latest
    // ownership is conflicted, or a uniquely published owner already holds
    // the slot. Does not mint.
    const char* typ = toIndex.type.c_str();
    if (alreadyUniquelyBound(graph, seed, feature, typ)) {
        return true;
    }
    if (hasOutputOwnershipConflict(graph, feature, toIndex)) {
        return true;
    }
    if (uniqueIdentityAtIndex(graph, feature, toIndex).valid()) {
        return true;
    }
    return false;
}

bool shouldRefuseDressUpMintKind(EventKind kind,
                                 const SemanticGraph* graph,
                                 ObjectId feature,
                                 const ElementIndex& toIndex)
{
    const bool mintsIdentity = kind == EventKind::Generated
        || kind == EventKind::Intersection
        || kind == EventKind::Split;
    if (!mintsIdentity) {
        return false;
    }
    const bool namedOutput = !toIndex.type.empty() && toIndex.index > 0;
    if (!namedOutput) {
        return true;
    }
    return hasOutputOwnershipConflict(graph, feature, toIndex);
}

bool shouldRefuseBoundAt(const SemanticGraph* graph,
                         ObjectId feature,
                         const ElementIndex& index)
{
    // I13 leave-unnamed / refuse remint for locate/bind publishers:
    // conflicted latest ownership or a uniquely published owner already
    // occupies the slot. Does not mint.
    if (hasOutputOwnershipConflict(graph, feature, index)) {
        return true;
    }
    return uniqueIdentityAtIndex(graph, feature, index).valid();
}

bool shouldRefuseNamedEmitSlot(const SemanticGraph* graph,
                               ObjectId feature,
                               const ElementIndex& index,
                               const char* expectedType,
                               bool protectOutputOwnership)
{
    // I13 leave-unnamed named-slot emit refuse: invalid typed Face/Edge
    // index (same semantics as PartDesign isValidNamedIndex(index,
    // expectedType)), or when protectOutputOwnership and
    // hasOutputOwnershipConflict. Does not mint; does not set Ambiguous.
    // Uniquely published alone is not a refuse reason.
    const bool typedOk = (index.type == "Face" || index.type == "Edge")
        && index.index > 0
        && (!expectedType || index.type == expectedType);
    if (!typedOk) {
        return true;
    }
    return protectOutputOwnership
        && hasOutputOwnershipConflict(graph, feature, index);
}

void applyRoleDefaults(SemanticReference& ref)
{
    ref.filter.flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    switch (ref.role) {
        case SemanticRole::SupportFace:
        case SemanticRole::UpToFace:
        case SemanticRole::AssemblyJointFace:
            ref.kind = SemanticKind::Face;
            ref.reducer = CardinalityReducer::RequireOne;
            break;
        case SemanticRole::ExternalEdge:
        case SemanticRole::PatternAxis:
            ref.kind = SemanticKind::Edge;
            ref.reducer = CardinalityReducer::RequireOne;
            break;
        case SemanticRole::DressUpEdge:
            ref.kind = SemanticKind::Edge;
            ref.reducer = CardinalityReducer::AcceptAll;
            break;
        case SemanticRole::BooleanToolFace:
            ref.filter.flags = ref.filter.flags | FilterFlag::SameGenerator;
            ref.reducer = CardinalityReducer::AcceptAll;
            break;
        default:
            break;
    }
}

std::unordered_set<SemanticHandle> SemanticResolver::walkDescendants(const SemanticGraph& graph,
                                                                     SemanticHandle seed)
{
    return graph.descendants(seed);
}

namespace
{

std::vector<SemanticBinding> liveBindingsFor(const SemanticGraph& graph,
                                     const std::unordered_set<SemanticHandle>& closed)
{
    std::vector<SemanticBinding> live;
    for (const SemanticBinding& b : graph.allBindings()) {
        if (closed.find(b.stid.handle) != closed.end()) {
            live.push_back(b);
        }
    }
    return live;
}

bool passesFilters(const SemanticReference& ref,
                   const SemanticBinding& b,
                   const SemanticGraph& graph)
{
    if (hasFilter(ref.filter.flags, FilterFlag::SameKind)) {
        if (b.kind != ref.kind && b.stid.kind != ref.kind) {
            return false;
        }
    }
    if (hasFilter(ref.filter.flags, FilterFlag::SameGenerator)) {
        const ObjectId gen = ref.filter.sameGenerator != 0 ? ref.filter.sameGenerator
                                                           : ref.seed.allocatedBy;
        if (gen != 0 && b.stid.allocatedBy != gen && b.feature != gen) {
            return false;
        }
    }
    if (hasFilter(ref.filter.flags, FilterFlag::SameRole)) {
        const SemanticRole role =
            ref.filter.sameRole != SemanticRole::None ? ref.filter.sameRole : ref.role;
        if (role != SemanticRole::None && b.stid.allocatedRole != role) {
            return false;
        }
    }
    (void)graph;
    return true;
}

const GeometryScore* findScore(const std::vector<GeometryScore>* geometry, SemanticHandle h)
{
    if (!geometry) {
        return nullptr;
    }
    for (const GeometryScore& s : *geometry) {
        if (s.handle == h) {
            return &s;
        }
    }
    return nullptr;
}

ResolutionState applyReducer(CardinalityReducer reducer,
                             const std::vector<SemanticBinding>& admitted,
                             const std::vector<GeometryScore>* geometry,
                             std::vector<SemanticBinding>& chosen)
{
    chosen.clear();
    const std::size_t n = admitted.size();
    if (n == 0) {
        return ResolutionState::Missing;
    }
    if (n == 1) {
        chosen.push_back(admitted.front());
        return ResolutionState::Resolved;
    }

    switch (reducer) {
        case CardinalityReducer::RequireOne:
            return ResolutionState::Ambiguous;
        case CardinalityReducer::AcceptAll:
            chosen = admitted;
            return ResolutionState::ResolvedSet;
        case CardinalityReducer::UniqueLargest: {
            // I10: rank only admitted candidates. Missing scores do not invent rows.
            double best = -std::numeric_limits<double>::infinity();
            int bestCount = 0;
            const SemanticBinding* bestB = nullptr;
            for (const SemanticBinding& b : admitted) {
                const GeometryScore* s = findScore(geometry, b.stid.handle);
                if (!s) {
                    continue;
                }
                if (s->areaOrSize > best) {
                    best = s->areaOrSize;
                    bestCount = 1;
                    bestB = &b;
                }
                else if (s->areaOrSize == best) {
                    ++bestCount;
                }
            }
            if (bestB && bestCount == 1) {
                chosen.push_back(*bestB);
                return ResolutionState::Resolved;
            }
            return ResolutionState::Ambiguous;
        }
        case CardinalityReducer::UniqueClosest: {
            double best = std::numeric_limits<double>::infinity();
            int bestCount = 0;
            const SemanticBinding* bestB = nullptr;
            for (const SemanticBinding& b : admitted) {
                const GeometryScore* s = findScore(geometry, b.stid.handle);
                if (!s) {
                    continue;
                }
                if (s->distanceToAnchor < best) {
                    best = s->distanceToAnchor;
                    bestCount = 1;
                    bestB = &b;
                }
                else if (s->distanceToAnchor == best) {
                    ++bestCount;
                }
            }
            if (bestB && bestCount == 1) {
                chosen.push_back(*bestB);
                return ResolutionState::Resolved;
            }
            return ResolutionState::Ambiguous;
        }
        case CardinalityReducer::UserReduce:
            return ResolutionState::Ambiguous;
    }
    return ResolutionState::Ambiguous;
}

ResolutionState applyRequirement(const ReferenceRequirement& req,
                                 CardinalityReducer reducer,
                                 ResolutionState state,
                                 const std::vector<SemanticBinding>& chosen)
{
    // I12: a consumer must explicitly accept the reducer that produced the
    // candidate set. An empty set is the deliberate wildcard; otherwise a
    // reducer outside the contract is incompatible even if its current
    // result happens to contain one or more bindings.
    if (!req.acceptedReducers.empty()
        && req.acceptedReducers.find(reducer) == req.acceptedReducers.end()) {
        return ResolutionState::Incompatible;
    }

    if (state == ResolutionState::ResolvedSet
        && req.acceptedCardinality == AcceptedCardinality::One) {
        return ResolutionState::Incompatible;
    }
    if (state == ResolutionState::Resolved
        && req.acceptedCardinality == AcceptedCardinality::One && chosen.size() != 1) {
        return ResolutionState::Incompatible;
    }
    for (const SemanticBinding& b : chosen) {
        if (b.kind != req.expectedKind && b.stid.kind != req.expectedKind) {
            return ResolutionState::Incompatible;
        }
    }
    return state;
}

}  // namespace

ResolutionResult SemanticResolver::resolve(const SemanticReference& ref,
                                           const SemanticGraph& graph,
                                           const ReferenceRequirement* requirement,
                                           const std::vector<GeometryScore>* geometry)
{
    ResolutionResult result;
    if (!ref.seed.valid()) {
        result.state = ResolutionState::Missing;
        return result;
    }

    // 1. Descendants via Event walk. Alias is ignored.
    const auto desc = walkDescendants(graph, ref.seed.handle);

    // 2. Close under Alias of those descendants only (order is mandatory).
    const auto closed = graph.closeUnderAlias(desc);

    // 3. Live B-Rep = current SemanticBinding rows in that closed set.
    const std::vector<SemanticBinding> live = liveBindingsFor(graph, closed);

    // 4. Remaining filters. DescendantsOfSeed already applied.
    std::vector<SemanticBinding> admitted;
    admitted.reserve(live.size());
    for (const SemanticBinding& b : live) {
        if (passesFilters(ref, b, graph)) {
            admitted.push_back(b);
        }
    }

    // I10: geometry is not consulted to grow `admitted`.
    (void)geometry;  // used only inside UniqueLargest / UniqueClosest

    // 5. Reducer
    std::vector<SemanticBinding> chosen;
    ResolutionState state = applyReducer(ref.reducer, admitted, geometry, chosen);

    // Deleted with no live remnant is Missing, not a neighbour guess.
    if (state == ResolutionState::Missing && graph.hasDeletedEvent(ref.seed.handle)
        && admitted.empty()) {
        state = ResolutionState::Missing;
    }

    // 6. Consumer contract (I12)
    if (requirement) {
        state = applyRequirement(*requirement, ref.reducer, state, chosen);
        if (state == ResolutionState::Incompatible) {
            result.state = state;
            return result;
        }
    }

    result.state = state;
    result.bindings = chosen;
    for (const SemanticBinding& b : chosen) {
        result.identities.push_back(b.stid);
    }
    return result;
}

}  // namespace App
