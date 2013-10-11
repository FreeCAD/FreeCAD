/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <Python.h>
# include <Standard_math.hxx>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include "ViewProviderFemMesh.h"
#include "ViewProviderFemMeshShape.h"
#include "ViewProviderFemMeshShapeNetgen.h"
#include "ViewProviderAnalysis.h"
#include "ViewProviderSetNodes.h"
#include "ViewProviderSetElements.h"
#include "ViewProviderSetFaces.h"
#include "ViewProviderSetGeometry.h"
#include "ViewProviderFemConstraint.h"
#include "ViewProviderFemConstraintBearing.h"
#include "ViewProviderFemConstraintFixed.h"
#include "ViewProviderFemConstraintForce.h"
#include "ViewProviderFemConstraintGear.h"
#include "ViewProviderFemConstraintPulley.h"
#include "Workbench.h"
//#include "resources/qrc_Fem.cpp"

// use a different name to CreateCommand()
void CreateFemCommands(void);

void loadFemResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Fem);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef FemGui_Import_methods[];


/* Python entry */
extern "C" {
void FemGuiExport initFemGui()  
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    (void) Py_InitModule("FemGui", FemGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Fem module... done\n");

    // instantiating the commands
    CreateFemCommands();

    // addition objects
    FemGui::Workbench                          ::init();
    FemGui::ViewProviderFemAnalysis            ::init();
    FemGui::ViewProviderFemAnalysisPython      ::init();
    FemGui::ViewProviderFemMesh                ::init();
    FemGui::ViewProviderFemMeshShape           ::init();
    FemGui::ViewProviderFemMeshShapeNetgen     ::init();
    FemGui::ViewProviderSetNodes               ::init();
    FemGui::ViewProviderSetElements            ::init();
    FemGui::ViewProviderSetFaces               ::init();
    FemGui::ViewProviderSetGeometry            ::init();
    FemGui::ViewProviderFemConstraint          ::init();
    FemGui::ViewProviderFemConstraintBearing   ::init();
    FemGui::ViewProviderFemConstraintFixed     ::init();
    FemGui::ViewProviderFemConstraintForce     ::init();
    FemGui::ViewProviderFemConstraintGear      ::init();
    FemGui::ViewProviderFemConstraintPulley    ::init();

    Base::Interpreter().loadModule("MechanicalAnalysis");
    Base::Interpreter().loadModule("MechanicalMaterial");

    Base::Interpreter().loadModule("FemLib");


     // add resources and reloads the translators
    loadFemResource();
}

} // extern "C" {
