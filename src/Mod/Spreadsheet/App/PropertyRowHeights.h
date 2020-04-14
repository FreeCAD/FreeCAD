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

#ifndef PROPERTYROWHEIGHTS_H
#define PROPERTYROWHEIGHTS_H

#include <map>
#include <App/Property.h>
#include <CXX/Objects.hxx>

namespace Spreadsheet {

class SpreadsheetExport PropertyRowHeights : public App::Property, std::map<int, int>
{
    TYPESYSTEM_HEADER();
public:
    PropertyRowHeights();

    void setValue() { }

    void setValue(int row, int height);

    void setValues(const std::map<int,int> &);

    int getValue(int row) const {
        std::map<int, int>::const_iterator i = find(row);
        return i != end() ? i->second : defaultHeight;
    }

    std::map<int, int> getValues() const {
        return *this;
    }

    virtual App::Property *Copy(void) const;

    virtual void Paste(const App::Property &from);

    virtual void Save (Base::Writer & writer) const;

    virtual void Restore(Base::XMLReader & reader);

    bool isDirty() const { return dirty.size() > 0; }

    void clearDirty() { dirty.clear(); }

    const std::set<int> & getDirty() const { return dirty; }

    PyObject *getPyObject(void);

    static const int defaultHeight;

    void clear();

private:

    PropertyRowHeights(const PropertyRowHeights & other);

    std::set<int> dirty;

    Py::Object PythonObject;
};

}

#endif // PROPERTYROWHEIGHTS_H
