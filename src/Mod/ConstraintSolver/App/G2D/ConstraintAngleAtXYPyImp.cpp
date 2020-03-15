#include "PreCompiled.h"

#include "G2D/ConstraintAngleAtXYPy.h"
#include "G2D/ConstraintAngleAtXYPy.cpp"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintAngleAtXYPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HConstraintAngleAtXY p = new ConstraintAngleAtXY;
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
            "\n(<ParaPlacement object>, **keyword_args) - make the constraint for the given placement"
            "\n(**keyword_args) - assigns attributes."
        );
    });
}

// constructor method
int ConstraintAngleAtXYPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintAngleAtXYPy::representation(void) const
{
    return getConstraintAngleAtXYPtr()->repr();
}



PyObject *ConstraintAngleAtXYPy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintAngleAtXYPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

