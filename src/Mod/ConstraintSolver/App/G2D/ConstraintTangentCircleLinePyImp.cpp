#include "PreCompiled.h"

#include "G2D/ConstraintTangentCircleLinePy.h"
#include "G2D/ConstraintTangentCircleLinePy.cpp"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintTangentCircleLinePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HConstraintTangentCircleLine p = new ConstraintTangentCircleLine;
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
int ConstraintTangentCircleLinePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintTangentCircleLinePy::representation(void) const
{
    return getConstraintTangentCircleLinePtr()->repr();
}



PyObject *ConstraintTangentCircleLinePy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintTangentCircleLinePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

