/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <App/ExtensionContainerPy.h>
#include <App/ExtensionContainerPy.cpp>
#include <App/Extension.h>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string ExtensionContainerPy::representation() const
{
    return {"<extension>"};
}

int  ExtensionContainerPy::initialization() {

    if (!this->ob_type->tp_dict) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }

    ExtensionContainer::ExtensionIterator it = this->getExtensionContainerPtr()->extensionBegin();
    for(; it != this->getExtensionContainerPtr()->extensionEnd(); ++it) {

        // The PyTypeObject is shared by all instances of this type and therefore
        // we have to add new methods only once.
        PyObject* obj = (*it).second->getExtensionPyObject();
        PyMethodDef* meth = obj->ob_type->tp_methods;
        PyTypeObject *type = this->ob_type;
        PyObject *dict = type->tp_dict;

        // make sure to do the initialization only once
        if (meth->ml_name) {
            PyObject* item = PyDict_GetItemString(dict, meth->ml_name);
            if (!item) {
                // Note: this adds the methods to the type object to make sure
                // it appears in the call tips. The function will not be bound
                // to an instance
                Py_INCREF(dict);
                while (meth->ml_name) {
                    PyObject *func;
                    func = PyCFunction_New(meth, 0);
                    if (!func)
                        break;
                    if (PyDict_SetItemString(dict, meth->ml_name, func) < 0)
                        break;
                    Py_DECREF(func);
                    ++meth;
                }

                Py_DECREF(dict);
            }
        }

        Py_DECREF(obj);
    }
    return 1;
}

int  ExtensionContainerPy::finalization() {
/*
    //we need to delete all added python extensions, as we are the owner! 
    ExtensionContainer::ExtensionIterator it = this->getExtensionContainerPtr()->extensionBegin();
    for(; it != this->getExtensionContainerPtr()->extensionEnd(); ++it) {
        if((*it).second->isPythonExtension())
            delete (*it).second;
    }*/
    return 1;
}

PyObject* ExtensionContainerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of @self.export.Name@ and the Twin object 
    return nullptr;
}

// constructor method
int ExtensionContainerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject *ExtensionContainerPy::getCustomAttributes(const char* attr) const
{
    if (Base::streq(attr, "__dict__")) {
        PyObject* dict = PyDict_New();
        PyObject* props = PropertyContainerPy::getCustomAttributes("__dict__");
        if (props && PyDict_Check(props)) {
            PyDict_Merge(dict, props, 0);
            Py_DECREF(props);
        }

        ExtensionContainer::ExtensionIterator it = this->getExtensionContainerPtr()->extensionBegin();
        for (; it != this->getExtensionContainerPtr()->extensionEnd(); ++it) {
            // The PyTypeObject is shared by all instances of this type and therefore
            // we have to add new methods only once.
            PyObject* obj = (*it).second->getExtensionPyObject();
            PyTypeObject *tp = Py_TYPE(obj);
            if (tp && tp->tp_dict) {
                Py_XINCREF(tp->tp_dict);
                PyDict_Merge(dict, tp->tp_dict, 0);
                Py_XDECREF(tp->tp_dict);
            }
            Py_DECREF(obj);
        }

        return dict;
    }
    // Search for the method called 'attr' in the extensions. If the search with
    // Py_FindMethod is successful then a PyCFunction_New instance is returned
    // with the PyObject pointer of the extension to make sure the method will
    // be called for the correct instance.
    PyObject *func = nullptr;
    ExtensionContainer::ExtensionIterator it = this->getExtensionContainerPtr()->extensionBegin();
    for (; it != this->getExtensionContainerPtr()->extensionEnd(); ++it) {
        // The PyTypeObject is shared by all instances of this type and therefore
        // we have to add new methods only once.
        PyObject* obj = (*it).second->getExtensionPyObject();
        PyObject *nameobj = PyUnicode_FromString(attr);
        func = PyObject_GenericGetAttr(obj, nameobj);
        Py_DECREF(nameobj);
        Py_DECREF(obj);
        if (func && PyCFunction_Check(func)) {
            PyCFunctionObject* cfunc = reinterpret_cast<PyCFunctionObject*>(func);

            // OK, that's what we wanted
            if (cfunc->m_self == obj)
                break;
            // otherwise cleanup the result again
            Py_DECREF(func);
            func = nullptr;
        }
        PyErr_Clear(); // clear the error set inside Py_FindMethod
    }

    return func;
}

int ExtensionContainerPy::setCustomAttributes(const char* /*attr*/, PyObject * /*obj*/)
{
    return 0;
}

PyObject* ExtensionContainerPy::hasExtension(PyObject *args) {

    char *type;
    PyObject *deriv = Py_True;
    if (!PyArg_ParseTuple(args, "s|O!", &type, &PyBool_Type, &deriv))
        return nullptr;

    //get the extension type asked for
    bool derived = Base::asBoolean(deriv);
    Base::Type extension =  Base::Type::fromName(type);
    if (extension.isBad() || !extension.isDerivedFrom(App::Extension::getExtensionClassTypeId())) {
        std::stringstream str;
        str << "No extension found of type '" << type << "'" << std::ends;
        throw Py::TypeError(str.str());
    }

    bool val = false;
    if (getExtensionContainerPtr()->hasExtension(extension, derived)) {
        val = true;
    }

    return PyBool_FromLong(val ? 1 : 0);
}

PyObject* ExtensionContainerPy::addExtension(PyObject *args) {

    char *typeId;
    PyObject* proxy = nullptr;
    if (!PyArg_ParseTuple(args, "s|O", &typeId, &proxy))
        return nullptr;

    if (proxy) {
        PyErr_SetString(PyExc_DeprecationWarning, "Second argument is deprecated. It is ignored and will be removed in future versions. "
                                                  "The default Python feature proxy is used for extension method overrides.");
        PyErr_Print();
    }

    //get the extension type asked for
    Base::Type extension =  Base::Type::fromName(typeId);
    if (extension.isBad() || !extension.isDerivedFrom(App::Extension::getExtensionClassTypeId())) {
        std::stringstream str;
        str << "No extension found of type '" << typeId << "'" << std::ends;
        throw Py::TypeError(str.str());
    }

    //register the extension
    App::Extension* ext = static_cast<App::Extension*>(extension.createInstance());
    //check if this really is a python extension!
    if (!ext->isPythonExtension()) {
        delete ext;
        std::stringstream str;
        str << "Extension is not a python addable version: '" << typeId << "'" << std::ends;
        throw Py::TypeError(str.str());
    }

    GetApplication().signalBeforeAddingDynamicExtension(*getExtensionContainerPtr(), typeId);
    ext->initExtension(getExtensionContainerPtr());

    // The PyTypeObject is shared by all instances of this type and therefore
    // we have to add new methods only once.
    PyObject* obj = ext->getExtensionPyObject();
    PyMethodDef* meth = obj->ob_type->tp_methods;
    PyTypeObject *type = this->ob_type;
    PyObject *dict = type->tp_dict;

    // make sure to do the initialization only once
    if (meth->ml_name) {
        PyObject* item = PyDict_GetItemString(dict, meth->ml_name);
        if (!item) {
            // Note: this adds the methods to the type object to make sure
            // it appears in the call tips. The function will not be bound
            // to an instance
            Py_INCREF(dict);
            while (meth->ml_name) {
                PyObject *func;
                func = PyCFunction_New(meth, 0);
                if (!func)
                    break;
                if (PyDict_SetItemString(dict, meth->ml_name, func) < 0)
                    break;
                Py_DECREF(func);
                ++meth;
            }

            Py_DECREF(dict);
        }
    }

    Py_DECREF(obj);
    
    //throw the appropriate event
    GetApplication().signalAddedDynamicExtension(*getExtensionContainerPtr(), typeId);

    Py_Return;
}
