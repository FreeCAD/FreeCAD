
#include "PreCompiled.h"

#include "Mod/Drawing/App/DrawViewSymbol.h"
#include "Mod/Drawing/App/DrawView.h"
#include "Mod/Drawing/App/DrawViewPy.h"

// inclusion of the generated files (generated out of DrawViewSymbolPy.xml)
#include "DrawViewSymbolPy.h"
#include "DrawViewSymbolPy.cpp"

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


