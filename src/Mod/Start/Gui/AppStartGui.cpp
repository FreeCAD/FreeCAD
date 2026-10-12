// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <QPointer>


#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Language/Translator.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/Utilities.h>
#include <Gui/WidgetFactory.h>
#include "DlgStartPreferencesImp.h"


#include <gsl/pointers>

#include "Manipulator.h"
#include "StartView.h"

void loadStartResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Start);
    Q_INIT_RESOURCE(Start_translation);
    Gui::Translator::instance()->refresh();
}

namespace StartGui
{
extern PyObject* initModule();
}


namespace StartGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("StartGui")
    {
        initialize("This module is the StartGui module.");  // register with Python
    }
};

PyObject* initModule()
{
    auto newModule = gsl::owner<Module*>(new Module);
    return Base::Interpreter().addModule(newModule);  // Transfer ownership
}

}  // namespace StartGui

/* Python entry */
PyMOD_INIT_FUNC(StartGui)
{
    Base::Console().log("Loading GUI of Start module… ");
    PyObject* mod = StartGui::initModule();
    auto manipulator = std::make_shared<StartGui::Manipulator>();
    Gui::WorkbenchManipulator::installManipulator(manipulator);
    loadStartResource();
    StartGui::StartView::init();
    Base::Console().log("done\n");

    // register preferences pages
    new Gui::PrefPageProducer<StartGui::DlgStartPreferencesImp>(QT_TRANSLATE_NOOP("QObject", "Start"));

    auto mw = Gui::getMainWindow();
    QObject::connect(mw, &Gui::MainWindow::guiInitialized, mw, [mw] {
        if (Gui::isInternalGuiTestRun()) {
            return;
        }

        auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Start"
        );
        if (hGrp->GetBool("ShowOnStartup", true)) {
            // An initialization script may already have opened a document.
            QPointer<Gui::MDIView> activeView = mw->activeWindow();
            Gui::Application::Instance->commandManager().runCommandByName("Start_Start");
            if (activeView) {
                mw->setActiveWindow(activeView);
            }
        }
    });

    PyMOD_Return(mod);
}
