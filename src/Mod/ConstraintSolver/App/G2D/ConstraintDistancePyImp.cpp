#include "PreCompiled.h"

#include "G2D/ConstraintDistancePy.h"
#include "G2D/ConstraintDistancePy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "G2D/ParaPointPy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintDistancePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{

        if (!PyArg_ParseTuple(args, "")){
            PyErr_SetString(PyExc_TypeError, "Only keyword arguments are supported");
            throw Py::Exception();
        }
        HConstraintDistance p = (new ConstraintDistance)->self().downcast<ConstraintDistance>();
        if (kwd && kwd != Py_None)
            p->initFromDict(Py::Dict(kwd));
        return p.getHandledObject();
    });
}

// constructor method
int ConstraintDistancePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintDistancePy::representation(void) const
{
    return SimpleConstraintPy::representation();
}



PyObject *ConstraintDistancePy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintDistancePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

