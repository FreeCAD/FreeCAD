
#include "PreCompiled.h"

#include "DrawGeomHatch.h"

// inclusion of the generated files (generated out of DrawGeomHatchPy.xml)
#include <Mod/TechDraw/App/DrawGeomHatchPy.h>
#include <Mod/TechDraw/App/DrawGeomHatchPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawGeomHatchPy::representation(void) const
{
    return std::string("<DrawGeomHatch object>");
}







PyObject *DrawGeomHatchPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawGeomHatchPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
