/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2013 Stefan Tröger  <stefantroeger@gmx.net>             *
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
#endif

#include <Base/Placement.h>
#include <Base/Uuid.h>
#include <Base/Console.h>

#include "Item.h"
#include "ItemPy.h"
#include <Standard_Failure.hxx>

using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE_ABSTRACT(Assembly::Item, App::GeoFeature)

Item::Item()
{
  ADD_PROPERTY_TYPE(Meta, (), 0, App::Prop_None, "Map with additional meta information");

}

short Item::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *Item::execute(void)
{
 
    Base::Console().Message("Recalculate Assembly::Item\n");
    return App::DocumentObject::StdReturn;
}

PyObject *Item::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new ItemPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

}