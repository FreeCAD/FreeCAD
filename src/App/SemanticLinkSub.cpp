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

#include "SemanticLinkSub.h"

#include <Base/Reader.h>

#include <cctype>
#include <cstring>
#include <optional>
#include <sstream>

namespace App
{
namespace
{

bool isGeometricElementType(const std::string& type)
{
    return type == "Face" || type == "Edge" || type == "Vertex" || type == "Wire"
        || type == "Shell" || type == "Solid";
}

std::string_view lastPathComponent(std::string_view subName)
{
    const auto dot = subName.rfind('.');
    if (dot == std::string_view::npos) {
        return subName;
    }
    return subName.substr(dot + 1);
}

std::string xmlEscape(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

unsigned parseUnsigned(const std::string& text, unsigned fallback = 0)
{
    if (text.empty()) {
        return fallback;
    }
    try {
        return static_cast<unsigned>(std::stoul(text, nullptr, 10));
    }
    catch (...) {
        return fallback;
    }
}

std::optional<SemanticHandle> parseSemanticHandle(std::string_view text)
{
    if (text.empty()) {
        return std::nullopt;
    }
    for (const unsigned char c : text) {
        if (!std::isxdigit(c)) {
            return std::nullopt;
        }
    }
    try {
        std::size_t parsed = 0;
        const auto handle = static_cast<SemanticHandle>(std::stoull(std::string(text), &parsed, 16));
        if (parsed != text.size() || handle == 0) {
            return std::nullopt;
        }
        return handle;
    }
    catch (...) {
        return std::nullopt;
    }
}

}  // namespace

ElementIndex elementIndexFromSubName(std::string_view subName)
{
    std::string_view elem = lastPathComponent(subName);
    const auto semi = elem.find(';');
    if (semi == 0) {
        // Mapped-name blob with no FaceN suffix on this component.
        return {};
    }
    const std::string_view indexed =
        semi == std::string_view::npos ? elem : elem.substr(0, semi);
    if (indexed.empty()) {
        return {};
    }
    ElementIndex idx = ElementIndex::fromString(indexed);
    if (idx.index <= 0 || !isGeometricElementType(idx.type)) {
        return {};
    }
    return idx;
}

SemanticId semanticIdFromSubName(std::string_view subName)
{
    // Keep in lockstep with Data::POSTFIX_SEMANTIC via SemanticId::mappedTokenPrefix.
    const std::string_view mark = SemanticId::mappedTokenPrefix();
    const auto pos = subName.find(mark);
    if (pos == std::string_view::npos) {
        return {};
    }
    const std::string_view rest = subName.substr(pos);
    // Token is ";:ST<hex>:<kind>". Cut at the next mapped postfix or path dot.
    auto cut = rest.size();
    const auto nextSemi = rest.find(';', 1);
    const auto nextDot = rest.find('.');
    if (nextSemi != std::string_view::npos) {
        cut = nextSemi;
    }
    if (nextDot != std::string_view::npos && nextDot < cut) {
        cut = nextDot;
    }
    return SemanticId::fromMappedToken(rest.substr(0, cut));
}

SemanticReference makeSemanticRefForSubName(std::string_view subName,
                                            const SemanticGraph* graph,
                                            SemanticRole role,
                                            ObjectId linkedFeature)
{
    SemanticReference ref;
    ref.role = role;
    ref.fallback = elementIndexFromSubName(subName);

    // I8: a ";:ST" token already encodes the handle. Decoding is not minting.
    const SemanticId token = semanticIdFromSubName(subName);
    if (token.valid()) {
        ref.seed = token;
        ref.kind = token.kind;
        applyRoleDefaults(ref);
        ref.state = ResolutionState::Resolved;
        return ref;
    }

    if (!ref.fallback.type.empty() && ref.fallback.index > 0 && graph
        && canPromoteIndexedNameToSeed(*graph, ref.fallback, linkedFeature)) {
        const std::optional<SemanticBinding> b =
            uniquePublishedBinding(*graph, ref.fallback, linkedFeature);
        if (b) {
            // Attach the already-published identity. Never allocate() here (I13).
            ref.seed = b->stid;
            ref.kind = b->kind != SemanticKind::Face ? b->kind : b->stid.kind;
            if (role == SemanticRole::None) {
                ref.role = b->stid.allocatedRole;
            }
            applyRoleDefaults(ref);
            ref.state = ResolutionState::Resolved;
            return ref;
        }
    }

    // I13 / D3b: raw unverified FaceN is not a seed.
    if (!ref.fallback.type.empty() && ref.fallback.index > 0) {
        ref.state = ResolutionState::Incompatible;
    }
    else {
        ref.state = ResolutionState::Missing;
    }
    return ref;
}

std::vector<SemanticReference>
makeSemanticRefsForSubNames(const std::vector<std::string>& subNames,
                            const SemanticGraph* graph,
                            SemanticRole role,
                            ObjectId linkedFeature)
{
    std::vector<SemanticReference> refs;
    refs.reserve(subNames.size());
    for (const std::string& sub : subNames) {
        refs.push_back(makeSemanticRefForSubName(sub, graph, role, linkedFeature));
    }
    return refs;
}

void syncSemanticRefsSize(std::vector<SemanticReference>& refs, std::size_t subCount)
{
    if (refs.size() > subCount) {
        refs.resize(subCount);
        return;
    }
    while (refs.size() < subCount) {
        SemanticReference empty;
        empty.state = ResolutionState::Missing;
        refs.push_back(std::move(empty));
    }
}

void keepValidSeedsForSameIndex(std::vector<SemanticReference>& dest,
                                const std::vector<std::string>& destSubs,
                                const std::vector<SemanticReference>& previous,
                                const std::vector<std::string>& previousSubs)
{
    const std::size_t nDest = dest.size() < destSubs.size() ? dest.size() : destSubs.size();
    const std::size_t nPrev =
        previous.size() < previousSubs.size() ? previous.size() : previousSubs.size();
    for (std::size_t i = 0; i < nDest; ++i) {
        if (dest[i].seed.valid()) {
            continue;
        }
        const ElementIndex want = elementIndexFromSubName(destSubs[i]);
        if (want.index <= 0 || want.type.empty()) {
            continue;
        }
        const SemanticReference* found = nullptr;
        int matches = 0;
        for (std::size_t j = 0; j < nPrev; ++j) {
            if (!previous[j].seed.valid()) {
                continue;
            }
            if (elementIndexFromSubName(previousSubs[j]) != want) {
                continue;
            }
            ++matches;
            found = &previous[j];
        }
        if (matches == 1 && found) {
            dest[i] = *found;
        }
    }
}

void promoteRefsWithGraph(std::vector<SemanticReference>& refs,
                          const std::vector<std::string>& subNames,
                          const SemanticGraph& graph,
                          ObjectId linkedFeature)
{
    syncSemanticRefsSize(refs, subNames.size());
    for (std::size_t i = 0; i < subNames.size(); ++i) {
        if (refs[i].seed.valid()) {
            // C1: never overwrite a valid restored seed. STG1 omits SemanticBinding, so
            // FaceN re-promotion against an empty graph would wipe stSeed.
            continue;
        }
        // A restored stFallback can be the only indexed cache when the live
        // Sub value is a mapped/whole-object name. It is still cache data,
        // not an identity: promotion below requires the same unique live
        // Binding proof as the ordinary Sub-name path.
        std::string promotionSubName = subNames[i];
        if (elementIndexFromSubName(promotionSubName).index <= 0
            && refs[i].fallback.index > 0 && !refs[i].fallback.type.empty()) {
            promotionSubName = refs[i].fallback.toString();
        }
        const SemanticReference promoted =
            makeSemanticRefForSubName(promotionSubName, &graph, refs[i].role, linkedFeature);
        if (promoted.seed.valid()) {
            // XML policy fields remain authoritative even when the seed itself
            // was malformed or absent. Do not let role defaults erase a
            // restored reducer/filter/anchor during C1 promotion.
            const CandidateFilter filter = refs[i].filter;
            const CardinalityReducer reducer = refs[i].reducer;
            const std::optional<std::string> anchor = refs[i].anchor;
            refs[i] = promoted;
            refs[i].filter = filter;
            refs[i].reducer = reducer;
            refs[i].anchor = anchor;
        }
    }
}

bool rewriteFallbackFromBinding(SemanticReference& ref, const SemanticGraph& graph)
{
    if (!ref.seed.valid()) {
        return false;
    }
    // D2: FaceN rewrite only from a live SemanticBinding (after first recompute).
    // On restore STG1 has no SemanticBinding — no-op, do not clobber restored state.
    if (!graph.hasBindings()) {
        return false;
    }
    const ResolutionResult result = SemanticResolver::resolve(ref, graph);
    ref.state = result.state;
    // I9: only a singleton Resolved rewrites the FaceN cache. A set is not a FaceN.
    if (result.state == ResolutionState::Resolved && result.bindings.size() == 1) {
        const ElementIndex next = result.bindings.front().index;
        if (next != ref.fallback) {
            ref.fallback = next;
            return true;
        }
        return false;
    }
    return false;
}

bool semanticSubNameMatchesKind(std::string_view subName, std::string_view kindPrefix)
{
    if (kindPrefix.empty()) {
        return true;
    }
    // Existing GUI convenience: "Edge" matches "Edge13" / "Edge13;:G;XTR".
    if (subName.size() >= kindPrefix.size()
        && subName.substr(0, kindPrefix.size()) == kindPrefix) {
        return true;
    }
    const ElementIndex idx = elementIndexFromSubName(subName);
    if (!idx.type.empty() && idx.type == kindPrefix) {
        return true;
    }
    const SemanticId token = semanticIdFromSubName(subName);
    if (token.valid() && SemanticId::kindMatchesElementType(token.kind, kindPrefix)) {
        return true;
    }
    return false;
}

ResolutionResult resolveSemanticSelection(const SemanticGraph* graph,
                                          ObjectId feature,
                                          std::string_view subName,
                                          const CandidateFilter& filter,
                                          CardinalityReducer reducer)
{
    ResolutionResult out;
    if (!graph) {
        out.state = ResolutionState::Missing;
        return out;
    }
    SemanticReference ref = makeSemanticRefForSubName(subName, graph, SemanticRole::None, feature);
    ref.filter = filter;
    ref.reducer = reducer;
    return SemanticResolver::resolve(ref, *graph);
}

std::string resolveSubNameFromSeed(const SemanticGraph* graph,
                                   const SemanticId& seed,
                                   ObjectId linkedFeature,
                                   std::string_view fallbackSubName)
{
    std::string fallback(fallbackSubName);
    if (!graph || !seed.valid() || linkedFeature == 0) {
        return fallback;
    }
    const ElementIndex parsed = elementIndexFromSubName(fallback);
    // The durable seed kind is authoritative. A stale FaceN/EdgeN cache can
    // have the wrong kind after a restore or property transform; using it to
    // choose the Binding table would let a valid seed consume another kind.
    // Kind→IndexedName type via semanticKindName (AG29-D1) — lockstep with
    // tryResolveSubNameFromSeed and kindMatchesElementType. Region Bindings
    // use index.type "Region" (S4-R1); do not fall through to Face via a
    // stale FaceN cache (I13). Soft path only: unknown kind still falls back
    // to cache then Face (I7 always-fallback); strict tryResolve refuses.
    const char* kindType = semanticKindName(seed.kind);
    std::string type;
    if (kindType && kindType[0] != '\0' && std::strcmp(kindType, "Unknown") != 0) {
        type = kindType;
    }
    if (type.empty()) {
        type = parsed.type;
        if (type.empty()) {
            type = "Face";
        }
    }
    const std::optional<SemanticBinding> unique =
        uniqueBindingOnFeature(graph, seed, linkedFeature, type.c_str());
    if (!unique.has_value() || unique->index.index <= 0) {
        return fallback;  // I7 / I10: 0 or >1 stays the cache; no mint
    }
    const std::string next = unique->index.toString();
    if (fallback.empty() || parsed.type.empty()) {
        return next;
    }
    const std::string oldIdx = parsed.toString();
    if (fallback == oldIdx) {
        return next;
    }
    if (fallback.size() >= oldIdx.size()
        && fallback.compare(fallback.size() - oldIdx.size(), oldIdx.size(), oldIdx) == 0) {
        return fallback.substr(0, fallback.size() - oldIdx.size()) + next;
    }
    return next;
}


std::optional<std::string>
tryResolveSubNameFromSeed(const SemanticGraph* graph,
                          const SemanticReference& ref,
                          ObjectId linkedFeature,
                          std::string_view fallbackSubName,
                          std::optional<SemanticKind> forceKind)
{
    std::string fallback(fallbackSubName);
    if (!ref.seed.valid()) {
        return fallback;
    }
    if (linkedFeature == 0) {
        return std::nullopt;
    }
    // Restore window: no live Bindings yet → keep FaceN/EdgeN cache. Once the
    // graph is live, fail closed (nullopt) so a stale cache cannot look resolved.
    if (!graph || !graph->hasBindings()) {
        return fallback;
    }
    const SemanticKind kind = forceKind.has_value() ? *forceKind : ref.kind;
    // AG29-D1: same semanticKindName table as soft resolveSubNameFromSeed.
    // Strict path refuses Unknown / empty (no Face invent — I13).
    const char* expectedType = semanticKindName(kind);
    if (!expectedType || expectedType[0] == '\0'
        || std::strcmp(expectedType, "Unknown") == 0) {
        return std::nullopt;
    }
    ReferenceRequirement requirement;
    requirement.expectedKind = kind;
    requirement.acceptedCardinality = AcceptedCardinality::One;
    const ResolutionResult result = SemanticResolver::resolve(ref, *graph, &requirement);
    if (result.state != ResolutionState::Resolved || result.bindings.size() != 1
        || result.bindings.front().feature != linkedFeature
        || result.bindings.front().index.type != expectedType
        || result.bindings.front().index.index <= 0) {
        return std::nullopt;
    }
    const ElementIndex next = result.bindings.front().index;
    const ElementIndex have = elementIndexFromSubName(fallback);
    const std::string oldIndex = have.toString();
    if (oldIndex.empty() || fallback == oldIndex) {
        return next.toString();
    }
    if (fallback.size() >= oldIndex.size()
        && fallback.compare(fallback.size() - oldIndex.size(), oldIndex.size(), oldIndex) == 0) {
        return fallback.substr(0, fallback.size() - oldIndex.size()) + next.toString();
    }
    return next.toString();
}

std::string semanticRefXmlAttributes(const SemanticReference& ref)
{
    if (!ref.seed.valid()) {
        return {};
    }
    std::ostringstream ss;
    ss << ' ' << SemanticLinkXml::Seed << "=\"" << std::hex << ref.seed.handle << std::dec << '"';
    ss << ' ' << SemanticLinkXml::Kind << "=\"" << SemanticId::kindChar(ref.kind) << '"';
    ss << ' ' << SemanticLinkXml::Role << "=\"" << static_cast<unsigned>(ref.role) << '"';
    ss << ' ' << SemanticLinkXml::Filter << "=\""
       << static_cast<unsigned>(ref.filter.flags) << '"';
    ss << ' ' << SemanticLinkXml::Reducer << "=\""
       << static_cast<unsigned>(ref.reducer) << '"';
    // I7 FaceN cache next to the seed.
    const std::string cache = dualWriteSubName(ref);
    if (!cache.empty()) {
        ss << ' ' << SemanticLinkXml::Fallback << "=\"" << xmlEscape(cache) << '"';
    }
    if (ref.anchor && !ref.anchor->empty()) {
        ss << ' ' << SemanticLinkXml::Anchor << "=\"" << xmlEscape(*ref.anchor) << '"';
    }
    if (ref.filter.sameGenerator != 0) {
        ss << ' ' << SemanticLinkXml::SameGenerator << "=\"" << ref.filter.sameGenerator << '"';
    }
    if (ref.filter.sameRole != SemanticRole::None) {
        ss << ' ' << SemanticLinkXml::SameRole << "=\""
           << static_cast<unsigned>(ref.filter.sameRole) << '"';
    }
    if (ref.filter.sameInstance != 0) {
        ss << ' ' << SemanticLinkXml::SameInstance << "=\"" << ref.filter.sameInstance << '"';
    }
    return ss.str();
}

SemanticReference semanticRefFromXmlFields(const SemanticRefXmlFields& fields)
{
    SemanticReference ref;
    std::optional<SemanticHandle> handle;
    if (!fields.seed.empty()) {
        // D3: the durable handle is hexadecimal, but XML input must be
        // consumed losslessly. Do not accept a valid prefix such as
        // "2a-garbage" or a zero handle and silently turn it into a seed.
        handle = parseSemanticHandle(fields.seed);
    }
    std::optional<SemanticKind> kind;
    if (!fields.kind.empty()) {
        // Exact one mapped kind letter. Reject "FX", "Face", and unknown
        // letters — kindFromChar would invent Face for garbage.
        if (fields.kind.size() == 1) {
            kind = SemanticId::tryKindFromChar(fields.kind.front());
        }
    }
    // Seed requires both a valid handle and a durable kind. Missing or
    // malformed kind must not default to Face (I13 / no FaceN-as-identity).
    if (handle && kind) {
        ref.seed.handle = *handle;
        ref.seed.kind = *kind;
        ref.kind = *kind;
    }
    if (!fields.role.empty()) {
        ref.role = static_cast<SemanticRole>(parseUnsigned(fields.role));
    }
    if (!fields.filter.empty()) {
        ref.filter.flags = static_cast<FilterFlag>(parseUnsigned(fields.filter));
    }
    if (!fields.reducer.empty()) {
        ref.reducer = static_cast<CardinalityReducer>(parseUnsigned(fields.reducer));
    }
    if (!fields.fallback.empty()) {
        ref.fallback = ElementIndex::fromString(fields.fallback);
    }
    if (!fields.anchor.empty()) {
        ref.anchor = fields.anchor;
    }
    if (!fields.sameGenerator.empty()) {
        ref.filter.sameGenerator = static_cast<ObjectId>(parseUnsigned(fields.sameGenerator));
    }
    if (!fields.sameRole.empty()) {
        ref.filter.sameRole = static_cast<SemanticRole>(parseUnsigned(fields.sameRole));
    }
    if (!fields.sameInstance.empty()) {
        ref.filter.sameInstance = parseUnsigned(fields.sameInstance);
    }
    // I4: restored filter/reducer are authority. Do not re-apply role defaults.
    if (ref.seed.valid()) {
        ref.state = ResolutionState::Resolved;
    }
    else if (!ref.fallback.type.empty() && ref.fallback.index > 0) {
        ref.state = ResolutionState::Missing;
    }
    return ref;
}


SemanticReference semanticRefFromXmlReader(Base::XMLReader& reader)
{
    // D1/D3: optional SemanticReference attributes. Absent = pre-migration file.
    // Do not mint a seed from any FaceN/EdgeN text on the element (I13 / D3b).
    if (reader.hasAttribute(SemanticLinkXml::Seed)) {
        SemanticRefXmlFields fields;
        fields.seed = reader.getAttribute<const char*>(SemanticLinkXml::Seed);
        if (reader.hasAttribute(SemanticLinkXml::Kind)) {
            fields.kind = reader.getAttribute<const char*>(SemanticLinkXml::Kind);
        }
        if (reader.hasAttribute(SemanticLinkXml::Role)) {
            fields.role = reader.getAttribute<const char*>(SemanticLinkXml::Role);
        }
        if (reader.hasAttribute(SemanticLinkXml::Filter)) {
            fields.filter = reader.getAttribute<const char*>(SemanticLinkXml::Filter);
        }
        if (reader.hasAttribute(SemanticLinkXml::Reducer)) {
            fields.reducer = reader.getAttribute<const char*>(SemanticLinkXml::Reducer);
        }
        if (reader.hasAttribute(SemanticLinkXml::Fallback)) {
            fields.fallback = reader.getAttribute<const char*>(SemanticLinkXml::Fallback);
        }
        if (reader.hasAttribute(SemanticLinkXml::Anchor)) {
            fields.anchor = reader.getAttribute<const char*>(SemanticLinkXml::Anchor);
        }
        if (reader.hasAttribute(SemanticLinkXml::SameGenerator)) {
            fields.sameGenerator =
                reader.getAttribute<const char*>(SemanticLinkXml::SameGenerator);
        }
        if (reader.hasAttribute(SemanticLinkXml::SameRole)) {
            fields.sameRole = reader.getAttribute<const char*>(SemanticLinkXml::SameRole);
        }
        if (reader.hasAttribute(SemanticLinkXml::SameInstance)) {
            fields.sameInstance =
                reader.getAttribute<const char*>(SemanticLinkXml::SameInstance);
        }
        return semanticRefFromXmlFields(fields);
    }
    if (reader.hasAttribute(SemanticLinkXml::Fallback)) {
        SemanticRefXmlFields fields;
        fields.fallback = reader.getAttribute<const char*>(SemanticLinkXml::Fallback);
        return semanticRefFromXmlFields(fields);
    }
    return {};
}

}  // namespace App
