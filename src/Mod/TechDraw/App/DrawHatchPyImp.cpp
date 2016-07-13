
#include "PreCompiled.h"

#include "DrawHatch.h"

// inclusion of the generated files (generated out of DrawHatchPy.xml)
#include <Mod/TechDraw/App/DrawHatchPy.h>
#include <Mod/TechDraw/App/DrawHatchPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawHatchPy::representation(void) const
{
    return std::string("<DrawHatch object>");
}







PyObject *DrawHatchPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawHatchPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
