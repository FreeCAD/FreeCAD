/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Base/Interpreter.h>
#include <Base/Console.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/WidgetFactory.h>
#include <Gui/Language/Translator.h>

#include <Mod/Mesh/App/MeshProperties.h>

#include "images.h"
#include "DlgEvaluateMeshImp.h"
#include "PropertyEditorMesh.h"
#include "DlgSettingsMeshView.h"
#include "SoFCMeshObject.h"
#include "SoFCIndexedFaceSet.h"
#include "SoPolygon.h"
#include "ViewProvider.h"
#include "ViewProviderMeshFaceSet.h"
#include "ViewProviderCurvature.h"
#include "ViewProviderTransform.h"
#include "ViewProviderTransformDemolding.h"
#include "ViewProviderDefects.h"
#include "ViewProviderPython.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateMeshCommands(void);

void loadMeshResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Mesh);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
static struct PyMethodDef MeshGui_methods[] = {
    {NULL, NULL}                   /* end of table marker */
};

/* Python entry */
extern "C" {
void MeshGuiExport initMeshGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    // load dependent module
    try {
        Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    (void) Py_InitModule("MeshGui", MeshGui_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Mesh module... done\n");

    // Register icons
    Gui::BitmapFactory().addXPM("mesh_fillhole", mesh_fillhole);
    Gui::BitmapFactory().addXPM("mesh_pipette", mesh_pipette);

    // instantiating the commands
    CreateMeshCommands();
    (void)new MeshGui::CleanupHandler;

    // register preferences pages
    (void)new Gui::PrefPageProducer<MeshGui::DlgSettingsMeshView> ("Display");

    MeshGui::SoFCMeshObjectElement              ::initClass();
    MeshGui::SoSFMeshObject                     ::initClass();
    MeshGui::SoFCMeshObjectNode                 ::initClass();
    MeshGui::SoFCMeshObjectShape                ::initClass();
    MeshGui::SoFCMeshSegmentShape               ::initClass();
    MeshGui::SoFCMeshObjectBoundary             ::initClass();
    MeshGui::SoFCIndexedFaceSet                 ::initClass();
    MeshGui::SoFCMeshPickNode                   ::initClass();
    MeshGui::SoFCMeshGridNode                   ::initClass();
    MeshGui::SoPolygon                          ::initClass();
    MeshGui::PropertyMeshKernelItem             ::init();
    MeshGui::ViewProviderMesh                   ::init();
    MeshGui::ViewProviderMeshObject             ::init();
    MeshGui::ViewProviderIndexedFaceSet         ::init();
    MeshGui::ViewProviderMeshFaceSet            ::init();
    MeshGui::ViewProviderPython                 ::init();
    MeshGui::ViewProviderExport                 ::init();
    MeshGui::ViewProviderMeshCurvature          ::init();
    MeshGui::ViewProviderMeshTransform          ::init();
    MeshGui::ViewProviderMeshTransformDemolding ::init();
    MeshGui::ViewProviderMeshDefects            ::init();
    MeshGui::ViewProviderMeshOrientation        ::init();
    MeshGui::ViewProviderMeshNonManifolds       ::init();
    MeshGui::ViewProviderMeshNonManifoldPoints  ::init();
    MeshGui::ViewProviderMeshDuplicatedFaces    ::init();
    MeshGui::ViewProviderMeshDuplicatedPoints   ::init();
    MeshGui::ViewProviderMeshDegenerations      ::init();
    MeshGui::ViewProviderMeshIndices            ::init();
    MeshGui::ViewProviderMeshSelfIntersections  ::init();
    MeshGui::ViewProviderMeshFolds              ::init();
    MeshGui::Workbench                          ::init();
    Gui::ViewProviderBuilder::add(
        Mesh::PropertyMeshKernel::getClassTypeId(),
        MeshGui::ViewProviderMeshFaceSet::getClassTypeId());

    // add resources and reloads the translators
    loadMeshResource();
}

} // extern "C" {
