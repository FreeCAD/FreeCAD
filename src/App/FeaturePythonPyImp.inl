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
#if PY_VERSION_HEX >= 0x03080000
    0,                                                /*tp_vectorcall_offset*/
#else
    nullptr,                                          /*tp_print*/
#endif
    nullptr,                                          /*tp_getattr*/
    nullptr,                                          /*tp_setattr*/
    nullptr,                                          /*tp_compare*/
    nullptr,                                          /*tp_repr*/
    nullptr,                                          /*tp_as_number*/
    nullptr,                                          /*tp_as_sequence*/
    nullptr,                                          /*tp_as_mapping*/
    nullptr,                                          /*tp_hash*/
    nullptr,                                          /*tp_call */
    nullptr,                                          /*tp_str  */
    FeaturePyT::__getattro,                           /*tp_getattro*/
    __setattro,                                       /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    nullptr,                                          /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_DEFAULT,           /*tp_flags */
    "This is the father of all Feature classes",      /*tp_doc */
    nullptr,                                          /*tp_traverse */
    nullptr,                                          /*tp_clear */
    nullptr,                                          /*tp_richcompare */
    0,                                                /*tp_weaklistoffset */
    nullptr,                                          /*tp_iter */
    nullptr,                                          /*tp_iternext */
    nullptr,                                          /*tp_methods */
    nullptr,                                          /*tp_members */
    nullptr,                                          /*tp_getset */
    &FeaturePyT::Type,                                /*tp_base */
    nullptr,                                          /*tp_dict */
    nullptr,                                          /*tp_descr_get */
    nullptr,                                          /*tp_descr_set */
    0,                                                /*tp_dictoffset */
    FeaturePyT::__PyInit,                             /*tp_init */
    nullptr,                                          /*tp_alloc */
    nullptr,                                          /*tp_new */
    nullptr,                                          /*tp_free   Low-level free-memory routine */
    nullptr,                                          /*tp_is_gc  For PyObject_IS_GC */
    nullptr,                                          /*tp_bases */
    nullptr,                                          /*tp_mro    method resolution order */
    nullptr,                                          /*tp_cache */
    nullptr,                                          /*tp_subclasses */
    nullptr,                                          /*tp_weaklist */
    nullptr,                                          /*tp_del */
    0,                                                /*tp_version_tag */
    nullptr                                           /*tp_finalize */
#if PY_VERSION_HEX >= 0x03080000
    ,0                                                /*tp_vectorcall */
#endif
};

template<class FeaturePyT>
FeaturePythonPyT<FeaturePyT>::FeaturePythonPyT(Base::BaseClass *pcObject, PyTypeObject *T)
    : FeaturePyT(static_cast<typename FeaturePyT::PointerType>(pcObject), T)
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
    attr = PyUnicode_AsUTF8(attro);
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
                dict_item = PyMethod_New(value, this);
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
        if (!tp->tp_dict) {
            if (PyType_Ready(tp) < 0)
                return nullptr;
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
    PyObject *dict_item = nullptr;
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
