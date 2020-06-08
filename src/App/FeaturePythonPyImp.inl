/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


namespace App
{

/// Type structure of FeaturePythonPyT
template<class FeaturePyT>
PyTypeObject FeaturePythonPyT<FeaturePyT>::Type = {
    PyVarObject_HEAD_INIT(&PyType_Type,0)
    "FeaturePython",                                  /*tp_name*/
    sizeof(FeaturePythonPyT),                         /*tp_basicsize*/
    0,                                                /*tp_itemsize*/
    /* methods */
    FeaturePyT::PyDestructor,                         /*tp_dealloc*/
    0,                                                /*tp_print*/
    0,                                                /*tp_getattr*/
    0,                                                /*tp_setattr*/
    0,                                                /*tp_compare*/
    0,                                                /*tp_repr*/
    0,                                                /*tp_as_number*/
    0,                                                /*tp_as_sequence*/
    0,                                                /*tp_as_mapping*/
    0,                                                /*tp_hash*/
    0,                                                /*tp_call */
    0,                                                /*tp_str  */
    FeaturePyT::__getattro,                           /*tp_getattro*/
    __setattro,                                       /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    0,                                                /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
#if PY_MAJOR_VERSION >= 3
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_DEFAULT,           /*tp_flags */
#else
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_CLASS,        /*tp_flags */
#endif
    "This is the father of all Feature classes",      /*tp_doc */
    0,                                                /*tp_traverse */
    0,                                                /*tp_clear */
    0,                                                /*tp_richcompare */
    0,                                                /*tp_weaklistoffset */
    0,                                                /*tp_iter */
    0,                                                /*tp_iternext */
    0,                                                /*tp_methods */
    0,                                                /*tp_members */
    0,                                                /*tp_getset */
    &FeaturePyT::Type,                                /*tp_base */
    0,                                                /*tp_dict */
    0,                                                /*tp_descr_get */
    0,                                                /*tp_descr_set */
    0,                                                /*tp_dictoffset */
    FeaturePyT::__PyInit,                             /*tp_init */
    0,                                                /*tp_alloc */
    0,                                                /*tp_new */
    0,                                                /*tp_free   Low-level free-memory routine */
    0,                                                /*tp_is_gc  For PyObject_IS_GC */
    0,                                                /*tp_bases */
    0,                                                /*tp_mro    method resolution order */
    0,                                                /*tp_cache */
    0,                                                /*tp_subclasses */
    0,                                                /*tp_weaklist */
    0,                                                /*tp_del */
    0                                                 /*tp_version_tag */
#if PY_MAJOR_VERSION >= 3
    ,0                                                /*tp_finalize */
#endif
#if PY_VERSION_HEX >= 0x03080000
    ,0                                                /*tp_vectorcall */
#endif
};

template<class FeaturePyT>
FeaturePythonPyT<FeaturePyT>::FeaturePythonPyT(Base::BaseClass *pcObject, PyTypeObject *T)
    : FeaturePyT(reinterpret_cast<typename FeaturePyT::PointerType>(pcObject), T)
{
    Base::PyGILStateLocker lock;
    dict_methods = PyDict_New();
}

template<class FeaturePyT>
FeaturePythonPyT<FeaturePyT>::~FeaturePythonPyT()
{
    Base::PyGILStateLocker lock;
    Py_DECREF(dict_methods);
}

template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::__setattro(PyObject *obj, PyObject *attro, PyObject *value)
{
    const char *attr;
#if PY_MAJOR_VERSION >= 3
    attr = PyUnicode_AsUTF8(attro);
#else
    attr = PyString_AsString(attro);
#endif
    // This overwrites PyObjectBase::__setattr because this actively disallows to delete an attribute
    //

    if (!static_cast<Base::PyObjectBase*>(obj)->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return -1;
    }

    int ret = static_cast<Base::PyObjectBase*>(obj)->_setattr(attr, value);
    if (ret == 0) {
        static_cast<Base::PyObjectBase*>(obj)->startNotify();
    }
    return ret;
}


template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::_setattr(const char *attr, PyObject *value)
{
    App::Property *prop = FeaturePyT::getPropertyContainerPtr()->getPropertyByName(attr);
    if (prop && !value) {
        PyErr_Format(PyExc_AttributeError, "Cannot delete attribute: '%s'", attr);
        return -1;
    }

    int returnValue = FeaturePyT::_setattr(attr, value);
    if (returnValue == -1) {
        PyObject* dict_item = value;
        if (value) {
            if (PyFunction_Check(value)) {
                PyErr_Clear();
#if PY_MAJOR_VERSION < 3
                dict_item = PyMethod_New(value, this, 0);
#else
                dict_item = PyMethod_New(value, this);
#endif
                returnValue = PyDict_SetItemString(dict_methods, attr, dict_item);
                Py_XDECREF(dict_item);
            }
        }
        else {
            // delete
            PyErr_Clear();
            returnValue = PyDict_DelItemString(dict_methods, attr);
            if (returnValue < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
                PyErr_SetString(PyExc_AttributeError, attr);
        }
    }
    return returnValue;
}

template<class FeaturePyT>
PyObject *FeaturePythonPyT<FeaturePyT>::_getattr(const char *attr)
{
    // See CallTipsList::extractTips
    if (Base::streq(attr, "__fc_template__")) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    // get only attributes of this type
    if (Base::streq(attr, "__dict__")) {
        // Return the default dict
        PyTypeObject *tp = this->ob_type;
        // register type if needed
        if (tp->tp_dict == NULL) {
            if (PyType_Ready(tp) < 0)
                return 0;
        }

        PyObject* dict = FeaturePyT::_getattr(attr);
        if (dict && PyDict_CheckExact(dict)) {
            PyObject* dict_old = dict;
            dict = PyDict_Copy(dict_old);
            Py_DECREF(dict_old); // delete old dict
            PyDict_Merge(dict, dict_methods, 0);
        }
        return dict;
    }

    // find the attribute in the dict
    PyObject *dict_item = NULL;
    dict_item = PyDict_GetItemString(dict_methods, attr);
    if (dict_item) {
        Py_INCREF(dict_item);
        return dict_item;
    }

    // search for the attribute in the base class
    PyErr_Clear();
    return FeaturePyT::_getattr(attr);
}

} //namespace App
