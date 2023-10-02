/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#ifndef _PreComp_
# include <sstream>
#endif

#include "DrawParametricTemplate.h"
// inclusion of the generated files (generated out of DrawParametricTemplateSFPy.xml)
#include <Mod/TechDraw/App/DrawParametricTemplatePy.h>
#include <Mod/TechDraw/App/DrawParametricTemplatePy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawParametricTemplatePy::representation(void) const
{
    return "<TechDraw::DrawParametricTemplate>";
}

PyObject *DrawParametricTemplatePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawParametricTemplatePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    // search in PropertyList
    App::Property *prop = getDrawParametricTemplatePtr()->getPropertyByName(attr);
    if (!prop) {
        return 0;
    }

    // Read-only attributes must not be set over its Python interface
    short Type =  getDrawParametricTemplatePtr()->getPropertyType(prop);
    if (Type & App::Prop_ReadOnly) {
        std::stringstream s;
        s << "Object attribute '" << attr << "' is read-only";
        throw Py::AttributeError(s.str());
    }

    prop->setPyObject(obj);
    return 1;
}

PyObject* DrawParametricTemplatePy::drawLine(PyObject *args)
{
    //PyObject *pcObj;
    double x1, y1;
    double x2, y2;

    if (!PyArg_ParseTuple(args, "dddd", &x1, &y1, &x2, &y2))
        return nullptr;

    getDrawParametricTemplatePtr()->drawLine(x1, y1, x2, y2);

    Py_Return;
}

Py::Long DrawParametricTemplatePy::getGeometryCount(void) const
{
    int size = getDrawParametricTemplatePtr()->getGeometry().size();
    return Py::Long(size);
}
