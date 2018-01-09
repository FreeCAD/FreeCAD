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
#include <Gui/Language/Translator.h>
#include "Workbench.h"

#include "ViewProvider.h"
#include "ViewProviderProduct.h"
#include "ViewProviderProductRef.h"
#include "ViewProviderConstraintGroup.h"
#include "ViewProviderConstraint.h"

#include <Mod/Assembly/App/Product.h>
#include <Mod/Assembly/App/ProductRef.h>

//#include "resources/qrc_Assembly.cpp"

// use a different name to CreateCommand()
void CreateAssemblyCommands(void);
void CreateAssemblyConstraintCommands(void);

void loadAssemblyResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Assembly);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef AssemblyGui_Import_methods[];



/* Python entry */
extern "C" {
void AssemblyGuiExport initAssemblyGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    (void) Py_InitModule("AssemblyGui", AssemblyGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Assembly module... done\n");

    // directly load the module for usage in commands
    Base::Interpreter().runString("import AssemblyGui");
    Base::Interpreter().runString("import PartGui");

    // instantiating the commands
    CreateAssemblyCommands();
    CreateAssemblyConstraintCommands();

    AssemblyGui::Workbench::init();

    AssemblyGui::ViewProviderItem        ::init();
    AssemblyGui::ViewProviderProduct     ::init();
    AssemblyGui::ViewProviderProductRef  ::init();

    AssemblyGui::ViewProviderConstraintGroup::init();
    AssemblyGui::ViewProviderConstraint::init();

     // add resources and reloads the translators
    loadAssemblyResource();
}

} // extern "C" {
