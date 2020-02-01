#include "PreCompiled.h"

#include "G2D/ParaCirclePy.h"
#include "G2D/ParaCirclePy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "PyUtils.h"


PyObject *ParaCirclePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{

    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HParaCircle p = new ParaCircle;
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p;
            }
            PyErr_Clear();
        }
        {
            int isFull = false;
            if (PyArg_ParseTuple(args, "p", &isFull)){
                HParaCircle p = new ParaCircle(bool(isFull));
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p;
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
int ParaCirclePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaCirclePy::representation(void) const
{
    return ParaCurvePy::representation();
}



PyObject *ParaCirclePy::getCustomAttributes(const char* attr) const
{
    return ParaCurvePy::getCustomAttributes(attr);
}

int ParaCirclePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaCurvePy::setCustomAttributes(attr, obj);
}

