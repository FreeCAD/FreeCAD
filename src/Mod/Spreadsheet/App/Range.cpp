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
#include <assert.h>
#include <string.h>

using namespace Spreadsheet;

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


