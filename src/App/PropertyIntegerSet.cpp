/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
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

#include <Base/PyObjectBase.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyIntegerSet.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyIntegerSet, App::Property)

PropertyIntegerSet::PropertyIntegerSet() = default;

PropertyIntegerSet::~PropertyIntegerSet() = default;


//**************************************************************************
// Base class implementer

void PropertyIntegerSet::setValue(long lValue)
{
    aboutToSetValue();
    _lValueSet.clear();
    _lValueSet.insert(lValue);
    hasSetValue();
}

void PropertyIntegerSet::setValues(const std::set<long>& values)
{
    aboutToSetValue();
    _lValueSet = values;
    hasSetValue();
}

PyObject* PropertyIntegerSet::getPyObject()
{
    PyObject* set = PySet_New(nullptr);
    for (long it : _lValueSet) {
        PySet_Add(set, PyLong_FromLong(it));
    }
    return set;
}

void PropertyIntegerSet::setPyObject(PyObject* value)
{
    if (PySequence_Check(value)) {

        Py::Sequence sequence(value);
        Py_ssize_t nSize = sequence.size();
        std::set<long> values;

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            Py::Object item = sequence.getItem(i);
            if (!PyLong_Check(item.ptr())) {
                std::string error = std::string("type in list must be int, not ");
                error += item.ptr()->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values.insert(PyLong_AsLong(item.ptr()));
        }

        setValues(values);
    }
    else if (PyLong_Check(value)) {
        setValue(PyLong_AsLong(value));
    }
    else {
        std::string error = std::string("type must be int or list of int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyIntegerSet::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<IntegerSet count=\"" << _lValueSet.size() << "\">" << std::endl;
    writer.incInd();
    for (long it : _lValueSet) {
        writer.Stream() << writer.ind() << "<I v=\"" << it << "\"/>" << std::endl;
    };
    writer.decInd();
    writer.Stream() << writer.ind() << "</IntegerSet>" << std::endl;
}

void PropertyIntegerSet::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("IntegerSet");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");

    std::set<long> values;
    for (int i = 0; i < count; i++) {
        reader.readElement("I");
        values.insert(reader.getAttributeAsInteger("v"));
    }

    reader.readEndElement("IntegerSet");

    // assignment
    setValues(values);
}

Property* PropertyIntegerSet::Copy() const
{
    PropertyIntegerSet* p = new PropertyIntegerSet();
    p->_lValueSet = _lValueSet;
    return p;
}

void PropertyIntegerSet::Paste(const Property& from)
{
    aboutToSetValue();
    _lValueSet = dynamic_cast<const PropertyIntegerSet&>(from)._lValueSet;
    hasSetValue();
}

unsigned int PropertyIntegerSet::getMemSize() const
{
    return static_cast<unsigned int>(_lValueSet.size() * sizeof(long));
}

}  // namespace App
