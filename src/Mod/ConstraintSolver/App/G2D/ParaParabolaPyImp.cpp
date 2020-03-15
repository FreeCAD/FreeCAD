#include "PreCompiled.h"

#include "G2D/ParaParabolaPy.h"
#include "G2D/ParaParabolaPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "PyUtils.h"


PyObject *ParaParabolaPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaParabola p = new ParaParabola;
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
int ParaParabolaPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaParabolaPy::representation(void) const
{
    return ParaCurvePy::representation();
}



PyObject *ParaParabolaPy::getCustomAttributes(const char* attr) const
{
    return ParaCurvePy::getCustomAttributes(attr);
}

int ParaParabolaPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaCurvePy::setCustomAttributes(attr, obj);
}

