/***************************************************************************
 *   Copyright (c) 2023-2024 David Carter <dcarter@david.carter.ca>        *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include <QMetaType>

#include <Base/Quantity.h>
#include <Base/QuantityPy.h>
#include <CXX/Objects.hxx>
#include <Gui/MetaTypes.h>

#include "MaterialLibrary.h"

#include "MaterialLibraryPy.h"

#include "MaterialLibraryPy.cpp"

using namespace Materials;

// Forward declaration
// static PyObject* _pyObjectFromVariant(const QVariant& value);
// static Py::List getList(const QVariant& value);

// returns a string which represents the object e.g. when printed in python
std::string MaterialLibraryPy::representation() const
{
    std::stringstream str;
    str << "<MaterialLibrary object at " << getMaterialLibraryPtr() << ">";

    return str.str();
}

PyObject* MaterialLibraryPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialLibraryPy(new MaterialLibrary());
}

// constructor method
int MaterialLibraryPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String MaterialLibraryPy::getName() const
{
    auto filterName = getMaterialLibraryPtr()->getName();
    return {filterName.toStdString()};
}

void MaterialLibraryPy::setName(const Py::String value)
{
    getMaterialLibraryPtr()->setName(QString::fromStdString(value));
}

Py::Object MaterialLibraryPy::getIcon() const
{
    auto icon = getMaterialLibraryPtr()->getIcon();
    return Py::Bytes(icon.data(), icon.size());
}

void MaterialLibraryPy::setIcon(const Py::Object value)
{
    if (value.isNone()) {
        getMaterialLibraryPtr()->setIcon(QByteArray());
    }
    else {
        auto pyBytes = Py::Bytes(value);
        getMaterialLibraryPtr()->setIcon(
            QByteArray(pyBytes.as_std_string().data(), pyBytes.size()));
    }
}

Py::String MaterialLibraryPy::getDirectory() const
{
    auto path = getMaterialLibraryPtr()->getDirectory();
    return {path.toStdString()};
}

void MaterialLibraryPy::setDirectory(const Py::String value)
{
    getMaterialLibraryPtr()->setDirectory(QString::fromStdString(value));
}

Py::Boolean MaterialLibraryPy::getReadOnly() const
{
    return getMaterialLibraryPtr()->isReadOnly();
}

void MaterialLibraryPy::setReadOnly(Py::Boolean value)
{
    getMaterialLibraryPtr()->setReadOnly(value);
}

Py::Boolean MaterialLibraryPy::getLocal() const
{
    return getMaterialLibraryPtr()->isLocal();
}

void MaterialLibraryPy::setLocal(Py::Boolean value)
{
    getMaterialLibraryPtr()->setLocal(value);
}

PyObject* MaterialLibraryPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialLibraryPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
