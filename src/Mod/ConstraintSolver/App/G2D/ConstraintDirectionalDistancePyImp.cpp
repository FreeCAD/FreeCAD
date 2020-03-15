#include "PreCompiled.h"

#include "G2D/ConstraintDirectionalDistancePy.h"
#include "G2D/ConstraintDirectionalDistancePy.cpp"

#include "PyUtils.h"

#include "G2D/VectorPy.h"

using namespace FCS;

PyObject *ConstraintDirectionalDistancePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HConstraintDirectionalDistance p = new ConstraintDirectionalDistance;
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
int ConstraintDirectionalDistancePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintDirectionalDistancePy::representation(void) const
{
    return getConstraintDirectionalDistancePtr()->repr();
}


Py::Object ConstraintDirectionalDistancePy::getDirection() const
{
    return Py::asObject(getConstraintDirectionalDistancePtr()->direction().getPyObject());
}

void ConstraintDirectionalDistancePy::setDirection(Py::Object val)
{
    if (!PyObject_TypeCheck(val.ptr(), &VectorPy::Type))
        throw Py::TypeError("ConstraintDirectionalDistance.Direction must be G2D.Vector, not " + val.type().as_string());
    getConstraintDirectionalDistancePtr()->setDirection(
        static_cast<VectorPy*>(val.ptr())->value
    );
}

PyObject *ConstraintDirectionalDistancePy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintDirectionalDistancePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

