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

PyObject* Base::BaseExceptionFreeCADError = 0;

// Constructor
PyObjectBase::PyObjectBase(void* p,PyTypeObject *T)
  : _pcTwinPointer(p), parent(0), attribute(0)
{
    Py_TYPE(this) = T;
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
    Base::Console().Log("PyO-: %s (%p)\n",Py_TYPE(this)->tp_name, this);
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
    PyVarObject_HEAD_INIT(&PyType_Type,0)
    "PyObjectBase",                                         /*tp_name*/
    sizeof(PyObjectBase),                                   /*tp_basicsize*/
    0,                                                      /*tp_itemsize*/
    /* --- methods ---------------------------------------------- */
    PyDestructor,                                           /*tp_dealloc*/
    0,                                                      /*tp_print*/
    0,                                                      /*tp_getattr*/
    0,                                                      /*tp_setattr*/
    0,                                                      /*tp_compare*/
    __repr,                                                 /*tp_repr*/
    0,                                                      /*tp_as_number*/
    0,                                                      /*tp_as_sequence*/
    0,                                                      /*tp_as_mapping*/
    0,                                                      /*tp_hash*/
    0,                                                      /*tp_call */
    0,                                                      /*tp_str  */
    __getattro,                                             /*tp_getattro*/
    __setattro,                                             /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    0,                                                      /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_DEFAULT,                 /*tp_flags */
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
    0,                                                      /*tp_del */
    0                                                       /*tp_version_tag */
};

/*------------------------------
 * PyObjectBase Methods 	-- Every class, even the abstract one should have a Methods
------------------------------*/
PyMethodDef PyObjectBase::Methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyObject* PyObjectBase::__getattro(PyObject * obj, PyObject *attro)
{
    char *attr;
#if PY_MAJOR_VERSION >= 3
    attr = PyUnicode_AsUTF8(attro);
#else
    attr = PyString_AsString(attro);
#endif
    // This should be the entry in Type
    PyObjectBase* pyObj = static_cast<PyObjectBase*>(obj);
    if (!pyObj->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return NULL;
    }

    PyObject* value = pyObj->_getattro(attro);
#if 1
    if (value && PyObject_TypeCheck(value, &(PyObjectBase::Type))) {
        if (!static_cast<PyObjectBase*>(value)->isConst())
            static_cast<PyObjectBase*>(value)->setAttributeOf(attr, pyObj);
    }
#endif
    return value;
}

int PyObjectBase::__setattro(PyObject *obj, PyObject *attro, PyObject *value)
{
    char *attr;
#if PY_MAJOR_VERSION >= 3
    attr = PyUnicode_AsUTF8(attro);
#else
    attr = PyString_AsString(attro);
#endif
    //FIXME: In general we don't allow to delete attributes (i.e. value=0). However, if we want to allow
    //we must check then in _setattro() of all subclasses whether value is 0.
    if ( value==0 ) {
        PyErr_Format(PyExc_AttributeError, "Cannot delete attribute: '%s'", attr);
        return -1;
    }
    else if (!static_cast<PyObjectBase*>(obj)->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return -1;
    }

    int ret = static_cast<PyObjectBase*>(obj)->_setattro(attro, value);
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
PyObject *PyObjectBase::_getattro(PyObject *attro)
{
    char *attr;
#if PY_MAJOR_VERSION >= 3
    if (PyUnicode_Check(attro))
        attr = PyUnicode_AsUTF8(attro);
#else
    if (PyString_Check(attro))
        attr = PyString_AsString(attro);
#endif
    if (streq(attr, "__class__")) {
        // Note: We must return the type object here, 
        // so that our own types feel as really Python objects 
        Py_INCREF(Py_TYPE(this));
        return (PyObject *)(Py_TYPE(this));
    }
    else if (streq(attr, "__members__")) {
        // Use __dict__ instead as __members__ is deprecated
        return NULL;
    }
    else if (streq(attr,"__dict__")) {
        // Return the default dict
        PyTypeObject *tp = Py_TYPE(this);
        Py_XINCREF(tp->tp_dict);
        return tp->tp_dict;
    }
    else if (streq(attr,"softspace")) {
        // Internal Python stuff
        return NULL;
    }
    else {
        // As fallback solution use Python's default method to get generic attributes
        PyObject *res;
        if (attro != NULL) {
            res = PyObject_GenericGetAttr(this, attro);
            return res;
        } else {
            // Throw an exception for unknown attributes
            PyTypeObject *tp = Py_TYPE(this);
            PyErr_Format(PyExc_AttributeError, "%.50s instance has no attribute '%.400s'", tp->tp_name, attr);
            return NULL;
        }
    }
}

int PyObjectBase::_setattro(PyObject *attro, PyObject *value)
{
    char *attr;
#if PY_MAJOR_VERSION >= 3
    attr = PyUnicode_AsUTF8(attro);
#else
    attr = PyString_AsString(attro);
#endif
    if (streq(attr,"softspace"))
        return -1; // filter out softspace
    // As fallback solution use Python's default method to get generic attributes
    if (attro != NULL) {
        // call methods from tp_getset if defined
        int res = PyObject_GenericSetAttr(this, attro, value);
        return res;
    } else {
        // Throw an exception for unknown attributes
        PyTypeObject *tp = Py_TYPE(this);
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
        PyObject *attro;
#if PY_MAJOR_VERSION >= 3
        attro = PyUnicode_FromString(this->attribute);
#else
        attro = PyString_FromString(this->attribute);
#endif
        __setattro(this->parent, attro, this);
        if (PyErr_Occurred())
            PyErr_Clear();
    }
}
