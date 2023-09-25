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
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "DlgSettingsMaterial.h"
#include "Workbench.h"

// use a different name to CreateCommand()
void CreateMaterialCommands();

void loadMaterialResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Material);
    Q_INIT_RESOURCE(Material_translation);
    Gui::Translator::instance()->refresh();
}

namespace MatGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MatGui")
    {
        initialize("This module is the MatGui module.");  // register with Python
    }

    ~Module() override
    {}

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace MatGui

PyMOD_INIT_FUNC(MatGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // load needed modules
    try {
        Base::Interpreter().runString("import Material");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* matGuiModule = MatGui::initModule();

    Base::Console().Log("Loading GUI of Material module... done\n");

    MatGui::Workbench ::init();

    // instantiating the commands
    CreateMaterialCommands();

    // register preferences pages on Material, the order here will be the order of the tabs in pref
    // widget
    new Gui::PrefPageProducer<MatGui::DlgSettingsMaterial>(
        QT_TRANSLATE_NOOP("QObject", "Material"));

    // add resources and reloads the translators
    loadMaterialResource();

    PyMOD_Return(matGuiModule);
}
