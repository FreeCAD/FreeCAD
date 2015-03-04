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

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <set>
#include <memory>
#include <Base/BaseClass.h>

namespace Spreadsheet {

struct CellAddress;

SpreadsheetExport std::string columnName(int col);
SpreadsheetExport std::string rowName(int row);
int decodeColumn(const std::string &colstr);
int decodeRow(const std::string &rowstr);
SpreadsheetExport CellAddress stringToAddress(const char *strAddress);
SpreadsheetExport void createRectangles(std::set<std::pair<int, int> > & cells, std::map<std::pair<int, int>, std::pair<int, int> > & rectangles);
SpreadsheetExport std::string quote(const std::string &input);
SpreadsheetExport std::string unquote(const std::string & input);

struct SpreadsheetExport CellAddress {

    CellAddress(int row = -1, int col = -1) : _row(row), _col(col) { }

    CellAddress(const char * address) {
        *this = stringToAddress(address);
    }

    CellAddress(const std::string & address) {
        *this = stringToAddress(address.c_str());
    }

    inline int row() const { return _row; }

    inline int col() const { return _col; }

    inline bool operator<(const CellAddress & other) const { return asInt() < other.asInt(); }

    inline bool operator==(const CellAddress & other) const { return asInt() == other.asInt(); }

    inline bool operator!=(const CellAddress & other) const { return asInt() != other.asInt(); }

    inline bool isValid() { return (row() >=0 && row() < MAX_ROWS && col() >= 0 && col() < MAX_COLUMNS); }

    std::string toString() const;

    // Static members

    static const int MAX_ROWS;

    static const int MAX_COLUMNS;

protected:

    inline unsigned int asInt() const { return ((_row << 16) | _col); }

    short _row;
    short _col;
};

template<typename T> T * freecad_dynamic_cast(Base::BaseClass * t)
{
    if (t && t->isDerivedFrom(T::getClassTypeId()))
        return static_cast<T*>(t);
    else
        return 0;
}

}

#endif // UTILS_H
