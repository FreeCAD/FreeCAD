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

#include "DrawViewClip.h"
#include "DrawView.h"
// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewClipPy.h>
#include <Mod/TechDraw/App/DrawViewClipPy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewClipPy::representation() const
{
    return std::string("<DrawViewClip object>");
}

PyObject* DrawViewClipPy::addView(PyObject* args)
{
    PyObject *pcDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcDocObj)) {
        return nullptr;
    }

    DrawViewClip* clip = getDrawViewClipPtr();                         //get DrawViewClip for pyClip
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView
    clip->addView(view);

    Py_Return;
}

PyObject* DrawViewClipPy::removeView(PyObject* args)
{
    PyObject *pcDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcDocObj)) {
        return nullptr;
    }

    DrawViewClip* clip = getDrawViewClipPtr();                         //get DrawViewClip for pyClip
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView
    clip->removeView(view);

    Py_Return;
}

//    std::vector<std::string> getChildViewNames();
PyObject* DrawViewClipPy::getChildViewNames(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    DrawViewClip* clip = getDrawViewClipPtr();
    std::vector<std::string> strings = clip->getChildViewNames();
    Py::List result;

    for (const auto& str: strings) {
        result.append(Py::String(str));
    }

    return Py::new_reference_to(result);
}

PyObject *DrawViewClipPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int DrawViewClipPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
