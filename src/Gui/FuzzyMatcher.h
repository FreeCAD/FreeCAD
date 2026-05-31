// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 tetektoza
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

#pragma once

#include <QString>

#include <FCGlobal.h>

namespace Gui
{

class GuiExport FuzzyMatcher
{
public:
    /// Case-insensitive substring/subsequence matching; larger scores rank first.
    /// An empty query matches with score zero. This does not perform edit-distance matching.
    static bool match(const QString& searchText, const QString& targetText, int& score);

    /// Equivalent to match() for inputs already normalized with QString::toLower().
    /// Use this with cached search keys to avoid allocating strings for every comparison.
    static bool matchLowercase(
        const QString& lowercaseSearchText,
        const QString& lowercaseTargetText,
        int& score
    );
};

}  // namespace Gui
