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

#ifndef _PreComp_
#endif

#include "PropertyRowHeights.h"
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <App/Range.h>
#include "Utils.h"
#include <PropertyRowHeightsPy.h>

using namespace Spreadsheet;

const int PropertyRowHeights::defaultHeight = 20;

TYPESYSTEM_SOURCE(Spreadsheet::PropertyRowHeights , App::Property);

PropertyRowHeights::PropertyRowHeights()
{
}

PropertyRowHeights::PropertyRowHeights(const PropertyRowHeights &other)
  : Property(), std::map<int, int>(other)
{
}

App::Property *PropertyRowHeights::Copy() const
{
    PropertyRowHeights * prop = new PropertyRowHeights(*this);

    return prop;
}

void PropertyRowHeights::Paste(const Property &from)
{
    setValues(static_cast<const PropertyRowHeights&>(from).getValues());
}

void PropertyRowHeights::setValues(const std::map<int,int> &values) {
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

void PropertyRowHeights::Save(Base::Writer &writer) const
{
    // Save row information
    writer.Stream() << writer.ind() << "<RowInfo Count=\"" << size() << "\">" << std::endl;
    writer.incInd(); // indentation for 'RowInfo'

    std::map<int, int>::const_iterator ri = begin();
    while (ri != end()) {
        writer.Stream() << writer.ind() << "<Row name=\"" << rowName(ri->first) << "\"  height=\"" << ri->second << "\" />" << std::endl;
        ++ri;
    }
    writer.decInd(); // indentation for 'RowInfo'
    writer.Stream() << writer.ind() << "</RowInfo>" << std::endl;
}

/**
  * Set height of row given by \a row to \a height in pixels.
  *
  * @param row    Address of row
  * @param height Height in pixels
  *
  */

void PropertyRowHeights::setValue(int row, int height)
{
    if (height >= 0) {
        aboutToSetValue();
        operator[](row) = height;
        dirty.insert(row);
        hasSetValue();
    }
}

void PropertyRowHeights::Restore(Base::XMLReader &reader)
{
    int Cnt;

    // Row info
    reader.readElement("RowInfo");
    Cnt = reader.hasAttribute("Count") ? reader.getAttributeAsInteger("Count") : 0;
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Row");
        const char* name = reader.hasAttribute("name") ? reader.getAttribute("name") : 0;
        const char * height = reader.hasAttribute("height") ? reader.getAttribute("height") : 0;

        try {
            if (name && height) {
                int row = App::decodeRow(name);
                int rowHeight = atoi(height);

                setValue(row, rowHeight);
            }
        }
        catch (...) {
            // Something is wrong, skip this row
        }
    }
    reader.readEndElement("RowInfo");
}

PyObject *PropertyRowHeights::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PropertyRowHeightsPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void PropertyRowHeights::clear()
{
    std::map<int, int>::const_iterator i = begin();

    while (i != end()) {
        dirty.insert(i->first);
        ++i;
    }
    std::map<int,int>::clear();
}
