#include "PreCompiled.h"

#include "G2D/ParaVectorPy.h"
#include "G2D/ParaVectorPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"

using namespace FCS;

PyObject *ParaVectorPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    {
        if (PyArg_ParseTuple(args, "")){
            HParaVector p = (new ParaVector)->self();
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        if (PyArg_ParseTuple(args, "O!",&(ParameterStorePy::Type), &store)){
            HParaVector p = (new ParaVector)->self();
            p->makeParameters(HParameterStore(store, false));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        const char* label;
        if (PyArg_ParseTuple(args, "sO!", &label, &(ParameterStorePy::Type), &store)){
            HParaVector p = (new ParaVector)->self();
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
            HParaVector p = (new ParaVector(*HParameterRef(x,false), *HParameterRef(y,false)))->self();
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
int ParaVectorPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaVectorPy::representation(void) const
{
    return ParaObjectPy::representation();
}



PyObject *ParaVectorPy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ParaVectorPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}

