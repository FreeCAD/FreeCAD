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

#include "ModelManagerPy.h"
#include "ModelManagerPy.cpp"
#include "ModelManager.h"
#include "ModelPy.h"


using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string ModelManagerPy::representation() const
{
    std::stringstream str;
    str << "<ModelManager object at " << getModelManagerPtr() << ">";

    return str.str();
}

PyObject *ModelManagerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    // return new ModelManagerPy(new ModelManager());
    return new ModelManagerPy(ModelManager::getManager()); // Does this delete the model manager on exit?
}

// constructor method
int ModelManagerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* ModelManagerPy::getModel(PyObject *args)
{
    char *uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid))
        return nullptr;

    try {
        const Model model = getModelManagerPtr()->getModel(QString::fromStdString(uuid));
        return new ModelPy(new Model(model));
    } catch (ModelNotFound const &) {
        return nullptr;
    }
}

PyObject* ModelManagerPy::getModelByPath(PyObject *args)
{
    char *path;
    char *lib="";
    if (!PyArg_ParseTuple(args, "s|s", &path, &lib))
        return nullptr;

    std::string libPath(lib);
    if (libPath.length() > 0)
    {
        try {
            const Model &model = getModelManagerPtr()->getModelByPath(QString::fromStdString(path), QString::fromStdString(libPath));
            return new ModelPy(new Model(model));
        } catch (ModelNotFound const &) {
            return nullptr;
        }
    }

    try {
        const Model &model = getModelManagerPtr()->getModelByPath(QString::fromStdString(path));
        return new ModelPy(new Model(model));
    } catch (ModelNotFound const &) {
        return nullptr;
    }
}

Py::List ModelManagerPy::getModelLibraries() const
{
    std::list<ModelLibrary*> *libraries = getModelManagerPtr()->getModelLibraries();
    Py::List list;

    for (auto it = libraries->begin(); it != libraries->end(); it++)
    {
        ModelLibrary *lib = *it;
        Py::Tuple libTuple(3);
        libTuple.setItem(0,Py::String(lib->getName().toStdString()));
        libTuple.setItem(1,Py::String(lib->getDirectoryPath().toStdString()));
        libTuple.setItem(2,Py::String(lib->getIconPath().toStdString()));

        list.append(libTuple);
    }

    return list;
}

Py::Dict ModelManagerPy::getModels() const
{
    std::map<QString, Model*> *models = getModelManagerPtr()->getModels();
    Py::Dict dict;

    for (auto it = models->begin(); it != models->end(); it++)
    {
        QString key = it->first;
        Model *model = it->second;

        PyObject *modelPy = new ModelPy(new Model(*model));
        dict.setItem(Py::String(key.toStdString()), Py::Object(modelPy,true));
    }

    return dict;
}

PyObject *ModelManagerPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ModelManagerPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
