#include "PreCompiled.h"

#include "G2D/ConstraintDistancePy.h"
#include "G2D/ConstraintDistancePy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;

PyObject *ConstraintDistancePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    if (!PyArg_ParseTuple(args, "")){
        PyErr_SetString(PyExc_TypeError, "Only keyword arguments are supported");
        return nullptr;
    }
    try {
        HConstraintDistance p = (new ConstraintDistance)->self();
        p->initFromDict(Py::Dict(kwd));
        return Py::new_reference_to(p);
    } catch (Py::Exception&){
        return nullptr;
    } catch (Base::Exception& e) {
        auto pye = e.getPyExceptionType();
        if(!pye)
            pye = Base::BaseExceptionFreeCADError;
        PyErr_SetObject(pye, e.getPyObject());
        return nullptr;
    }
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

