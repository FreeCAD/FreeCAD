#include "PreCompiled.h"

#include "G2D/ConstraintPlacementRulesPy.h"
#include "G2D/ConstraintPlacementRulesPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"
#include "G2D/ParaPointPy.h"
#include "G2D/ParaPlacementPy.h"

#include "PyUtils.h"

using namespace FCS;

PyObject *ConstraintPlacementRulesPy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{
        {
            if (PyArg_ParseTuple(args, "")){
                HConstraintPlacementRules p = new ConstraintPlacementRules;
                if (kwd && kwd != Py_None)
                    p->initFromDict(Py::Dict(kwd));
                return p.getHandledObject();
            }
            PyErr_Clear();
        }
        {
            PyObject* placement;
            if (PyArg_ParseTuple(args, "O!",&(ParaPlacementPy::Type), &placement)){
                HConstraintPlacementRules p = new ConstraintPlacementRules(HParaPlacement(placement, false));
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
int ConstraintPlacementRulesPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ConstraintPlacementRulesPy::representation(void) const
{
    return getConstraintPlacementRulesPtr()->repr();
}



PyObject *ConstraintPlacementRulesPy::getCustomAttributes(const char* attr) const
{
    return SimpleConstraintPy::getCustomAttributes(attr);
}

int ConstraintPlacementRulesPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return SimpleConstraintPy::setCustomAttributes(attr, obj);
}

