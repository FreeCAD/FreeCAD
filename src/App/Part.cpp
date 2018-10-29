/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2014    *
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

#include <App/Document.h>

#include "Part.h"
#include "PartPy.h"


using namespace App;


PROPERTY_SOURCE_WITH_EXTENSIONS(App::Part, App::GeoFeature)


//===========================================================================
// Part
//===========================================================================


Part::Part(void)
{
    ADD_PROPERTY(Type,(""));
    ADD_PROPERTY_TYPE(Material, (), 0, App::Prop_None, "Map with material properties");
    ADD_PROPERTY_TYPE(Meta, (), 0, App::Prop_None, "Map with additional meta information");

    // create the uuid for the document
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Id, (""), 0, App::Prop_None, "ID (Part-Number) of the Item");
    ADD_PROPERTY_TYPE(Uid, (id), 0, App::Prop_None, "UUID of the Item");

    // license stuff (leave them empty to avoid confusion with imported 3rd party STEP/IGES files)
    ADD_PROPERTY_TYPE(License, (""), 0, App::Prop_None, "License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL, (""), 0, App::Prop_None, "URL to the license text/contract");
    // color and appearance
    ADD_PROPERTY(Color, (1.0, 1.0, 1.0, 1.0)); // set transparent -> not used

    GroupExtension::initExtension(this);
}

Part::~Part(void)
{
}

App::Part *Part::getPartOfObject (const DocumentObject* obj) {
    
    //as a Part is a geofeaturegroup it must directly link to all objects it contains, even 
    //if they are in additional groups etc.
    auto list = obj->getInList();
    for (auto obj : list) {
        if(obj->isDerivedFrom(App::Part::getClassTypeId()))
            return static_cast<App::Part*>(obj);
    }

    return nullptr;
}


PyObject *Part::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PartPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python feature ---------------------------------------------------------

// Not quite sure yet making Part derivable in Python is good Idea!
// JR 2014

//namespace App {
///// @cond DOXERR
//PROPERTY_SOURCE_TEMPLATE(App::PartPython, App::Part)
//template<> const char* App::PartPython::getViewProviderName(void) const {
//    return "Gui::ViewProviderPartPython";
//}
//template<> PyObject* App::PartPython::getPyObject(void) {
//    if (PythonObject.is(Py::_None())) {
//        // ref counter is set to 1
//        PythonObject = Py::Object(new FeaturePythonPyT<App::PartPy>(this),true);
//    }
//    return Py::new_reference_to(PythonObject);
//}
///// @endcond
//
//// explicit template instantiation
//template class AppExport FeaturePythonT<App::Part>;
//}
