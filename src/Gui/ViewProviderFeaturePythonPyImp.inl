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


namespace Gui
{

/// Type structure of ViewProviderFeaturePythonPyT
template<class PyT>
PyTypeObject ViewProviderFeaturePythonPyT<PyT>::Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                                /*ob_size*/
    "ViewProviderDocumentObject",                                 /*tp_name*/
    sizeof(ViewProviderFeaturePythonPyT<PyT>),             /*tp_basicsize*/
    0,                                                /*tp_itemsize*/
    /* methods */
    object_deallocator/*FeaturePyT::PyDestructor*/,                         /*tp_dealloc*/
    0,                                                /*tp_print*/
    0,                            /*tp_getattr*/
    0/*__setattr*/,                                        /*tp_setattr*/
    0,                                                /*tp_compare*/
    0,                                                /*tp_repr*/
    0,                                                /*tp_as_number*/
    0,                                                /*tp_as_sequence*/
    0,                                                /*tp_as_mapping*/
    0,                                                /*tp_hash*/
    0,                                                /*tp_call */
    0,                                                /*tp_str  */
    getattro_handler,                                                /*tp_getattro*/
    setattro_handler,                                                /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    0,                                                /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_CLASS,        /*tp_flags */
    "This is the father of all Feature classes",      /*tp_doc */
    0,                                                /*tp_traverse */
    0,                                                /*tp_clear */
    0,                                                /*tp_richcompare */
    0,                                                /*tp_weaklistoffset */
    0,                                                /*tp_iter */
    0,                                                /*tp_iternext */
    ViewProviderFeaturePythonPyT<PyT>::Methods,       /*tp_methods */
    0,                                                /*tp_members */
    ViewProviderFeaturePythonPyT<PyT>::GetterSetter,                                                /*tp_getset */
    &PyT::Type,                                /*tp_base */
    0,                                                /*tp_dict */
    0,                                                /*tp_descr_get */
    0,                                                /*tp_descr_set */
    0,                                                /*tp_dictoffset */
    ViewProviderFeaturePythonPyT<PyT>::object_init,   /*tp_init */
    0,                                                /*tp_alloc */
    ViewProviderFeaturePythonPyT<PyT>::object_make,   /*tp_new */
    0,                                                /*tp_free   Low-level free-memory routine */
    0,                                                /*tp_is_gc  For PyObject_IS_GC */
    0,                                                /*tp_bases */
    0,                                                /*tp_mro    method resolution order */
    0,                                                /*tp_cache */
    0,                                                /*tp_subclasses */
    0,                                                /*tp_weaklist */
    0                                                 /*tp_del */
};

/// Methods structure of ViewProviderFeaturePythonPyT
template<class PyT>
PyMethodDef ViewProviderFeaturePythonPyT<PyT>::Methods[] = {
    {"addDisplayMode",
        (PyCFunction) staticCallback_addDisplayMode,
        METH_VARARGS,
        "Add a new display mode to the view provider"
    },
    {"addProperty",
        (PyCFunction) staticCallback_addProperty,
        METH_VARARGS,
        "addProperty(string, string) -- Add a generic property.\nThe first argument specifies the type, the second the\nname of the property.\n		"
    },
    {"removeProperty",
        (PyCFunction) staticCallback_removeProperty,
        METH_VARARGS,
        "removeProperty(string) -- Remove a generic property.\nNote, you can only remove user-defined properties but not built-in ones.\n		"
    },
    {"supportedProperties",
        (PyCFunction) staticCallback_supportedProperties,
        METH_VARARGS,
        "A list of supported property types"
    },
    {NULL, NULL, 0, NULL}		/* Sentinel */
};

/// Attribute structure of ViewProviderFeaturePythonPyT
template<class PyT>
PyGetSetDef ViewProviderFeaturePythonPyT<PyT>::GetterSetter[] = {
    {NULL, NULL, NULL, NULL, NULL}		/* Sentinel */
};

template<class PyT>
bool ViewProviderFeaturePythonPyT<PyT>::checkExact(PyObject* op)
{
    return Py_TYPE(op) == &ViewProviderFeaturePythonPyT<PyT>::Type;
}

template<class PyT>
PyObject *ViewProviderFeaturePythonPyT<PyT>::object_make(PyTypeObject *type, PyObject *args, PyObject *)  // Python wrapper
{
    if (type == &ViewProviderFeaturePythonPyT<PyT>::Type) {
        std::stringstream out;
        out << "Cannot create an instance of '" << type->tp_name << "'.";
        PyErr_SetString(PyExc_RuntimeError, out.str().c_str());
        return 0;
    }

    PyObject* address;
    if (PyArg_ParseTuple(args, "O!", &PyLong_Type, &address)) {
        ViewProviderFeaturePythonPyT<PyT>* self = new ViewProviderFeaturePythonPyT<PyT>(0);
        PyObject *cls = reinterpret_cast<PyObject *>(type->tp_alloc(type, 0));
        self->pyClassObject = cls;
        void* ptr = PyLong_AsVoidPtr(address);
        self->_pcTwinPointer = ptr;
        return self;
    }
    else {
        PyErr_Clear();
        PyObject* d;
        char* s;
        if (PyArg_ParseTuple(args, "O!s", &(App::DocumentPy::Type),&d, &s)) {
            ViewProviderFeaturePythonPyT<PyT>* self = new ViewProviderFeaturePythonPyT<PyT>(0);
            PyObject *cls = reinterpret_cast<PyObject *>(type->tp_alloc(type, 0));
            self->pyClassObject = cls;
            // This is a trick to enter the tp_init slot
            self->ob_type = type;
            return self;
        }
    }

    PyErr_Format(PyExc_RuntimeError, "Cannot create an instance of '%.100s'.", type->tp_name);
    return 0;
}

template<class PyT>
int ViewProviderFeaturePythonPyT<PyT>::object_init(PyObject* _self, PyObject* args, PyObject* /*kwd*/)
{
    PyObject* d;
    char* s;
    if (!PyArg_ParseTuple(args, "O!s", &(App::DocumentPy::Type),&d, &s)) {
        PyErr_Format(PyExc_RuntimeError, "Cannot create an instance of '%.100s'.", _self->ob_type->tp_name);
        return -1;
    }
    
    App::Document* doc = static_cast<App::DocumentPy*>(d)->getDocumentPtr();
    App::DocumentObject* obj = doc->getObject(s);
    if (!obj) {
        PyErr_Format(PyExc_RuntimeError, "object '%.100s' doesn't exist.", s);
        return -1;
    }
    Gui::ViewProvider* view = Gui::Application::Instance->getViewProvider(obj);
    if (!view) {
        PyErr_Format(PyExc_RuntimeError, "object '%.100s' doesn't have a view provider.", s);
        return -1;
    }

    ViewProviderFeaturePythonPyT<PyT> *self = static_cast<ViewProviderFeaturePythonPyT<PyT> *>(_self);
    self->ob_type = &ViewProviderFeaturePythonPyT<PyT>::Type; // restore the actual type
    self->_pcTwinPointer = view;

    try {
        view->setPyObject(_self);
        return 0;
    }
    catch (const Base::Exception&) {
        PyErr_SetString(PyExc_TypeError, "DocumentObject has wrong type");
        return -1;
    }
}

template<class PyT>
void ViewProviderFeaturePythonPyT<PyT>::object_deallocator(PyObject *_self)
{
    if (checkExact(_self)) {
        ViewProviderFeaturePythonPyT<PyT>* self = static_cast< ViewProviderFeaturePythonPyT<PyT>* >(_self);
        PyObject* cls = self->pyClassObject;
        Py_XDECREF(cls);
        delete self;
    }
    else {
        _self->ob_type->tp_free(_self);
    }
}

template<class PyT>
PyObject *ViewProviderFeaturePythonPyT<PyT>::_repr(void)
{
    std::string txt;
    txt = PyT::representation();

    PyObject* cls = this->pyClassObject;
    if (cls) {
        std::stringstream a;
        a << "<" << cls->ob_type->tp_name << " object at " << cls << ">";
        txt = a.str();
    }

    return Py_BuildValue("s", txt.c_str());
}

template<class PyT>
PyObject * ViewProviderFeaturePythonPyT<PyT>::getattro_handler(PyObject *_self, PyObject *attr)
{
    char* name = PyString_AsString(attr);
    if (checkExact(_self)) {
        ViewProviderFeaturePythonPyT<PyT>* self = static_cast< ViewProviderFeaturePythonPyT<PyT>* >(_self);
        // special handling
        if (Base::streq(name, "__proxyclass__")) {
            PyObject* cls = self->pyClassObject;
            if (cls) {
                return PyString_FromString(cls->ob_type->tp_name);
            }
            return 0;
        }
        else if (Base::streq(name, "__module__")) {
            PyObject* cls = self->pyClassObject;
            if (cls) {
                return PyObject_GenericGetAttr(cls, attr);
            }
            return 0;
        }

        PyObject* rvalue = __getattr(self, name);
        if (rvalue)
            return rvalue;
        PyObject* cls = self->pyClassObject;
        if (cls) {
            PyErr_Clear();
            return PyObject_GenericGetAttr(cls, attr);
        }
        else {
            return 0;
        }
    }
    // a subclass
    else {
        return PyObject_GenericGetAttr(_self, attr);
    }
}

template<class PyT>
int ViewProviderFeaturePythonPyT<PyT>::setattro_handler(PyObject *self, PyObject *attr, PyObject *value)
{
    char* name = PyString_AsString(attr);
    if (checkExact(self)) {
        int ret = __setattr(self, name, value);
        if (ret < 0) {
            PyErr_Clear();
            ViewProviderFeaturePythonPyT<PyT>* self_ = static_cast<ViewProviderFeaturePythonPyT<PyT>* >(self);
            PyObject* cls = self_->pyClassObject;
            ret = PyObject_GenericSetAttr(cls, attr, value);
        }

        return ret;
    }
    else {
        return PyObject_GenericSetAttr(self, attr, value);
    }
}

template<class PyT>
PyObject * ViewProviderFeaturePythonPyT<PyT>::staticCallback_addDisplayMode (PyObject *_self, PyObject *args)
{
    ViewProviderFeaturePythonPyT<PyT> *self = static_cast< ViewProviderFeaturePythonPyT<PyT> * >(_self);
    // test if twin object not allready deleted
    if (!self->isValid()){
        PyErr_SetString(PyExc_ReferenceError, "This object is already deleted most likely through closing a document. This reference is no longer valid!");
        return NULL;
    }

    // test if object is set Const
    if (self->isConst()){
        PyErr_SetString(PyExc_ReferenceError, "This object is immutable, you can not set any attribute or call a non const method");
        return NULL;
    }

    try {
        PyObject* ret = self->addDisplayMode(args);
        if (ret != 0)
            self->startNotify();
        return ret;
    }
    catch(const Base::Exception& e) {
        std::string str;
        str += "FreeCAD exception thrown (";
        str += e.what();
        str += ")";
        e.ReportException();
        PyErr_SetString(PyExc_Exception,str.c_str());
        return NULL;
    }
    catch(const Py::Exception&) {
        // The exception text is already set
        return NULL;
    }
}

template<class PyT>
PyObject * ViewProviderFeaturePythonPyT<PyT>::staticCallback_addProperty (PyObject *_self, PyObject *args)
{
    ViewProviderFeaturePythonPyT<PyT> *self = static_cast< ViewProviderFeaturePythonPyT<PyT> * >(_self);
    // test if twin object not allready deleted
    if (!self->isValid()){
        PyErr_SetString(PyExc_ReferenceError, "This object is already deleted most likely through closing a document. This reference is no longer valid!");
        return NULL;
    }

    // test if object is set Const
    if (self->isConst()){
        PyErr_SetString(PyExc_ReferenceError, "This object is immutable, you can not set any attribute or call a non const method");
        return NULL;
    }

    try {
        PyObject* ret = self->addProperty(args);
        if (ret != 0)
            self->startNotify();
        return ret;
    }
    catch(const Base::Exception& e) {
        std::string str;
        str += "FreeCAD exception thrown (";
        str += e.what();
        str += ")";
        e.ReportException();
        PyErr_SetString(PyExc_Exception,str.c_str());
        return NULL;
    }
    catch(const Py::Exception&) {
        // The exception text is already set
        return NULL;
    }
}

template<class PyT>
PyObject * ViewProviderFeaturePythonPyT<PyT>::staticCallback_removeProperty (PyObject *_self, PyObject *args)
{
    ViewProviderFeaturePythonPyT<PyT> *self = static_cast< ViewProviderFeaturePythonPyT<PyT> * >(_self);
    // test if twin object not allready deleted
    if (!self->isValid()){
        PyErr_SetString(PyExc_ReferenceError, "This object is already deleted most likely through closing a document. This reference is no longer valid!");
        return NULL;
    }

    // test if object is set Const
    if (self->isConst()){
        PyErr_SetString(PyExc_ReferenceError, "This object is immutable, you can not set any attribute or call a non const method");
        return NULL;
    }

    try {
        PyObject* ret = self->removeProperty(args);
        if (ret != 0)
            self->startNotify();
        return ret;
    }
    catch(const Base::Exception& e) {
        std::string str;
        str += "FreeCAD exception thrown (";
        str += e.what();
        str += ")";
        e.ReportException();
        PyErr_SetString(PyExc_Exception,str.c_str());
        return NULL;
    }
    catch(const Py::Exception&) {
        // The exception text is already set
        return NULL;
    }
}

template<class PyT>
PyObject * ViewProviderFeaturePythonPyT<PyT>::staticCallback_supportedProperties (PyObject *_self, PyObject *args)
{
    ViewProviderFeaturePythonPyT<PyT> *self = static_cast< ViewProviderFeaturePythonPyT<PyT> * >(_self);
    // test if twin object not allready deleted
    if (!self->isValid()){
        PyErr_SetString(PyExc_ReferenceError, "This object is already deleted most likely through closing a document. This reference is no longer valid!");
        return NULL;
    }

    // test if object is set Const
    if (self->isConst()){
        PyErr_SetString(PyExc_ReferenceError, "This object is immutable, you can not set any attribute or call a non const method");
        return NULL;
    }

    try {
        PyObject* ret = self->supportedProperties(args);
        if (ret != 0)
            self->startNotify();
        return ret;
    }
    catch(const Base::Exception& e) {
        std::string str;
        str += "FreeCAD exception thrown (";
        str += e.what();
        str += ")";
        e.ReportException();
        PyErr_SetString(PyExc_Exception,str.c_str());
        return NULL;
    }
    catch(const Py::Exception&) {
        // The exception text is already set
        return NULL;
    }
}

template<class PyT>
ViewProviderFeaturePythonPyT<PyT>::ViewProviderFeaturePythonPyT(ViewProviderDocumentObject *pcObject, PyTypeObject *T)
    : PyT(reinterpret_cast<typename PyT::PointerType>(pcObject), T), pyClassObject(0)
{
}

template<class PyT>
ViewProviderFeaturePythonPyT<PyT>::~ViewProviderFeaturePythonPyT()
{
}

template<class PyT>
PyObject* ViewProviderFeaturePythonPyT<PyT>::__getattr(PyObject * obj, char *attr)
{
    // This should be the entry in Type
    Base::PyObjectBase* base = static_cast<Base::PyObjectBase*>(obj);
    if (!base->isValid()) {
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return NULL;
    }

    PyObject* value = base->_getattr(attr);
    if (value && PyObject_TypeCheck(value, &(Base::PyObjectBase::Type))) {
        if (!static_cast<Base::PyObjectBase*>(value)->isConst())
            static_cast<Base::PyObjectBase*>(value)->setAttributeOf(attr, base);
    }
    return value;
}

template<class PyT>
int ViewProviderFeaturePythonPyT<PyT>::__setattr(PyObject *obj, char *attr, PyObject *value)
{
    Base::PyObjectBase* base = static_cast<Base::PyObjectBase*>(obj);
    if (!base->isValid()) {
        PyErr_Format(PyExc_ReferenceError, "Cannot access attribute '%s' of deleted object", attr);
        return -1;
    }

    int ret = base->_setattr(attr, value);
    if (ret == 0) {
        base->startNotify();
    }
    return ret;
}

template<class PyT>
PyObject *ViewProviderFeaturePythonPyT<PyT>::_getattr(char *attr)
{
    try {
        // getter method for special Attributes (e.g. dynamic ones)
        PyObject *r = getCustomAttributes(attr);
        if(r) return r;
    }
    catch(const Base::Exception& e) {// catch the FreeCAD exceptions
        std::string str;
        str += "FreeCAD exception thrown (";
        str += e.what();
        str += ")";
        e.ReportException();
        PyErr_SetString(PyExc_Exception,str.c_str());
        return NULL;
    }
    catch(const Py::Exception&) {
        // The exception text is already set
        return NULL;
    }

    PyObject *rvalue = Py_FindMethod(Methods, this, attr);
    if (rvalue == NULL) {
        PyErr_Clear();
        return PyT::_getattr(attr);
    }
    else {
        return rvalue;
    }
}

template<class PyT>
int ViewProviderFeaturePythonPyT<PyT>::_setattr(char *attr, PyObject *value)
{
    try {
        // setter for  special Attributes (e.g. dynamic ones)
        int r = setCustomAttributes(attr, value);
        if(r==1) return 0;
    }
    catch(const Base::Exception& e) { // catch the FreeCAD exceptions
        std::string str;
        str += "FreeCAD exception thrown (";
        str += e.what();
        str += ")";
        e.ReportException();
        PyErr_SetString(PyExc_Exception,str.c_str());
        return -1;
    }
    catch(const Py::Exception&) {
        // The exception text is already set
        return -1;
    }

    return PyT::_setattr(attr, value);
}

// -------------------------------------------------------------

template<class PyT>
PyObject* ViewProviderFeaturePythonPyT<PyT>::addDisplayMode(PyObject * args)
{
    char* mode;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "Os", &obj, &mode))
        return NULL;

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin","SoNode *", obj, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return 0;
    }

    PY_TRY {
        SoNode* node = reinterpret_cast<SoNode*>(ptr);
        PyT::getViewProviderDocumentObjectPtr()->addDisplayMaskMode(node,mode);
        Py_Return;
    } PY_CATCH;
}

template<class PyT>
PyObject*  ViewProviderFeaturePythonPyT<PyT>::addProperty(PyObject *args)
{
    char *sType,*sName=0,*sGroup=0,*sDoc=0;
    short attr=0;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssshO!O!", &sType,&sName,&sGroup,&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))     // convert args: Python->C
        return NULL;                             // NULL triggers exception 

    App::Property* prop=0;
    prop = PyT::getPropertyContainerPtr()->addDynamicProperty(sType,sName,sGroup,sDoc,attr,
        PyObject_IsTrue(ro) ? true : false, PyObject_IsTrue(hd) ? true : false);
    
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::Exception(PyExc_Exception,str.str());
    }

    return Py::new_reference_to(this);
}

template<class PyT>
PyObject*  ViewProviderFeaturePythonPyT<PyT>::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return NULL;

    bool ok = PyT::getPropertyContainerPtr()->removeDynamicProperty(sName);
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

template<class PyT>
PyObject*  ViewProviderFeaturePythonPyT<PyT>::supportedProperties(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception
    
    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(App::Property::getClassTypeId(), ary);
    Py::List res;
    for (std::vector<Base::Type>::iterator it = ary.begin(); it != ary.end(); ++it) {
        Base::BaseClass *data = static_cast<Base::BaseClass*>(it->createInstance());
        if (data) {
            delete data;
            res.append(Py::String(it->getName()));
        }
    }
    return Py::new_reference_to(res);
}

template<class PyT>
PyObject *ViewProviderFeaturePythonPyT<PyT>::getCustomAttributes(const char* attr) const
{
    PY_TRY{
        if (Base::streq(attr, "__dict__")){
            // Return the default dict
            PyTypeObject *tp = this->ob_type;
            PyObject* dict = PyDict_Copy(tp->tp_dict);
            std::map<std::string,App::Property*> Map;
            PyT::getPropertyContainerPtr()->getPropertyMap(Map);
            for (std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it)
                PyDict_SetItem(dict, PyString_FromString(it->first.c_str()), PyString_FromString(""));
            PyObject* cls = this->pyClassObject;
            if (cls) {
                PyObject *w = PyString_InternFromString(attr);
                PyObject *d = PyObject_GenericGetAttr(cls, w);
                Py_DECREF(w);
                if (d) {
                    PyDict_Merge(dict, d, 0);
                    Py_DECREF(d);
                }
            }
            if (PyErr_Occurred()) {
                Py_DECREF(dict);
                dict = 0;
            }
            return dict;
        }

        // search for dynamic property
        App::Property* prop = PyT::getPropertyContainerPtr()->getDynamicPropertyByName(attr);
        if (prop) return prop->getPyObject();
    } PY_CATCH;

    return 0;
}

template<class PyT>
int ViewProviderFeaturePythonPyT<PyT>::setCustomAttributes(const char* attr, PyObject *value)
{
    // search for dynamic property
    App::Property* prop = PyT::getPropertyContainerPtr()->getDynamicPropertyByName(attr);

    if (!prop)
        return PyT::setCustomAttributes(attr, value);
    else {
        try {
            prop->setPyObject(value);
        } catch (Base::Exception &exc) {
            PyErr_Format(PyExc_AttributeError, "Attribute (Name: %s) error: '%s' ", attr, exc.what());
            return -1;
        } catch (...) {
            PyErr_Format(PyExc_AttributeError, "Unknown error in attribute %s", attr);
            return -1;
        }

        return 1;
    }
}

} //namespace Gui
