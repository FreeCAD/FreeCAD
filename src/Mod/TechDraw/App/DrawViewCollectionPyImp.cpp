
#include "PreCompiled.h"

#include "DrawViewCollection.h"

// inclusion of the generated files (generated out of DrawViewCollectionPy.xml)
#include <Mod/TechDraw/App/DrawViewCollectionPy.h>
#include <Mod/TechDraw/App/DrawViewCollectionPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewCollectionPy::representation(void) const
{
    return std::string("<DrawViewCollection object>");
}


PyObject *DrawViewCollectionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewCollectionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
