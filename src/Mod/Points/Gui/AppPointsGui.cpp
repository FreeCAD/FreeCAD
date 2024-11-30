/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <Mod/Points/App/PropertyPointKernel.h>

#include "ViewProvider.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreatePointsCommands();

void loadPointsResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Points);
    Q_INIT_RESOURCE(Points_translation);
    Gui::Translator::instance()->refresh();
}

namespace PointsGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("PointsGui")
    {
        initialize("This module is the PointsGui module.");  // register with Python
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace PointsGui


/* Python entry */
PyMOD_INIT_FUNC(PointsGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // load dependent module
    try {
        Base::Interpreter().loadModule("Points");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    Base::Console().Log("Loading GUI of Points module... done\n");
    PyObject* mod = PointsGui::initModule();

    // instantiating the commands
    CreatePointsCommands();

    // clang-format off
    PointsGui::ViewProviderPoints       ::init();
    PointsGui::ViewProviderScattered    ::init();
    PointsGui::ViewProviderStructured   ::init();
    PointsGui::ViewProviderPython       ::init();
    PointsGui::Workbench                ::init();
    // clang-format on
    Gui::ViewProviderBuilder::add(Points::PropertyPointKernel::getClassTypeId(),
                                  PointsGui::ViewProviderPoints::getClassTypeId());

    // add resources and reloads the translators
    loadPointsResource();

    PyMOD_Return(mod);
}
