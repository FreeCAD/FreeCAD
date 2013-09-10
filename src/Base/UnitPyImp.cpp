
#include "PreCompiled.h"

#include "Base/Unit.h"

// inclusion of the generated files (generated out of UnitPy.xml)
#include "UnitPy.h"
#include "UnitPy.cpp"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string UnitPy::representation(void) const
{
    return std::string("<Unit object>");
}

PyObject *UnitPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of UnitPy and the Twin object 
    return new UnitPy(new Unit);
}

// constructor method
int UnitPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* UnitPy::multiply(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* UnitPy::getType(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}


PyObject* UnitPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Unit");
        return 0;
    }
    Base::Unit *a = static_cast<UnitPy*>(self)->getUnitPtr();
    Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

    if (*a != *b) {
        PyErr_SetString(PyExc_TypeError, "Units not matching!");
        return 0;
    }

    return new UnitPy(new Unit(*a));
}

PyObject* UnitPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Unit");
        return 0;
    }
    Base::Unit *a = static_cast<UnitPy*>(self)->getUnitPtr();
    Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

    if (*a != *b) {
        PyErr_SetString(PyExc_TypeError, "Units not matching!");
        return 0;
    }

    return new UnitPy(new Unit(*a));
}

PyObject* UnitPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return 0;
    }

    if (PyObject_TypeCheck(other, &(UnitPy::Type))) {
        Base::Unit *a = static_cast<UnitPy*>(self) ->getUnitPtr();
        Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();
        
        return new UnitPy(new Unit( (*a) * (*b) ) );
    }
    else {
        PyErr_SetString(PyExc_TypeError, "A Unit can only be multiplied by a Unit");
        return 0;
    }
}


Py::Object UnitPy::getDimensions(void) const
{
    //return Py::Object();
    throw Py::AttributeError("Not yet implemented");
}

void UnitPy::setDimensions(Py::Object /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

PyObject *UnitPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int UnitPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


