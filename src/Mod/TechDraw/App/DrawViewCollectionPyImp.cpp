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
PyObject* DrawViewCollectionPy::addView(PyObject* args)
{
    PyObject *pcDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcDocObj)) {
        return nullptr;
    }

    DrawViewCollection* collect = getDrawViewCollectionPtr();
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    int i = collect->addView(view);

    return PyLong_FromLong(i);
}

PyObject* DrawViewCollectionPy::removeView(PyObject* args)
{
    PyObject *pcDocObj;
    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::DrawViewPy::Type), &pcDocObj)) {
        return nullptr;
    }

    DrawViewCollection* collect = getDrawViewCollectionPtr();
    DrawViewPy* pyView = static_cast<TechDraw::DrawViewPy*>(pcDocObj);
    DrawView* view = pyView->getDrawViewPtr();                 //get DrawView for pyView

    int i = collect->removeView(view);

    return PyLong_FromLong(i);
}


PyObject *DrawViewCollectionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawViewCollectionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
