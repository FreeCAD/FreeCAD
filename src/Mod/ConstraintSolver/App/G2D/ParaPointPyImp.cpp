#include "PreCompiled.h"

#include "G2D/ParaPointPy.h"
#include "G2D/ParaPointPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"

using namespace FCS;

PyObject *ParaPointPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    {
        if (PyArg_ParseTuple(args, "")){
            HParaPoint p = (new ParaPoint)->self();
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        if (PyArg_ParseTuple(args, "O!",&(ParameterStorePy::Type), &store)){
            HParaPoint p = (new ParaPoint)->self();
            p->makeParameters(HParameterStore(store, false));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        const char* label;
        if (PyArg_ParseTuple(args, "sO!", &label, &(ParameterStorePy::Type), &store)){
            HParaPoint p = (new ParaPoint)->self();
            p->label = label;
            p->makeParameters(HParameterStore(store, false));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* x;
        PyObject* y;
        if (PyArg_ParseTuple(args, "O!O!",&(ParameterRefPy::Type), &x, &(ParameterRefPy::Type), &y)){
            HParaPoint p = (new ParaPoint(*HParameterRef(x,false), *HParameterRef(y,false)))->self();
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }

    PyErr_SetString(PyExc_TypeError,
        "Wrong argument count or type."
        "\n\nsupported signatures:"
        "\n() - all parameters set to null references"
        "\n(<ParameterStore object>) - creates new parameters into the store"
        "\n(label, <ParameterStore object>) - assigns the label (string), and creates new parameters into the store"
        "\n(<ParameterRef object>, <ParameterRef object>) - assigns x and y parameter refs"
    );

    return nullptr;
}

// constructor method
int ParaPointPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaPointPy::representation(void) const
{
    return ParaGeometryPy::representation();
}



PyObject *ParaPointPy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaPointPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

