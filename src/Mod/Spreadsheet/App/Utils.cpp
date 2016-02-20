/***************************************************************************
 *   Copyright (c) Eivind Kvedalen (eivind@kvedalen.name) 2015             *
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
#endif

#include "Utils.h"
#include <sstream>
#include <stdlib.h>
#include <boost/regex.hpp>
#include <Base/Exception.h>
#include "Sheet.h"

const int Spreadsheet::CellAddress::MAX_ROWS = 16384;
const int Spreadsheet::CellAddress::MAX_COLUMNS = 26 * 26 + 26;

/**
  * Encode \a col as a string.
  *
  * @param col Column given as a 0-based column position.
  *
  * @returns String with column position, with "A" being the first column, "B" being the second and so on.
  *
  */

std::string Spreadsheet::columnName(int col)
{
    std::stringstream s;

    if (col < 26)
        s << ((char)('A' + col));
    else
        s << ((char)('A' + (col - 26) / 26 )) << ((char)('A' + (col - 26) % 26));

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

/**
  * Decode a row specification into a 0-based integer.
  *
  * @param rowstr Row specified as a string, with "1" being the first row.
  *
  * @returns The row.
  */

int Spreadsheet::decodeRow(const std::string &rowstr)
{
    int row = validRow(rowstr);

    if (row >= 0)
        return row;
    else
        throw Base::Exception("Invalid row specification.");
}

/**
  * Decode a column specification into a 0-based integer.
  *
  * @param colstr Column specified as a string, with "A" begin the first column.
  *
  * @returns The column.
  *
  */

int Spreadsheet::decodeColumn(const std::string &colstr)
{
    int col = validColumn(colstr);

    if (col >= 0)
        return col;
    else
        throw Base::Exception("Invalid column specification");
}

/**
  * Determine wheter a row specification is valid or not.
  *
  * @param rowstr Row specified as a string, with "1" being the first row.
  *
  * @returns 0 or positive on success, -1 on error.
  */

int Spreadsheet::validRow(const std::string &rowstr)
{
    char * end;
    int i = strtol(rowstr.c_str(), &end, 10);

    if (i <0 || i >= CellAddress::MAX_ROWS || *end)
        return -1;

    return i - 1;
}

/**
  * Determine whether a column specification is valid or not.
  *
  * @param colstr Column specified as a string, with "A" begin the first column.
  *
  * @returns 0 or positive on success, -1 on error.
  *
  */

int Spreadsheet::validColumn(const std::string &colstr)
{
    int col = 0;

    if (colstr.length() == 1) {
        if ((colstr[0] >= 'A' && colstr[0] <= 'Z'))
            col = colstr[0] - 'A';
        else
            return -1;
    }
    else {
        col = 0;
        for (std::string::const_reverse_iterator i = colstr.rbegin(); i != colstr.rend(); ++i) {
            int v;

            if ((*i >= 'A' && *i <= 'Z'))
                v = *i - 'A';
            else
                return -1;

            col = col * 26 + v;
        }
        col += 26;
    }
    return col;
}

/**
  * Convert a string address into integer \a row and \a column.
  * row and col are 0-based.
  *
  * This function will throw an exception if the specified \a address is invalid.
  *
  * @param address Address to parse.
  * @param row     Reference to integer where row position is stored.
  * @param col     Reference to integer where col position is stored.
  *
  */

Spreadsheet::CellAddress Spreadsheet::stringToAddress(const char * strAddress)
{
    static const boost::regex e("\\${0,1}([A-Z]{1,2})\\${0,1}([0-9]{1,5})");
    boost::cmatch cm;

    Q_ASSERT(strAddress != 0);

    if (boost::regex_match(strAddress, cm, e)) {
        const boost::sub_match<const char *> colstr = cm[1];
        const boost::sub_match<const char *> rowstr = cm[2];

        return CellAddress(Spreadsheet::decodeRow(rowstr.str()), Spreadsheet::decodeColumn(colstr.str()));
    }
    else
        throw Base::Exception("Invalid cell specifier.");
}

void Spreadsheet::createRectangles(std::set<std::pair<int, int> > & cells, std::map<std::pair<int, int>, std::pair<int, int> > & rectangles)
{
    while (cells.size() != 0) {
        int row, col;
        int orgRow;
        int rows = 1;
        int cols = 1;

        orgRow = row = (*cells.begin()).first;
        col = (*cells.begin()).second;

        // Expand right first
        while (cells.find(std::make_pair(row, col + cols)) != cells.end())
            ++cols;

        // Expand left
        while (cells.find(std::make_pair(row, col + cols)) != cells.end()) {
            col--;
            ++cols;
        }

        // Try to expand cell up (the complete row above from [col,col + cols> needs to be in the cells variable)
        bool ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if ( cells.find(std::make_pair(row - 1, i)) == cells.end()) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                // Complete row
                row--;
                rows++;
            }
            else
                break;
        }

        // Try to expand down (the complete row below from [col,col + cols> needs to be in the cells variable)
        ok = true;
        while (ok) {
            for (int i = col; i < col + cols; ++i) {
                if ( cells.find(std::make_pair(orgRow + 1, i)) == cells.end()) {
                   ok = false;
                   break;
                }
            }
            if (ok) {
                // Complete row
                orgRow++;
                rows++;
            }
            else
                break;
        }

        // Remove entries from cell set for this rectangle
        for (int r = row; r < row + rows; ++r)
            for (int c = col; c < col + cols; ++c)
                cells.erase(std::make_pair(r, c));

        // Insert into output variable
        rectangles[std::make_pair(row, col)] = std::make_pair(rows, cols);
    }
}

std::string Spreadsheet::quote(const std::string &input)
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

std::string Spreadsheet::unquote(const std::string & input)
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
            if (*cur == '\\')
                escaped = true;
            else
                output += *cur;
        }
        ++cur;
    }

    return output;
}


/**
  * Convert given \a cell address into its string representation.
  *
  * @returns Address given as a string.
  */

std::string Spreadsheet::CellAddress::toString() const
{
    std::stringstream s;

    if (col() < 26)
        s << (char)('A' + col());
    else {
        int colnum = col() - 26;

        s << (char)('A' + (colnum / 26));
        s << (char)('A' + (colnum % 26));
    }

    s << (row() + 1);

    return s.str();
}
