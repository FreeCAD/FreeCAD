
#include "PreCompiled.h"
#include <Base/Console.h>

#include "Mod/Drawing/App/DrawPage.h"
#include "Mod/Drawing/App/DrawView.h"
#include "DrawViewPy.h"

// inclusion of the generated files (generated out of DrawPagePy.xml)
#include "DrawPagePy.h"
#include "DrawPagePy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawPagePy::representation(void) const
{
    return std::string("<DrawPage object>");
}

PyObject* DrawPagePy::addView(PyObject* args)
{
    PyObject *pcFeatView;

    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcFeatView)) {     // convert args: Python->C
        Base::Console().Error("Error: DrawPagePy::addView - Bad Args\n");
        return NULL;                             // NULL triggers exception
    }

    //page->addView(page->getDocument()->getObject(FeatName.c_str()));

    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

//    double getPageWidth() const;
PyObject* DrawPagePy::getPageWidth(PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

//    double getPageHeight() const;
PyObject* DrawPagePy::getPageHeight(PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

//    const char* getPageOrientation() const;
PyObject* DrawPagePy::getPageOrientation(PyObject *args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject *DrawPagePy::getCustomAttributes(const char* attr) const
{
    return 0;
}

int DrawPagePy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0;
}
