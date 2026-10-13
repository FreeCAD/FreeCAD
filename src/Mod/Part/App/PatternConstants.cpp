// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <algorithm>

#include <App/Application.h>

#include "PatternConstants.h"

namespace Part::PatternConstants
{

const App::PropertyIntegerConstraint::Constraints* occurrenceConstraints()
{
    static const App::PropertyIntegerConstraint::Constraints constraints {
        1,
        std::max(
            1L,
            App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Part")
                ->GetInt("MaximumPatternOccurrences", 1000)
        ),
        1
    };
    return &constraints;
}

}  // namespace Part::PatternConstants
