// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef BASE_PROGRAM_VERSION_H
#define BASE_PROGRAM_VERSION_H

#include <algorithm>
#include <array>
#include <map>
#include <string_view>
#include <FCGlobal.h>

namespace Base
{

enum class Version
{
    v0_1x,
    v0_16,
    v0_17,
    v0_18,
    v0_19,
    v0_20,
    v0_21,
    v0_22,
    v1_0,
    v1_1,
    v1_x,
};

Version getVersion(std::string_view str)
{
    // clang-format off
    using VersionItem = std::pair<std::string_view, Version>;
    static constexpr std::array<VersionItem, 9> items = {{
        {"0.16", Version::v0_16},
        {"0.17", Version::v0_17},
        {"0.18", Version::v0_18},
        {"0.19", Version::v0_19},
        {"0.20", Version::v0_20},
        {"0.21", Version::v0_21},
        {"0.22", Version::v0_22},
        {"1.0" , Version::v1_0 },
        {"1.1" , Version::v1_1 },
    }};
    // clang-format on
    auto it = std::find_if(items.begin(), items.end(), [str](const auto& item) {
        return str.compare(0, item.first.size(), item.first) == 0;
    });
    if (it != items.end()) {
        return it->second;
    }
    if (!str.empty() && str[0] == '0') {
        return Version::v0_1x;
    }
    return Version::v1_x;
}

}  // namespace Base

#endif  // BASE_PROGRAM_VERSION_H
