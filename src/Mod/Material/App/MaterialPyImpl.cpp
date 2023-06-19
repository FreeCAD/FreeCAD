/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <boost/uuid/uuid_io.hpp>
#endif

#include "MaterialPy.h"
#include "MaterialPy.cpp"
#include "Materials.h"


using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string MaterialPy::representation() const
{
    MaterialPy::PointerType ptr = getMaterialPtr();
    std::stringstream str;
    str << "Property [Name=(";
    str << ptr->getName();
    str << "), UUID=(";
    str << ptr->getUUID();
    str << "), Library Name=(";
    str << ptr->getLibrary().getName();
    str << "), Library Root=(";
    str << ptr->getLibrary().getDirectoryPath();
     str << "), Library Icon=(";
    str << ptr->getLibrary().getIconPath();
    str << "), Relative Path=(";
    str << ptr->getRelativePath();
    str << "), Directory=(";
    str << ptr->getDirectory().absolutePath().toStdString();
    // str << "), URL=(";
    // str << ptr->getURL();
    // str << "), DOI=(";
    // str << ptr->getDOI();
    // str << "), Description=(";
    // str << ptr->getDescription();
    // str << "), Inherits=[";
    // const std::vector<std::string> &inherited = getMaterialPtr()->getInheritance();
    // for (auto it = inherited.begin(); it != inherited.end(); it++)
    // {
    //     std::string uuid = *it;
    //     if (it != inherited.begin())
    //         str << "), UUID=(";
    //     else
    //         str << "UUID=(";
    //     str << uuid << ")";
    // }
    // str << "]]";
    str << ")]";

    return str.str();
}

PyObject *MaterialPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialPy(new Material());
}

// constructor method
int MaterialPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String MaterialPy::getLibraryName() const
{
    return Py::String(getMaterialPtr()->getLibrary().getName());
}

Py::String MaterialPy::getLibraryRoot() const
{
    return Py::String(getMaterialPtr()->getLibrary().getDirectoryPath());
}

Py::String MaterialPy::getRelativePath() const
{
    return Py::String(getMaterialPtr()->getRelativePath());
}

Py::String MaterialPy::getLibraryIcon() const
{
    return Py::String(getMaterialPtr()->getLibrary().getIconPath());
}

Py::String MaterialPy::getName() const
{
    return Py::String(getMaterialPtr()->getName());
}

Py::String MaterialPy::getDirectory() const
{
    return Py::String(getMaterialPtr()->getDirectory().absolutePath().toStdString());
}

Py::String MaterialPy::getUUID() const
{
    return Py::String(getMaterialPtr()->getUUID());
}

Py::String MaterialPy::getDescription() const
{
    return Py::String(getMaterialPtr()->getDescription());
}

Py::String MaterialPy::getURL() const
{
    return Py::String(getMaterialPtr()->getURL());
}

Py::String MaterialPy::getReference() const
{
    return Py::String(getMaterialPtr()->getReference());
}

Py::String MaterialPy::getParent() const
{
    return Py::String(getMaterialPtr()->getParentUUID());
}

Py::String MaterialPy::getAuthorAndLicense() const
{
    return Py::String(getMaterialPtr()->getAuthorAndLicense());
}

Py::List MaterialPy::getPhysicalModels() const
{
    const std::vector<std::string> *models = getMaterialPtr()->getPhysicalModels();
    Py::List list;

    for (auto it = models->begin(); it != models->end(); it++)
    {
        std::string uuid = *it;

        list.append(Py::String(uuid));
    }

    return list;
}

Py::List MaterialPy::getTags() const
{
    const std::list<std::string> &tags = getMaterialPtr()->getTags();
    Py::List list;

    for (auto it = tags.begin(); it != tags.end(); it++)
    {
        std::string uuid = *it;

        list.append(Py::String(uuid));
    }

    return list;
}

PyObject *MaterialPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
