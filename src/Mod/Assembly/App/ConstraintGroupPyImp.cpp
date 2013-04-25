
#include "PreCompiled.h"

#include "Mod/Assembly/App/ConstraintGroup.h"
#include "Mod/Assembly/App/ConstraintPy.h"

// inclusion of the generated files (generated out of ConstraintGroupPy.xml)
#include "ConstraintGroupPy.h"
#include "ConstraintGroupPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ConstraintGroupPy::representation(void) const
{
    return std::string("<ConstraintGroup object>");
}

PyObject*  ConstraintGroupPy::addConstraint(PyObject * args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(Assembly::ConstraintPy::Type))) {
	Base::Console().Message("Add constraint\n");
        Assembly::Constraint *c = static_cast<Assembly::ConstraintPy*>(pcObj)->getConstraintPtr();
        this->getConstraintGroupPtr()->addConstraint(c);
    }
    Py_Return; 
}

PyObject *ConstraintGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ConstraintGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


