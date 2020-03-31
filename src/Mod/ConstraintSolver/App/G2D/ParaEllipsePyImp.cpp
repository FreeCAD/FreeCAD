#include "PreCompiled.h"

#include "G2D/ParaEllipsePy.h"
#include "G2D/ParaEllipsePy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "PyUtils.h"


PyObject *ParaEllipsePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaEllipse p = new ParaEllipse;
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
int ParaEllipsePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaEllipsePy::representation(void) const
{
    return ParaConicPy::representation();
}



PyObject *ParaEllipsePy::getCustomAttributes(const char* attr) const
{
    return ParaConicPy::getCustomAttributes(attr);
}

int ParaEllipsePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaConicPy::setCustomAttributes(attr, obj);
}

