/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
#include <Gui/WorkbenchManager.h>
#include <Gui/Language/Translator.h>
#include "Workbench.h"

#include <Mod/Complete/App/CompleteConfiguration.h>


// use a different name to CreateCommand()
void CreateCompleteCommands(void);

void loadCompleteResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Complete);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef CompleteGui_Import_methods[];


/* Python entry */
extern "C" {
void CompleteGuiExport initCompleteGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    // load dependent module
    try {
        Base::Interpreter().loadModule("PartGui");
        Base::Interpreter().loadModule("MeshGui");
        try {
            Base::Interpreter().loadModule("MeshPartGui");
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to load MeshPartGui: %s\n", e.what());
            PyErr_Clear();
        }
        Base::Interpreter().loadModule("PointsGui");
        //Base::Interpreter().loadModule("MeshPartGui");
        //Base::Interpreter().loadModule("AssemblyGui");
        Base::Interpreter().loadModule("DrawingGui");
        Base::Interpreter().loadModule("RaytracingGui");
#       ifdef COMPLETE_SHOW_SKETCHER
        Base::Interpreter().loadModule("SketcherGui");
#       endif
        Base::Interpreter().loadModule("PartDesignGui");
        Base::Interpreter().loadModule("ImageGui");
        //Base::Interpreter().loadModule("CamGui");
        Base::Interpreter().loadModule("TestGui");
#       ifdef COMPLETE_USE_DRAFTING
        Py::Module module(PyImport_ImportModule("FreeCADGui"),true);
        Py::Callable method(module.getAttr(std::string("getWorkbench")));

        // Get the CompleteWorkbench handler
        Py::Tuple args(1);
        args.setItem(0,Py::String("DraftWorkbench"));
        Py::Object handler(method.apply(args));

        std::string type;
        if (!handler.hasAttr(std::string("__Workbench__"))) {
            // call its GetClassName method if possible
            Py::Callable method(handler.getAttr(std::string("GetClassName")));
            Py::Tuple args;
            Py::String result(method.apply(args));
            type = result.as_std_string();
            if (type == "Gui::PythonWorkbench") {
                Gui::Workbench* wb = Gui::WorkbenchManager::instance()->createWorkbench("DraftWorkbench", type);
                handler.setAttr(std::string("__Workbench__"), Py::Object(wb->getPyObject(), true));
            }

            // import the matching module first
            Py::Callable activate(handler.getAttr(std::string("Initialize")));
            activate.apply(args);
        }

        // Get the CompleteWorkbench handler
        args.setItem(0,Py::String("CompleteWorkbench"));
#       endif
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    catch (Py::Exception& e) {
        Py::Object o = Py::type(e);
        if (o.isString()) {
            Py::String s(o);
            Base::Console().Error("%s\n", s.as_std_string().c_str());
        }
        else {
            Py::String s(o.repr());
            Base::Console().Error("%s\n", s.as_std_string().c_str());
        }
        // Prints message to console window if we are in interactive mode
        PyErr_Print();
    }

    (void) Py_InitModule("CompleteGui", CompleteGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Complete module... done\n");

    // instantiating the commands
    CreateCompleteCommands();
    CompleteGui::Workbench::init();

     // add resources and reloads the translators
    loadCompleteResource();
}

} // extern "C" {
