
#include "PreCompiled.h"

#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "DrawViewClip.h"
#include "DrawView.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewClipPy.h>
#include <Mod/TechDraw/App/DrawViewClipPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewClipPy::representation(void) const
{
    return std::string("<DrawViewClip object>");
}

PyObject* DrawViewClipPy::addView(PyObject* args)
{
    //this implements iRC = pyClip.addView(pyView)  -or-
    //doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    PyObject *pcDocObj;

    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &pcDocObj)) {
        Base::Console().Error("Error: DrawViewClipPy::addView - Bad Arg - not DocumentObject\n");
        return NULL;
        //TODO: sb PyErr??
        //PyErr_SetString(PyExc_TypeError,"addView expects a DrawView");
        //return -1;
    }

    DrawViewClip* clip = getDrawViewClipPtr();                         //get DrawViewClip for pyClip
    //TODO: argument 1 arrives as "DocumentObjectPy", not "DrawViewPy"
    //how to validate that obj is DrawView before use??
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    clip->addView(view);
    Py_Return;
}

PyObject* DrawViewClipPy::removeView(PyObject* args)
{
    //this implements iRC = pyClip.removeView(pyView)  -or-
    //doCommand(Doc,"App.activeDocument().%s.removeView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    PyObject *pcDocObj;

    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &pcDocObj)) {
        Base::Console().Error("Error: DrawViewClipPy::removeView - Bad Arg - not DocumentObject\n");
        return NULL;
        //TODO: sb PyErr??
        //PyErr_SetString(PyExc_TypeError,"removeView expects a DrawView");
        //return -1;
    }

    DrawViewClip* clip = getDrawViewClipPtr();                         //get DrawViewClip for pyClip
    //TODO: argument 1 arrives as "DocumentObjectPy", not "DrawViewPy"
    //how to validate that obj is DrawView before use??
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    clip->removeView(view);
    Py_Return;
}

//    std::vector<std::string> getChildViewNames();
PyObject* DrawViewClipPy::getChildViewNames(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    DrawViewClip* clip = getDrawViewClipPtr();
    std::vector<std::string> strings = clip->getChildViewNames();
    int stringSize = strings.size();

    PyObject* result = PyList_New(stringSize);

    std::vector<std::string>::iterator it = strings.begin();
    for( ; it != strings.end(); it++) {
#if PY_MAJOR_VERSION < 3
        PyObject* pString = PyString_FromString(it->c_str());           //TODO: unicode & py3
#else
        PyObject* pString = PyUnicode_FromString(it->c_str());
#endif
        //int rc =
        static_cast<void> (PyList_Append(result, pString));
    }

//    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return result;
}

PyObject *DrawViewClipPy::getCustomAttributes(const char* ) const
{
    return 0;
}

int DrawViewClipPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
