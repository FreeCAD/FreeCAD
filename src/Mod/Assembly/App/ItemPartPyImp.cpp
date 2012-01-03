
#include "PreCompiled.h"

#include "Mod/Assembly/App/ItemPart.h"

// inclusion of the generated files (generated out of ItemPartPy.xml)
#include "ItemPartPy.h"
#include "ItemPartPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ItemPartPy::representation(void) const
{
    return std::string("<ItemPart object>");
}







PyObject *ItemPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ItemPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


