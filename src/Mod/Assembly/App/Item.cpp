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
    ADD_PROPERTY_TYPE(CreatedBy,(""),0,App::Prop_None,"The creator of the Item");
    ADD_PROPERTY_TYPE(CreationDate,(Base::TimeInfo::currentDateTimeString()),0,App::Prop_ReadOnly,"Date of creation");
    ADD_PROPERTY_TYPE(LastModifiedBy,(""),0,App::Prop_None,0);
    ADD_PROPERTY_TYPE(LastModifiedDate,("Unknown"),0,App::Prop_ReadOnly,"Date of last modification");
    ADD_PROPERTY_TYPE(Company,(""),0,App::Prop_None,"Additional tag to save the the name of the company");
    ADD_PROPERTY_TYPE(Comment,(""),0,App::Prop_None,"Additional tag to save a comment");
    ADD_PROPERTY_TYPE(Meta,(),0,App::Prop_None,"Map with additional meta information");
    ADD_PROPERTY_TYPE(Material,(),0,App::Prop_None,"Map with material properties");
    // create the uuid for the document
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Id,(""),0,App::Prop_None,"ID (Part-Number) of the Item");
    ADD_PROPERTY_TYPE(Uid,(id),0,App::Prop_None,"UUID of the Item");

    // license stuff
    ADD_PROPERTY_TYPE(License,("CC BY 3.0"),0,App::Prop_None,"License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL,("http://creativecommons.org/licenses/by/3.0/"),0,App::Prop_None,"URL to the license text/contract");
    // color and apperance
    ADD_PROPERTY(Color,(1.0,1.0,1.0,1.0)); // set transparent -> not used
    ADD_PROPERTY(Visibility,(true));
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