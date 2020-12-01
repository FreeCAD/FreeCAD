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

#include "PropertyTooltable.h"
#include "TooltablePy.h"

using namespace Path;

TYPESYSTEM_SOURCE(Path::PropertyTooltable, App::Property)

PropertyTooltable::PropertyTooltable()
{
}

PropertyTooltable::~PropertyTooltable()
{
}

void PropertyTooltable::setValue(const Tooltable& tt)
{
    aboutToSetValue();
    _Tooltable = tt;
    hasSetValue();
}


const Tooltable &PropertyTooltable::getValue(void)const
{
    return _Tooltable;
}

PyObject *PropertyTooltable::getPyObject(void)
{
    return new TooltablePy(new Tooltable(_Tooltable));
}

void PropertyTooltable::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(TooltablePy::Type))) {
        TooltablePy *pcObject = static_cast<TooltablePy*>(value);
        setValue(*pcObject->getTooltablePtr());
    }
    else {
        std::string error = std::string("type must be 'Tooltable', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

App::Property *PropertyTooltable::Copy(void) const
{
    PropertyTooltable *prop = new PropertyTooltable();
    prop->_Tooltable = this->_Tooltable;

    return prop;
}

void PropertyTooltable::Paste(const App::Property &from)
{
    aboutToSetValue();
    _Tooltable = dynamic_cast<const PropertyTooltable&>(from)._Tooltable;
    hasSetValue();
}

unsigned int PropertyTooltable::getMemSize (void) const
{
    return _Tooltable.getMemSize();
}

void PropertyTooltable::Save (Base::Writer &writer) const
{
    _Tooltable.Save(writer);
}

void PropertyTooltable::Restore(Base::XMLReader &reader)
{
    Path::Tooltable temp;
    temp.Restore(reader);
    setValue(temp);
}


