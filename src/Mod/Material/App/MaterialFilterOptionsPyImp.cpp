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

#include "MaterialFilter.h"

#include "MaterialFilterOptionsPy.h"

#include "MaterialFilterOptionsPy.cpp"

using namespace Materials;

// Forward declaration
// static PyObject* _pyObjectFromVariant(const QVariant& value);
// static Py::List getList(const QVariant& value);

// returns a string which represents the object e.g. when printed in python
std::string MaterialFilterOptionsPy::representation() const
{
    std::stringstream str;
    str << "<MaterialFilterOptions object at " << getMaterialFilterOptionsPtr() << ">";

    return str.str();
}

PyObject* MaterialFilterOptionsPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialFilterOptionsPy(new MaterialFilterOptions());
}

// constructor method
int MaterialFilterOptionsPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::Boolean MaterialFilterOptionsPy::getIncludeFavorites() const
{
    return getMaterialFilterOptionsPtr()->includeFavorites();
}

void MaterialFilterOptionsPy::setIncludeFavorites(const Py::Boolean value)
{
    getMaterialFilterOptionsPtr()->setIncludeFavorites(value);
}

Py::Boolean MaterialFilterOptionsPy::getIncludeRecent() const
{
    return getMaterialFilterOptionsPtr()->includeRecent();
}

void MaterialFilterOptionsPy::setIncludeRecent(const Py::Boolean value)
{
    getMaterialFilterOptionsPtr()->setIncludeRecent(value);
}

Py::Boolean MaterialFilterOptionsPy::getIncludeEmptyFolders() const
{
    return getMaterialFilterOptionsPtr()->includeEmptyFolders();
}

void MaterialFilterOptionsPy::setIncludeEmptyFolders(const Py::Boolean value)
{
    getMaterialFilterOptionsPtr()->setIncludeEmptyFolders(value);
}

Py::Boolean MaterialFilterOptionsPy::getIncludeEmptyLibraries() const
{
    return getMaterialFilterOptionsPtr()->includeEmptyLibraries();
}

void MaterialFilterOptionsPy::setIncludeEmptyLibraries(const Py::Boolean value)
{
    getMaterialFilterOptionsPtr()->setIncludeEmptyLibraries(value);
}

Py::Boolean MaterialFilterOptionsPy::getIncludeLegacy() const
{
    return getMaterialFilterOptionsPtr()->includeLegacy();
}

void MaterialFilterOptionsPy::setIncludeLegacy(const Py::Boolean value)
{
    getMaterialFilterOptionsPtr()->setIncludeLegacy(value);
}

PyObject* MaterialFilterOptionsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialFilterOptionsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
