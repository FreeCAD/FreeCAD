/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <BRep_Builder.hxx>
# include <TopoDS_Compound.hxx>
#include <boost/exception/get_error_info.hpp>
#endif

#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "Product.h"
#include "ConstraintGroup.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::Product, Assembly::Item)

Product::Product() {
    ADD_PROPERTY(Items,(0));
    ADD_PROPERTY_TYPE(Material,(),0,App::Prop_None,"Map with material properties");
    // create the uuid for the document
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Id,(""),0,App::Prop_None,"ID (Part-Number) of the Item");
    ADD_PROPERTY_TYPE(Uid,(id),0,App::Prop_None,"UUID of the Item");

    // license stuff
    ADD_PROPERTY_TYPE(License,("CC BY 3.0"),0,App::Prop_None,"License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL,("http://creativecommons.org/licenses/by/3.0/"),0,App::Prop_None,"URL to the license text/contract");
    // color and appearance
    ADD_PROPERTY(Color,(1.0,1.0,1.0,1.0)); // set transparent -> not used
    ADD_PROPERTY(Visibility,(true));

}

short Product::mustExecute() const {
    return 0;
}

App::DocumentObjectExecReturn* Product::execute(void) {

    Base::Console().Message("Execute\n");

    return App::DocumentObject::StdReturn;
}


//PyObject* Product::getPyObject(void) {
//    if(PythonObject.is(Py::_None())) {
//        // ref counter is set to 1
//        PythonObject = Py::Object(new ProductPy(this),true);
//    }
//
//    return Py::new_reference_to(PythonObject);
//}



} //assembly



