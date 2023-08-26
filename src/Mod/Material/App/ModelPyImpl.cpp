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

#include "ModelPy.h"
#include "ModelPy.cpp"
#include "ModelPropertyPy.h"
#include "Model.h"


using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string ModelPy::representation() const
{
    ModelPy::PointerType ptr = getModelPtr();
    std::stringstream str;
    str << "Property [Name=(";
    str << ptr->getName().toStdString();
    str << "), UUID=(";
    str << ptr->getUUID().toStdString();
    str << "), Library Name=(";
    str << ptr->getLibrary().getName().toStdString();
    str << "), Library Root=(";
    str << ptr->getLibrary().getDirectoryPath().toStdString();
     str << "), Library Icon=(";
    str << ptr->getLibrary().getIconPath().toStdString();
    str << "), Relative Path=(";
    str << ptr->getRelativePath().toStdString();
    str << "), Directory=(";
    str << ptr->getDirectory().absolutePath().toStdString();
    str << "), URL=(";
    str << ptr->getURL().toStdString();
    str << "), DOI=(";
    str << ptr->getDOI().toStdString();
    str << "), Description=(";
    str << ptr->getDescription().toStdString();
    str << "), Inherits=[";
    const std::vector<QString> &inherited = getModelPtr()->getInheritance();
    for (auto it = inherited.begin(); it != inherited.end(); it++)
    {
        QString uuid = *it;
        if (it != inherited.begin())
            str << "), UUID=(";
        else
            str << "UUID=(";
        str << uuid.toStdString() << ")";
    }
    str << "]]";

    return str.str();
}

PyObject *ModelPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    return new ModelPy(new Model());
}

// constructor method
int ModelPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String ModelPy::getLibraryName() const
{
    return Py::String(getModelPtr()->getLibrary().getName().toStdString());
}

Py::String ModelPy::getLibraryRoot() const
{
    return Py::String(getModelPtr()->getLibrary().getDirectoryPath().toStdString());
}

Py::String ModelPy::getRelativePath() const
{
    return Py::String(getModelPtr()->getRelativePath().toStdString());
}

Py::String ModelPy::getLibraryIcon() const
{
    return Py::String(getModelPtr()->getLibrary().getIconPath().toStdString());
}

Py::String ModelPy::getName() const
{
    return Py::String(getModelPtr()->getName().toStdString());
}

Py::String ModelPy::getDirectory() const
{
    return Py::String(getModelPtr()->getDirectory().absolutePath().toStdString());
}

Py::String ModelPy::getUUID() const
{
    return Py::String(getModelPtr()->getUUID().toStdString());
}

Py::String ModelPy::getDescription() const
{
    return Py::String(getModelPtr()->getDescription().toStdString());
}

Py::String ModelPy::getURL() const
{
    return Py::String(getModelPtr()->getURL().toStdString());
}

Py::String ModelPy::getDOI() const
{
    return Py::String(getModelPtr()->getDOI().toStdString());
}

Py::List ModelPy::getInherited() const
{
    const std::vector<QString> &inherited = getModelPtr()->getInheritance();
    Py::List list;

    for (auto it = inherited.begin(); it != inherited.end(); it++)
    {
        QString uuid = *it;

        list.append(Py::String(uuid.toStdString()));
    }

    return list;
}

Py::Dict ModelPy::getProperties() const
{
    // std::map<std::string, Model*> *models = getModelPtr()->getModels();
    Py::Dict dict;

    for (auto it = getModelPtr()->begin(); it != getModelPtr()->end(); it++)
    {
        QString key = it->first;
        ModelProperty &modelProperty = it->second;

        PyObject *modelPropertyPy = new ModelPropertyPy(new ModelProperty(modelProperty));
        dict.setItem(Py::String(key.toStdString()), Py::Object(modelPropertyPy, true));
    }

    return dict;
}

PyObject *ModelPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ModelPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
