// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <string>
#include <CXX/Objects.hxx>

#include "Property.h"


namespace Base
{
class Writer;
class XMLReader;
}  // namespace Base

namespace App
{

/**
 * PropertyPythonObject is used to manage Py::Object instances as properties.
 * @author Werner Mayer
 */
class AppExport PropertyPythonObject: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPythonObject();
    ~PropertyPythonObject() override;

    void setValue(const Py::Object& py);
    Py::Object getValue() const;

    PyObject* getPyObject() override;
    void setPyObject(PyObject* obj) override;

    /** Use Python's pickle module to save the object */
    void Save(Base::Writer& writer) const override;
    /** Use Python's pickle module to restore the object */
    void Restore(Base::XMLReader& reader) override;
    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    unsigned int getMemSize() const override;
    Property* Copy() const override;
    void Paste(const Property& from) override;

    std::string toString() const;
    void fromString(const std::string&);

private:
    void saveObject(Base::Writer& writer) const;
    void restoreObject(Base::XMLReader& reader);
    std::string encodeValue(const std::string& str) const;
    std::string decodeValue(const std::string& str) const;
    void loadPickle(const std::string& str);
    Py::Object object;
};


}  // namespace App