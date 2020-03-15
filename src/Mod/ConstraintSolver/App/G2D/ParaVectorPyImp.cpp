#include "PreCompiled.h"

#include "G2D/ParaVectorPy.h"
#include "G2D/ParaVectorPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ParaVectorPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{

        {
            if (PyArg_ParseTuple(args, "")){
                HParaVector p = (new ParaVector)->self().downcast<ParaVector>();
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }
        {
            PyObject* x;
            PyObject* y;
            if (PyArg_ParseTuple(args, "O!O!",&(ParameterRefPy::Type), &x, &(ParameterRefPy::Type), &y)){
                HParaVector p = (new ParaVector(*HParameterRef(x,false), *HParameterRef(y,false)))->self().downcast<ParaVector>();
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }

        throw Py::TypeError(
            "Wrong argument count or type."
            "\n\nsupported signatures:"
            "\n() - all parameters set to null references"
            "\n(<ParameterRef object>, <ParameterRef object>, **keyword_args = {}) - assigns x and y parameter refs"
            "\n(**keyword_args) - assigns attributes."
        );

    });
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

