
#include "PreCompiled.h"

#include "Mod/TechDraw/Gui/MDIViewPage.h"

// inclusion of the generated files (generated out of MDIViewPagePy.xml)
#include "MDIViewPagePy.h"
#include "MDIViewPagePy.cpp"

#include <Mod/TechDraw/App/DrawPagePy.h>

using namespace TechDrawGui;

// returns a string which represents the object e.g. when printed in python
std::string MDIViewPagePy::representation() const
{
    return std::string("<TechDrawView object>");
}



PyObject* MDIViewPagePy::getPage(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return new TechDraw::DrawPagePy(getMDIViewPagePtr()->getPage());
}





PyObject *MDIViewPagePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MDIViewPagePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


