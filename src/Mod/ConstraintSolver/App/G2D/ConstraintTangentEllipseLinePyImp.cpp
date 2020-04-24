#include "PreCompiled.h"

#include "G2D/ConstraintTangentEllipseLinePy.h"
#include "G2D/ConstraintTangentEllipseLinePy.cpp"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintTangentEllipseLinePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HConstraintTangentEllipseLine p = new ConstraintTangentEllipseLine;
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
int ConstraintTangentEllipseLinePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintTangentEllipseLinePy::representation(void) const
{
    return getConstraintTangentEllipseLinePtr()->repr();
}



PyObject *ConstraintTangentEllipseLinePy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintTangentEllipseLinePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

