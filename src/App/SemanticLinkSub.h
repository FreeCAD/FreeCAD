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

/// PropertyLinkSub dual-write helpers (Rev 3.1 Phase 3).
/// FaceN is a cache. Seeds are never minted from an unverified FaceN (I13 / D3b).
/// ";:ST" tokens are decoded as the existing handle encoding (I8), not a second heap.

#include "SemanticReference.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Base
{
class XMLReader;
}

namespace App
{

/// XML attribute names written next to existing Sub / ShadowSub strings (D1).
namespace SemanticLinkXml
{
constexpr const char* Seed = "stSeed";
constexpr const char* Kind = "stKind";
constexpr const char* Role = "stRole";
constexpr const char* Filter = "stFilter";
constexpr const char* Reducer = "stReducer";
constexpr const char* Fallback = "stFallback";
constexpr const char* Anchor = "stAnchor";
constexpr const char* SameGenerator = "stSameGen";
constexpr const char* SameRole = "stSameRole";
constexpr const char* SameInstance = "stSameInst";
}  // namespace SemanticLinkXml

/// Last FaceN/EdgeN/... component of a subname, or empty if none.
AppExport ElementIndex elementIndexFromSubName(std::string_view subName);

/// Decode a ";:ST<hex>:<kind>" token anywhere in the subname. Invalid if absent.
/// This is I8 interchange, not promotion and not a second STID heap.
AppExport SemanticId semanticIdFromSubName(std::string_view subName);

/// Build one SemanticReference for a stored subname.
///
/// Policy (I7 / I13 / D3b):
/// 1. Always store fallback IndexedName parsed from the subname (FaceN cache).
/// 2. If the subname already carries a ";:ST" token, that handle is the seed.
/// 3. Else if graph != nullptr AND canPromoteIndexedNameToSeed(graph, index,
///    linkedFeature): attach the unique published SemanticBinding's SemanticId
///    for that linked feature. Does not call allocate().
/// 4. Else: no seed. Raw FaceN assignment is Incompatible, not a mint.
/// linkedFeature 0 → document-wide uniqueness (still refuse 0 or >1).
AppExport SemanticReference makeSemanticRefForSubName(std::string_view subName,
                                                      const SemanticGraph* graph,
                                                      SemanticRole role = SemanticRole::None,
                                                      ObjectId linkedFeature = 0);

AppExport std::vector<SemanticReference>
makeSemanticRefsForSubNames(const std::vector<std::string>& subNames,
                            const SemanticGraph* graph,
                            SemanticRole role = SemanticRole::None,
                            ObjectId linkedFeature = 0);

/// Pad or truncate refs so refs.size() == subCount. New slots have no seed.
AppExport void syncSemanticRefsSize(std::vector<SemanticReference>& refs, std::size_t subCount);

/// C1 carry-forward: Task/Python setValue rebuilds refs from subnames and can
/// drop an in-session seed. Copy a previous valid seed onto an empty dest slot
/// when the FaceN/EdgeN matches uniquely. Different EdgeN does not keep it.
/// 0 or >1 previous seeds for that index → no keep (I13).
AppExport void keepValidSeedsForSameIndex(std::vector<SemanticReference>& dest,
                                          const std::vector<std::string>& destSubs,
                                          const std::vector<SemanticReference>& previous,
                                          const std::vector<std::string>& previousSubs);

/// C1: attach a published SemanticBinding seed only for empty/invalid-seed slots.
/// Never overwrite a valid restored seed. Does not mint (I13).
/// linkedFeature scopes uniqueness (PropertyLinkSub::_pcLinkSub->getID()).
AppExport void promoteRefsWithGraph(std::vector<SemanticReference>& refs,
                                    const std::vector<std::string>& subNames,
                                    const SemanticGraph& graph,
                                    ObjectId linkedFeature = 0);

/// D2: if seed + lineage resolve to exactly one SemanticBinding, rewrite fallback FaceN
/// from that SemanticBinding and return true. No-op when SemanticBinding is empty (restore /
/// pre-recompute). ResolvedSet / Ambiguous / Missing do not coerce a singleton
/// FaceN (I9). Does not clobber restored state when there is no SemanticBinding.
AppExport bool rewriteFallbackFromBinding(SemanticReference& ref, const SemanticGraph& graph);

/// True when a selection subname is the requested Face/Edge/Vertex kind.
/// Matches a leading type prefix, the last FaceN/EdgeN component, or a
/// decoded ;:ST kind. Does not mint and does not treat raw FaceN as identity.
/// Used by GUI SelectionFilter so users still pick Face/Edge labels.
AppExport bool semanticSubNameMatchesKind(std::string_view subName,
                                          std::string_view kindPrefix);

/// Filter-then-reducer resolve of a FaceN/EdgeN (or ;:ST) selection.
/// FaceN-only promotes only via uniquePublishedBinding (I13). Null graph
/// or an unpromotable FaceN returns Missing / Incompatible. Internal App
/// bridge; not a user-facing vocabulary.
AppExport ResolutionResult resolveSemanticSelection(const SemanticGraph* graph,
                                                    ObjectId feature,
                                                    std::string_view subName,
                                                    const CandidateFilter& filter,
                                                    CardinalityReducer reducer);

/// I7 consume: unique Binding index on linkedFeature, else fallback unchanged.
/// 0 or >1 matches → fallback (I10 unnamed). Invalid seed / null graph /
/// linkedFeature 0 → fallback. Does not mint. Does not overwrite a seed (C1).
/// Last FaceN/EdgeN/VertexN component is replaced when a unique Binding exists.
/// Kind→IndexedName type via semanticKindName (AG29-D1); soft Face fallback only
/// when kind is Unknown — strict tryResolveSubNameFromSeed refuses instead.
AppExport std::string resolveSubNameFromSeed(const SemanticGraph* graph,
                                             const SemanticId& seed,
                                             ObjectId linkedFeature,
                                             std::string_view fallbackSubName);

/// Strict consume (TechDraw): when the graph has Bindings, require exactly one
/// matching Binding and return nullopt on fail so a stale FaceN cannot look
/// resolved (I13). When there is no graph / no Bindings yet (restore window),
/// keep fallback. Differs from resolveSubNameFromSeed (I7 always-fallback).
/// Kind→IndexedName type via semanticKindName (AG29-D1); Unknown/empty refused
/// (no Face invent). Soft resolveSubNameFromSeed still Face-falls-back (I7).
/// forceKind: when set, require that SemanticKind (DrawHatch Face); when nullopt,
/// use ref.kind (DrawViewDimension).
/// linkedFeature 0 → nullopt (caller must pass the linked object id).
AppExport std::optional<std::string>
tryResolveSubNameFromSeed(const SemanticGraph* graph,
                          const SemanticReference& ref,
                          ObjectId linkedFeature,
                          std::string_view fallbackSubName,
                          std::optional<SemanticKind> forceKind = std::nullopt);

/// Fields parsed from Sub element attributes. Empty seed string means absent.
struct AppExport SemanticRefXmlFields
{
    std::string seed;  ///< hex handle, no 0x prefix
    std::string kind;
    std::string role;
    std::string filter;
    std::string reducer;
    std::string fallback;
    std::string anchor;
    std::string sameGenerator;
    std::string sameRole;
    std::string sameInstance;
};

/// Attribute fragment starting with a space, e.g. ` stSeed="2a" stKind="F" ...`.
/// Empty if the reference has no valid seed (pre-migration / refused FaceN).
/// stFallback is dualWriteSubName() — the non-authoritative FaceN cache.
AppExport std::string semanticRefXmlAttributes(const SemanticReference& ref);

AppExport SemanticReference semanticRefFromXmlFields(const SemanticRefXmlFields& fields);

/// Parse optional stSeed* attributes from the current XML element (D1/D3).
/// Seed path fills all known SemanticLinkXml attrs; Fallback-only is allowed.
/// Neither attribute → empty reference. Does **not** mint from FaceN (I13 / D3b)
/// and does not call makeSemanticRefForSubName — LinkSub wraps that itself.
/// Spreadsheet Cell restore uses this; PropertyLinks routes through it too.
AppExport SemanticReference semanticRefFromXmlReader(Base::XMLReader& reader);

}  // namespace App
