
#include "PreCompiled.h"

#include "Mod/TechDraw/App/DrawProjGroupItem.h"

// inclusion of the generated files (generated out of DrawProjGroupItemPy.xml)
#include "DrawProjGroupItemPy.h"
#include "DrawProjGroupItemPy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawProjGroupItemPy::representation(void) const
{
    return std::string("<DrawProjGroupItem object>");
}







PyObject *DrawProjGroupItemPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawProjGroupItemPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


