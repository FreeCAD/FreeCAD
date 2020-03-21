#include "PreCompiled.h"

#include "G2D/ConstraintAnglePy.h"
#include "G2D/ConstraintAnglePy.cpp"

#include "ValueSetPy.h"

#include "PyUtils.h"

using namespace FCS;


// returns a string which represents the object e.g. when printed in python
std::string ConstraintAnglePy::representation(void) const
{
    return getConstraintAnglePtr()->repr();
}


PyObject* ConstraintAnglePy::convertToSupplement(PyObject* args)
{
    PyObject* pcvals = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    HValueSet hvals = pcvals ? HValueSet(pcvals, false) : nullptr;
    getConstraintAnglePtr()->convertToSupplement(hvals);
    return Py::new_reference_to(Py::None());
}

PyObject* ConstraintAnglePy::convertToReversed(PyObject* args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* ConstraintAnglePy::calculateAngle(PyObject* args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}



Py::Boolean ConstraintAnglePy::getSupplementAngle(void) const
{
    //return Py::Boolean();
    throw Py::AttributeError("Not yet implemented");
}

void  ConstraintAnglePy::setSupplementAngle(Py::Boolean arg)
{
    throw Py::AttributeError("Not yet implemented");
}



PyObject* ConstraintAnglePy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintAnglePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

