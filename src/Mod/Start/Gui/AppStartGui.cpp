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
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "DlgStartPreferencesImp.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateStartCommands();

void loadStartResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Start);
    Q_INIT_RESOURCE(Start_translation);
    Gui::Translator::instance()->refresh();
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

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace StartGui


/* Python entry */
PyMOD_INIT_FUNC(StartGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // load dependent module
    try {
        Base::Interpreter().runString("import WebGui");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    catch (Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().Error("%s\n", s.as_std_string("utf-8").c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().Error("%s\n", s.as_std_string("utf-8").c_str());
        }
        // Prints message to console window if we are in interactive mode
        PyErr_Print();
    }

    PyObject* mod = StartGui::initModule();
    Base::Console().Log("Loading GUI of Start module... done\n");

    // clang-format off
    // register preferences pages
    new Gui::PrefPageProducer<StartGui::DlgStartPreferencesImp> (QT_TRANSLATE_NOOP("QObject", "Start"));
    new Gui::PrefPageProducer<StartGui::DlgStartPreferencesAdvancedImp> (QT_TRANSLATE_NOOP("QObject", "Start"));
    // clang-format on

    // instantiating the commands
    CreateStartCommands();
    StartGui::Workbench::init();

    // add resources and reloads the translators
    loadStartResource();
    PyMOD_Return(mod);
}
