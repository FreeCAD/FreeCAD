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

#ifndef RANGE_H
#define RANGE_H

#include <string>
#include <Base/Bitmask.h>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

namespace App {

struct CellAddress;

AppExport CellAddress stringToAddress(const char *strAddress, bool silent=false);
AppExport int decodeColumn(const std::string &colstr, bool silent=false);
AppExport int decodeRow(const std::string &rowstr, bool silent=false);
AppExport bool validColumn(const std::string &colstr);
AppExport int validRow(const std::string &rowstr);

struct AppExport CellAddress {
    // See call of ENABLE_BITMASK_OPERATORS
    enum class Cell {
        Absolute = 1,
        ShowRow = 2,
        ShowColumn = 4,
        ShowRowColumn = ShowRow | ShowColumn,
        ShowFull = Absolute | ShowRow | ShowColumn
    };

    explicit CellAddress(int row = -1, int col = -1, bool absRow=false, bool absCol=false)
        : _row(row), _col(col), _absRow(absRow), _absCol(absCol)
    { }

    explicit CellAddress(const char * address) {
        *this = stringToAddress(address);
    }

    explicit CellAddress(const std::string & address) {
        *this = stringToAddress(address.c_str());
    }

    bool parseAbsoluteAddress(const char *txt);

    inline int row() const { return _row; }

    inline int col() const { return _col; }

    void setRow(int r, bool clip=false) { _row = (clip && r>=MAX_ROWS) ? MAX_ROWS-1 : r; }

    void setCol(int c, bool clip=false) { _col = (clip && c>=MAX_COLUMNS) ? MAX_COLUMNS-1 : c; }

    inline bool operator<(const CellAddress & other) const { return asInt() < other.asInt(); }

    inline bool operator>(const CellAddress & other) const { return asInt() > other.asInt(); }

    inline bool operator==(const CellAddress & other) const { return asInt() == other.asInt(); }

    inline bool operator!=(const CellAddress & other) const { return asInt() != other.asInt(); }

    inline bool isValid() { return (row() >=0 && row() < MAX_ROWS && col() >= 0 && col() < MAX_COLUMNS); }

    inline bool isAbsoluteRow() const { return _absRow; }

    inline bool isAbsoluteCol() const { return _absCol; }

    std::string toString(Cell = Cell::ShowFull) const;

    // Static members

    static const int MAX_ROWS;

    static const int MAX_COLUMNS;

protected:

    inline unsigned int asInt() const { return ((_row << 16) | _col); }

    short _row;
    short _col;
    bool _absRow;
    bool _absCol;
};

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

class AppExport Range {
public:
    explicit Range(const char *range, bool normalize=false);

    Range(int _row_begin, int _col_begin, int _row_end, int _col_end, bool normalize=false);

    Range(const CellAddress & from, const CellAddress & to, bool normalize=false);

    bool next();

    /** Make sure the range starts from top left and ends with bottom right corner **/
    void normalize();

    /** Current row */
    inline int row() const { return row_curr; }

    /** Current column */
    inline int column() const { return col_curr; }

    /** Row count */
    inline int rowCount() const { return row_end - row_begin + 1; }

    /** Column count */
    inline int colCount() const { return col_end - col_begin + 1; }

    /** Position of start of range */
    inline CellAddress from() const { return CellAddress(row_begin, col_begin); }

    /** Position of end of range */
    inline CellAddress to() const { return CellAddress(row_end, col_end); }

    /** Start of range as a string */
    inline std::string fromCellString() const { return CellAddress(row_begin, col_begin).toString(); }

    /** End of range as a string */
    inline std::string toCellString() const { return CellAddress(row_end, col_end).toString(); }

    /** Current cell as a string */
    inline std::string address() const { return CellAddress(row_curr, col_curr).toString(); }

    /** The raneg as a string */
    inline std::string rangeString() const {
        return CellAddress(row_begin, col_begin).toString() + ":" + CellAddress(row_end, col_end).toString();
    }

    CellAddress operator*() const { return CellAddress(row_curr, col_curr); }

    inline bool operator<(const Range & other) const {
        if(from() < other.from())
            return true;
        if(from() > other.from())
            return false;
        return to() < other.to();
    }

    /** Number of elements in range */
    inline int size() const { return (row_end - row_begin + 1) * (col_end - col_begin + 1); }

private:
    int row_curr, col_curr;
    int row_begin, col_begin;
    int row_end, col_end;
};

}

ENABLE_BITMASK_OPERATORS(App::CellAddress::Cell)

#endif // RANGE_H
