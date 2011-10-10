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
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include "Workbench.h"
//#include "resources/qrc_ReverseEngineering.cpp"

// use a different name to CreateCommand()
void CreateReverseEngineeringCommands(void);

void loadReverseEngineeringResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(ReverseEngineering);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef ReverseEngineeringGui_Import_methods[];


/* Python entry */
extern "C" {
void ReenGuiExport initReverseEngineeringGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    (void) Py_InitModule("ReverseEngineeringGui", ReverseEngineeringGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of ReverseEngineering module... done\n");

    // instantiating the commands
    CreateReverseEngineeringCommands();
    ReverseEngineeringGui::Workbench::init();

     // add resources and reloads the translators
    loadReverseEngineeringResource();
}

} // extern "C" {
