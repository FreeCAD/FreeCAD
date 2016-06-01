/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2016     *
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
# include <sstream>
#endif

#include "Application.h"

// inclution of the generated files (generated out of PropertyContainerPy.xml)
#include "ExtensionContainerPy.h"
#include "ExtensionContainerPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string ExtensionContainerPy::representation(void) const
{
    return std::string("<extension>");
}

int  ExtensionContainerPy::initialisation() {
        
    if (this->ob_type->tp_dict == NULL) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }
        
    ExtensionContainer::ExtensionIterator it = this->getExtensionContainerPtr()->extensionBegin();
    for(; it != this->getExtensionContainerPtr()->extensionEnd(); ++it) {
        
        PyObject* obj = (*it).second->getExtensionPyObject();
        PyMethodDef* tmpptr = (PyMethodDef*)obj->ob_type->tp_methods;
        while(tmpptr->ml_name) {
            //Note: to add methods the call to PyMethod_New is required. However, than the PyObject 
            //      self is added to the functions arguments list. FreeCAD py implementations are not 
            //      made to handle this, the do not accept self as argument. Hence we only use function
            PyObject *func = PyCFunction_New(tmpptr,obj);
            //PyObject *method = PyMethod_New(func, (PyObject*)this, PyObject_Type((PyObject*)this));  
            PyDict_SetItem(this->ob_type->tp_dict, PyString_FromString(tmpptr->ml_name), func);
            Py_DECREF(func);
            //Py_DECREF(method);
            ++tmpptr;
        }
    }
    return 0;   
}

PyObject* ExtensionContainerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of @self.export.Name@ and the Twin object 
    Base::Console().Message("Make\n");
    return 0;
}

// constructor method
int ExtensionContainerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    Base::Console().Message("Init\n");
    return 0;
}

PyObject *ExtensionContainerPy::getCustomAttributes(const char* attr) const
{   
    return 0;
}

int ExtensionContainerPy::setCustomAttributes(const char* attr, PyObject *obj)
{        
    return 0;
}
