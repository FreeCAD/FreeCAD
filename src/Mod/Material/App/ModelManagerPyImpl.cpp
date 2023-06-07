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


using namespace Material;

// returns a string which represents the object e.g. when printed in python
std::string ModelManagerPy::representation() const
{
    return "<Model object>";
}

PyObject *ModelManagerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    return new ModelManagerPy(new ModelManager());

    // PyErr_SetString(PyExc_RuntimeError,
    //     "Please use ModelManager.getManager() to get an instance of the ModelManager");
    // return nullptr;
}

// constructor method
int ModelManagerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::List ModelManagerPy::getModelLibraries() const
{
    std::list<ModelLibrary*> *libraries = getModelManagerPtr()->getModelLibraries();
    Py::List list;

    for (auto it = libraries->begin(); it != libraries->end(); it++)
    {
        ModelLibrary *lib = *it;
        Py::Tuple libTuple(3);
        libTuple.setItem(0,Py::String(lib->getName()));
        libTuple.setItem(1,Py::String(lib->getDirectoryPath()));
        libTuple.setItem(2,Py::String(lib->getIconPath()));

        list.append(libTuple);
    }

    return list;
}

PyObject *ModelManagerPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ModelManagerPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
