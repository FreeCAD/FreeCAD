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
#include <boost/regex.hpp>
#include <cassert>
#include <sstream>
#include <string>
#endif

#include <Base/Exception.h>
#include "Range.h"


using namespace App;

const int App::CellAddress::MAX_ROWS = 16384;
const int App::CellAddress::MAX_COLUMNS = 26 * 26 + 26;

Range::Range(const char * range, bool normalize)
{
    std::string from;
    std::string to;

    assert(range != nullptr);

    if (strchr(range, ':') == nullptr) {
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

    if (normalize)
        this->normalize();
    row_curr = row_begin;
    col_curr = col_begin;
}

Range::Range(int _row_begin, int _col_begin, int _row_end, int _col_end, bool normalize)
    : row_begin(_row_begin)
    , col_begin(_col_begin)
    , row_end(_row_end)
    , col_end(_col_end)
{
    if (normalize)
        this->normalize();
    row_curr = row_begin;
    col_curr = col_begin;
}

Range::Range(const CellAddress &from, const CellAddress &to, bool normalize)
    : row_begin(from.row())
    , col_begin(from.col())
    , row_end(to.row())
    , col_end(to.col())
{
    if (normalize)
        this->normalize();
    row_curr = row_begin;
    col_curr = col_begin;
}

void Range::normalize()
{
    if (row_begin > row_end)
        std::swap(row_begin, row_end);
    if (col_begin > col_end)
        std::swap(col_begin, col_end);
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
  * @brief Decode a row specification into a 0-based integer.
  *
  * @param rowstr Row specified as a string, with "1" being the first row.
  *
  * @returns The row.
  */

int App::decodeRow(const std::string &rowstr, bool silent)
{
    int row = validRow(rowstr);

    if (silent || row >= 0)
        return row;
    else
        throw Base::IndexError("Invalid row specification.");
}

/**
  * @brief Decode a column specification into a 0-based integer.
  *
  * @param colstr Column specified as a string, with "A" begin the first column.
  *
  * @returns The column.
  *
  */

int App::decodeColumn(const std::string &colstr, bool silent)
{
    int col = validColumn(colstr);

    if (silent || col >= 0)
        return col;
    else
        throw Base::IndexError("Invalid column specification");
}

/**
  * @brief Determine whether a row specification is valid or not.
  *
  * @param rowstr Row specified as a string, with "1" being the first row.
  *
  * @returns 0 or positive on success, -1 on error.
  */

int App::validRow(const std::string &rowstr)
{
    char * end;
    int i = strtol(rowstr.c_str(), &end, 10);

    if (i <=0 || i > CellAddress::MAX_ROWS || *end)
        return -1;

    return i - 1;
}

/**
  * @brief Determine whether a column specification is valid or not.
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
        for (std::string::const_iterator i = colstr.begin(); i != colstr.end(); ++i) {
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
  * @brief Convert a string address into integer \a row and \a column.
  * row and col are 0-based.
  *
  * This function will throw an exception if the specified \a address is invalid.
  *
  * @param strAddress Address to parse.
  *
  */

App::CellAddress App::stringToAddress(const char * strAddress, bool silent)
{
    assert(strAddress != nullptr);

    static boost::regex e("(\\$?[A-Z]{1,2})(\\$?[0-9]{1,5})");
    boost::cmatch cm;

    if (boost::regex_match(strAddress, cm, e)) {
        bool absCol = (cm[1].first[0]=='$');
        std::string r,c;
        if(absCol)
            c = std::string(cm[1].first+1,cm[1].second);
        else
            c = std::string(cm[1].first,cm[1].second);
        bool absRow = (cm[2].first[0]=='$');
        if(absRow) 
            r = std::string(cm[2].first+1,cm[2].second);
        else
            r = std::string(cm[2].first,cm[2].second);
        return CellAddress(decodeRow(r,silent), decodeColumn(c,silent), absRow, absCol);
    }
    else if(silent)
        return CellAddress();
    else
        throw Base::RuntimeError("Invalid cell specifier.");
}

/**
  * @brief Convert given \a cell address into its string representation.
  *
  * @returns Address given as a string.
  */

std::string App::CellAddress::toString(Cell cell) const
{
    std::stringstream s;

    Base::Flags<Cell> flags(cell);
    if (flags.testFlag(Cell::ShowColumn)) {
        if (_absCol && flags.testFlag(Cell::Absolute))
            s << '$';
        if (col() < 26) {
            s << static_cast<char>('A' + col());
        }
        else {
            int colnum = col() - 26;

            s << static_cast<char>('A' + (colnum / 26));
            s << static_cast<char>('A' + (colnum % 26));
        }
    }

    if (flags.testFlag(Cell::ShowRow)) {
        if (_absRow && flags.testFlag(Cell::Absolute))
            s << '$';
        s << (row() + 1);
    }

    return s.str();
}

bool App::CellAddress::parseAbsoluteAddress(const char *txt) {
    if(txt[0]=='$' || (txt[0] && txt[1] && (txt[1]=='$' || txt[2]=='$'))) {
        CellAddress addr = stringToAddress(txt,true);
        if(addr.isValid()) {
            *this = addr;
            return true;
        }
    }
    return false;
}

