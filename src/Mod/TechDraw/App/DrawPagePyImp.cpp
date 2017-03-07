
#include "PreCompiled.h"

#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "DrawPage.h"
#include "DrawView.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawPagePy.h>
#include <Mod/TechDraw/App/DrawPagePy.cpp>

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
        PyErr_SetString(PyExc_TypeError, "DrawPagePy::AddView - Bad Arg - not DocumentObject");
        return nullptr;
    }

    DrawPage* page = getDrawPagePtr();                         //get DrawPage for pyPage
    //TODO: argument 1 arrives as "DocumentObjectPy", not "DrawViewPy"
    //how to validate that obj is DrawView before use??
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    int rc = page->addView(view);
#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong((long) rc);
#else
    return PyLong_FromLong((long) rc);
#endif
}

PyObject* DrawPagePy::removeView(PyObject* args)
{
    //this implements iRC = pyPage.removeView(pyView)  -or-
    //doCommand(Doc,"App.activeDocument().%s.removeView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    PyObject *pcDocObj;

    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &pcDocObj)) {
        PyErr_SetString(PyExc_TypeError, "DrawPagePy::removeView - Bad Arg - not DocumentObject");
        return nullptr;
    }

    DrawPage* page = getDrawPagePtr();                         //get DrawPage for pyPage
    //how to validate that obj is DrawView before use??
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    int rc = page->removeView(view);

#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong((long) rc);
#else
    return PyLong_FromLong((long) rc);
#endif
}


//    double getPageWidth() const;
PyObject* DrawPagePy::getPageWidth(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

//    double getPageHeight() const;
PyObject* DrawPagePy::getPageHeight(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

//    const char* getPageOrientation() const;
PyObject* DrawPagePy::getPageOrientation(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject *DrawPagePy::getCustomAttributes(const char* ) const
{
    return 0;
}

int DrawPagePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
