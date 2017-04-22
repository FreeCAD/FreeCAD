#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Containers/ContainerBase.h>

// inclusion of the generated files (generated out of AttachableObjectPy.xml)
#include "ContainerBasePy.h"
#include "ContainerBasePy.cpp"
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


PyObject* ContainerBasePy::getCustomAttributes(const char*) const
{
    return 0;
}

int ContainerBasePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

