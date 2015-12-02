/***************************************************************************
 *   Copyright (c) 2013 J¨¹rgen Riegel (FreeCAD@juergen-riegel.net)        *
 *   Copyright (c) 2015 Qingfeng Xia (FreeCAD@iesensor.com)                *
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

#include "FemSolverObject.h"

#include <Base/FileInfo.h>
#include <App/FeaturePythonPyImp.h>
#include <App/DocumentObjectPy.h>


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemSolverObject, App::DocumentObject)


FemSolverObject::FemSolverObject()
{

    /*
    ADD_PROPERTY_TYPE(SolverName,("Calculix"), "Data",Prop_None,"Solver program name");
    ADD_PROPERTY_TYPE(Category,("FEM"), "Data",Prop_None,"FEM, CFD ...");
    ADD_PROPERTY_TYPE(Module,(""), "Data",Prop_None,"Python module name");
    ADD_PROPERTY_TYPE(ExternalCaseEditor,(""), "Data",Prop_None,"External case editor programe");
    ADD_PROPERTY_TYPE(ExternalResultViewer,(""), "Data",Prop_None,"External result viewer name");

    ADD_PROPERTY_TYPE(AnalysisType,("Static"), "Solver",Prop_None,"Specific analysis type");
    ADD_PROPERTY_TYPE(WorkingDir,(Base::FileInfo::getTempPath()), "Solver",Prop_None,"Solver working directory");
    ADD_PROPERTY_TYPE(InputCaseName,("TestCase"), "Solver",Prop_None,"Solver input file without suffix");
    ADD_PROPERTY_TYPE(Parallel,(false), "Solver",Prop_None,"Run solver in parallel like MPI");
    ADD_PROPERTY_TYPE(ResultObtained,(false), "Solver",Prop_None,"if true, result has been obtained");
    */
}

FemSolverObject::~FemSolverObject()
{
}

short FemSolverObject::mustExecute(void) const
{
    return 0;
}

PyObject *FemSolverObject::getPyObject()
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
PROPERTY_SOURCE_TEMPLATE(Fem::FemSolverObjectPython, Fem::FemSolverObject)
template<> const char* Fem::FemSolverObjectPython::getViewProviderName(void) const {
    return "FemGui::ViewProviderSolverPython";
}

template<> PyObject* Fem::FemSolverObjectPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new App::FeaturePythonPyT<App::DocumentObjectPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// explicit template instantiation
template class AppFemExport FeaturePythonT<Fem::FemSolverObject>;

}
