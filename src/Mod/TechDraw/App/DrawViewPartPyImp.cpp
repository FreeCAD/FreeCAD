
#include "PreCompiled.h"

#include "DrawViewPart.h"

// inclusion of the generated files (generated out of DrawViewPartPy.xml)
#include <Mod/TechDraw/App/DrawViewPartPy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewPartPy::representation(void) const
{
    return std::string("<DrawViewPart object>");
}

PyObject* DrawViewPartPy::clearCV(PyObject *args)
{
    (void) args;
    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCV();
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *DrawViewPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
