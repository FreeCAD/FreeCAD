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

#ifndef PROPERTYCOLUMNWIDTHS_H
#define PROPERTYCOLUMNWIDTHS_H

#include <map>
#include <App/Property.h>
#include <CXX/Objects.hxx>

namespace Spreadsheet {

class SpreadsheetExport PropertyColumnWidths : public App::Property, std::map<int, int>
{
    TYPESYSTEM_HEADER();
public:
    PropertyColumnWidths();

    void setValue() { }

    void setValue(int col, int width);

    void setValues(const std::map<int,int> &);

    std::map<int, int> getValues() const {
        return *this;
    }

    int getValue(int column) const {
        std::map<int, int>::const_iterator i = find(column);
        return i != end() ? i->second : defaultWidth;
    }

    virtual Property *Copy(void) const;

    virtual void Paste(const Property &from);

    virtual void Save (Base::Writer & writer) const;

    virtual void Restore(Base::XMLReader & reader);

    bool isDirty() const { return dirty.size() > 0; }

    void clearDirty() { dirty.clear(); }

    const std::set<int> & getDirty() const { return dirty; }

    PyObject *getPyObject(void);

    void clear();

    static const int defaultWidth;
    static const int defaultHeaderWidth;

private:

    PropertyColumnWidths(const PropertyColumnWidths & other);

    std::set<int> dirty;

    Py::Object PythonObject;
};

}

#endif // PROPERTYCOLUMNWIDTHS_H
