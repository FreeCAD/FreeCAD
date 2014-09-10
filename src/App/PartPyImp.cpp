
#include "PreCompiled.h"

#include "App/Part.h"

// inclusion of the generated files (generated out of PartPy.xml)
#include "PartPy.h"
#include "PartPy.cpp"

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string PartPy::representation(void) const
{
    return std::string("<Part object>");
}



PyObject*  PartPy::newObject(PyObject *args)
{
    char *sType,*sName=0;
    if (!PyArg_ParseTuple(args, "s|s", &sType,&sName))     // convert args: Python->C
        return NULL;

    DocumentObject *object = getPartPtr()->addObject(sType, sName);
    if ( object ) {
        return object->getPyObject();
    } 
    else {
        PyErr_Format(PyExc_Exception, "Cannot create object of type '%s'", sType);
        return NULL;
    }
}

PyObject*  PartPy::addObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot add an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getPartPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot add an object from another document to this Part");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr() == this->getPartPtr()) {
        PyErr_SetString(PyExc_Exception, "Cannot add a Part to itself");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getTypeId().isDerivedFrom(Part::getClassTypeId())) {
        PyErr_SetString(PyExc_Exception, "Cannot add a Part to a Part");
        return NULL;
    }

    Part* part = getPartPtr();

    part->addObject(docObj->getDocumentObjectPtr());
    Py_Return;
}

PyObject*  PartPy::removeObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot remove an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getPartPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot remove an object from another document from this group");
        return NULL;
    }

    Part* part = getPartPtr();


    part->removeObject(docObj->getDocumentObjectPtr());
    Py_Return;
}

PyObject*  PartPy::removeObjectsFromDocument(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    getPartPtr()->removeObjectsFromDocument();
    Py_Return;
}

PyObject*  PartPy::getObject(PyObject *args)
{
    char* pcName;
    if (!PyArg_ParseTuple(args, "s", &pcName))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    DocumentObject* obj = getPartPtr()->getObject(pcName);
    if ( obj ) {
        return obj->getPyObject();
    } else {
        Py_Return;
    }
}

PyObject*  PartPy::hasObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getPartPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check an object from another document with this group");
        return NULL;
    }

    if (getPartPtr()->hasObject(docObj->getDocumentObjectPtr())) {
        Py_INCREF(Py_True);
        return Py_True;
    } 
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}




PyObject *PartPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


