#include "PreCompiled.h"

#include "G2D/ParaBSplineBasePy.h"
#include "G2D/ParaBSplineBasePy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "PyUtils.h"


PyObject *ParaBSplineBasePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaBSplineBase p = new ParaBSplineBase;
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }
        throw Py::TypeError(
            "Wrong argument count or type."
            "\n\nsupported signatures:"
            "\n() - all references set to None"
            "\n(**keyword_args) - assigns attributes."
        );
    });
}

// constructor method
int ParaBSplineBasePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaBSplineBasePy::representation(void) const
{
    return ParaCurvePy::representation();
}



PyObject *ParaBSplineBasePy::getCustomAttributes(const char* attr) const
{
    return ParaCurvePy::getCustomAttributes(attr);
}

int ParaBSplineBasePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaCurvePy::setCustomAttributes(attr, obj);
}

