/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer@users.sourceforge.net>        *
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
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/WidgetFactory.h>

#include "Workbench.h"

// use a different name to CreateCommand()
void CreateCamCommands(void);

/* registration table  */
static struct PyMethodDef CamGui_methods[] =
{
    {
        NULL, NULL
    }                   /* end of table marker */
};

extern "C"
{
    void AppCamGuiExport initCamGui()
    {
        if (!Gui::Application::Instance)
        {
            PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
            return;
        }

        // load dependend module
        try {
            Base::Interpreter().loadModule("Cam");
        }
        catch(const Base::Exception& e) {
            PyErr_SetString(PyExc_ImportError, e.what());
            return;
        }

        (void) Py_InitModule("CamGui", CamGui_methods);   /* mod name, table ptr */
        Base::Console().Log("Loading GUI of Cam module... done\n");

        CamGui::Workbench           ::init();

        // instanciating the commands
        CreateCamCommands();

        return;
    }
} // extern "C"
