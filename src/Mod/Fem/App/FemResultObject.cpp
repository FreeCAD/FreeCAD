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
#include <App/FeaturePythonPyImp.h>
#include <App/DocumentObjectPy.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemResultObject, App::DocumentObject)


FemResultObject::FemResultObject()
{
    ADD_PROPERTY_TYPE(Mesh,(0), "General",Prop_None,"Link to the corresponding mesh");
    ADD_PROPERTY_TYPE(NodeNumbers,(0), "Data",Prop_None,"Numbers of the result nodes");
    ADD_PROPERTY_TYPE(Stats,(0), "Fem",Prop_None,"Statistics of the results");
    ADD_PROPERTY_TYPE(Time,(0), "Fem",Prop_None,"Time of analysis incement");

    /*
    ADD_PROPERTY_TYPE(DisplacementVectors,(), "Fem",Prop_None,"List of displacement vectors");
    ADD_PROPERTY_TYPE(DisplacementLengths,(0), "Fem",Prop_None,"List of displacement lengths");
    ADD_PROPERTY_TYPE(StressVectors,(), "Fem",Prop_None,"List of Stress vectors");
    ADD_PROPERTY_TYPE(StrainVectors,(), "Fem",Prop_None,"List of Strain vectors");
    ADD_PROPERTY_TYPE(StressValues,(0), "Fem",Prop_None,"List of Von Misses stress values");
    ADD_PROPERTY_TYPE(PrincipalMax,(0), "Fem",Prop_None,"List of First Principal (Max) stress values");
    ADD_PROPERTY_TYPE(PrincipalMed,(0), "Fem",Prop_None,"List of Second Principal (Med) stress values");
    ADD_PROPERTY_TYPE(PrincipalMin,(0), "Fem",Prop_None,"List of Third Principal (Min) stress values");
    ADD_PROPERTY_TYPE(MaxShear,(0), "Fem",Prop_None,"List of Maximum Shear stress values");
    ADD_PROPERTY_TYPE(Temperature,(0), "Fem",Prop_None,"Nodal temperatures");
    ADD_PROPERTY_TYPE(MassFlowRate,(0), "Fem",Prop_None,"Nodal network mass flow rate");
    ADD_PROPERTY_TYPE(NetworkPressure,(0), "Fem",Prop_None,"Nodal network pressure");
    ADD_PROPERTY_TYPE(Eigenmode,(0), "Fem",Prop_None,"Number of the eigenmode");
    ADD_PROPERTY_TYPE(EigenmodeFrequency,(0), "Fem",Prop_None,"Frequency of the eigenmode");
    ADD_PROPERTY_TYPE(UserDefined,(0), "Fem",Prop_None,"User Defined Results");
    */

    // make read-only for property editor
    NodeNumbers.setStatus(App::Property::ReadOnly, true);
    Stats.setStatus(App::Property::ReadOnly, true);
    Time.setStatus(App::Property::ReadOnly, true);
    /*
    DisplacementVectors.setStatus(App::Property::ReadOnly, true);
    DisplacementLengths.setStatus(App::Property::ReadOnly, true);
    StressVectors.setStatus(App::Property::ReadOnly, true);
    StrainVectors.setStatus(App::Property::ReadOnly, true);
    StressValues.setStatus(App::Property::ReadOnly, true);
    PrincipalMax.setStatus(App::Property::ReadOnly, true);
    PrincipalMed.setStatus(App::Property::ReadOnly, true);
    PrincipalMin.setStatus(App::Property::ReadOnly, true);
    MaxShear.setStatus(App::Property::ReadOnly, true);
    Temperature.setStatus(App::Property::ReadOnly, true);
    MassFlowRate.setStatus(App::Property::ReadOnly, true);
    NetworkPressure.setStatus(App::Property::ReadOnly, true);
    Eigenmode.setStatus(App::Property::ReadOnly, true);
    EigenmodeFrequency.setStatus(App::Property::ReadOnly, true);
    UserDefined.setStatus(App::Property::ReadOnly, false);
    */
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
PROPERTY_SOURCE_TEMPLATE(Fem::FemResultObjectPython, Fem::FemResultObject)
template<> const char* Fem::FemResultObjectPython::getViewProviderName(void) const {
    return "FemGui::ViewProviderResultPython";
}
/// @endcond

template<> PyObject* Fem::FemResultObjectPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new App::FeaturePythonPyT<App::DocumentObjectPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// explicit template instantiation
template class AppFemExport FeaturePythonT<Fem::FemResultObject>;

}
