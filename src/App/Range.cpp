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
#include "Range.h"
#include <Base/Exception.h>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <boost/regex.hpp>

using namespace App;

const int App::CellAddress::MAX_ROWS = 16384;
const int App::CellAddress::MAX_COLUMNS = 26 * 26 + 26;

Range::Range(const char * range)
{
    std::string from;
    std::string to;

    assert(range != NULL);

    if (strchr(range, ':') == NULL) {
        from = range;
        to = range;
    }
    else {
        std::string s = range;
        from = s.substr(0, s.find(':'));
        to = s.substr(s.find(':') + 1);
    }

    CellAddress begin(from);
    CellAddress end(to);

    row_begin = begin.row();
    col_begin = begin.col();
    row_end = end.row();
    col_end = end.col();

    row_curr = row_begin;
    col_curr = col_begin;
}

Range::Range(int _row_begin, int _col_begin, int _row_end, int _col_end)
    : row_curr(_row_begin)
    , col_curr(_col_begin)
    , row_begin(_row_begin)
    , col_begin(_col_begin)
    , row_end(_row_end)
    , col_end(_col_end)
{
}

Range::Range(const CellAddress &from, const CellAddress &to)
    : row_curr(from.row())
    , col_curr(from.col())
    , row_begin(from.row())
    , col_begin(from.col())
    , row_end(to.row())
    , col_end(to.col())
{
}

bool Range::next()
{
    if (row_curr < row_end) {
        row_curr++;

        return true;
    }
    if (col_curr < col_end) {
        if (row_curr == row_end + 1)
            return false;
        row_curr = row_begin;
        ++col_curr;
        return true;
    }
    return false;
}

/**
  * Decode a row specification into a 0-based integer.
  *
  * @param rowstr Row specified as a string, with "1" being the first row.
  *
  * @returns The row.
  */

int App::decodeRow(const std::string &rowstr)
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

int App::decodeColumn(const std::string &colstr)
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

int App::validRow(const std::string &rowstr)
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

int App::validColumn(const std::string &colstr)
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

App::CellAddress App::stringToAddress(const char * strAddress)
{
    static const boost::regex e("\\${0,1}([A-Z]{1,2})\\${0,1}([0-9]{1,5})");
    boost::cmatch cm;

    assert(strAddress != 0);

    if (boost::regex_match(strAddress, cm, e)) {
        const boost::sub_match<const char *> colstr = cm[1];
        const boost::sub_match<const char *> rowstr = cm[2];

        return CellAddress(decodeRow(rowstr.str()), decodeColumn(colstr.str()));
    }
    else
        throw Base::Exception("Invalid cell specifier.");
}

/**
  * Convert given \a cell address into its string representation.
  *
  * @returns Address given as a string.
  */

std::string App::CellAddress::toString() const
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
