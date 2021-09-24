/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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


#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>

#include "PropertyTool.h"
#include "ToolPy.h"

using namespace Path;

TYPESYSTEM_SOURCE(Path::PropertyTool, App::Property)

PropertyTool::PropertyTool()
{
}

PropertyTool::~PropertyTool()
{
}

void PropertyTool::setValue(const Tool& tt)
{
    aboutToSetValue();
    _Tool = tt;
    hasSetValue();
}


const Tool &PropertyTool::getValue(void)const
{
    return _Tool;
}

PyObject *PropertyTool::getPyObject(void)
{
    return new ToolPy(new Tool(_Tool));
}

void PropertyTool::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(ToolPy::Type))) {
        ToolPy *pcObject = static_cast<ToolPy*>(value);
        setValue(*pcObject->getToolPtr());
    }
    else {
        std::string error = std::string("type must be 'Tool', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

App::Property *PropertyTool::Copy(void) const
{
    PropertyTool *prop = new PropertyTool();
    prop->_Tool = this->_Tool;

    return prop;
}

void PropertyTool::Paste(const App::Property &from)
{
    aboutToSetValue();
    _Tool = dynamic_cast<const PropertyTool&>(from)._Tool;
    hasSetValue();
}

unsigned int PropertyTool::getMemSize (void) const
{
    return _Tool.getMemSize();
}

void PropertyTool::Save (Base::Writer &writer) const
{
    _Tool.Save(writer);
}

void PropertyTool::Restore(Base::XMLReader &reader)
{
    Path::Tool temp;
    temp.Restore(reader);
    setValue(temp);
}


