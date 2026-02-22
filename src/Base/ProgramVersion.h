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

#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <FCGlobal.h>

namespace Base
{

enum class Version : std::uint8_t
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

inline Version getVersion(std::string_view str)
{
    // clang-format off
    struct VersionItem
    {
        std::string_view name;
        Version version;
    };
    static const std::initializer_list<VersionItem> items = {
        {.name="0.16", .version=Version::v0_16},
        {.name="0.17", .version=Version::v0_17},
        {.name="0.18", .version=Version::v0_18},
        {.name="0.19", .version=Version::v0_19},
        {.name="0.20", .version=Version::v0_20},
        {.name="0.21", .version=Version::v0_21},
        {.name="0.22", .version=Version::v0_22},
        {.name="1.0" , .version=Version::v1_0 },
        {.name="1.1" , .version=Version::v1_1 },
    };
    // clang-format on
    auto it = std::ranges::find_if(items, [str](const auto& item) {
        return str.compare(0, item.name.size(), item.name) == 0;
    });
    if (it != items.end()) {
        return it->version;
    }
    if (!str.empty() && str[0] == '0') {
        return Version::v0_1x;
    }
    return Version::v1_x;
}

}  // namespace Base
