
#include "PreCompiled.h"

#include "App/GeoFeatureGroup.h"

// inclusion of the generated files (generated out of GeoFeatureGroupPy.xml)
#include "GeoFeatureGroupPy.h"
#include "GeoFeatureGroupPy.cpp"

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string GeoFeatureGroupPy::representation(void) const
{
    return std::string("<GeoFeatureGroup object>");
}



PyObject*  GeoFeatureGroupPy::newObject(PyObject *args)
{
    char *sType,*sName=0;
    if (!PyArg_ParseTuple(args, "s|s", &sType,&sName))     // convert args: Python->C
        return NULL;

    DocumentObject *object = getGeoFeatureGroupPtr()->addObject(sType, sName);
    if ( object ) {
        return object->getPyObject();
    } 
    else {
        PyErr_Format(PyExc_Exception, "Cannot create object of type '%s'", sType);
        return NULL;
    }
}

PyObject*  GeoFeatureGroupPy::addObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot add an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getGeoFeatureGroupPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot add an object from another document to this GeoFeatureGroup");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr() == this->getGeoFeatureGroupPtr()) {
        PyErr_SetString(PyExc_Exception, "Cannot add a GeoFeatureGroup to itself");
        return NULL;
    }

    getGeoFeatureGroupPtr()->addObject(docObj->getDocumentObjectPtr());

    Py_Return;
}

PyObject*  GeoFeatureGroupPy::removeObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot remove an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getGeoFeatureGroupPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot remove an object from another document from this group");
        return NULL;
    }

    getGeoFeatureGroupPtr()->removeObject(docObj->getDocumentObjectPtr());

    Py_Return;
}

PyObject*  GeoFeatureGroupPy::removeObjectsFromDocument(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    getGeoFeatureGroupPtr()->removeObjectsFromDocument();
    Py_Return;
}

PyObject*  GeoFeatureGroupPy::getObject(PyObject *args)
{
    char* pcName;
    if (!PyArg_ParseTuple(args, "s", &pcName))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    DocumentObject* obj = getGeoFeatureGroupPtr()->getObject(pcName);
    if ( obj ) {
        return obj->getPyObject();
    } else {
        Py_Return;
    }
}

PyObject*  GeoFeatureGroupPy::hasObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getGeoFeatureGroupPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check an object from another document with this group");
        return NULL;
    }

    if (getGeoFeatureGroupPtr()->hasObject(docObj->getDocumentObjectPtr())) {
        Py_INCREF(Py_True);
        return Py_True;
    } 
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}




PyObject *GeoFeatureGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeoFeatureGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


