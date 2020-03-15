#include "PreCompiled.h"

#include "G2D/ConstraintPointCoincidentPy.h"
#include "G2D/ConstraintPointCoincidentPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "G2D/ParaPointPy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintPointCoincidentPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{

        if (!PyArg_ParseTuple(args, "")){
            PyErr_SetString(PyExc_TypeError, "Only keyword arguments are supported");
            throw Py::Exception();
        }
        /*HConstraintPointCoincident p = (new ConstraintPointCoincident)->self().downcast<ConstraintPointCoincident>();*/
        HConstraintPointCoincident p = HConstraintPointCoincident(new ConstraintPointCoincident);
        if (kwd && kwd != Py_None)
            p->initFromDict(Py::Dict(kwd));
        return p.getHandledObject();
    });
}

// constructor method
int ConstraintPointCoincidentPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintPointCoincidentPy::representation(void) const
{
    return ConstraintPy::representation();
}



PyObject *ConstraintPointCoincidentPy::getCustomAttributes(const char* attr) const
{
    return ConstraintPy::getCustomAttributes(attr);
}

int ConstraintPointCoincidentPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ConstraintPy::setCustomAttributes(attr, obj);
}

