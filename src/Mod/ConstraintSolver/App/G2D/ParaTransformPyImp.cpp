#include "PreCompiled.h"

#include "G2D/ParaTransformPy.h"
#include "G2D/ParaTransformPy.cpp"

using namespace FCS;

PyObject *ParaTransformPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    {
        if (PyArg_ParseTuple(args, "")){
            HParaTransform p = new ParaTransform();
            return Py::new_reference_to(p);
        }
        PyErr_Clear();
    }
    //{
    //    PyObject* x;
    //    PyObject* y;
    //    if (PyArg_ParseTuple(args, "O!O!",&(ParameterRefPy::Type), &x, &(ParameterRefPy::Type), &y)){
    //        HParaTransform p = (new ParaTransform(*HParameterRef(x,false), *HParameterRef(y,false)))->self();
    //        return Py::new_reference_to(p);
    //    }
    //    PyErr_Clear();
    //}

    PyErr_SetString(PyExc_TypeError,
        "Wrong argument count or type."
        "\n\nsupported signatures:"
        "\n() - trivial transform"
    );

    return nullptr;
}

// constructor method
int ParaTransformPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaTransformPy::representation(void) const
{
    return ParaObjectPy::representation();
}



PyObject *ParaTransformPy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ParaTransformPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}

