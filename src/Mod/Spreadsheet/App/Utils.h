// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

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
