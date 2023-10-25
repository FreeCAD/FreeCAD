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

#include "CurveOnMesh.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateMeshPartCommands();

void loadMeshPartResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(MeshPart);
    Q_INIT_RESOURCE(MeshPart_translation);
    Gui::Translator::instance()->refresh();
}

namespace MeshPartGui
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MeshPartGui")
    {
        initialize("This module is the MeshPartGui module.");  // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace MeshPartGui


/* Python entry */
PyMOD_INIT_FUNC(MeshPartGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    PyObject* mod = MeshPartGui::initModule();
    Base::Console().Log("Loading GUI of MeshPart module... done\n");

    // clang-format off
    // instantiating the commands
    CreateMeshPartCommands();
    MeshPartGui::Workbench                  ::init();
    MeshPartGui::ViewProviderCurveOnMesh    ::init();
    // clang-format on

    // add resources and reloads the translators
    loadMeshPartResource();

    PyMOD_Return(mod);
}
