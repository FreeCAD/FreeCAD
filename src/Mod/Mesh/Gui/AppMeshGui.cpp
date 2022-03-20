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
# include <Inventor/SoDB.h>
# include <Inventor/SoInput.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/annex/ForeignFiles/SoSTLFileKit.h>
#endif

#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <Base/PyObjectBase.h>

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/WidgetFactory.h>
#include <Gui/Language/Translator.h>

#include <Mod/Mesh/App/MeshProperties.h>

#include "images.h"
#include "DlgEvaluateMeshImp.h"
#include "PropertyEditorMesh.h"
#include "DlgSettingsMeshView.h"
#include "DlgSettingsImportExportImp.h"
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
void CreateMeshCommands();

void loadMeshResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Mesh);
    Gui::Translator::instance()->refresh();
}

namespace MeshGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("MeshGui")
    {
        add_varargs_method("convertToSTL",&Module::convertToSTL,
            "Convert a scene into an STL."
        );
        initialize("This module is the MeshGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object convertToSTL(const Py::Tuple& args)
    {
        char* inname;
        char* outname;
        if (!PyArg_ParseTuple(args.ptr(), "etet","utf-8",&inname,"utf-8",&outname))
            throw Py::Exception();
        std::string inputName = std::string(inname);
        PyMem_Free(inname);
        std::string outputName = std::string(outname);
        PyMem_Free(outname);

        bool ok = false;
        SoInput in;
        if (in.openFile(inputName.c_str())) {
            SoSeparator * node = SoDB::readAll(&in);
            if (node) {
                node->ref();
                SoSTLFileKit* stlKit = new SoSTLFileKit();
                stlKit->ref();
                ok = stlKit->readScene(node);
                stlKit->writeFile(outputName.c_str());
                stlKit->unref();
                node->unref();
            }
        }

        return Py::Boolean(ok);
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace MeshGui

/* Python entry */
PyMOD_INIT_FUNC(MeshGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // load dependent module
    try {
        Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = MeshGui::initModule();
    Base::Console().Log("Loading GUI of Mesh module... done\n");

    // Register icons
    Gui::BitmapFactory().addXPM("mesh_fillhole", mesh_fillhole);

    // instantiating the commands
    CreateMeshCommands();
    (void)new MeshGui::CleanupHandler;
    
    // try to instantiate flat-mesh commands
    try{
        Base::Interpreter().runString("import MeshFlatteningCommand");
    } catch (Base::PyException &err){
        err.ReportException();
    }

    // register preferences pages
    (void)new Gui::PrefPageProducer<MeshGui::DlgSettingsMeshView> ("Display");
    (void)new Gui::PrefPageProducer<MeshGui::DlgSettingsImportExport>     ( QT_TRANSLATE_NOOP("QObject", "Import-Export") );

    MeshGui::SoFCMeshObjectElement              ::initClass();
    MeshGui::SoSFMeshObject                     ::initClass();
    MeshGui::SoFCMeshObjectNode                 ::initClass();
    MeshGui::SoFCMeshObjectShape                ::initClass();
    MeshGui::SoFCMeshSegmentShape               ::initClass();
    MeshGui::SoFCMeshObjectBoundary             ::initClass();
    MeshGui::SoFCMaterialEngine                 ::initClass();
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

    PyMOD_Return(mod);
}
