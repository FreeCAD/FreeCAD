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

/// Rev 3.1 typed identity API over MappedName / ElementMap (I8).
/// A SemanticId is a document-scoped handle, never a FaceN and never a
/// second STID heap. The V2 interchange token is POSTFIX_SEMANTIC (";:ST").

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#ifndef SEMANTIC_TOPOLOGY_STANDALONE
#include <FCGlobal.h>
#else
#ifndef AppExport
#define AppExport
#endif
#endif

namespace App
{

using ObjectId = std::uint64_t;
using EvalSerial = std::uint64_t;
using EventId = std::uint64_t;
using SemanticHandle = std::uint64_t;

/// B-Rep / region kind. FaceN is not a kind.
enum class SemanticKind : std::uint8_t
{
    Vertex = 0,
    Edge,
    Wire,
    Face,
    Shell,
    Solid,
    Region,
};

/// Stored role on a seed or an event. Policy defaults are role-specific.
enum class SemanticRole : std::uint8_t
{
    None = 0,
    SupportFace,
    DressUpEdge,
    UpToFace,
    ExternalEdge,
    PatternAxis,
    BooleanToolFace,
    AssemblyJointFace,
    Generator,  ///< 0-to-1 Generated feature/role marker (not a B-Rep entity)
    User,
};

/// Durable semantic identity. allocatedAtEval is creation only, not lineage.
struct AppExport SemanticId
{
    SemanticHandle handle = 0;
    SemanticKind kind = SemanticKind::Face;
    ObjectId allocatedBy = 0;
    EvalSerial allocatedAtEval = 0;
    SemanticRole allocatedRole = SemanticRole::None;

    constexpr bool valid() const
    {
        return handle != 0;
    }

    constexpr explicit operator bool() const
    {
        return valid();
    }

    constexpr bool operator==(const SemanticId& other) const
    {
        return handle == other.handle;
    }

    constexpr bool operator!=(const SemanticId& other) const
    {
        return handle != other.handle;
    }

    constexpr bool operator<(const SemanticId& other) const
    {
        return handle < other.handle;
    }

    /// V2 interchange encoding: ";:ST" + hex handle + ':' + kind. Not a second heap key.
    std::string toMappedToken() const;

    /// Literal prefix shared by encode/decode/stamp (must match Data::POSTFIX_SEMANTIC).
    static const char* mappedTokenPrefix();

    /// Parse a strict ";:ST<hex>:<kind>" token. Returns an invalid id on failure.
    static SemanticId fromMappedToken(std::string_view token);

    /// Kind letter used in tokens and diagnostics: V E W F S O R
    static char kindChar(SemanticKind kind);
    /// True for the V E W F S O R alphabet used by mapped tokens / XML / STG1.
    static bool isMappedKindChar(char c);
    /// Strict kind parse; nullopt for unknown letters (does not invent Face).
    static std::optional<SemanticKind> tryKindFromChar(char c);
    /// Legacy helper: unknown letters become Face. Prefer tryKindFromChar at
    /// restore/XML boundaries so garbage cannot mint a Face kind by default.
    static SemanticKind kindFromChar(char c);
    /// Face/Region → Face*; Edge/Vertex/Wire/Shell/Solid/Region → exact type. Cross-kind false (Region is not Edge*).
    static bool kindMatchesElementType(SemanticKind kind, std::string_view elementType);
};

/// IndexedName type string for Binding consume ("Face", "Edge", ...).
/// Keep lockstep with kindMatchesElementType / soft+strict resolve (AG29-D1).
AppExport const char* semanticKindName(SemanticKind kind);
AppExport const char* semanticRoleName(SemanticRole role);

/// FaceN lives only on SemanticBinding. This is the IndexedName payload without Qt.
struct AppExport ElementIndex
{
    std::string type;  ///< "Face", "Edge", "Vertex", ...
    int index = 0;

    std::string toString() const;
    static ElementIndex fromString(std::string_view text);

    bool operator==(const ElementIndex& other) const
    {
        return type == other.type && index == other.index;
    }

    bool operator!=(const ElementIndex& other) const
    {
        return !(*this == other);
    }
};

}  // namespace App
