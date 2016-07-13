
#include "PreCompiled.h"

#include <Mod/TechDraw/App/DrawView.h>

// inclusion of the generated files (generated out of DrawViewPy.xml)
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewPy::representation(void) const
{
    return std::string("<DrawView object>");
}







PyObject *DrawViewPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


