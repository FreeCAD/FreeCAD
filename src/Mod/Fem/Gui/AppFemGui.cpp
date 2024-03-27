/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/WidgetFactory.h>
#include <Gui/Language/Translator.h>

#include "DlgSettingsFemCcxImp.h"
#include "DlgSettingsFemElmerImp.h"
#include "DlgSettingsFemExportAbaqusImp.h"
#include "DlgSettingsFemGeneralImp.h"
#include "DlgSettingsFemGmshImp.h"
#include "DlgSettingsFemInOutVtkImp.h"
#include "DlgSettingsFemMystranImp.h"
#include "DlgSettingsFemZ88Imp.h"
#include "PropertyFemMeshItem.h"
#include "ViewProviderAnalysis.h"
#include "ViewProviderFemMesh.h"
#include "ViewProviderFemMeshShape.h"
#include "ViewProviderFemMeshShapeNetgen.h"
#include "ViewProviderSetElements.h"
#include "ViewProviderSetFaces.h"
#include "ViewProviderSetGeometry.h"
#include "ViewProviderSetNodes.h"
#include "ViewProviderSolver.h"
#include "ViewProviderFemConstraint.h"
#include "ViewProviderFemConstraintBearing.h"
#include "ViewProviderFemConstraintContact.h"
#include "ViewProviderFemConstraintDisplacement.h"
#include "ViewProviderFemConstraintFixed.h"
#include "ViewProviderFemConstraintRigidBody.h"
#include "ViewProviderFemConstraintForce.h"
#include "ViewProviderFemConstraintFluidBoundary.h"
#include "ViewProviderFemConstraintGear.h"
#include "ViewProviderFemConstraintHeatflux.h"
#include "ViewProviderFemConstraintInitialTemperature.h"
#include "ViewProviderFemConstraintOnBoundary.h"
#include "ViewProviderFemConstraintPlaneRotation.h"
#include "ViewProviderFemConstraintPressure.h"
#include "ViewProviderFemConstraintPulley.h"
#include "ViewProviderFemConstraintSpring.h"
#include "ViewProviderFemConstraintTemperature.h"
#include "ViewProviderFemConstraintTransform.h"
#include "ViewProviderResult.h"
#include "Workbench.h"

#ifdef FC_USE_VTK
#include "ViewProviderFemPostFilter.h"
#include "ViewProviderFemPostFunction.h"
#include "ViewProviderFemPostObject.h"
#include "ViewProviderFemPostPipeline.h"
#endif


// use a different name to CreateCommand()
void CreateFemCommands();

void loadFemResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Fem);
    Q_INIT_RESOURCE(Fem_translation);
    Gui::Translator::instance()->refresh();
}

namespace FemGui
{
extern PyObject* initModule();
}


/* Python entry */
PyMOD_INIT_FUNC(FemGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    PyObject* mod = FemGui::initModule();
    Base::Console().Log("Loading GUI of Fem module... done\n");

    // instantiating the commands
    CreateFemCommands();

    // clang-format off
    // addition objects
    FemGui::Workbench                                           ::init();

    FemGui::ViewProviderFemAnalysis                             ::init();
    FemGui::ViewProviderFemAnalysisPython                       ::init();

    FemGui::ViewProviderFemConstraint                           ::init();
    FemGui::ViewProviderFemConstraintPython                     ::init();

    FemGui::ViewProviderFemConstraintOnBoundary                 ::init();
    FemGui::ViewProviderFemConstraintBearing                    ::init();
    FemGui::ViewProviderFemConstraintContact                    ::init();
    FemGui::ViewProviderFemConstraintDisplacement               ::init();
    FemGui::ViewProviderFemConstraintFixed                      ::init();
    FemGui::ViewProviderFemConstraintRigidBody                  ::init();
    FemGui::ViewProviderFemConstraintFluidBoundary              ::init();
    FemGui::ViewProviderFemConstraintForce                      ::init();
    FemGui::ViewProviderFemConstraintGear                       ::init();
    FemGui::ViewProviderFemConstraintHeatflux                   ::init();
    FemGui::ViewProviderFemConstraintInitialTemperature         ::init();
    FemGui::ViewProviderFemConstraintPlaneRotation              ::init();
    FemGui::ViewProviderFemConstraintPressure                   ::init();
    FemGui::ViewProviderFemConstraintPulley                     ::init();
    FemGui::ViewProviderFemConstraintTemperature                ::init();
    FemGui::ViewProviderFemConstraintTransform                  ::init();
    FemGui::ViewProviderFemConstraintSpring                     ::init();

    FemGui::ViewProviderFemMesh                                 ::init();
    FemGui::ViewProviderFemMeshPython                           ::init();
    FemGui::ViewProviderFemMeshShape                            ::init();
    FemGui::ViewProviderFemMeshShapeNetgen                      ::init();
    FemGui::PropertyFemMeshItem                                 ::init();

    FemGui::ViewProviderSetElements                             ::init();
    FemGui::ViewProviderSetFaces                                ::init();
    FemGui::ViewProviderSetGeometry                             ::init();
    FemGui::ViewProviderSetNodes                                ::init();

    FemGui::ViewProviderSolver                                  ::init();
    FemGui::ViewProviderSolverPython                            ::init();

    FemGui::ViewProviderResult                                  ::init();
    FemGui::ViewProviderResultPython                            ::init();


#ifdef FC_USE_VTK
    FemGui::ViewProviderFemPostObject                           ::init();
    FemGui::ViewProviderFemPostPipeline                         ::init();
    FemGui::ViewProviderFemPostClip                             ::init();
    FemGui::ViewProviderFemPostContours                         ::init();
    FemGui::ViewProviderFemPostCut                              ::init();
    FemGui::ViewProviderFemPostDataAlongLine                    ::init();
    FemGui::ViewProviderFemPostDataAtPoint                      ::init();
    FemGui::ViewProviderFemPostScalarClip                       ::init();
    FemGui::ViewProviderFemPostWarpVector                       ::init();

    FemGui::ViewProviderFemPostFunction                         ::init();
    FemGui::ViewProviderFemPostFunctionProvider                 ::init();
    FemGui::ViewProviderFemPostBoxFunction                      ::init();
    FemGui::ViewProviderFemPostCylinderFunction                 ::init();
    FemGui::ViewProviderFemPostPlaneFunction                    ::init();
    FemGui::ViewProviderFemPostSphereFunction                   ::init();
#endif


    // register preferences pages on FEM, the order here will be the order of the tabs in pref widget
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemGeneralImp>(QT_TRANSLATE_NOOP("QObject", "FEM"));
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemGmshImp>(QT_TRANSLATE_NOOP("QObject", "FEM"));
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemCcxImp>(QT_TRANSLATE_NOOP("QObject", "FEM"));
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemElmerImp>(QT_TRANSLATE_NOOP("QObject", "FEM"));
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemMystranImp>(QT_TRANSLATE_NOOP("QObject", "FEM"));
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemZ88Imp>(QT_TRANSLATE_NOOP("QObject", "FEM"));

    // register preferences pages on Import-Export
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemExportAbaqusImp>(QT_TRANSLATE_NOOP("QObject", "Import-Export"));
    new Gui::PrefPageProducer<FemGui::DlgSettingsFemInOutVtkImp>(QT_TRANSLATE_NOOP("QObject", "Import-Export"));

    // add resources and reloads the translators
    loadFemResource();
    // clang-format on

    PyMOD_Return(mod);
}
