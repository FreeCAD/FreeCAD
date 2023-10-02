/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "DrawPage.h"
#include "DrawView.h"
#include "DrawViewPart.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
// inclusion of the generated files
#include <Mod/TechDraw/App/DrawPagePy.h>
#include <Mod/TechDraw/App/DrawPagePy.cpp>
#include <Mod/TechDraw/App/DrawProjGroupItemPy.h>
#include <Mod/TechDraw/App/DrawViewAnnotationPy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.h>
#include <Mod/TechDraw/App/DrawViewPy.h>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawPagePy::representation() const
{
    return std::string("<DrawPage object>");
}

PyObject* DrawPagePy::addView(PyObject* args)
{
    PyObject *pcDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcDocObj)) {
        return nullptr;
    }

    DrawPage* page = getDrawPagePtr();                         //get DrawPage for pyPage
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView
    int rc = page->addView(view);

    return PyLong_FromLong(rc);
}

PyObject* DrawPagePy::removeView(PyObject* args)
{
    PyObject *pcDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcDocObj)) {
        return nullptr;
    }

    DrawPage* page = getDrawPagePtr();                         //get DrawPage for pyPage
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView
    int rc = page->removeView(view);

    return PyLong_FromLong(rc);
}

PyObject* DrawPagePy::getAllViews(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawPage* page = getDrawPagePtr();
    std::vector<App::DocumentObject*> allViews = page->getAllViews();

    Py::List ret;
    for (auto v: allViews) {
        if (v->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            TechDraw::DrawProjGroupItem* dpgi = static_cast<TechDraw::DrawProjGroupItem*>(v);
            ret.append(Py::asObject(new TechDraw::DrawProjGroupItemPy(dpgi)));
        }
        else if (v->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(v);
            ret.append(Py::asObject(new TechDraw::DrawViewPartPy(dvp)));
        }
        else if (v->isDerivedFrom(TechDraw::DrawViewAnnotation::getClassTypeId())) {
            TechDraw::DrawViewAnnotation* dva = static_cast<TechDraw::DrawViewAnnotation*>(v);
            ret.append(Py::asObject(new TechDraw::DrawViewAnnotationPy(dva)));
        }
        else {
            TechDraw::DrawView* dv = static_cast<TechDraw::DrawView*>(v);
            ret.append(Py::asObject(new TechDraw::DrawViewPy(dv)));
        }
    }

    return Py::new_reference_to(ret);
}

PyObject* DrawPagePy::requestPaint(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawPage* page = getDrawPagePtr();
    page->requestPaint();

    Py_Return;
}

//! replace the current Label with a translated version
PyObject* DrawPagePy::translateLabel(PyObject *args)
{
    PyObject* pyContext;
    PyObject* pyBaseName;
    PyObject* pyUniqueName;
    std::string context;
    std::string baseName;
    std::string uniqueName;

    if (!PyArg_ParseTuple(args, "OOO", &pyContext, &pyBaseName, &pyUniqueName)) {
            throw Py::TypeError("Could not translate label - bad parameters.");
    }

    Py_ssize_t size = 0;
    const char* cContext = PyUnicode_AsUTF8AndSize(pyContext, &size);
    if (cContext) {
        context = std::string(cContext, size);
    } else {
        throw Py::TypeError("Could not translate label - context not available.");
    }

    const char* cBaseName = PyUnicode_AsUTF8AndSize(pyBaseName, &size);
    if (cBaseName) {
        baseName = std::string(cBaseName, size);
    } else {
        throw Py::TypeError("Could not translate label - base name not available.");
    }

    const char* cUniqueName = PyUnicode_AsUTF8AndSize(pyUniqueName, &size);
    if (cUniqueName) {
        uniqueName = std::string(cUniqueName, size);
    } else {
        throw Py::TypeError("Could not translate label - unique name not available.");
    }

    // we have the 3 parameters we need for DrawPage::translateLabel
    DrawPage* page = getDrawPagePtr();
    page->translateLabel(context, baseName, uniqueName);

    Py_Return;
}

Py::Float DrawPagePy::getPageWidth() const
{
    return Py::Float(getDrawPagePtr()->getPageWidth());
}

Py::Float DrawPagePy::getPageHeight() const
{
    return Py::Float(getDrawPagePtr()->getPageHeight());
}

Py::String DrawPagePy::getPageOrientation() const
{
    return Py::String(getDrawPagePtr()->getPageOrientation());
}

PyObject *DrawPagePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int DrawPagePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
