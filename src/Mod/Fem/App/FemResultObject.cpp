/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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

#include "FemResultObject.h"
#include <App/DocumentObjectPy.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemResultObject, App::DocumentObject)


FemResultObject::FemResultObject()
{
    ADD_PROPERTY_TYPE(DataType,(""), "General",Prop_None,"Type identifier of the result data");
    ADD_PROPERTY_TYPE(Unit,(Base::Quantity()), "General",Prop_None,"Unit of the data");
    ADD_PROPERTY_TYPE(ElementNumbers,(0), "Data",Prop_None,"Numbers of the result elements");
}

FemResultObject::~FemResultObject()
{
}

short FemResultObject::mustExecute(void) const
{
    return 0;
}

PyObject *FemResultObject::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

// Python feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Fem::FemResultPython, Fem::FemResultObject)
template<> const char* Fem::FemResultPython::getViewProviderName(void) const {
    return "FemGui::ViewProviderFemResultPython";
}
/// @endcond

// explicit template instantiation
template class AppFemExport FeaturePythonT<Fem::FemResultObject>;

}