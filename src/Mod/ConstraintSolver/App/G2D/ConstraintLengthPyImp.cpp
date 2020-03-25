#include "PreCompiled.h"

#include "G2D/ConstraintLengthPy.h"
#include "G2D/ConstraintLengthPy.cpp"

#include "G2D/ParaCurvePy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintLengthPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HConstraintLength p = new ConstraintLength;
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
int ConstraintLengthPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintLengthPy::representation(void) const
{
    return getConstraintLengthPtr()->repr();
}


Py::List ConstraintLengthPy::getEdges() const
{
    return asPyObjectList(getConstraintLengthPtr()->edges());
}

void ConstraintLengthPy::setEdges(Py::List val)
{
    std::vector<HParaCurve> edges;
    for (Py::Object it : val){
        if (!PyObject_TypeCheck( it.ptr(), &ParaCurvePy::Type))
            throw Py::TypeError("list items must be of type G2D.ParaCurve, not " + it.type().as_string());
        edges.push_back(HParaCurve(it));
    }
    getConstraintLengthPtr()->setEdges(edges);
}


PyObject *ConstraintLengthPy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintLengthPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

