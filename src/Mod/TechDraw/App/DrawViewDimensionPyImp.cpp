
#include "PreCompiled.h"

#include "Mod/TechDraw/App/DrawViewDimension.h"

// inclusion of the generated files (generated out of DrawViewDimensionPy.xml)
#include "DrawViewDimensionPy.h"
#include "DrawViewDimensionPy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewDimensionPy::representation(void) const
{
    return std::string("<DrawViewDimension object>");
}







PyObject *DrawViewDimensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewDimensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
