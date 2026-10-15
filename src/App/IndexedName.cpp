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

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string_view>
#include <unordered_set>

#include "IndexedName.h"

using namespace Data;

/// Check whether the input character is an underscore or an ASCII letter a-Z or A-Z
inline bool isInvalidChar(char test)
{
    return test != '_' && (test < 'a' || test > 'z') && (test < 'A' || test > 'Z');
}

/// Get the integer suffix of name. Returns a tuple of (suffix, suffixPosition). Calling code
/// should check to ensure that suffixPosition is not equal to name.size() (in which case there was
/// no suffix).
///
/// \param name The name to check
/// \returns An integer pair of the suffix itself and the position of that suffix in name
std::pair<int, int> getIntegerSuffix(std::string_view name)
{
    const auto it = std::find_if_not(name.rbegin(), name.rend(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });
    const auto suffixPosition = static_cast<int>(name.rend() - it);
    int suffix {0};
    const auto digits = name.substr(suffixPosition);
    std::from_chars(digits.data(), std::to_address(digits.end()), suffix);
    return std::make_pair(suffix, suffixPosition);
}

void IndexedName::set(const char* name,
                      int length,
                      const std::vector<const char*>& allowedNames,
                      bool allowOthers)
{
    // Storage for names that we weren't given external storage for
    static std::unordered_set<std::string> NameSet;

    if (length < 0) {
        length = static_cast<int>(std::strlen(name));
    }
    const std::string_view nameView(name, length);
    // Name typically ends with an integer: find that integer
    auto [suffix, suffixPosition] = getIntegerSuffix(nameView);
    if (suffixPosition < length) {
        this->index = suffix;
    }

    // Make sure that every character is either an ASCII letter (upper or lowercase), or an
    // underscore. If any other character appears, reject the entire string.
    const auto typePart = nameView.substr(0, suffixPosition);
    if (std::ranges::any_of(typePart, isInvalidChar)) {
        this->type = "";
        this->index = 0;
        return;
    }

    // If a list of allowedNames was provided, see if our set name matches one of those
    // allowedNames: if it does, reference that memory location and return.
    for (const auto* typeName : allowedNames) {
        if (std::strncmp(name, typeName, suffixPosition) == 0) {
            this->type = typeName;
            return;
        }
    }

    // If the type was NOT in the list of allowedNames, but the caller has set the allowOthers flag
    // to true, then add the new type to the static NameSet (if it is not already there).
    if (allowOthers) {
        auto res = NameSet.insert(std::string(name, suffixPosition));
        this->type = res.first->c_str();
    }
    else {
        // The passed-in type is not in the allowed list, and allowOthers was not true, so don't
        // store the type
        this->type = "";
    }
}
