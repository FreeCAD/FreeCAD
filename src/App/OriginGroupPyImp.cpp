
#include "PreCompiled.h"

#include "App/OriginGroup.h"

// inclusion of the generated files (generated out of OriginGroupPy.xml)
#include "OriginGroupPy.h"
#include "OriginGroupPy.cpp"

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string OriginGroupPy::representation(void) const
{
    return std::string("<OriginGroup object>");
}







PyObject *OriginGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int OriginGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


