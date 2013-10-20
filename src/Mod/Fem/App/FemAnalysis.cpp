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

#include "FemAnalysis.h"
#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>
#include <Base/Uuid.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemAnalysis, App::DocumentObject)


FemAnalysis::FemAnalysis()
{
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Member,(0), "Analysis member",Prop_None,"All objects belonging to the Analysis");
    ADD_PROPERTY_TYPE(Uid,(id),0,App::Prop_None,"UUID of the Analysis");
}

FemAnalysis::~FemAnalysis()
{
}

short FemAnalysis::mustExecute(void) const
{
    return 0;
}

PyObject *FemAnalysis::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

void FemAnalysis::onChanged(const Property* prop)
{
    App::DocumentObject::onChanged(prop);
}



// Python feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Fem::FemAnalysisPython, Fem::FemAnalysis)
template<> const char* Fem::FemAnalysisPython::getViewProviderName(void) const {
    return "FemGui::ViewProviderFemAnalysisPython";
}
//template<> PyObject* Fem::FemAnalysisPython::getPyObject(void) {
//    if (PythonObject.is(Py::_None())) {
//        // ref counter is set to 1
//        PythonObject = Py::Object(new App::DocumentObjectPy(this),true);
//    }
//    return Py::new_reference_to(PythonObject);
//}
/// @endcond

// explicit template instantiation
template class AppFemExport FeaturePythonT<Fem::FemAnalysis>;
}