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
#include <cmath>
#include <regex>
#endif

#include <string_view>
#include <Base/Exception.h>
#include "Range.h"


using namespace App;

const int App::CellAddress::MAX_ROWS = 16384;
const int App::CellAddress::MAX_COLUMNS = 26 * 26 + 26;

namespace App {
// From a given cell address the '$' must be at within the first
// few characters
bool maybeAbsolute(std::string_view address)
{
    const int MAX_COLUMNS_LETTERS = 2;

    address = address.substr(0, MAX_COLUMNS_LETTERS + 1);
    return address.find("$") != std::string_view::npos;
}
}

Range::Range(const char * range, bool normalize)
{
    std::string from;
    std::string to;

    assert(range);

    if (!strchr(range, ':')) {
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

    if (normalize) {
        this->normalize();
    }
    row_curr = row_begin;
    col_curr = col_begin;
}

Range::Range(int _row_begin, int _col_begin, int _row_end, int _col_end, bool normalize)
    : row_begin(_row_begin)
    , col_begin(_col_begin)
    , row_end(_row_end)
    , col_end(_col_end)
{
    if (normalize) {
        this->normalize();
    }
    row_curr = row_begin;
    col_curr = col_begin;
}

Range::Range(const CellAddress &from, const CellAddress &to, bool normalize)
    : row_begin(from.row())
    , col_begin(from.col())
    , row_end(to.row())
    , col_end(to.col())
{
    if (normalize) {
        this->normalize();
    }
    row_curr = row_begin;
    col_curr = col_begin;
}

void Range::normalize()
{
    if (row_begin > row_end) {
        std::swap(row_begin, row_end);
    }
    if (col_begin > col_end) {
        std::swap(col_begin, col_end);
    }
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

    if (silent || row >= 0) {
        return row;
    }

    throw Base::IndexError("Invalid row specification.");
}

/**
 * Assumes well-formed input. A through ZZZ. 0-based output
 */
int columnStringToNum(const std::string &colstr) {
    double out {0};
    int pos {0};
    for (auto chr = colstr.crbegin(); chr != colstr.crend(); chr++){
        out += (*chr - 'A' + 1) * std::pow(26, pos++);
    }

    return static_cast<int>(out - 1);
}

/**
  * @brief Decode a column name string into a 0-based integer.
  *
  * @param colstr input string.
  *
  * @returns The column.
  *
  */

int App::decodeColumn( const std::string &colstr, bool silent )
{
    if (validColumn(colstr)) {
        return columnStringToNum(colstr);
    }

    if (silent) {
        return -1;
    }

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

    if (i <=0 || i > CellAddress::MAX_ROWS || *end) {
        return -1;
    }

    return i - 1;
}

/**
  * @brief Determine if a string is a valid column specification.
  *
  * @param colstr input string.
  *
  * @returns true if valid, false if not.
  *
  */

bool App::validColumn( const std::string &colstr )
{
    return boost::regex_match(colstr, boost::regex("[A-Z]{1,3}"));
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
    assert(strAddress);

    static boost::regex e("(\\$?[A-Z]{1,2})(\\$?[0-9]{1,5})");
    boost::cmatch cm;

    if (boost::regex_match(strAddress, cm, e)) {
        bool absCol = (cm[1].first[0]=='$');
        std::string r,c;
        if (absCol) {
            c = std::string(cm[1].first+1,cm[1].second);
        }
        else {
            c = std::string(cm[1].first,cm[1].second);
        }

        bool absRow = (cm[2].first[0]=='$');
        if (absRow) {
            r = std::string(cm[2].first+1,cm[2].second);
        }
        else {
            r = std::string(cm[2].first,cm[2].second);
        }

        return CellAddress(decodeRow(r,silent), decodeColumn(c,silent), absRow, absCol);
    }
    else if (silent) {
        return CellAddress();
    }

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
        if (_absCol && flags.testFlag(Cell::Absolute)) {
            s << '$';
        }
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
        if (_absRow && flags.testFlag(Cell::Absolute)) {
            s << '$';
        }
        s << (row() + 1);
    }

    return s.str();
}

/*!
 * \brief App::CellAddress::parseAbsoluteAddress
 * \param address
 * If the passed string is a valid and absolute cell address it will be assigned to this instance.
 * \return True if it's an absolute cell address and false otherwise
 */
bool App::CellAddress::parseAbsoluteAddress(const char *address) {
    if (maybeAbsolute(address)) {
        CellAddress addr = stringToAddress(address, true);
        if (addr.isValid()) {
            *this = addr;
            return true;
        }
    }
    return false;
}

