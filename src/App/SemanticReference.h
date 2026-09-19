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

/// Policy is filter then reducer. Failure is a value (I9). Geometry may rank
/// admitted candidates only (I10). Consumer arity is data (I12).

#include "SemanticId.h"
#include "SemanticTopology.h"

#include <Base/Bitmask.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace App
{

enum class ResolutionState : std::uint8_t
{
    Resolved = 0,
    ResolvedSet,
    Missing,
    Ambiguous,
    Incompatible,
};

enum class CardinalityReducer : std::uint8_t
{
    RequireOne = 0,
    AcceptAll,
    UniqueLargest,
    UniqueClosest,
    UserReduce,
};

enum class AcceptedCardinality : std::uint8_t
{
    One = 0,
    OneOrMore,
    ZeroOrMore,
};

enum class FilterFlag : std::uint32_t
{
    DescendantsOfSeed = 1u << 0,  ///< mandatory first step, implicit
    SameKind = 1u << 1,
    SameGenerator = 1u << 2,
    SameRole = 1u << 3,
    SameInstance = 1u << 4,
    UserPredicate = 1u << 5,
};

}  // namespace App

// Free App::operator|(FilterFlag,...) poisons other App enum bitmasks under MSVC
// (AddObjectOption / RemoveObjectOption). Use the project bitmask trait instead.
ENABLE_BITMASK_OPERATORS(App::FilterFlag)

namespace App
{

constexpr bool hasFilter(FilterFlag set, FilterFlag bit)
{
    return (static_cast<std::uint32_t>(set) & static_cast<std::uint32_t>(bit)) != 0;
}

struct AppExport CandidateFilter
{
    FilterFlag flags = FilterFlag::DescendantsOfSeed | FilterFlag::SameKind;
    ObjectId sameGenerator = 0;
    SemanticRole sameRole = SemanticRole::None;
    std::uint64_t sameInstance = 0;
};

struct AppExport SemanticReference
{
    SemanticId seed;
    SemanticKind kind = SemanticKind::Face;
    SemanticRole role = SemanticRole::None;
    CandidateFilter filter;
    CardinalityReducer reducer = CardinalityReducer::RequireOne;
    std::optional<std::string> anchor;
    ElementIndex fallback;  ///< dual-write, non-authoritative FaceN
    ResolutionState state = ResolutionState::Missing;
};

struct AppExport ReferenceRequirement
{
    SemanticKind expectedKind = SemanticKind::Face;
    AcceptedCardinality acceptedCardinality = AcceptedCardinality::One;
    std::unordered_set<CardinalityReducer> acceptedReducers = {CardinalityReducer::RequireOne};
};

/// Geometry evidence over an already-admitted candidate. I10: cannot introduce one.
struct AppExport GeometryScore
{
    SemanticHandle handle = 0;
    double areaOrSize = 0.0;
    double distanceToAnchor = 0.0;
};

struct AppExport ResolutionResult
{
    ResolutionState state = ResolutionState::Missing;
    std::vector<SemanticId> identities;
    std::vector<SemanticBinding> bindings;
};

/// I13: a raw FaceN must not become a durable seed without verified provenance.
/// When linkedFeature != 0, count only SemanticBinding rows with b.feature ==
/// linkedFeature. Highest eval is per feature so Generated children piled
/// across evals do not I13-refuse a fresh Fillet.Base. 0 or >1 at that
/// (feature, eval) → false (no mint). Document-wide (linkedFeature 0)
/// still refuses if two features own the index, even at different evals.
AppExport bool canPromoteIndexedNameToSeed(const SemanticGraph& graph,
                                           const ElementIndex& index,
                                           ObjectId linkedFeature = 0);

/// Unique published Binding for (index, linkedFeature) at that feature's
/// highest eval. nullopt if 0 or >1 at that eval, or unpublished (I13).
AppExport std::optional<SemanticBinding> uniquePublishedBinding(const SemanticGraph& graph,
                                                                const ElementIndex& index,
                                                                ObjectId linkedFeature = 0);

/// True when an existing output slot has no single published owner at the
/// feature's highest evaluation. An empty slot is not a conflict.
AppExport bool hasOutputOwnershipConflict(const SemanticGraph* graph,
                                          ObjectId feature,
                                          const ElementIndex& index);

/// Feature-scoped unique published identity at an output index. Empty/invalid
/// args or a missing, unpublished, or convergent latest Binding return an
/// invalid SemanticId (I13). Thin wrapper over uniquePublishedBinding.
AppExport SemanticId uniqueIdentityAtIndex(const SemanticGraph* graph,
                                           ObjectId feature,
                                           const ElementIndex& index);

/// Feature-scoped unique Binding for a seed (consume). Count only rows with
/// b.feature == linkedFeature, b.index.type == indexType, and index>0.
/// Counts **all live evals** (no highest-eval filter — unlike
/// uniquePublishedBinding). Safe fail-closed if stale multi-eval rows linger;
/// publishers normally clearBindings(feature) first (AG13-E1 watchlist:
/// align max-eval only with TESTS re-gate if a Binding leak appears).
/// 0 or >1 → nullopt (I13). Never first-Binding-wins. Null graph, invalid
/// seed, linkedFeature 0, or empty indexType → nullopt. Does not mint.
AppExport std::optional<SemanticBinding> uniqueBindingOnFeature(
    const SemanticGraph* graph,
    const SemanticId& seed,
    ObjectId linkedFeature,
    const char* indexType);

/// True when seed OR a descendant already has a unique feature-scoped Binding
/// of indexType on feature (I13 consume / C1 reuse). Null graph, invalid seed,
/// feature 0, or empty indexType → false. Does not mint.
AppExport bool alreadyUniquelyBound(const SemanticGraph* graph,
                                     const SemanticId& seed,
                                     ObjectId feature,
                                     const char* indexType);

/// True when applyHistory must not mint a Generated Binding for this
/// (seed → toIndex) on feature: already uniquely bound (C1), ownership
/// conflict on the slot, or a uniquely published owner already occupies it
/// (I13 leave-unnamed). Composes alreadyUniquelyBound +
/// hasOutputOwnershipConflict + uniqueIdentityAtIndex. Does not mint.
AppExport bool shouldRefuseGeneratedMint(const SemanticGraph* graph,
                                         const SemanticId& seed,
                                         ObjectId feature,
                                         const ElementIndex& toIndex);

/// DressUp fromMaker filter: Generated/Intersection/Split rows must have a
/// named output slot with no ownership conflict; otherwise leave unnamed (I13).
/// Non-minting EventKinds never refuse via this helper. Does not mint.
AppExport bool shouldRefuseDressUpMintKind(EventKind kind,
                                           const SemanticGraph* graph,
                                           ObjectId feature,
                                           const ElementIndex& toIndex);

/// Locate/bind mint refusal (Part Extrusion/Revolution/Mirroring boundAt):
/// true when the feature-scoped output slot has an ownership conflict or a
/// uniquely published owner already (I13 leave-unnamed / refuse remint).
/// Composes hasOutputOwnershipConflict + uniqueIdentityAtIndex. Does not mint.
AppExport bool shouldRefuseBoundAt(const SemanticGraph* graph,
                                   ObjectId feature,
                                   const ElementIndex& index);

/// Named-slot emit refuse (PartDesign Pad/Pocket/Revolution-family +
/// emitNamedEdgesFromRequest): true when the typed Face/Edge index is
/// invalid, or when protectOutputOwnership and the slot has an ownership
/// conflict (I13 leave-unnamed). Does not mint; does not set Ambiguous.
/// Default protectOutputOwnership=true. Uniquely published alone is not
/// a refuse reason (unlike shouldRefuseBoundAt).
AppExport bool shouldRefuseNamedEmitSlot(const SemanticGraph* graph,
                                         ObjectId feature,
                                         const ElementIndex& index,
                                         const char* expectedType,
                                         bool protectOutputOwnership = true);

/// Dual-write helper: the non-authoritative FaceN string stored next to the seed.
/// PropertyLinkSub Save/Restore of SemanticReference is Phase 3 — see phase2-status.md.
AppExport std::string dualWriteSubName(const SemanticReference& ref);

/// Role defaults from Rev 3.1 §4.2 (prototype encoding of filter+reducer).
AppExport void applyRoleDefaults(SemanticReference& ref);

class AppExport SemanticResolver
{
public:
    /// Resolve a stored seed against the current graph and SemanticBinding table.
    ///
    /// Order is mandatory:
    ///   1. Walk hypergraph descendants (ignore Alias)
    ///   2. Close under Alias of those descendants
    ///   3. Intersect with current SemanticBinding (live B-Rep)
    ///   4. Apply remaining filters
    ///   5. Apply reducer (geometry ranks only inside UniqueLargest / UniqueClosest)
    ///   6. Intersect with ReferenceRequirement (I12)
    static ResolutionResult resolve(const SemanticReference& ref,
                                    const SemanticGraph& graph,
                                    const ReferenceRequirement* requirement = nullptr,
                                    const std::vector<GeometryScore>* geometry = nullptr);

    /// Descendants of seed via Event walk. Does not close Alias.
    static std::unordered_set<SemanticHandle> walkDescendants(const SemanticGraph& graph,
                                                              SemanticHandle seed);
};

AppExport const char* resolutionStateName(ResolutionState state);

}  // namespace App
