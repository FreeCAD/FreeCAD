/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#ifndef Fem_FemSolverObject_H
#define Fem_FemSolverObject_H

#include <App/DocumentObject.h>
#include <App/PropertyUnits.h>
#include <App/PropertyStandard.h>
#include <App/FeaturePython.h>
#include "FemSolverObject.h"

namespace Fem
{
/// Father of all result data in a Fem Analysis
class AppFemExport FemSolverObject : public App::DocumentObject
{
    PROPERTY_HEADER(Fem::FemSolverObject);

public:
    /// Constructor
    FemSolverObject(void);
    virtual ~FemSolverObject();

    /*
    /// Solver name, unique to identify solver in registered_solver dict
    App::PropertyString SolverName;
    /// CAE category like FEM, all capitalised letters
    App::PropertyString Category;
    /// python module name
    App::PropertyString Module;
    /// Path or program name for external case editor, empty string means using FreeCAD to view
    App::PropertyString ExternalCaseEditor;
    /// Path to External Result Viewer like Paraview, empty string means using FreeCAD
    App::PropertyString ExternalResultViewer;
    
    /// for FEM: Static, Frequency, etc
    App::PropertyString AnalysisType;
    /// Path of working dir for the solver
    App::PropertyString WorkingDir;
    /// name for the case file without suffix
    App::PropertyString InputCaseName;
    /// run parallel in MPI (message passing interface)/multiple cores or serial(single CPU)
    App::PropertyBool Parallel;
    /// result has been obtained, purge result may be needed for rerun
    App::PropertyBool ResultObtained;
    */

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "FemGui::ViewProviderSolver";
    }
    virtual App::DocumentObjectExecReturn *execute(void) {
        return App::DocumentObject::StdReturn;
    }
    virtual short mustExecute(void) const;
    virtual PyObject *getPyObject(void);

};

typedef App::FeaturePythonT<FemSolverObject> FemSolverObjectPython;

} //namespace Fem


#endif // Fem_FemSolverObject_H
