
#include "PreCompiled.h"

#include "Mod/Part/App/Part2DObject.h"
#include "Mod/PartDesign/App/Body.h"

// inclusion of the generated files (generated out of ItemPy.xml)
#include "BodyPy.h"
#include "BodyPy.cpp"

using namespace PartDesign;

// returns a string which represents the object e.g. when printed in python
std::string BodyPy::representation(void) const
{
    return std::string("<body object>");
}



PyObject *BodyPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BodyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

PyObject* BodyPy::addFeature(PyObject *args)
{
    PyObject* featurePy;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &featurePy))
        return 0;

    App::DocumentObject* feature = static_cast<App::DocumentObjectPy*>(featurePy)->getDocumentObjectPtr();

    if (!Body::isAllowed(feature)) {
        PyErr_SetString(PyExc_SystemError, "Only PartDesign features, datum features and sketches can be inserted into a Body");
        return 0;
    }
    Body* body = this->getBodyPtr();

    try {
        body->addFeature(feature);
    } catch (Base::Exception& e) {
        PyErr_SetString(PyExc_SystemError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* BodyPy::removeFeature(PyObject *args)
{
    PyObject* featurePy;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &featurePy))
        return 0;

    App::DocumentObject* feature = static_cast<App::DocumentObjectPy*>(featurePy)->getDocumentObjectPtr();
    Body* body = this->getBodyPtr();

    try {
        body->removeFeature(feature);
    } catch (Base::Exception& e) {
        PyErr_SetString(PyExc_SystemError, e.what());
        return 0;
    }

    Py_Return;
}


