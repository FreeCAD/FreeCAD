/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#include <Gui/Language/Translator.h>
#include <Mod/Points/App/PropertyPointKernel.h>

#include "ViewProvider.h"
#include "Workbench.h"

// use a different name to CreateCommand()
void CreatePointsCommands(void);

void loadPointsResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Points);
    Gui::Translator::instance()->refresh();
}


/* registration table  */
static struct PyMethodDef PointsGui_methods[] = {
    {NULL, NULL}                   /* end of table marker */
};

/* Python entry */
extern "C" {
void PointsGuiExport initPointsGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    // load dependent module
    try {
        Base::Interpreter().loadModule("Points");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    Base::Console().Log("Loading GUI of Points module... done\n");
    (void) Py_InitModule("PointsGui", PointsGui_methods);   /* mod name, table ptr */

    // instantiating the commands
    CreatePointsCommands();

    PointsGui::ViewProviderPoints::init();
    PointsGui::ViewProviderPython::init();
    PointsGui::Workbench         ::init();
    Gui::ViewProviderBuilder::add(
        Points::PropertyPointKernel::getClassTypeId(),
        PointsGui::ViewProviderPoints::getClassTypeId());

    // add resources and reloads the translators
    loadPointsResource();
}

} // extern "C"
