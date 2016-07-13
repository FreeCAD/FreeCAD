
#include "PreCompiled.h"

#include "DrawViewSymbol.h"
#include "DrawView.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewSymbolPy.h>
#include <Mod/TechDraw/App/DrawViewSymbolPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewSymbolPy::representation(void) const
{
    return std::string("<DrawViewSymbol object>");
}


PyObject *DrawViewSymbolPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewSymbolPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
