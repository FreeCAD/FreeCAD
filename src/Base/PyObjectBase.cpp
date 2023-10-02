/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "PyObjectBase.h"
#include "Console.h"
#include "Interpreter.h"


#define ATTR_TRACKING

using namespace Base;

PyObject* Base::PyExc_FC_GeneralError = nullptr;
PyObject* Base::PyExc_FC_FreeCADAbort = nullptr;
PyObject* Base::PyExc_FC_XMLBaseException = nullptr;
PyObject* Base::PyExc_FC_XMLParseException = nullptr;
PyObject* Base::PyExc_FC_XMLAttributeError = nullptr;
PyObject* Base::PyExc_FC_UnknownProgramOption = nullptr;
PyObject* Base::PyExc_FC_BadFormatError = nullptr;
PyObject* Base::PyExc_FC_BadGraphError = nullptr;
PyObject* Base::PyExc_FC_ExpressionError = nullptr;
PyObject* Base::PyExc_FC_ParserError = nullptr;
PyObject* Base::PyExc_FC_CADKernelError = nullptr;

typedef struct {
    PyObject_HEAD
    PyObject* baseobject;
    PyObject* weakreflist;  /* List of weak references */
} PyBaseProxy;

// Constructor
PyObjectBase::PyObjectBase(void* voidp, PyTypeObject *T)
  : _pcTwinPointer(voidp)
{
#if PY_VERSION_HEX < 0x030b0000
    Py_TYPE(this) = T;
#else
    this->ob_type = T;
#endif
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
    if (baseProxy && reinterpret_cast<PyBaseProxy*>(baseProxy)->baseobject == this)
        Py_DECREF(baseProxy);
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

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

static void
PyBaseProxy_dealloc(PyObject* self)
{
    /* Clear weakrefs first before calling any destructors */
    if (reinterpret_cast<PyBaseProxy*>(self)->weakreflist)
        PyObject_ClearWeakRefs(self);
    Py_TYPE(self)->tp_free(self);
}

static PyTypeObject PyBaseProxyType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "PyBaseProxy",                                          /*tp_name*/
    sizeof(PyBaseProxy),                                    /*tp_basicsize*/
    0,                                                      /*tp_itemsize*/
    PyBaseProxy_dealloc,                                    /*tp_dealloc*/
#if PY_VERSION_HEX >= 0x03080000
    0,                                                      /*tp_vectorcall_offset*/
#else
    nullptr,                                                /*tp_print*/
#endif
    nullptr,                                                /*tp_getattr*/
    nullptr,                                                /*tp_setattr*/
    nullptr,                                                /*tp_compare*/
    nullptr,                                                /*tp_repr*/
    nullptr,                                                /*tp_as_number*/
    nullptr,                                                /*tp_as_sequence*/
    nullptr,                                                /*tp_as_mapping*/
    nullptr,                                                /*tp_hash*/
    nullptr,                                                /*tp_call */
    nullptr,                                                /*tp_str  */
    nullptr,                                                /*tp_getattro*/
    nullptr,                                                /*tp_setattro*/
    nullptr,                                                /*tp_as_buffer*/
    Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DEFAULT,               /*tp_flags */
    "Proxy class",                                          /*tp_doc */
    nullptr,                                                /*tp_traverse */
    nullptr,                                                /*tp_clear */
    nullptr,                                                /*tp_richcompare */
    offsetof(PyBaseProxy, weakreflist),                     /*tp_weaklistoffset */
    nullptr,                                                /*tp_iter */
    nullptr,                                                /*tp_iternext */
    nullptr,                                                /*tp_methods */
    nullptr,                                                /*tp_members */
    nullptr,                                                /*tp_getset */
    nullptr,                                                /*tp_base */
    nullptr,                                                /*tp_dict */
    nullptr,                                                /*tp_descr_get */
    nullptr,                                                /*tp_descr_set */
    0,                                                      /*tp_dictoffset */
    nullptr,                                                /*tp_init */
    nullptr,                                                /*tp_alloc */
    nullptr,                                                /*tp_new */
    nullptr,                                                /*tp_free   Low-level free-memory routine */
    nullptr,                                                /*tp_is_gc  For PyObject_IS_GC */
    nullptr,                                                /*tp_bases */
    nullptr,                                                /*tp_mro    method resolution order */
    nullptr,                                                /*tp_cache */
    nullptr,                                                /*tp_subclasses */
    nullptr,                                                /*tp_weaklist */
    nullptr,                                                /*tp_del */
    0,                                                      /*tp_version_tag */
    nullptr                                                 /*tp_finalize */
#if PY_VERSION_HEX >= 0x03090000
    ,0                                                      /*tp_vectorcall */
#elif PY_VERSION_HEX >= 0x03080000
    ,0                                                      /*tp_vectorcall */
    /* bpo-37250: kept for backwards compatibility in CPython 3.8 only */
    ,0                                                      /*tp_print */
#endif
};

PyTypeObject PyObjectBase::Type = {
    PyVarObject_HEAD_INIT(&PyType_Type,0)
    "PyObjectBase",                                         /*tp_name*/
    sizeof(PyObjectBase),                                   /*tp_basicsize*/
    0,                                                      /*tp_itemsize*/
    /* --- methods ---------------------------------------------- */
    PyDestructor,                                           /*tp_dealloc*/
#if PY_VERSION_HEX >= 0x03080000
    0,                                                      /*tp_vectorcall_offset*/
#else
    nullptr,                                                /*tp_print*/
#endif
    nullptr,                                                /*tp_getattr*/
    nullptr,                                                /*tp_setattr*/
    nullptr,                                                /*tp_compare*/
    __repr,                                                 /*tp_repr*/
    nullptr,                                                /*tp_as_number*/
    nullptr,                                                /*tp_as_sequence*/
    nullptr,                                                /*tp_as_mapping*/
    nullptr,                                                /*tp_hash*/
    nullptr,                                                /*tp_call */
    nullptr,                                                /*tp_str  */
    __getattro,                                             /*tp_getattro*/
    __setattro,                                             /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    nullptr,                                                /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_DEFAULT,                 /*tp_flags */
    "The most base class for Python binding",               /*tp_doc */
    nullptr,                                                /*tp_traverse */
    nullptr,                                                /*tp_clear */
    nullptr,                                                /*tp_richcompare */
    0,                                                      /*tp_weaklistoffset */
    nullptr,                                                /*tp_iter */
    nullptr,                                                /*tp_iternext */
    nullptr,                                                /*tp_methods */
    nullptr,                                                /*tp_members */
    nullptr,                                                /*tp_getset */
    nullptr,                                                /*tp_base */
    nullptr,                                                /*tp_dict */
    nullptr,                                                /*tp_descr_get */
    nullptr,                                                /*tp_descr_set */
    0,                                                      /*tp_dictoffset */
    nullptr,                                                /*tp_init */
    nullptr,                                                /*tp_alloc */
    nullptr,                                                /*tp_new */
    nullptr,                                                /*tp_free   Low-level free-memory routine */
    nullptr,                                                /*tp_is_gc  For PyObject_IS_GC */
    nullptr,                                                /*tp_bases */
    nullptr,                                                /*tp_mro    method resolution order */
    nullptr,                                                /*tp_cache */
    nullptr,                                                /*tp_subclasses */
    nullptr,                                                /*tp_weaklist */
    nullptr,                                                /*tp_del */
    0,                                                      /*tp_version_tag */
    nullptr                                                 /*tp_finalize */
#if PY_VERSION_HEX >= 0x03090000
    ,0                                                      /*tp_vectorcall */
#elif PY_VERSION_HEX >= 0x03080000
    ,0                                                      /*tp_vectorcall */
    /* bpo-37250: kept for backwards compatibility in CPython 3.8 only */
    ,0                                                      /*tp_print */
#endif
};

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

PyObject* createWeakRef(PyObjectBase* ptr)
{
    static bool init = false;
    if (!init) {
       init = true;
       if (PyType_Ready(&PyBaseProxyType) < 0)
           return nullptr;
    }

    PyObject* proxy = ptr->baseProxy;
    if (!proxy) {
        proxy = PyType_GenericAlloc(&PyBaseProxyType, 0);
        ptr->baseProxy = proxy;
        reinterpret_cast<PyBaseProxy*>(proxy)->baseobject = ptr;
    }

    PyObject* ref = PyWeakref_NewRef(proxy, nullptr);
    return ref;
}

PyObjectBase* getFromWeakRef(PyObject* ref)
{
    if (ref) {
        PyObject* proxy = PyWeakref_GetObject(ref);
        if (proxy && PyObject_TypeCheck(proxy, &PyBaseProxyType)) {
            return static_cast<PyObjectBase*>(reinterpret_cast<PyBaseProxy*>(proxy)->baseobject);
        }
    }

    return nullptr;
}

/*------------------------------
 * PyObjectBase Methods 	-- Every class, even the abstract one should have a Methods
------------------------------*/
PyMethodDef PyObjectBase::Methods[] = {
    {nullptr, nullptr, 0, nullptr}        /* Sentinel */
};

PyObject* PyObjectBase::__getattro(PyObject * obj, PyObject *attro)
{
    const char *attr{};
    attr = PyUnicode_AsUTF8(attro);

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
        return nullptr;
    }

#ifdef ATTR_TRACKING
    // If an attribute references this as parent then reset it (bug #0002902)
    PyObject* cur = pyObj->getTrackedAttribute(attr);
    if (cur) {
        if (PyObject_TypeCheck(cur, &(PyObjectBase::Type))) {
            PyObjectBase* base = static_cast<PyObjectBase*>(cur);
            base->resetAttribute();
            pyObj->untrackAttribute(attr);
        }
    }
#endif

    PyObject* value = pyObj->_getattr(attr);
#ifdef ATTR_TRACKING
    if (value && PyObject_TypeCheck(value, &(PyObjectBase::Type))) {
        if (!static_cast<PyObjectBase*>(value)->isConst() &&
            !static_cast<PyObjectBase*>(value)->isNotTracking()) {
            static_cast<PyObjectBase*>(value)->setAttributeOf(attr, pyObj);
            pyObj->trackAttribute(attr, value);
        }
    }
    else
#endif
    if (value && PyCFunction_Check(value)) {
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
            value = nullptr;
            PyErr_Format(PyExc_AttributeError, "<no object bound to built-in method %s>", attr);
        }
    }

    return value;
}

int PyObjectBase::__setattro(PyObject *obj, PyObject *attro, PyObject *value)
{
    const char *attr{};
    attr = PyUnicode_AsUTF8(attro);

    //Hint: In general we don't allow to delete attributes (i.e. value=0). However, if we want to allow
    //we must check then in _setattr() of all subclasses whether value is 0.
    if (!value) {
        PyErr_Format(PyExc_AttributeError, "Cannot delete attribute: '%s'", attr);
        return -1;
    }
    else if (!static_cast<PyObjectBase*>(obj)->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return -1;
    }

#ifdef ATTR_TRACKING
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
#endif

    int ret = static_cast<PyObjectBase*>(obj)->_setattr(attr, value);
#ifdef ATTR_TRACKING
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
        return reinterpret_cast<PyObject *>(Py_TYPE(this));
    }
    else if (streq(attr, "__members__")) {
        // Use __dict__ instead as __members__ is deprecated
        return nullptr;
    }
    else if (streq(attr,"__dict__")) {
        // Return the default dict
        PyTypeObject *tp = Py_TYPE(this);
        Py_XINCREF(tp->tp_dict);
        return tp->tp_dict;
    }
    else if (streq(attr,"softspace")) {
        // Internal Python stuff
        return nullptr;
    }
    else {
        // As fallback solution use Python's default method to get generic attributes
        PyObject *w{}, *res{};
        w = PyUnicode_InternFromString(attr);
        if (w) {
            res = PyObject_GenericGetAttr(this, w);
            Py_XDECREF(w);
            return res;
        } else {
            // Throw an exception for unknown attributes
            PyTypeObject *tp = Py_TYPE(this);
            PyErr_Format(PyExc_AttributeError, "%.50s instance has no attribute '%.400s'", tp->tp_name, attr);
            return nullptr;
        }
    }
}

int PyObjectBase::_setattr(const char *attr, PyObject *value)
{
    if (streq(attr,"softspace"))
        return -1; // filter out softspace
    PyObject *w{};
    // As fallback solution use Python's default method to get generic attributes
    w = PyUnicode_InternFromString(attr); // new reference
    if (w) {
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
PyObject *PyObjectBase::_repr()
{
    std::stringstream a;
    a << "<base object at " << _pcTwinPointer << ">";
# ifdef FCDebug
    Console().Log("PyObjectBase::_repr() not overwritten representation!");
# endif
    return Py_BuildValue("s", a.str().c_str());
}

// Tracking functions

void PyObjectBase::resetAttribute()
{
    if (attrDict) {
        // This is the attribute name to the parent structure
        // which we search for in the dict
        PyObject* key1 = PyBytes_FromString("__attribute_of_parent__");
        PyObject* key2 = PyBytes_FromString("__instance_of_parent__");
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

    PyObject* key1 = PyBytes_FromString("__attribute_of_parent__");
    PyObject* key2 = PyBytes_FromString("__instance_of_parent__");
    PyObject* attro = PyUnicode_FromString(attr);
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
        PyObject* key1 = PyBytes_FromString("__attribute_of_parent__");
        PyObject* key2 = PyBytes_FromString("__instance_of_parent__");
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
    PyObject* obj = nullptr;
    if (attrDict) {
        obj = PyDict_GetItemString(attrDict, attr);
        obj = getFromWeakRef(obj);
    }
    return obj;
}

void PyObjectBase::trackAttribute(const char* attr, PyObject* obj)
{
    if (!attrDict) {
        attrDict = PyDict_New();
    }

    PyObject* obj_ref = createWeakRef(static_cast<PyObjectBase*>(obj));
    if (obj_ref) {
        PyDict_SetItemString(attrDict, attr, obj_ref);
    }
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
