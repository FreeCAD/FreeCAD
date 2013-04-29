
#include "PreCompiled.h"

#include "Mod/Assembly/App/Constraint.h"

// inclusion of the generated files (generated out of ItemAssemblyPy.xml)
#include "ConstraintAxisPy.h"
#include "ConstraintAxisPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ConstraintAxisPy::representation(void) const
{
    return std::string("<ConstraintAxisAssembly object>");
}


PyObject *ConstraintAxisPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ConstraintAxisPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


