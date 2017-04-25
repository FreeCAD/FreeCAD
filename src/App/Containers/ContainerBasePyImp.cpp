#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Containers/ContainerBase.h>

// inclusion of the generated files (generated out of AttachableObjectPy.xml)
#include <Containers/ContainerBasePy.h>
#include <Containers/ContainerBasePy.cpp>
#include <App/PropertyContainerPy.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentPy.h>

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string ContainerBasePy::representation(void) const
{
    std::stringstream repr;
    repr << "<App::ContainerBase around " ;
    if (getContainerBasePtr()->object())
        repr << getContainerBasePtr()->getName();
    else
        repr << "None";
    repr << ">";
    return std::string(repr.str());
}

Py::Object ContainerBasePy::getObject(void) const
{
    if (getContainerBasePtr()->object())
        return Py::asObject(getContainerBasePtr()->object()->getPyObject());
    else
        return Py::None();
}

/**
  * @brief macro CONTAINERBASEPY_STDCATCH_ATTR: catch for exceptions in attribute
  * code (re-throws the exception converted to Py::Exception). It is a helper
  * to avoid repeating the same error handling code over and over again.
  */
#define CONTAINERBASEPY_STDCATCH_ATTR \
    catch (Base::Exception &e) {\
        throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());\
    }
    //FIXME: container errors to python

Py::List makePyList(std::vector<DocumentObject*> objects) {
    Py::List ret;
    for (DocumentObject* obj : objects)
        ret.append(Py::asObject(obj->getPyObject()));
    return ret;
}
Py::List makePyList(std::vector<PropertyContainer*> objects) {
    Py::List ret;
    for (PropertyContainer* obj : objects)
        ret.append(Py::asObject(obj->getPyObject()));
    return ret;
}

Py::List ContainerBasePy::getStaticChildren(void) const
{
    try {
        return makePyList(getContainerBasePtr()->staticChildren());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getDynamicChildren(void) const
{
    try {
        return makePyList(getContainerBasePtr()->dynamicChildren());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getAllChildren(void) const
{
    try {
        return makePyList(getContainerBasePtr()->allChildren());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}


Py::List ContainerBasePy::getStaticChildrenRecursive(void) const
{
    try {
        return makePyList(getContainerBasePtr()->staticChildrenRecursive());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getDynamicChildrenRecursive(void) const
{
    try {
        return makePyList(getContainerBasePtr()->dynamicChildrenRecursive());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getAllChildrenRecursive(void) const
{
    try {
        return makePyList(getContainerBasePtr()->allChildrenRecursive());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::String ContainerBasePy::getName(void) const
{
    try {
        return Py::String(getContainerBasePtr()->getName());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::Object ContainerBasePy::getDocument(void) const
{
    try {
        return Py::asObject(getContainerBasePtr()->getDocument()->getPyObject());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::Object ContainerBasePy::getParent(void) const
{
    try {
        App::PropertyContainer* parent = getContainerBasePtr()->parent();
        if (parent)
            return Py::asObject(parent->getPyObject());
        else
            return Py::None();
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::List ContainerBasePy::getParents(void) const
{
    try {
        return makePyList(getContainerBasePtr()->parents());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

/**
  * @brief macro CONTAINERPY_STDCATCH_METH: catch for exceptions in method code
  * (returns NULL if an exception is caught). It is a helper to avoid repeating
  * the same error handling code over and over again.
  */
#define CONTAINERPY_STDCATCH_METH \
    catch (Base::Exception &e) {\
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());\
        return NULL;\
    } catch (const Py::Exception &){\
        return NULL;\
    }
    //FIXME: add container errors!


PyObject* ContainerBasePy::getObject(PyObject* args)
{
    char* objName;
    if (!PyArg_ParseTuple(args, "s", &objName))
        return 0;

    try {
        return getContainerBasePtr()->getObject(objName)->getPyObject();
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::hasObject(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->hasObject(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr())   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isRoot(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isRoot()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAWorkspace(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAWorkspace()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isADocument(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isADocument()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAGroup(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAGroup()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAGeoGroup(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAGeoGroup()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAnOrigin(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAnOrigin()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isADocumentObject(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isADocumentObject()   ));
    } CONTAINERPY_STDCATCH_METH;
}



PyObject* ContainerBasePy::getCustomAttributes(const char* attr) const
{
    //objects accessible as attributes of Container...

    if (this->ob_type->tp_dict == NULL) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }
    PyObject* item = PyDict_GetItemString(this->ob_type->tp_dict, attr);
    if (item) return 0; //don't replace an existing attribute if object name happens to match it...

    if (getContainerBasePtr()->isNull())
        return nullptr;
    // search for an object with this name
    try {
        DocumentObject* obj = getContainerBasePtr()->getObject(attr);
        return obj->getPyObject();
    } catch (ObjectNotFoundError&){
        return nullptr;
    }
}

int ContainerBasePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

