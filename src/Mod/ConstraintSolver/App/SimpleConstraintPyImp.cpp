#include "PreCompiled.h"

#include "SimpleConstraintPy.h"
#include "SimpleConstraintPy.cpp"

#include "ConstraintPy.h"
#include "ValueSetPy.h"
#include "PyUtils.h"


std::string SimpleConstraintPy::representation(void) const
{
    return ConstraintPy::representation();
}

PyObject* SimpleConstraintPy::error1(PyObject *args)
{
    PyObject* pyvals = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ValueSetPy::Type), &pyvals))
        return nullptr;
    HValueSet vals(pyvals, false);
    return getSimpleConstraintPtr()->error1(*vals).getPyObject();
}


PyObject *SimpleConstraintPy::getCustomAttributes(const char* attr) const
{
    return ConstraintPy::getCustomAttributes(attr);
}

int SimpleConstraintPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ConstraintPy::setCustomAttributes(attr, obj);
}
