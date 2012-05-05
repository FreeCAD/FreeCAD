
#include "PreCompiled.h"

#include "Mod/PartDesign/App/Body.h"

// inclusion of the generated files (generated out of ItemPy.xml)
#include "BodyPy.h"
#include "BodyPy.cpp"

using namespace PartDesign;

// returns a string which represents the object e.g. when printed in python
std::string BodyPy::representation(void) const
{
    return std::string("<body object>");
}



PyObject *BodyPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BodyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


