#include "PreCompiled.h"

#include "G2D/ParaHyperbolaPy.h"
#include "G2D/ParaHyperbolaPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "PyUtils.h"


PyObject *ParaHyperbolaPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaHyperbola p = new ParaHyperbola;
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
int ParaHyperbolaPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaHyperbolaPy::representation(void) const
{
    return ParaConicPy::representation();
}



PyObject *ParaHyperbolaPy::getCustomAttributes(const char* attr) const
{
    return ParaConicPy::getCustomAttributes(attr);
}

int ParaHyperbolaPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaConicPy::setCustomAttributes(attr, obj);
}

