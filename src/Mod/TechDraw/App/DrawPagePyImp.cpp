
#include "PreCompiled.h"

#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "DrawPage.h"
#include "DrawView.h"
#include "DrawViewPart.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.h>
#include <Mod/TechDraw/App/DrawProjGroupItemPy.h>
#include <Mod/TechDraw/App/DrawViewAnnotationPy.h>
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
    return PyLong_FromLong((long) rc);
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

    return PyLong_FromLong((long) rc);
}

PyObject* DrawPagePy::getAllViews(PyObject* args)
{
    (void) args;
    DrawPage* page = getDrawPagePtr();
    std::vector<App::DocumentObject*> allViews = page->getAllViews();

    Py::List ret;
    for (auto&v: allViews) {
        if (v->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(v);
            ret.append(Py::asObject(new TechDraw::DrawProjGroupItemPy(dpgi)));   //is this legit? or need to make new copy of dv?
        } else if (v->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(v);
            ret.append(Py::asObject(new TechDraw::DrawViewPartPy(dvp)));
        } else if (v->isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId())) {
            TechDraw::DrawViewAnnotation* dva = static_cast<TechDraw::DrawViewAnnotation*>(v);
            ret.append(Py::asObject(new TechDraw::DrawViewAnnotationPy(dva)));
        } else {
            TechDraw::DrawView* dv = static_cast<TechDraw::DrawView*>(v);
            ret.append(Py::asObject(new TechDraw::DrawViewPy(dv)));
        }
    }
    return Py::new_reference_to(ret);
}

PyObject* DrawPagePy::requestPaint(PyObject* args)
{
    (void) args;
    DrawPage* page = getDrawPagePtr();
    page->requestPaint();
    Py_INCREF(Py_None);
    return Py_None;
}


//    double getPageWidth() const;
PyObject* DrawPagePy::getPageWidth(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

//    double getPageHeight() const;
PyObject* DrawPagePy::getPageHeight(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

//    const char* getPageOrientation() const;
PyObject* DrawPagePy::getPageOrientation(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject *DrawPagePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int DrawPagePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
