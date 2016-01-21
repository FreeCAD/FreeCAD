
#include "PreCompiled.h"
#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "Mod/TechDraw/App/DrawPage.h"
#include "Mod/TechDraw/App/DrawView.h"
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
    //this implements iRC = pyPage.addView(pyView)  -or-
    //doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    PyObject *pcDocObj;

    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &pcDocObj)) {
        Base::Console().Error("Error: DrawPagePy::addView - Bad Arg - not DocumentObject\n");
        return NULL;
        //TODO: sb PyErr??
        //PyErr_SetString(PyExc_TypeError,"addView expects a DrawView");
        //return -1;
    }

    DrawPage* page = getDrawPagePtr();                         //get DrawPage for pyPage
    //TODO: argument 1 arrives as "DocumentObjectPy", not "DrawViewPy"
    //how to validate that obj is DrawView before use??
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    int rc = page->addView(view);

    return PyInt_FromLong((long) rc);
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
