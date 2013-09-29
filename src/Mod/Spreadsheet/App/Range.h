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

#ifndef RANGE_H
#define RANGE_H

#include "Utils.h"

namespace Spreadsheet {

/**
 * @brief The Range class is a spreadsheet range iterator. It takes
 * a starting (row, col) and an ending (row, col). Notice that ranges
 * are always at least one element. The next() functions is therefore
 * used e.g as follows:
 *
 * do {
 *   ...
 * while (range.next());
 *
 */

class SpreadsheetExport Range {
public:
    Range(const char *range);

    Range(int _row_begin, int _col_begin, int _row_end, int _col_end);

    bool next();

    /** Current row */
    inline int row() const { return row_curr; }

    /** Current column */
    inline int column() const { return col_curr; }

    /** Position of start of range */
    inline CellAddress from() const { return CellAddress(row_begin, col_begin); }

    /** Position of end of range */
    inline CellAddress to() const { return CellAddress(row_end, col_end); }

    /** Start of range as a string */
    inline std::string fromCellString() const { return addressToString(CellAddress(row_begin, col_begin)); }

    /** End of range as a string */
    inline std::string toCellString() const { return addressToString(CellAddress(row_end, col_end)); }

    /** Current cell as a string */
    inline std::string address() const { return addressToString(CellAddress(row_curr, col_curr)); }

    /** The raneg as a string */
    inline std::string rangeString() const {
        return addressToString(CellAddress(row_begin, col_begin)) + ":" + addressToString(CellAddress(row_end, col_end));
    }

    CellAddress operator*() const { return CellAddress(row_curr, col_curr); }

    /** Number of elements in range */
    inline int size() const { return (row_end - row_begin + 1) * (col_end - col_begin + 1); }

private:
    int row_curr, col_curr;
    int row_begin, col_begin;
    int row_end, col_end;
};

}

#endif // RANGE_H
