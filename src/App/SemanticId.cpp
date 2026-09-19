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

#include "SemanticId.h"

#include <cctype>
#include <optional>
#include <sstream>

#ifndef SEMANTIC_TOPOLOGY_STANDALONE
#include "ElementNamingUtils.h"
#endif

namespace App
{

#ifndef SEMANTIC_TOPOLOGY_STANDALONE
namespace
{
constexpr const char* semanticPrefix()
{
    return Data::POSTFIX_SEMANTIC;
}
}  // namespace
#else
namespace
{
constexpr const char* semanticPrefix()
{
    return ";:ST";
}
}  // namespace
#endif

const char* SemanticId::mappedTokenPrefix()
{
    return semanticPrefix();
}

char SemanticId::kindChar(SemanticKind kind)
{
    switch (kind) {
        case SemanticKind::Vertex:
            return 'V';
        case SemanticKind::Edge:
            return 'E';
        case SemanticKind::Wire:
            return 'W';
        case SemanticKind::Face:
            return 'F';
        case SemanticKind::Shell:
            return 'S';
        case SemanticKind::Solid:
            return 'O';
        case SemanticKind::Region:
            return 'R';
    }
    return '?';
}

bool SemanticId::isMappedKindChar(char c)
{
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) {
        case 'V':
        case 'E':
        case 'W':
        case 'F':
        case 'S':
        case 'O':
        case 'R':
            return true;
        default:
            return false;
    }
}

std::optional<SemanticKind> SemanticId::tryKindFromChar(char c)
{
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) {
        case 'V':
            return SemanticKind::Vertex;
        case 'E':
            return SemanticKind::Edge;
        case 'W':
            return SemanticKind::Wire;
        case 'F':
            return SemanticKind::Face;
        case 'S':
            return SemanticKind::Shell;
        case 'O':
            return SemanticKind::Solid;
        case 'R':
            return SemanticKind::Region;
        default:
            return std::nullopt;
    }
}

SemanticKind SemanticId::kindFromChar(char c)
{
    if (const auto kind = tryKindFromChar(c)) {
        return *kind;
    }
    // Legacy callers only. Restore/XML paths must use tryKindFromChar.
    return SemanticKind::Face;
}

bool SemanticId::kindMatchesElementType(SemanticKind kind, std::string_view elementType)
{
    // Face UX also admits Region seeds (caps / profile regions present as Face*).
    // Exact Wire/Shell/Solid/Region prefixes match only their own kind — used by
    // semanticSubNameMatchesKind for ;:ST tokens under SelectionFilter gates.
    if (elementType == "Face") {
        return kind == SemanticKind::Face || kind == SemanticKind::Region;
    }
    if (elementType == "Edge") {
        return kind == SemanticKind::Edge;
    }
    if (elementType == "Vertex") {
        return kind == SemanticKind::Vertex;
    }
    if (elementType == "Wire") {
        return kind == SemanticKind::Wire;
    }
    if (elementType == "Shell") {
        return kind == SemanticKind::Shell;
    }
    if (elementType == "Solid") {
        return kind == SemanticKind::Solid;
    }
    if (elementType == "Region") {
        return kind == SemanticKind::Region;
    }
    return false;
}

std::string SemanticId::toMappedToken() const
{
    std::ostringstream ss;
    ss << semanticPrefix();
    ss << std::hex << handle;
    ss << ':' << kindChar(kind);
    return ss.str();
}

SemanticId SemanticId::fromMappedToken(std::string_view token)
{
    SemanticId id;
    const std::string prefix = semanticPrefix();
    if (token.size() <= prefix.size() || token.substr(0, prefix.size()) != prefix) {
        return id;
    }
    const std::string rest(token.substr(prefix.size()));
    const auto colon = rest.find(':');
    if (colon == std::string::npos || colon == 0 || colon + 2 != rest.size()
        || !SemanticId::isMappedKindChar(rest[colon + 1])) {
        return id;
    }
    const std::string hexPart = rest.substr(0, colon);
    for (const unsigned char c : hexPart) {
        if (!std::isxdigit(c)) {
            return id;
        }
    }
    try {
        std::size_t parsed = 0;
        id.handle = static_cast<SemanticHandle>(std::stoull(hexPart, &parsed, 16));
        if (parsed != hexPart.size() || id.handle == 0) {
            id.handle = 0;
            return id;
        }
    }
    catch (...) {
        id.handle = 0;
        return id;
    }
    // isMappedKindChar already gated the letter; still prefer tryKindFromChar
    // so a future alphabet drift cannot invent Face via kindFromChar (I13 / AG29-K1).
    const auto parsedKind = tryKindFromChar(rest[colon + 1]);
    if (!parsedKind) {
        id.handle = 0;
        return id;
    }
    id.kind = *parsedKind;
    return id;
}

const char* semanticKindName(SemanticKind kind)
{
    // IndexedName Binding type strings (Face/Edge/...). Soft + strict seed
    // resolve (resolveSubNameFromSeed / tryResolveSubNameFromSeed) consume
    // this helper so kind→type stays one table with kindMatchesElementType
    // (AG29-D1). "Unknown" is not a Binding type — callers must refuse or
    // fall back explicitly (never treat as Face identity).
    switch (kind) {
        case SemanticKind::Vertex:
            return "Vertex";
        case SemanticKind::Edge:
            return "Edge";
        case SemanticKind::Wire:
            return "Wire";
        case SemanticKind::Face:
            return "Face";
        case SemanticKind::Shell:
            return "Shell";
        case SemanticKind::Solid:
            return "Solid";
        case SemanticKind::Region:
            return "Region";
    }
    return "Unknown";
}

const char* semanticRoleName(SemanticRole role)
{
    switch (role) {
        case SemanticRole::None:
            return "None";
        case SemanticRole::SupportFace:
            return "SupportFace";
        case SemanticRole::DressUpEdge:
            return "DressUpEdge";
        case SemanticRole::UpToFace:
            return "UpToFace";
        case SemanticRole::ExternalEdge:
            return "ExternalEdge";
        case SemanticRole::PatternAxis:
            return "PatternAxis";
        case SemanticRole::BooleanToolFace:
            return "BooleanToolFace";
        case SemanticRole::AssemblyJointFace:
            return "AssemblyJointFace";
        case SemanticRole::Generator:
            return "Generator";
        case SemanticRole::User:
            return "User";
    }
    return "Unknown";
}

std::string ElementIndex::toString() const
{
    if (index <= 0) {
        return type;
    }
    return type + std::to_string(index);
}

ElementIndex ElementIndex::fromString(std::string_view text)
{
    ElementIndex out;
    std::size_t i = text.size();
    while (i > 0 && std::isdigit(static_cast<unsigned char>(text[i - 1]))) {
        --i;
    }
    out.type = std::string(text.substr(0, i));
    if (i < text.size()) {
        try {
            out.index = std::stoi(std::string(text.substr(i)));
        }
        catch (...) {
            out.index = 0;
        }
    }
    return out;
}

}  // namespace App
