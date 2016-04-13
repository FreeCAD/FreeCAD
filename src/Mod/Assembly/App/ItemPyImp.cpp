
#include "PreCompiled.h"

#include "Mod/Assembly/App/Item.h"

// inclusion of the generated files (generated out of ItemPy.xml)
#include "ItemPy.h"
#include "ItemPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ItemPy::representation(void) const
{
    return std::string("<Item object>");
}







PyObject *ItemPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ItemPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


