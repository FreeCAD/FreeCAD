/***************************************************************************
 *   Copyright (c) Ajinkya Dahale       (ajinkyadahale@gmail.com) 2008     *
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
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#endif //_PreComp_

#include "TopoHistory.h"
#include <Mod/Part/App/TopoHistoryPy.h>
#include <Mod/Part/App/TopoHistoryPy.cpp>
#include "TopoShape.h"
#include <Mod/Part/App/TopoShapePy.h>

#include "OCCError.h"

using namespace Part;

namespace Part
{
PartExport Py::Object shape2pyshape(const TopoDS_Shape &shape);
}

// returns a string which represents the object e.g. when printed in python
std::string TopoHistoryPy::representation(void) const
{
    std::stringstream str;
    str << "<History object at " << getTopoHistoryPtr() << ">";

    return str.str();
}

PyObject* TopoHistoryPy::PyMake(_typeobject *, PyObject *, PyObject *)
{
    // create a new instance of TopoHistoryPy and the Twin object
    return new TopoHistoryPy(new TopoHistory);
}

int TopoHistoryPy::PyInit(PyObject* /*args*/, PyObject* /*k*/)
{
    return -1;
}

PyObject* TopoHistoryPy::modified(PyObject *args)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoShape* shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
        try {
            Py::List resShapesPy;
            TopTools_ListOfShape newShapes = this->getTopoHistoryPtr()->modified(*shape);
            for(TopTools_ListIteratorOfListOfShape it(newShapes); it.More(); it.Next()){
                resShapesPy.append(shape2pyshape(it.Value()));
            }
            return Py::new_reference_to(resShapesPy);
        }
        catch (Standard_Failure) {
            Handle(Standard_Failure) e = Standard_Failure::Caught();
            PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject*  TopoHistoryPy::generated(PyObject *args)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoShape* shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
        try {
            Py::List resShapesPy;
            TopTools_ListOfShape newShapes = this->getTopoHistoryPtr()->generated(*shape);
            for(TopTools_ListIteratorOfListOfShape it(newShapes); it.More(); it.Next()){
                resShapesPy.append(shape2pyshape(it.Value()));
            }
            return Py::new_reference_to(resShapesPy);
        }
        catch (Standard_Failure) {
            Handle(Standard_Failure) e = Standard_Failure::Caught();
            PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject*  TopoHistoryPy::isDeleted(PyObject *args)
{
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj)) {
        TopoShape* shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr();
        try {
            bool _isDeleted = this->getTopoHistoryPtr()->isDeleted(*shape);
            return Py_BuildValue("O", (_isDeleted ? Py_True : Py_False));
        }
        catch (Standard_Failure) {
            Handle(Standard_Failure) e = Standard_Failure::Caught();
            PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
            return NULL;
        }
        catch (const std::exception& e) {
            PyErr_SetString(PartExceptionOCCError, e.what());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "shape or sequence of shape expected");
    return 0;
}

PyObject* TopoHistoryPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoHistoryPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
