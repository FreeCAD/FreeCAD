/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
#include "Base/Reader.h"
#include "Mod/Path/App/Tool.h"
#include "Mod/Path/App/Tooltable.h"

// inclusion of the generated files (generated out of ToolPy.xml and TooltablePy.xml)
#include "ToolPy.h"
#include "TooltablePy.h"
#include "TooltablePy.cpp"

using namespace Path;

#if PY_MAJOR_VERSION >= 3
#  define PYSTRING_FROMSTRING(str)  PyUnicode_FromString(str)
#  define PYINT_TYPE                PyLong_Type
#  define PYINT_FROMLONG(l)         PyLong_FromLong(l)
#  define PYINT_ASLONG(o)           PyLong_AsLong(o)
#else
#  define PYSTRING_FROMSTRING(str)  PyString_FromString(str)
#  define PYINT_TYPE                PyInt_Type
#  define PYINT_FROMLONG(l)         PyInt_FromLong(l)
#  define PYINT_ASLONG(o)           PyInt_AsLong(o)
#endif

// returns a string which represents the object e.g. when printed in python
std::string TooltablePy::representation(void) const
{
    std::stringstream str;
    str.precision(5);
    str << "Tooltable containing ";
    str << getTooltablePtr()->getSize() << " tools";
    return str.str();
}

PyObject *TooltablePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new TooltablePy(new Tooltable);
}

// constructor method
int TooltablePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    //char *name="Tooltable";
    //int version = 1;

    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(PyDict_Type), &pcObj)) {
        try {
            Py::Dict dict(pcObj);
            setTools(dict);
        } catch(...) {
            PyErr_SetString(PyExc_TypeError, "The dictionary can only contain int:tool pairs");
            return -1;
        }
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &pcObj)) {
        Py::List list(pcObj);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Path::ToolPy::Type))) {
                Path::Tool &tool = *static_cast<Path::ToolPy*>((*it).ptr())->getToolPtr();
                getTooltablePtr()->addTool(tool);
            }
        }
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Argument must be either empty or a list or a dictionary");
    return -1;
}

// Commands get/set

Py::Dict TooltablePy::getTools(void) const
{
    Py::Dict dict;
    for(std::map<int,Path::Tool*>::iterator i = getTooltablePtr()->Tools.begin(); i != getTooltablePtr()->Tools.end(); ++i) {
        PyObject *tool = new Path::ToolPy(new Tool(*i->second));
        dict.setItem(Py::Long(i->first), Py::asObject(tool));
    }
    return dict;
}

void TooltablePy::setTools(Py::Dict arg)
{
    getTooltablePtr()->Tools.clear();
    PyObject* dict_copy = PyDict_Copy(arg.ptr());
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict_copy, &pos, &key, &value)) {
        if ( PyObject_TypeCheck(key,&(PYINT_TYPE)) && ((PyObject_TypeCheck(value, &(Path::ToolPy::Type))) || PyObject_TypeCheck(value, &PyDict_Type))) {
            int ckey = (int)PYINT_ASLONG(key);
            if (PyObject_TypeCheck(value, &(Path::ToolPy::Type))) {
              Path::Tool &tool = *static_cast<Path::ToolPy*>(value)->getToolPtr();
              getTooltablePtr()->setTool(tool, ckey);
            } else {
              PyErr_Clear();
              Path::Tool *tool = new Path::Tool;
              // The 'pyTool' object must be created on the heap otherwise Python
              // will fail to properly track the reference counts and aborts
              // in debug mode.
              Path::ToolPy* pyTool = new Path::ToolPy(tool);
              PyObject* success = pyTool->setFromTemplate(value);
              if (!success) {
                Py_DECREF(pyTool);
                throw Py::Exception();
              }
              getTooltablePtr()->setTool(*tool, ckey);
              Py_DECREF(pyTool);
              Py_DECREF(success);
            }
        } else {
            throw Py::TypeError("The dictionary can only contain int:tool pairs");
        }
    }
}

// specific methods

PyObject* TooltablePy::copy(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
    }
    throw Py::TypeError("This method accepts no argument");
}

PyObject* TooltablePy::addTools(PyObject * args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Path::ToolPy::Type), &o)) {
        Path::Tool &tool = *static_cast<Path::ToolPy*>(o)->getToolPtr();
        getTooltablePtr()->addTool(tool);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &o)) {
        Py::List list(o);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Path::ToolPy::Type))) {
                Path::Tool &tool = *static_cast<Path::ToolPy*>((*it).ptr())->getToolPtr();
                getTooltablePtr()->addTool(tool);
            }
        }
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters - tool or list of tools expected");
}

PyObject* TooltablePy::setTool(PyObject * args)
{
    PyObject* o;
    int pos = -1;
    if (PyArg_ParseTuple(args, "iO!", &pos, &(Path::ToolPy::Type), &o)) {
        Path::Tool &tool = *static_cast<Path::ToolPy*>(o)->getToolPtr();
        getTooltablePtr()->setTool(tool,pos);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters - expected tool and optional integer");
}

PyObject* TooltablePy::getTool(PyObject * args)
{
    int pos = -1;
    if (PyArg_ParseTuple(args, "i", &pos)) {
        if (getTooltablePtr()->hasTool(pos))
        {
            Path::Tool tool = getTooltablePtr()->getTool(pos);
            return new ToolPy(new Path::Tool(tool));
        }
        else
        {
            Py_INCREF(Py_None);
            return Py_None;
        }
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Argument must be integer");
}

PyObject* TooltablePy::deleteTool(PyObject * args)
{
    int pos = -1;
    if (PyArg_ParseTuple(args, "|i", &pos)) {
        getTooltablePtr()->deleteTool(pos);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters - expected an integer (optional)");
}

// custom attributes get/set

PyObject *TooltablePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TooltablePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

Py::Int TooltablePy::getVersion(void) const
{
    return Py::Int(getTooltablePtr()->Version);
}

void TooltablePy::setVersion(Py::Int version) {
    getTooltablePtr()->Version = version;
}

Py::String TooltablePy::getName(void) const
{
    return Py::String(getTooltablePtr()->Name.c_str());
}

void TooltablePy::setName(Py::String arg)
{
    std::string name = arg.as_std_string();
    getTooltablePtr()->Name = name;
}

PyObject* TooltablePy::setFromTemplate(PyObject * args)
{
    PyObject *dict = 0;
    if (PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict)) {
      Py::Dict d(dict);
      setTools(d);
      Py_Return ;
    }

    PyErr_SetString(PyExc_TypeError, "argument must be a dictionary returned from templateAttrs()");
    return 0;
}

PyObject* TooltablePy::templateAttrs(PyObject * args)
{
    (void)args;
    PyObject *dict = PyDict_New();
    for(std::map<int,Path::Tool*>::iterator i = getTooltablePtr()->Tools.begin(); i != getTooltablePtr()->Tools.end(); ++i) {
        // The 'tool' object must be created on the heap otherwise Python
        // will fail to properly track the reference counts and aborts
        // in debug mode.
        Path::ToolPy* tool = new Path::ToolPy(new Path::Tool(*i->second));
        PyObject *attrs = tool->templateAttrs(0);
        PyDict_SetItem(dict, PYINT_FROMLONG(i->first), attrs);
        Py_DECREF(tool);
    }
    return dict;
}

