#include "PreCompiled.h"

#include "G2D/ConstraintDistancePy.h"
#include "G2D/ConstraintDistancePy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;

PyObject *ConstraintDistancePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    {
        if (PyArg_ParseTuple(args, "")){
            HConstraintDistance p = (new ConstraintDistance)->self();
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        if (PyArg_ParseTuple(args, "O!",&(ParameterStorePy::Type), &store)){
            HConstraintDistance p = (new ConstraintDistance)->self();
            p->makeParameters(HParameterStore(store, false));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        const char* label;
        if (PyArg_ParseTuple(args, "sO!", &label, &(ParameterStorePy::Type), &store)){
            HConstraintDistance p = (new ConstraintDistance)->self();
            p->label = label;
            p->makeParameters(HParameterStore(store, false));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* p1;
        PyObject* p2;
        PyObject* dist;
        if (PyArg_ParseTuple(args, "O!O!O!",&(ParaPointPy::Type), &p1, &(ParaPointPy::Type), &p2, &p1, &(ParameterRefPy::Type), &dist)){
            HConstraintDistance p = (new ConstraintDistance(HParaPoint(p1,false), HParaPoint(p2,false), *HParameterRef(dist, false)))->self();
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }

    PyErr_SetString(PyExc_TypeError,
        "Wrong argument count or type."
        "\n\nsupported signatures:"
        "\n() - all references are none/null"
        "\n(<ParameterStore object>) - creates dist parameter into the store; Point references are left None."
        "\n(label, <ParameterStore object>) - same, + assigns the label (string)"
        "\n(<ParaPoint object>, <ParaPoint object>, <ParameterRef object>) - inits all references"
    );

    return nullptr;
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

