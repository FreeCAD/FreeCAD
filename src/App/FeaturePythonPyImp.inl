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

/*
class MyDocumentObject(App.DocumentObject):
  def __init__(self,a,b):
    App.DocumentObject.__init__(self,a,b)
    self.addProperty("App::PropertyFloat","MyFloat")
  def execute(self):
    print "execute"

App.newDocument()
App.ActiveDocument.addObject("App::FeaturePython","Test")
my=MyDocumentObject(App.ActiveDocument,"Test")
my.Proxy=my
my.MyFloat
my.execute()
my.MyFloat=3.0
*/

// See http://www.python.org/dev/peps/pep-0253/

/// Type structure of FeaturePythonPyT
template<class FeaturePyT>
PyTypeObject FeaturePythonPyT<FeaturePyT>::Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                                /*ob_size*/
    "DocumentObject",                                 /*tp_name*/
    sizeof(FeaturePythonPyT<FeaturePyT>),             /*tp_basicsize*/
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
    App::FeaturePythonPyT<FeaturePyT>::Methods,       /*tp_methods */
    0,                                                /*tp_members */
    0,                                                /*tp_getset */
    0/*&FeaturePyT::Type*/,                                /*tp_base */
    0,                                                /*tp_dict */
    0,                                                /*tp_descr_get */
    0,                                                /*tp_descr_set */
    0,                                                /*tp_dictoffset */
    App::FeaturePythonPyT<FeaturePyT>::object_init,   /*tp_init */
    0,                                                /*tp_alloc */
    App::FeaturePythonPyT<FeaturePyT>::object_make,   /*tp_new */
    0,                                                /*tp_free   Low-level free-memory routine */
    0,                                                /*tp_is_gc  For PyObject_IS_GC */
    0,                                                /*tp_bases */
    0,                                                /*tp_mro    method resolution order */
    0,                                                /*tp_cache */
    0,                                                /*tp_subclasses */
    0,                                                /*tp_weaklist */
    0                                                 /*tp_del */
};

/// Methods structure of FeaturePythonPyT
template<class FeaturePyT>
PyMethodDef FeaturePythonPyT<FeaturePyT>::Methods[] = {
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

template<class FeaturePyT>
PyObject *FeaturePythonPyT<FeaturePyT>::object_make(PyTypeObject *type, PyObject *args, PyObject *)  // Python wrapper
{
    if (type == &FeaturePythonPyT<FeaturePyT>::Type) {
        std::stringstream out;
        out << "Cannot create an instance of '" << type->tp_name << "'.";
        PyErr_SetString(PyExc_RuntimeError, out.str().c_str());
        return 0;
    }
    FeaturePythonClassInstance *o = reinterpret_cast<FeaturePythonClassInstance *>(type->tp_alloc(type, 0));
    o->py_object = NULL;
    PyObject *self = reinterpret_cast<PyObject *>(o);
    return self;
}

template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::object_init(PyObject* _self, PyObject* args, PyObject* /*kwd*/)
{
    PyObject* d;
    char* s;
    if (!PyArg_ParseTuple(args, "O!s", &(App::DocumentPy::Type),&d, &s)) {
        std::stringstream out;
        out << "Cannot create an instance of '" << _self->ob_type->tp_name << "'.";
        PyErr_SetString(PyExc_RuntimeError, out.str().c_str());
        return 0;
    }
    
    App::Document* doc = static_cast<App::DocumentPy*>(d)->getDocumentPtr();
    App::DocumentObject* obj = doc->getObject(s);

    FeaturePythonClassInstance *self = reinterpret_cast<FeaturePythonClassInstance *>(_self);
    try {
        if (!self->py_object)
            self->py_object = new FeaturePythonPyT<FeaturePyT>(obj);
        obj->setPyObject(_self);
        return 0;
    }
    catch (const Base::Exception&) {
        PyErr_SetString(PyExc_TypeError, "DocumentObject has wrong type");
        return -1;
    }
}

template<class FeaturePyT>
void FeaturePythonPyT<FeaturePyT>::object_deallocator(PyObject *_self)
{
    if (_self->ob_type == &FeaturePythonPyT<FeaturePyT>::Type) {
        FeaturePythonPyT<FeaturePyT>* self = static_cast< FeaturePythonPyT<FeaturePyT>* >(_self);
        delete self;
    }
    else {
        App::FeaturePythonClassInstance *self = reinterpret_cast< App::FeaturePythonClassInstance * >(_self);
        Py_DECREF(self->py_object);
        _self->ob_type->tp_free(_self);
    }
}

template<class FeaturePyT>
PyObject * FeaturePythonPyT<FeaturePyT>::getattro_handler(PyObject *self, PyObject *attr)
{
    char* name = PyString_AsString(attr);
    PyObject* rvalue = PyObject_GenericGetAttr(self, attr);
    if (rvalue)
        return rvalue;
    PyErr_Clear();
    App::FeaturePythonClassInstance *instance = reinterpret_cast< App::FeaturePythonClassInstance * >(self);
    return __getattr(instance->py_object, name);
}

template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::setattro_handler(PyObject *self, PyObject *attr, PyObject *value)
{
    char* name = PyString_AsString(attr);
    App::FeaturePythonClassInstance *instance = reinterpret_cast< App::FeaturePythonClassInstance * >(self);
    return __setattr(instance->py_object, name, value);
}

template<class FeaturePyT>
PyObject * FeaturePythonPyT<FeaturePyT>::staticCallback_addProperty (PyObject *_self, PyObject *args)
{
    App::FeaturePythonClassInstance *self_python = reinterpret_cast< App::FeaturePythonClassInstance * >(_self);
    FeaturePythonPyT<FeaturePyT> *self = static_cast< FeaturePythonPyT<FeaturePyT> * >(self_python->py_object);
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

template<class FeaturePyT>
PyObject * FeaturePythonPyT<FeaturePyT>::staticCallback_removeProperty (PyObject *_self, PyObject *args)
{
    App::FeaturePythonClassInstance *self_python = reinterpret_cast< App::FeaturePythonClassInstance * >(_self);
    FeaturePythonPyT<FeaturePyT> *self = static_cast< FeaturePythonPyT<FeaturePyT> * >(self_python->py_object);
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

template<class FeaturePyT>
PyObject * FeaturePythonPyT<FeaturePyT>::staticCallback_supportedProperties (PyObject *_self, PyObject *args)
{
    App::FeaturePythonClassInstance *self_python = reinterpret_cast< App::FeaturePythonClassInstance * >(_self);
    FeaturePythonPyT<FeaturePyT> *self = static_cast< FeaturePythonPyT<FeaturePyT> * >(self_python->py_object);
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

template<class FeaturePyT>
FeaturePythonPyT<FeaturePyT>::FeaturePythonPyT(DocumentObject *pcObject, PyTypeObject *T)
    : FeaturePyT(reinterpret_cast<typename FeaturePyT::PointerType>(pcObject), T)
{
}

template<class FeaturePyT>
FeaturePythonPyT<FeaturePyT>::~FeaturePythonPyT()
{
}

template<class FeaturePyT>
PyObject* FeaturePythonPyT<FeaturePyT>::__getattr(PyObject * obj, char *attr)
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

template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::__setattr(PyObject *obj, char *attr, PyObject *value)
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

template<class FeaturePyT>
PyObject *FeaturePythonPyT<FeaturePyT>::_getattr(char *attr)
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
    //if (rvalue == NULL) {
    //    std::map<std::string, PyObject*>::iterator it = dyn_methods.find(attr);
    //    if (it != dyn_methods.end()) {
    //        Py_INCREF(it->second);
    //        rvalue = it->second;
    //        PyErr_Clear();
    //    }
    //}
    if (rvalue == NULL) {
        PyErr_Clear();
        return FeaturePyT::_getattr(attr);
    }
    else {
        return rvalue;
    }
}

template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::_setattr(char *attr, PyObject *value)
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

    int returnValue = FeaturePyT::_setattr(attr, value);
    //if (returnValue == -1) {
    //    if (value) {
    //        if (PyFunction_Check(value)) {
    //            std::map<std::string, PyObject*>::iterator it = dyn_methods.find(attr);
    //            if (it != dyn_methods.end()) {
    //                Py_XDECREF(it->second);
    //            }
    //            dyn_methods[attr] = PyMethod_New(value, this, 0);
    //            returnValue = 0;
    //            PyErr_Clear();
    //        }
    //    }
    //    else {
    //        // delete
    //        std::map<std::string, PyObject*>::iterator it = dyn_methods.find(attr);
    //        if (it != dyn_methods.end()) {
    //            Py_XDECREF(it->second);
    //            dyn_methods.erase(it);
    //            returnValue = 0;
    //            PyErr_Clear();
    //        }
    //    }
    //}
    return returnValue;
}

// -------------------------------------------------------------

template<class FeaturePyT>
PyObject*  FeaturePythonPyT<FeaturePyT>::addProperty(PyObject *args)
{
    char *sType,*sName=0,*sGroup=0,*sDoc=0;
    short attr=0;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssshO!O!", &sType,&sName,&sGroup,&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))     // convert args: Python->C
        return NULL;                             // NULL triggers exception 

    Property* prop=0;
    prop = FeaturePyT::getDocumentObjectPtr()->addDynamicProperty(sType,sName,sGroup,sDoc,attr,
        PyObject_IsTrue(ro) ? true : false, PyObject_IsTrue(hd) ? true : false);
    
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::Exception(PyExc_Exception,str.str());
    }

    return Py::new_reference_to(this);
}

template<class FeaturePyT>
PyObject*  FeaturePythonPyT<FeaturePyT>::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return NULL;

    bool ok = FeaturePyT::getDocumentObjectPtr()->removeDynamicProperty(sName);
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

template<class FeaturePyT>
PyObject*  FeaturePythonPyT<FeaturePyT>::supportedProperties(PyObject *args)
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

template<class FeaturePyT>
PyObject *FeaturePythonPyT<FeaturePyT>::getCustomAttributes(const char* attr) const
{
    PY_TRY{
        if (Base::streq(attr, "__dict__")){
            // Return the default dict
            PyTypeObject *tp = this->ob_type;
            PyObject* dict = PyDict_Copy(tp->tp_dict);
            std::map<std::string,App::Property*> Map;
            FeaturePyT::getPropertyContainerPtr()->getPropertyMap(Map);
            for (std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it)
                PyDict_SetItem(dict, PyString_FromString(it->first.c_str()), PyString_FromString(""));
            //for (std::map<std::string, PyObject*>::const_iterator it = dyn_methods.begin(); it != dyn_methods.end(); ++it)
            //    PyDict_SetItem(dict, PyString_FromString(it->first.c_str()), PyString_FromString(""));
            if (PyErr_Occurred()) {
                Py_DECREF(dict);
                dict = 0;
            }
            return dict;
        }

        // search for dynamic property
        Property* prop = FeaturePyT::getDocumentObjectPtr()->getDynamicPropertyByName(attr);
        if (prop) return prop->getPyObject();
    } PY_CATCH;

    return 0;
}

template<class FeaturePyT>
int FeaturePythonPyT<FeaturePyT>::setCustomAttributes(const char* attr, PyObject *value)
{
    // search for dynamic property
    Property* prop = FeaturePyT::getDocumentObjectPtr()->getDynamicPropertyByName(attr);

    if (!prop)
        return FeaturePyT::setCustomAttributes(attr, value);
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

} //namespace App
