#include "PreCompiled.h"

#include "G2D/ParaPlacementPy.h"
#include "G2D/ParaPlacementPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"

using namespace FCS;

PyObject *ParaPlacementPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    {
        if (PyArg_ParseTuple(args, "")){
            HParaPlacement p = (new ParaPlacement)->self();
            if (!(kwd == Py_None))
                p->initFromDict(Py::Dict(kwd));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    {
        PyObject* store;
        if (PyArg_ParseTuple(args, "O!",&(ParameterStorePy::Type), &store)){
            HParaPlacement p = (new ParaPlacement)->self();
            p->makeParameters(HParameterStore(store, false));
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }

    PyErr_SetString(PyExc_TypeError,
        "Wrong argument count or type."
        "\n\nsupported signatures:"
        "\n() - all parameters set to null references"
        "\n(<ParameterStore object>) - creates new parameters into the store"
        "\n(**keyword_args) - assigns attributes. If 'store':<ParameterStore object> is given, the unlisted references are created into the store automatically."
    );

    return nullptr;
}

// constructor method
int ParaPlacementPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaPlacementPy::representation(void) const
{
    return ParaObjectPy::representation();
}



PyObject *ParaPlacementPy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ParaPlacementPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}

