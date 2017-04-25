#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Containers/Container.h>
#include <App/PropertyContainerPy.h>

// inclusion of the generated files (generated out of ContainerPy.xml)
#include <Containers/ContainerPy.h>
#include <Containers/ContainerPy.cpp>
#include <App/DocumentObjectPy.h>

using namespace App;

PyObject* ContainerPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of AttachEngine3D
    return new ContainerPy(new Container);
}

// constructor method
int ContainerPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(PropertyContainerPy::Type), &o)) {
        PropertyContainer* cnt = static_cast<PropertyContainerPy*>(o)->getPropertyContainerPtr();
        this->_pcTwinPointer = new Container(cnt);
        return 0;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "Wrong set of constructor arguments. Can be: (), (document), (documentObject).");
    return -1;

}


// returns a string which represents the object e.g. when printed in python
std::string ContainerPy::representation(void) const
{
    std::stringstream repr;
    repr << "<App::Container around " ;
    if (getContainerBasePtr()->object())
        repr << getContainerBasePtr()->getName();
    else
        repr << "None";
    repr << ">";
    return std::string(repr.str());
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

PyObject* ContainerPy::canAccept(PyObject* args)
{
    PyObject* obj = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj)){
        try {
            return Py::new_reference_to(Py::Boolean(
                getContainerPtr()->canAccept(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr())   ));
        } CONTAINERPY_STDCATCH_METH;
    }
    PyErr_Clear();

    char* type = nullptr;
    char* typepy = nullptr;
    if (PyArg_ParseTuple(args, "s|s", &type, &typepy)){
        try {
            return Py::new_reference_to(Py::Boolean(
                getContainerPtr()->canAccept(type, typepy ? typepy : "")   ));
        } CONTAINERPY_STDCATCH_METH;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "Wrong argument types. Can be: (object) or (string, string)");
    return 0;
}

PyObject* ContainerPy::newObject(PyObject* args)
{
    char* type = nullptr;
    char* name = nullptr;
    if (!PyArg_ParseTuple(args, "s|s", &type, &name))
        return 0;

    try {
        DocumentObject* newobj = getContainerPtr()->newObject(type, name);
        return newobj->getPyObject();
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerPy::adoptObject(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        return Py::new_reference_to(Py::Boolean(
            getContainerPtr()->adoptObject(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr())   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerPy::addObject(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        getContainerPtr()->addObject(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr());
        return Py::new_reference_to(Py::None());
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerPy::withdrawObject(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        getContainerPtr()->withdrawObject(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr());
        return Py::new_reference_to(Py::None());
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerPy::deleteObject(PyObject* args)
{
    PyObject* obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        getContainerPtr()->deleteObject(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr());
        return Py::new_reference_to(Py::None());
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerPy::getCustomAttributes(const char*) const
{
    return 0;
}

int ContainerPy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

