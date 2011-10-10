/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <stdlib.h>
#endif

#include "PyObjectBase.h"
#include "Console.h"

using namespace Base;

// Constructor
PyObjectBase::PyObjectBase(void* p,PyTypeObject *T)
  : _pcTwinPointer(p), parent(0), attribute(0)
{
    this->ob_type = T;
    _Py_NewReference(this);
#ifdef FC_LOGPYOBJECTS
    Base::Console().Log("PyO+: %s (%p)\n",T->tp_name, this);
#endif
    StatusBits.set(0); // valid, the second bit is NOT set, i.e. it's mutable
}

/// destructor
PyObjectBase::~PyObjectBase() 
{
#ifdef FC_LOGPYOBJECTS
    Base::Console().Log("PyO-: %s (%p)\n",this->ob_type->tp_name, this);
#endif
    if (this->parent)
        this->parent->DecRef();
    if (this->attribute)
        free(this->attribute); /* it's a strdup */
}

/*------------------------------
 * PyObjectBase Type		-- Every class, even the abstract one should have a Type
------------------------------*/

/** \brief 
 * To prevent subclasses of PyTypeObject to be subclassed in Python we should remove 
 * the Py_TPFLAGS_BASETYPE flag. For example, the classes App::VectorPy and App::MatrixPy
 * have removed this flag and its Python proxies App.Vector and App.Matrix cannot be subclassed.
 * In case we want to allow to derive from subclasses of PyTypeObject in Python
 * we must either reimplment tp_new, tp_dealloc, tp_getattr, tp_setattr, tp_repr or set them to
 * 0 and define tp_base as 0.
 */

PyTypeObject PyObjectBase::Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                                      /*ob_size*/
    "PyObjectBase",                                         /*tp_name*/
    sizeof(PyObjectBase),                                   /*tp_basicsize*/
    0,                                                      /*tp_itemsize*/
    /* --- methods ---------------------------------------------- */
    PyDestructor,                                           /*tp_dealloc*/
    0,                                                      /*tp_print*/
    __getattr,                                              /*tp_getattr*/
    __setattr,                                              /*tp_setattr*/
    0,                                                      /*tp_compare*/
    __repr,                                                 /*tp_repr*/
    0,                                                      /*tp_as_number*/
    0,                                                      /*tp_as_sequence*/
    0,                                                      /*tp_as_mapping*/
    0,                                                      /*tp_hash*/
    0,                                                      /*tp_call */
    0,                                                      /*tp_str  */
    0,                                                      /*tp_getattro*/
    0,                                                      /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    0,                                                      /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_CLASS,              /*tp_flags */
    "The most base class for Python binding",               /*tp_doc */
    0,                                                      /*tp_traverse */
    0,                                                      /*tp_clear */
    0,                                                      /*tp_richcompare */
    0,                                                      /*tp_weaklistoffset */
    0,                                                      /*tp_iter */
    0,                                                      /*tp_iternext */
    0,                                                      /*tp_methods */
    0,                                                      /*tp_members */
    0,                                                      /*tp_getset */
    0,                                                      /*tp_base */
    0,                                                      /*tp_dict */
    0,                                                      /*tp_descr_get */
    0,                                                      /*tp_descr_set */
    0,                                                      /*tp_dictoffset */
    0,                                                      /*tp_init */
    0,                                                      /*tp_alloc */
    0,                                                      /*tp_new */
    0,                                                      /*tp_free   Low-level free-memory routine */
    0,                                                      /*tp_is_gc  For PyObject_IS_GC */
    0,                                                      /*tp_bases */
    0,                                                      /*tp_mro    method resolution order */
    0,                                                      /*tp_cache */
    0,                                                      /*tp_subclasses */
    0,                                                      /*tp_weaklist */
    0                                                       /*tp_del */
};

/*------------------------------
 * PyObjectBase Methods 	-- Every class, even the abstract one should have a Methods
------------------------------*/
PyMethodDef PyObjectBase::Methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

/*------------------------------
 * PyObjectBase Parents		-- Every class, even the abstract one should have parents
------------------------------*/
PyParentObject PyObjectBase::Parents[] = {&PyObjectBase::Type, NULL};

PyObject* PyObjectBase::__getattr(PyObject * obj, char *attr)
{
    // This should be the entry in Type
    PyObjectBase* pyObj = static_cast<PyObjectBase*>(obj);
    if (!pyObj->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return NULL;
    }

    PyObject* value = pyObj->_getattr(attr);
#if 1
    if (value && PyObject_TypeCheck(value, &(PyObjectBase::Type))) {
        if (!static_cast<PyObjectBase*>(value)->isConst())
            static_cast<PyObjectBase*>(value)->setAttributeOf(attr, pyObj);
    }
#endif
    return value;
}

int PyObjectBase::__setattr(PyObject *obj, char *attr, PyObject *value)
{
    //FIXME: In general we don't allow to delete attributes (i.e. value=0). However, if we want to allow
    //we must check then in _setattr() of all subclasses whether value is 0.
    if ( value==0 ) {
        PyErr_Format(PyExc_AttributeError, "Cannot delete attribute: '%s'", attr);
        return -1;
    }
    else if (!static_cast<PyObjectBase*>(obj)->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return -1;
    }

    int ret = static_cast<PyObjectBase*>(obj)->_setattr(attr, value);
#if 1
    if (ret == 0) {
        static_cast<PyObjectBase*>(obj)->startNotify();
    }
#endif
    return ret;
}

/*------------------------------
 * PyObjectBase attributes	-- attributes
------------------------------*/
PyObject *PyObjectBase::_getattr(char *attr)
{
    if (streq(attr, "__class__")) {
        // Note: We must return the type object here, 
        // so that our own types feel as really Python objects 
        Py_INCREF(this->ob_type);
        return (PyObject *)(this->ob_type);
    }
    else if (streq(attr, "__members__")) {
        // Use __dict__ instead as __members__ is deprecated
        return NULL;
    }
    else if (streq(attr,"__dict__")) {
        // Return the default dict
        PyTypeObject *tp = this->ob_type;
        Py_XINCREF(tp->tp_dict);
        return tp->tp_dict;
    }
    else if (streq(attr,"softspace")) {
        // Internal Python stuff
        return NULL;
    }
    else {
        // As fallback solution use Python's default method to get generic attributes
        PyObject *w, *res;
        w = PyString_InternFromString(attr);
        if (w != NULL) {
            res = PyObject_GenericGetAttr(this, w);
            Py_XDECREF(w);
            return res;
        } else {
            // Throw an exception for unknown attributes
            PyTypeObject *tp = this->ob_type;
            PyErr_Format(PyExc_AttributeError, "%.50s instance has no attribute '%.400s'", tp->tp_name, attr);
            return NULL;
        }
    }
}

int PyObjectBase::_setattr(char *attr, PyObject *value)
{
    if (streq(attr,"softspace"))
        return -1; // filter out softspace
    PyObject *w;
    // As fallback solution use Python's default method to get generic attributes
    w = PyString_InternFromString(attr); // new reference
    if (w != NULL) {
        // call methods from tp_getset if defined
        int res = PyObject_GenericSetAttr(this, w, value);
        Py_DECREF(w);
        return res;
    } else {
        // Throw an exception for unknown attributes
        PyTypeObject *tp = this->ob_type;
        PyErr_Format(PyExc_AttributeError, "%.50s instance has no attribute '%.400s'", tp->tp_name, attr);
        return -1;
    }
}

/*------------------------------
 * PyObjectBase repr    representations
------------------------------*/
PyObject *PyObjectBase::_repr(void)
{
    std::stringstream a;
    a << "<base object at " << _pcTwinPointer << ">";
# ifdef FCDebug
    Console().Log("PyObjectBase::_repr() not overwritten representation!");
# endif
    return Py_BuildValue("s", a.str().c_str());
}

void PyObjectBase::setAttributeOf(const char* attr, const PyObjectBase* par)
{
    if (this->parent != par) {
        Py_XDECREF(this->parent);
        this->parent = const_cast<PyObjectBase*>(par);
        Py_XINCREF(this->parent);
    }

    if (this->attribute) {
        if (strcmp(this->attribute, attr) != 0) {
            free(this->attribute);
#if defined (__GNUC__)
            this->attribute =  strdup(attr);
#else
            this->attribute = _strdup(attr);
#endif
        }
    }
    else {
#if defined (__GNUC__)
        this->attribute =  strdup(attr);
#else
        this->attribute = _strdup(attr);
#endif
    }
}

void PyObjectBase::startNotify()
{
    if (this->attribute && this->parent) {
        __setattr(this->parent, this->attribute, this);
        if (PyErr_Occurred())
            PyErr_Clear();
    }
}
