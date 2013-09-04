
#include "PreCompiled.h"

#include "Base/Quantity.h"

// inclusion of the generated files (generated out of QuantityPy.xml)
#include "QuantityPy.h"
#include "QuantityPy.cpp"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string QuantityPy::representation(void) const
{
    return std::string("<Quantity object>");
}

PyObject *QuantityPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of QuantityPy and the Twin object 
    return new QuantityPy(new Quantity);
}

// constructor method
int QuantityPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* QuantityPy::multiply(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}





Py::Float QuantityPy::getValue(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void QuantityPy::setValue(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Object QuantityPy::getUnit(void) const
{
    //return Py::Object();
    throw Py::AttributeError("Not yet implemented");
}

void QuantityPy::setUnit(Py::Object /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

PyObject *QuantityPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int QuantityPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


