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

#include "DrawTemplate.h"
// inclusion of the generated files (generated out of DrawTemplateSFPy.xml)
#include <Mod/TechDraw/App/DrawTemplatePy.h>
#include <Mod/TechDraw/App/DrawTemplatePy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawTemplatePy::representation() const
{
    return "<TechDraw::DrawTemplate>";
}

PyObject *DrawTemplatePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawTemplatePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    // search in PropertyList
    App::Property *prop = getDrawTemplatePtr()->getPropertyByName(attr);
    if (!prop) {
        return 0;
    }

    // Read-only attributes must not be set over its Python interface
    short Type =  getDrawTemplatePtr()->getPropertyType(prop);
    if (Type & App::Prop_ReadOnly) {
        std::stringstream s;
        s << "Object attribute '" << attr << "' is read-only";
        throw Py::AttributeError(s.str());
    }

    prop->setPyObject(obj);
    return 1;
}
