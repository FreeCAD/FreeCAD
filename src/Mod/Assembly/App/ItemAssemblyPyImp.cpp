
#include "PreCompiled.h"

#include "Mod/Assembly/App/ItemAssembly.h"

// inclusion of the generated files (generated out of ItemAssemblyPy.xml)
#include "ItemAssemblyPy.h"
#include "ItemAssemblyPy.cpp"
#include <ItemPartPy.h>

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ItemAssemblyPy::representation(void) const
{
    return std::string("<ItemAssembly object>");
}


PyObject *ItemAssemblyPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ItemAssemblyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
