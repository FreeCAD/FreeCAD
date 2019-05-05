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
#include "Interpreter.h"

using namespace Base;

PyObject* Base::BaseExceptionFreeCADError = 0;
PyObject* Base::BaseExceptionFreeCADAbort = 0;

// Constructor
PyObjectBase::PyObjectBase(void* p,PyTypeObject *T)
  : _pcTwinPointer(p), attrDict(0)
{
    Py_TYPE(this) = T;
    _Py_NewReference(this);
#ifdef FC_LOGPYOBJECTS
    Base::Console().Log("PyO+: %s (%p)\n",T->tp_name, this);
#endif
    StatusBits.set(Valid); // valid, the second bit is NOT set, i.e. it's mutable
    StatusBits.set(Notify);
}

/// destructor
PyObjectBase::~PyObjectBase() 
{
    PyGILStateLocker lock;
#ifdef FC_LOGPYOBJECTS
    Base::Console().Log("PyO-: %s (%p)\n",Py_TYPE(this)->tp_name, this);
#endif
    Py_XDECREF(attrDict);
}

/*------------------------------
 * PyObjectBase Type		-- Every class, even the abstract one should have a Type
------------------------------*/

/** \brief 
 * To prevent subclasses of PyTypeObject to be subclassed in Python we should remove 
 * the Py_TPFLAGS_BASETYPE flag. For example, the classes App::VectorPy and App::MatrixPy
 * have removed this flag and its Python proxies App.Vector and App.Matrix cannot be subclassed.
 * In case we want to allow to derive from subclasses of PyTypeObject in Python
 * we must either reimplement tp_new, tp_dealloc, tp_getattr, tp_setattr, tp_repr or set them to
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
#if PY_MAJOR_VERSION >= 3
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_DEFAULT,                 /*tp_flags */
#else
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_CLASS,              /*tp_flags */
#endif
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
#if PY_MAJOR_VERSION >= 3
    ,0                                                      /*tp_finalize */
#endif
};

/*------------------------------
 * PyObjectBase Methods 	-- Every class, even the abstract one should have a Methods
------------------------------*/
PyMethodDef PyObjectBase::Methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyObject* PyObjectBase::__getattro(PyObject * obj, PyObject *attro)
{
    const char *attr;
#if PY_MAJOR_VERSION >= 3
    attr = PyUnicode_AsUTF8(attro);
#else
    attr = PyString_AsString(attro);
#endif

    // For the __class__ attribute get it directly as with
    // ExtensionContainerPy::getCustomAttributes we may get
    // the wrong type object (#0003311)
    if (streq(attr, "__class__")) {
        PyObject* res = PyObject_GenericGetAttr(obj, attro);
        if (res)
            return res;
    }

    // This should be the entry in Type
    PyObjectBase* pyObj = static_cast<PyObjectBase*>(obj);
    if (!pyObj->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return NULL;
    }

    // If an attribute references this as parent then reset it (bug #0002902)
    PyObject* cur = pyObj->getTrackedAttribute(attr);
    if (cur) {
        if (PyObject_TypeCheck(cur, &(PyObjectBase::Type))) {
            PyObjectBase* base = static_cast<PyObjectBase*>(cur);
            base->resetAttribute();
            pyObj->untrackAttribute(attr);
        }
    }

    PyObject* value = pyObj->_getattr(attr);
#if 1
    if (value && PyObject_TypeCheck(value, &(PyObjectBase::Type))) {
        if (!static_cast<PyObjectBase*>(value)->isConst() &&
            !static_cast<PyObjectBase*>(value)->isNotTracking()) {
            static_cast<PyObjectBase*>(value)->setAttributeOf(attr, pyObj);
            pyObj->trackAttribute(attr, value);
        }
    }
    else if (value && PyCFunction_Check(value)) {
        // ExtensionContainerPy::initialization() transfers the methods of an
        // extension object by creating PyCFunction objects.
        // At this point no 'self' object is passed but is handled and determined
        // in ExtensionContainerPy::getCustomAttributes().
        // So, if we come through this section then it's an indication that
        // something is wrong with the Python types. For example, a C++ class
        // that adds an extension uses the same Python type as a wrapper than
        // another C++ class without this extension.
        PyCFunctionObject* cfunc = reinterpret_cast<PyCFunctionObject*>(value);
        if (!cfunc->m_self) {
            Py_DECREF(cfunc);
            value = 0;
            PyErr_Format(PyExc_AttributeError, "<no object bound to built-in method %s>", attr);
        }
    }
#endif
    return value;
}

int PyObjectBase::__setattro(PyObject *obj, PyObject *attro, PyObject *value)
{
    const char *attr;
#if PY_MAJOR_VERSION >= 3
    attr = PyUnicode_AsUTF8(attro);
#else
    attr = PyString_AsString(attro);
#endif

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

    // If an attribute references this as parent then reset it
    // before setting the new attribute
    PyObject* cur = static_cast<PyObjectBase*>(obj)->getTrackedAttribute(attr);
    if (cur) {
        if (PyObject_TypeCheck(cur, &(PyObjectBase::Type))) {
            PyObjectBase* base = static_cast<PyObjectBase*>(cur);
            base->resetAttribute();
            static_cast<PyObjectBase*>(obj)->untrackAttribute(attr);
        }
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
PyObject *PyObjectBase::_getattr(const char *attr)
{
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
        PyObject *w, *res;
#if PY_MAJOR_VERSION >= 3
        w = PyUnicode_InternFromString(attr);
#else
        w = PyString_InternFromString(attr);
#endif
        if (w != NULL) {
            res = PyObject_GenericGetAttr(this, w);
            Py_XDECREF(w);
            return res;
        } else {
            // Throw an exception for unknown attributes
            PyTypeObject *tp = Py_TYPE(this);
            PyErr_Format(PyExc_AttributeError, "%.50s instance has no attribute '%.400s'", tp->tp_name, attr);
            return NULL;
        }
    }
}

int PyObjectBase::_setattr(const char *attr, PyObject *value)
{
    if (streq(attr,"softspace"))
        return -1; // filter out softspace
    PyObject *w;
    // As fallback solution use Python's default method to get generic attributes
#if PY_MAJOR_VERSION >= 3
    w = PyUnicode_InternFromString(attr); // new reference
#else
    w = PyString_InternFromString(attr); // new reference
#endif
    if (w != NULL) {
        // call methods from tp_getset if defined
        int res = PyObject_GenericSetAttr(this, w, value);
        Py_DECREF(w);
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

void PyObjectBase::resetAttribute()
{
    if (attrDict) {
        // This is the attribute name to the parent structure
        // which we search for in the dict
#if PY_MAJOR_VERSION < 3
        PyObject* key1 = PyString_FromString("__attribute_of_parent__");
        PyObject* key2 = PyString_FromString("__instance_of_parent__");
#else
        PyObject* key1 = PyBytes_FromString("__attribute_of_parent__");
        PyObject* key2 = PyBytes_FromString("__instance_of_parent__");
#endif
        PyObject* attr = PyDict_GetItem(attrDict, key1);
        PyObject* inst = PyDict_GetItem(attrDict, key2);
        if (attr) {
            PyDict_DelItem(attrDict, key1);
        }
        if (inst) {
            PyDict_DelItem(attrDict, key2);
        }
        Py_DECREF(key1);
        Py_DECREF(key2);
    }
}

void PyObjectBase::setAttributeOf(const char* attr, PyObject* par)
{
    if (!attrDict) {
        attrDict = PyDict_New();
    }
#if PY_MAJOR_VERSION < 3
    PyObject* key1 = PyString_FromString("__attribute_of_parent__");
    PyObject* key2 = PyString_FromString("__instance_of_parent__");
    PyObject* attro = PyString_FromString(attr);
#else
    PyObject* key1 = PyBytes_FromString("__attribute_of_parent__");
    PyObject* key2 = PyBytes_FromString("__instance_of_parent__");
    PyObject* attro = PyUnicode_FromString(attr);
#endif
    PyDict_SetItem(attrDict, key1, attro);
    PyDict_SetItem(attrDict, key2, par);
    Py_DECREF(attro);
    Py_DECREF(key1);
    Py_DECREF(key2);
}

void PyObjectBase::startNotify()
{
    if (!shouldNotify())
        return;

    if (attrDict) {
        // This is the attribute name to the parent structure
        // which we search for in the dict
#if PY_MAJOR_VERSION < 3
        PyObject* key1 = PyString_FromString("__attribute_of_parent__");
        PyObject* key2 = PyString_FromString("__instance_of_parent__");
#else
        PyObject* key1 = PyBytes_FromString("__attribute_of_parent__");
        PyObject* key2 = PyBytes_FromString("__instance_of_parent__");
#endif
        PyObject* attr = PyDict_GetItem(attrDict, key1);
        PyObject* parent = PyDict_GetItem(attrDict, key2);
        if (attr && parent) {
            // Inside __setattr of the parent structure the 'attr'
            // is being removed from the dict and thus its reference
            // counter will be decremented. To avoid to be deleted we
            // must tmp. increment it and afterwards decrement it again.
            Py_INCREF(parent);
            Py_INCREF(attr);
            Py_INCREF(this);

            __setattro(parent, attr, this);

            Py_DECREF(parent); // might be destroyed now
            Py_DECREF(attr); // might be destroyed now
            Py_DECREF(this); // might be destroyed now

            if (PyErr_Occurred())
                PyErr_Clear();
        }
        Py_DECREF(key1);
        Py_DECREF(key2);
    }
}

PyObject* PyObjectBase::getTrackedAttribute(const char* attr)
{
    PyObject* obj = 0;
    if (attrDict) {
        obj = PyDict_GetItemString(attrDict, attr);
    }
    return obj;
}

void PyObjectBase::trackAttribute(const char* attr, PyObject* obj)
{
    if (!attrDict) {
        attrDict = PyDict_New();
    }

    PyDict_SetItemString(attrDict, attr, obj);
}

void PyObjectBase::untrackAttribute(const char* attr)
{
    if (attrDict) {
        PyDict_DelItemString(attrDict, attr);
    }
}

void PyObjectBase::clearAttributes()
{
    if (attrDict) {
        PyDict_Clear(attrDict);
    }
}
