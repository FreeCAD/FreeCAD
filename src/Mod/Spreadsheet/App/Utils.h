// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
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
 

#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <memory>
#include <set>
#include <string>

#include <Mod/Spreadsheet/SpreadsheetGlobal.h>


namespace Spreadsheet
{

SpreadsheetExport std::string columnName(int col);
SpreadsheetExport std::string rowName(int row);

SpreadsheetExport void createRectangles(
    std::set<std::pair<int, int>>& cells,
    std::map<std::pair<int, int>, std::pair<int, int>>& rectangles
);
SpreadsheetExport std::string quote(const std::string& input);
SpreadsheetExport std::string unquote(const std::string& input);

}  // namespace Spreadsheet

#endif  // UTILS_H
