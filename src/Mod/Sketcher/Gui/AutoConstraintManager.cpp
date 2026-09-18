// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak                                 *
 *   <39885728+Reqrefusion@users.noreply.github.com>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <algorithm>
#include <array>
#include <string>

#include <App/Application.h>

#include "AutoConstraintManager.h"

namespace SketcherGui::AutoConstraintManager
{
namespace Detail
{

constexpr char parameterPath[] = "User parameter:BaseApp/Preferences/Mod/Sketcher/AutoConstraints";
constexpr char parameterName[] = "autoConstraintModes";
constexpr char defaultModes[] = "11111111";
constexpr std::array constraintTypes {
    Sketcher::Coincident,
    Sketcher::PointOnObject,
    Sketcher::Horizontal,
    Sketcher::Vertical,
    Sketcher::Parallel,
    Sketcher::Perpendicular,
    Sketcher::Tangent,
    Sketcher::Symmetric,
};

static_assert(constraintTypes.size() == static_cast<std::size_t>(Mode::Count));

ParameterGrp::handle getParameters()
{
    return App::GetApplication().GetParameterGroupByPath(parameterPath);
}

std::string getNormalizedModes()
{
    std::string modes = getParameters()->GetASCII(parameterName, defaultModes);
    modes.resize(constraintTypes.size(), '1');

    for (char& mode : modes) {
        if (mode != '0' && mode != '1') {
            mode = '1';
        }
    }

    return modes;
}

}  // namespace Detail

bool isModeActive(Mode mode)
{
    const auto index = static_cast<std::size_t>(mode);
    if (index >= Detail::constraintTypes.size()) {
        return true;
    }

    return Detail::getNormalizedModes()[index] == '1';
}

void setModeActive(Mode mode, bool active)
{
    const auto index = static_cast<std::size_t>(mode);
    if (index >= Detail::constraintTypes.size()) {
        return;
    }

    std::string modes = Detail::getNormalizedModes();
    modes[index] = active ? '1' : '0';
    Detail::getParameters()->SetASCII(Detail::parameterName, modes.c_str());
}

void toggleMode(Mode mode)
{
    setModeActive(mode, !isModeActive(mode));
}

bool isConstraintActive(Sketcher::ConstraintType type)
{
    const auto typePosition = std::ranges::find(Detail::constraintTypes, type);
    if (typePosition == Detail::constraintTypes.end()) {
        return true;
    }

    const auto index = static_cast<std::size_t>(typePosition - Detail::constraintTypes.begin());
    return isModeActive(static_cast<Mode>(index));
}

}  // namespace SketcherGui::AutoConstraintManager
