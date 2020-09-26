/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller         <Nathan.A.Mill[at]gmail.com>,*
 *                      Balázs Bámer                                       *
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
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>

#include "Workbench.h"
#include "TaskGeomFillSurface.h"
#include "TaskFilling.h"
#include "TaskSections.h"

// use a different name to CreateCommand()
void CreateSurfaceCommands(void);


namespace SurfaceGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("SurfaceGui")
    {
        initialize("This module is the SurfaceGui module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace SurfaceGui

/* Python entry */
PyMOD_INIT_FUNC(SurfaceGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(0);
    }

    Base::Interpreter().runString("import Surface");
    Base::Interpreter().runString("import PartGui");

    // instantiating the commands
    CreateSurfaceCommands();

    SurfaceGui::Workbench::init();
    SurfaceGui::ViewProviderGeomFillSurface ::init();
    SurfaceGui::ViewProviderFilling         ::init();
    SurfaceGui::ViewProviderSections        ::init();
    
//    SurfaceGui::ViewProviderCut::init();

    PyObject* mod = SurfaceGui::initModule();
    Base::Console().Log("Loading GUI of Surface module... done\n");
    PyMOD_Return(mod);
}
