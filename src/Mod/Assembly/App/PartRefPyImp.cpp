
#include "PreCompiled.h"

#include "Mod/Assembly/App/PartRef.h"

// inclusion of the generated files (generated out of PartRefPy.xml)
#include "PartRefPy.h"
#include "PartRefPy.cpp"

using namespace Assembly;

// returns a string which represents the object e.g. when printed in python
std::string PartRefPy::representation(void) const
{
    return std::string("<PartRef object>");
}







PyObject *PartRefPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PartRefPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


