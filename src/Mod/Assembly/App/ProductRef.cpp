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
#include <App/Part.h>

#include "ProductRef.h"
#include "ConstraintGroup.h"
#include "ProductRefPy.h"

using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ProductRef,  Assembly::Item)

ProductRef::ProductRef() {
    ADD_PROPERTY(Item,(0));
}

short ProductRef::mustExecute() const {
    return 0;
}

App::DocumentObjectExecReturn* ProductRef::execute(void) 
{
    return App::DocumentObject::StdReturn;
}

PyObject* ProductRef::getPyObject(void) {
    if(PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new ProductRefPy(this),true);
    }

    return Py::new_reference_to(PythonObject);
}


} //assembly



