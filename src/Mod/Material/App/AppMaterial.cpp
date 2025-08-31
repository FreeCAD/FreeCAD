/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include <App/CleanupProcess.h>

#include "MaterialLoader.h"
#include "MaterialManagerLocal.h"
#include "ModelManagerLocal.h"
#include "PropertyMaterial.h"
#if defined(BUILD_MATERIAL_EXTERNAL)
#include "ModelManagerExternal.h"
#include "MaterialManagerExternal.h"
#endif

#include "Array2DPy.h"
#include "Array3DPy.h"
#include "ModelManagerPy.h"
#include "ModelPropertyPy.h"
#include "ModelPy.h"
#include "UUIDsPy.h"

#include "MaterialFilterPy.h"
#include "MaterialFilterOptionsPy.h"
#include "MaterialLibraryPy.h"
#include "MaterialManagerPy.h"
#include "MaterialPropertyPy.h"
#include "MaterialPy.h"

namespace Materials
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Materials")
    {
        initialize("This module is the Materials module.");  // register with Python
    }

    ~Module() override = default;

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace Materials

PyMOD_INIT_FUNC(Materials)
{
#ifdef FC_DEBUG
    App::CleanupProcess::registerCleanup([]() {
        Materials::MaterialManager::cleanup();
        Materials::ModelManager::cleanup();
    });
#endif
    PyObject* module = Materials::initModule();

    Base::Console().log("Loading Material module… done\n");

    Base::Interpreter().addType(&Materials::Array2DPy::Type, module, "Array2D");
    Base::Interpreter().addType(&Materials::Array3DPy::Type, module, "Array3D");
    Base::Interpreter().addType(&Materials::MaterialFilterPy::Type, module, "MaterialFilter");
    Base::Interpreter().addType(&Materials::MaterialFilterOptionsPy::Type, module, "MaterialFilterOptions");
    Base::Interpreter().addType(&Materials::MaterialLibraryPy::Type, module, "MaterialLibrary");
    Base::Interpreter().addType(&Materials::MaterialManagerPy::Type, module, "MaterialManager");
    Base::Interpreter().addType(&Materials::MaterialPropertyPy::Type, module, "MaterialProperty");
    Base::Interpreter().addType(&Materials::MaterialPy::Type, module, "Material");
    Base::Interpreter().addType(&Materials::ModelManagerPy::Type, module, "ModelManager");
    Base::Interpreter().addType(&Materials::ModelPropertyPy::Type, module, "ModelProperty");
    Base::Interpreter().addType(&Materials::ModelPy::Type, module, "Model");
    Base::Interpreter().addType(&Materials::UUIDsPy::Type, module, "UUIDs");


    // clang-format off
    // Initialize types

    Materials::Material                 ::init();
    Materials::MaterialFilter           ::init();
    Materials::MaterialFilterOptions    ::init();
    Materials::MaterialManager          ::init();
    Materials::MaterialManagerLocal     ::init();
    Materials::Model                    ::init();
    Materials::ModelManager             ::init();
#if defined(BUILD_MATERIAL_EXTERNAL)
    Materials::MaterialManagerExternal  ::init();
    Materials::ModelManagerExternal     ::init();
#endif
    Materials::ModelManagerLocal        ::init();
    Materials::ModelUUIDs               ::init();

    Materials::Library                  ::init();
    Materials::MaterialLibrary          ::init();
    Materials::MaterialLibraryLocal     ::init();
    Materials::ModelLibrary             ::init();

    Materials::ModelProperty            ::init();
    Materials::MaterialProperty         ::init();

    Materials::MaterialValue            ::init();
    Materials::Array2D                  ::init();
    Materials::Array3D                  ::init();

    Materials::PropertyMaterial         ::init();
    // clang-format on

    PyMOD_Return(module);
}
