#include "PreCompiled.h"

#include "G2D/ConstraintPointSymmetryPy.h"
#include "G2D/ConstraintPointSymmetryPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "G2D/ParaPointPy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintPointSymmetryPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{

        if (!PyArg_ParseTuple(args, "")){
            PyErr_SetString(PyExc_TypeError, "Only keyword arguments are supported");
            throw Py::Exception();
        }
        HConstraintPointSymmetry p = (new ConstraintPointSymmetry)->getHandle<ConstraintPointSymmetry>();
        if (kwd && kwd != Py_None)
            p->initFromDict(Py::Dict(kwd));
        return p.getHandledObject();
    });
}

// constructor method
int ConstraintPointSymmetryPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintPointSymmetryPy::representation(void) const
{
    return ConstraintPy::representation();
}



PyObject *ConstraintPointSymmetryPy::getCustomAttributes(const char* attr) const
{
    return ConstraintPy::getCustomAttributes(attr);
}

int ConstraintPointSymmetryPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ConstraintPy::setCustomAttributes(attr, obj);
}

