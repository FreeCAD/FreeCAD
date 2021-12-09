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
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include "Workbench.h"

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

// use a different name to CreateCommand()
void CreateReverseEngineeringCommands(void);

void loadReverseEngineeringResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(ReverseEngineering);
    Gui::Translator::instance()->refresh();
}

namespace ReverseEngineeringGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("ReverseEngineeringGui")
    {
        initialize("This module is the ReverseEngineeringGui module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace ReverseEngineeringGui


/* Python entry */
PyMOD_INIT_FUNC(ReverseEngineeringGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(0);
    }

    // load dependent module
    try {
        Base::Interpreter().loadModule("MeshGui");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(0);
    }

    PyObject* mod = ReverseEngineeringGui::initModule();
    Base::Console().Log("Loading GUI of ReverseEngineering module... done\n");

    // instantiating the commands
    CreateReverseEngineeringCommands();
    ReverseEngineeringGui::Workbench::init();

     // add resources and reloads the translators
    loadReverseEngineeringResource();
    PyMOD_Return(mod);
}
