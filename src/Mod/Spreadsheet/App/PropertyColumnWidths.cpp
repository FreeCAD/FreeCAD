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

#include <App/Range.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyColumnWidths.h"
#include "PropertyColumnWidthsPy.h"
#include "Utils.h"


using namespace Spreadsheet;

const int PropertyColumnWidths::defaultWidth = 100;
const int PropertyColumnWidths::defaultHeaderWidth = 50;

TYPESYSTEM_SOURCE(Spreadsheet::PropertyColumnWidths, App::Property)

PropertyColumnWidths::PropertyColumnWidths() = default;

PropertyColumnWidths::PropertyColumnWidths(const PropertyColumnWidths& other)
    : Property()
    , std::map<int, int>(other)
{
    std::map<int, int>::const_iterator i = other.begin();

    while (i != other.end()) {
        insert(*i);
        ++i;
    }
}

App::Property* PropertyColumnWidths::Copy() const
{
    PropertyColumnWidths* prop = new PropertyColumnWidths(*this);

    return prop;
}

void PropertyColumnWidths::Paste(const App::Property& from)
{
    setValues(dynamic_cast<const PropertyColumnWidths&>(from).getValues());
}

void PropertyColumnWidths::setValues(const std::map<int, int>& values)
{
    aboutToSetValue();

    std::map<int, int>::const_iterator i;

    /* Mark all as dirty first */
    i = begin();
    while (i != end()) {
        dirty.insert(i->first);
        ++i;
    }

    /* Clear old map */
    clear();

    /* Copy new map from from */
    i = values.begin();
    while (i != values.end()) {
        insert(*i);
        dirty.insert(i->first);
        ++i;
    }
    hasSetValue();
}

/**
 * Set the width (in pixels) of column \a col to \a width.
 *
 * @param col   Column to set
 * @param width Width in pixels
 *
 */

void PropertyColumnWidths::setValue(int col, int width)
{
    if (width >= 0) {
        aboutToSetValue();
        operator[](col) = width;
        dirty.insert(col);
        hasSetValue();
    }
}

void PropertyColumnWidths::Save(Base::Writer& writer) const
{
    // Save column information
    writer.Stream() << writer.ind() << "<ColumnInfo Count=\"" << size() << "\">" << std::endl;
    writer.incInd();  // indentation for 'ColumnInfo'
    std::map<int, int>::const_iterator coli = begin();
    while (coli != end()) {
        writer.Stream() << writer.ind() << "<Column name=\"" << columnName(coli->first)
                        << "\" width=\"" << coli->second << "\" />" << std::endl;
        ++coli;
    }
    writer.decInd();  // indentation for 'ColumnInfo'
    writer.Stream() << writer.ind() << "</ColumnInfo>" << std::endl;
}

void PropertyColumnWidths::Restore(Base::XMLReader& reader)
{
    int Cnt;

    // Column info
    reader.readElement("ColumnInfo");
    Cnt = reader.hasAttribute("Count") ? reader.getAttributeAsInteger("Count") : 0;
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Column");
        const char* name = reader.hasAttribute("name") ? reader.getAttribute("name") : nullptr;
        const char* width = reader.hasAttribute("width") ? reader.getAttribute("width") : nullptr;

        try {
            if (name && width) {
                int col = App::decodeColumn(name);
                int colWidth = atoi(width);

                setValue(col, colWidth);
            }
        }
        catch (...) {
            // Something is wrong, skip this column
        }
    }
    reader.readEndElement("ColumnInfo");
}

PyObject* PropertyColumnWidths::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new PropertyColumnWidthsPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void PropertyColumnWidths::clear()
{
    std::map<int, int>::const_iterator i = begin();

    while (i != end()) {
        dirty.insert(i->first);
        ++i;
    }
    std::map<int, int>::clear();
}
