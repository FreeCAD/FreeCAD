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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <sstream>
#endif

#include "Sheet.h"
#include "Utils.h"


/**
 * Encode \a col as a string.
 *
 * @param col Column given as a 0-based column position.
 *
 * @returns String with column position, with "A" being the first column, "B" being the second and
 * so on.
 *
 */

std::string Spreadsheet::columnName(int col)
{
    std::stringstream s;

    if (col < 26) {
        s << ((char)('A' + col));
    }
    else {
        s << ((char)('A' + (col - 26) / 26)) << ((char)('A' + (col - 26) % 26));
    }

    return s.str();
}

/**
 * Encode \a row as a string.
 *
 * @param row Row given as a 0-based row position.
 *
 * @returns String with row position, with "1" being the first row.
 *
 */

std::string Spreadsheet::rowName(int row)
{
    std::stringstream s;

    s << (row + 1);

    return s.str();
}


void Spreadsheet::createRectangles(std::set<std::pair<int, int>>& cells,
                                   std::map<std::pair<int, int>, std::pair<int, int>>& rectangles)
{
    while (!cells.empty()) {
        int row, col;
        int orgRow;
        int rows = 1;
        int cols = 1;

        orgRow = row = (*cells.begin()).first;
        col = (*cells.begin()).second;

        // Expand right first
        while (cells.find(std::make_pair(row, col + cols)) != cells.end()) {
            ++cols;
        }

        // Expand left
        while (cells.find(std::make_pair(row, col + cols)) != cells.end()) {
            col--;
            ++cols;
        }

        // Try to expand cell up (the complete row above from [col,col + cols> needs to be in the
        // cells variable)
        bool ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if (cells.find(std::make_pair(row - 1, i)) == cells.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                // Complete row
                row--;
                rows++;
            }
            else {
                break;
            }
        }

        // Try to expand down (the complete row below from [col,col + cols> needs to be in the cells
        // variable)
        ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if (cells.find(std::make_pair(orgRow + 1, i)) == cells.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                // Complete row
                orgRow++;
                rows++;
            }
            else {
                break;
            }
        }

        // Remove entries from cell set for this rectangle
        for (int r = row; r < row + rows; ++r) {
            for (int c = col; c < col + cols; ++c) {
                cells.erase(std::make_pair(r, c));
            }
        }

        // Insert into output variable
        rectangles[std::make_pair(row, col)] = std::make_pair(rows, cols);
    }
}

std::string Spreadsheet::quote(const std::string& input)
{
    std::stringstream output;

    std::string::const_iterator cur = input.begin();
    std::string::const_iterator end = input.end();

    output << "<<";
    while (cur != end) {
        switch (*cur) {
            case '\t':
                output << "\\t";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\\':
                output << "\\\\";
                break;
            case '\'':
                output << "\\'";
                break;
            case '"':
                output << "\\\"";
                break;
            case '>':
                output << "\\>";
                break;
            default:
                output << *cur;
        }
        ++cur;
    }
    output << ">>";

    return output.str();
}

std::string Spreadsheet::unquote(const std::string& input)
{
    assert(input.size() >= 4);

    std::string output;
    std::string::const_iterator cur = input.begin() + 2;
    std::string::const_iterator end = input.end() - 2;

    output.reserve(input.size());

    bool escaped = false;
    while (cur != end) {
        if (escaped) {
            switch (*cur) {
                case 't':
                    output += '\t';
                    break;
                case 'n':
                    output += '\n';
                    break;
                case 'r':
                    output += '\r';
                    break;
                case '\\':
                    output += '\\';
                    break;
                case '\'':
                    output += '\'';
                    break;
                case '"':
                    output += '"';
                    break;
            }
            escaped = false;
        }
        else {
            if (*cur == '\\') {
                escaped = true;
            }
            else {
                output += *cur;
            }
        }
        ++cur;
    }

    return output;
}
