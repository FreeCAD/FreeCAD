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

#include "MaterialManagerPy.h"
#include "MaterialManagerPy.cpp"
#include "MaterialManager.h"
#include "MaterialPy.h"
#include "Materials.h"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string MaterialManagerPy::representation() const
{
    std::stringstream str;
    str << "<MaterialManager object at " << getMaterialManagerPtr() << ">";

    return str.str();
}

PyObject *MaterialManagerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialManagerPy(new MaterialManager());
}

// constructor method
int MaterialManagerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* MaterialManagerPy::getMaterial(PyObject *args)
{
    char *uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid))
        return nullptr;

    const Material &material = getMaterialManagerPtr()->getMaterial(QString::fromStdString(uuid));
    return new MaterialPy(new Material(material));
}

PyObject* MaterialManagerPy::getMaterialByPath(PyObject *args)
{
    char *path;
    char *lib="";
    if (!PyArg_ParseTuple(args, "s|s", &path, &lib))
        return nullptr;

    std::string libPath(lib);
    if (libPath.length() > 0)
    {
        const Material &material = getMaterialManagerPtr()->getMaterialByPath(QString::fromStdString(path), QString::fromStdString(libPath));
        return new MaterialPy(new Material(material));
    }

    const Material &material = getMaterialManagerPtr()->getMaterialByPath(QString::fromStdString(path));
    return new MaterialPy(new Material(material));
}

Py::List MaterialManagerPy::getMaterialLibraries() const
{
    std::list<MaterialLibrary*> *libraries = getMaterialManagerPtr()->getMaterialLibraries();
    Py::List list;

    for (auto it = libraries->begin(); it != libraries->end(); it++)
    {
        MaterialLibrary *lib = *it;
        Py::Tuple libTuple(3);
        libTuple.setItem(0,Py::String(lib->getName().toStdString()));
        libTuple.setItem(1,Py::String(lib->getDirectoryPath().toStdString()));
        libTuple.setItem(2,Py::String(lib->getIconPath().toStdString()));

        list.append(libTuple);
    }

    return list;
}

Py::Dict MaterialManagerPy::getMaterials() const
{
    std::map<QString, Material*> *Materials = getMaterialManagerPtr()->getMaterials();
    Py::Dict dict;

    for (auto it = Materials->begin(); it != Materials->end(); it++)
    {
        QString key = it->first;
        Material *material = it->second;

        PyObject *materialPy = new MaterialPy(new Material(*material));
        dict.setItem(Py::String(key.toStdString()), Py::Object(materialPy,true));
    }

    return dict;
}

PyObject *MaterialManagerPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialManagerPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
