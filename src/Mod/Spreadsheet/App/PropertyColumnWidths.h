// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef PROPERTYCOLUMNWIDTHS_H
#define PROPERTYCOLUMNWIDTHS_H

#include <App/Property.h>
#include <CXX/Objects.hxx>
#include <Mod/Spreadsheet/SpreadsheetGlobal.h>
#include <map>

namespace Spreadsheet
{

class SpreadsheetExport PropertyColumnWidths: public App::Property, std::map<int, int>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyColumnWidths();

    void setValue()
    {}

    void setValue(int col, int width);

    void setValues(const std::map<int, int>&);

    std::map<int, int> getValues() const
    {
        return *this;
    }

    int getValue(int column) const
    {
        std::map<int, int>::const_iterator i = find(column);
        return i != end() ? i->second : defaultWidth;
    }

    Property* Copy() const override;

    void Paste(const Property& from) override;

    void Save(Base::Writer& writer) const override;

    void Restore(Base::XMLReader& reader) override;

    bool isDirty() const
    {
        return !dirty.empty();
    }

    void clearDirty()
    {
        dirty.clear();
    }

    const std::set<int>& getDirty() const
    {
        return dirty;
    }

    PyObject* getPyObject() override;

    void clear();

    static const int defaultWidth;
    static const int defaultHeaderWidth;

private:
    PropertyColumnWidths(const PropertyColumnWidths& other);

    std::set<int> dirty;

    Py::Object PythonObject;
};

}  // namespace Spreadsheet

#endif  // PROPERTYCOLUMNWIDTHS_H
