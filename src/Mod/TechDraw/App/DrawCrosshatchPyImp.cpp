
#include "PreCompiled.h"

#include "DrawCrosshatch.h"

// inclusion of the generated files (generated out of DrawCrosshatchPy.xml)
#include <Mod/TechDraw/App/DrawCrosshatchPy.h>
#include <Mod/TechDraw/App/DrawCrosshatchPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawCrosshatchPy::representation(void) const
{
    return std::string("<DrawCrosshatch object>");
}







PyObject *DrawCrosshatchPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawCrosshatchPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
