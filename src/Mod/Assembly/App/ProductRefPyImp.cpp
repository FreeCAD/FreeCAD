
#include "PreCompiled.h"

#include "Mod/Assembly/App/ProductRef.h"

// inclusion of the generated files (generated out of ProductRefPy.xml)
#include "ProductRefPy.h"
#include "ProductRefPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string ProductRefPy::representation(void) const
{
    return std::string("<ProductRef object>");
}


PyObject *ProductRefPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ProductRefPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
