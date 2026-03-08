// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <FCConfig.h>

#include <algorithm>
#include <array>
#include <string>

/**
 * @brief A namespace for utilities.
 *
 * Currently, this namespace contains functionality to define custom syntax to
 * parse command line options.
 */
namespace App::Util
{

std::pair<std::string, std::string> customSyntax(std::string_view strIn)
{
    if (strIn.size() < 2) {
        return {};
    }

    char leadChr {strIn[0]};
    std::string rest {strIn.substr(1)};

    if (leadChr == '@') {
        return {"response-file", rest};
    }

    if (leadChr != '-') {
        return {};
    }

#if defined(FC_OS_MACOSX)
    if (rest.find("psn_") == 0) {
        return {"psn", rest.substr(4)};
    }
#endif

    if (rest == "widgetcount") {
        return {rest, ""};
    }

    constexpr std::array knowns {"display",
                                 "style",
                                 "graphicssystem",
                                 "geometry",
                                 "font",
                                 "fn",
                                 "background",
                                 "bg",
                                 "foreground",
                                 "fg",
                                 "button",
                                 "btn",
                                 "name",
                                 "title",
                                 "visual"};

    if (std::ranges::find(knowns, rest) != knowns.end()) {
        return {rest, "null"};
    }
    return {};
}

}  // namespace App::Util
