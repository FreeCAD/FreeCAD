// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

// NOLINTNEXTLINE
#include "PreCompiled.h"

#ifndef _PreComp_
# include <cstdlib>
# include <unordered_set>
#endif

#include "IndexedName.h"

using namespace Data;

/// Check whether the input character is an underscore or an ASCII letter a-Z or A-Z
inline bool isInvalidChar(char test)
{
    return test != '_' && (test < 'a' || test > 'z' ) && (test < 'A' || test > 'Z');
}

/// Get the integer suffix of name. Returns a tuple of (suffix, suffixPosition). Calling code
/// should check to ensure that suffixPosition is not equal to nameLength (in which case there was no
/// suffix).
///
/// \param name The name to check
/// \param nameLength The length of the string in name
/// \returns An integer pair of the suffix itself and the position of that suffix in name
std::pair<int,int> getIntegerSuffix(const char *name, int nameLength)
{
    int suffixPosition {nameLength - 1};

    for (; suffixPosition >= 0; --suffixPosition) {
        // When we support C++20 we can use std::span<> to eliminate the clang-tidy warning
        // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
        if (!isdigit(name[suffixPosition])) {
            break;
        }
    }
    ++suffixPosition;
    int suffix {0};
    if (suffixPosition < nameLength) {
        // When we support C++20 we can use std::span<> to eliminate the clang-tidy warning
        // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
        suffix = std::atoi(name + suffixPosition);
    }
    return std::make_pair(suffix, suffixPosition);
}

void IndexedName::set(
    const char* name,
    int length,
    const std::vector<const char*>& allowedNames,
    bool allowOthers)
{
    // Storage for names that we weren't given external storage for
    static std::unordered_set<ByteArray, ByteArrayHasher> NameSet;

    if (length < 0) {
        length = static_cast<int>(std::strlen(name));
    }
    // Name typically ends with an integer: find that integer
    auto [suffix, suffixPosition] = getIntegerSuffix(name, length);
    if (suffixPosition < length) {
        this->index = suffix;
    }

    // Make sure that every character is either an ASCII letter (upper or lowercase), or an
    // underscore. If any other character appears, reject the entire string.
    // When we support C++20 we can use std::span<> to eliminate the clang-tidy warning
    // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
    if (std::any_of(name, name+suffixPosition, isInvalidChar)) {
        this->type = "";
        return;
    }

    // If a list of allowedNames was provided, see if our set name matches one of those allowedNames: if it
    // does, reference that memory location and return.
    for (const auto *typeName : allowedNames) {
        if (std::strncmp(name, typeName, suffixPosition) == 0) {
            this->type = typeName;
            return;
        }
    }

    // If the type was NOT in the list of allowedNames, but the caller has set the allowOthers flag to
    // true, then add the new type to the static NameSet (if it is not already there).
    if (allowOthers) {
        auto res = NameSet.insert(ByteArray(QByteArray::fromRawData(name, suffixPosition)));
        if (res.second /*The insert succeeded (the type was new)*/) {
            // Make sure that the data in the set is a unique (unshared) copy of the text
            res.first->ensureUnshared();
        }
        this->type = res.first->bytes.constData();
    }
    else {
        // The passed-in type is not in the allowed list, and allowOthers was not true, so don't
        // store the type
        this->type = "";
    }
}
