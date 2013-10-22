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

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/WidgetFactory.h>
#include <Gui/Language/Translator.h>

#include "DlgSettingsRayImp.h"
#include "ViewProvider.h"
#include "Workbench.h"


using namespace RaytracingGui;

// use a different name to CreateCommand()
void CreateRaytracingCommands(void);

void loadRaytracingResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Raytracing);
    Gui::Translator::instance()->refresh();
}

extern struct PyMethodDef RaytracingGui_methods[];


extern "C" {
void AppRaytracingGuiExport initRaytracingGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    try {
        Base::Interpreter().loadModule("Raytracing");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    (void) Py_InitModule("RaytracingGui", RaytracingGui_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Raytracing module... done\n");

    // instantiating the commands
    CreateRaytracingCommands();
    RaytracingGui::ViewProviderLux      ::init();
    RaytracingGui::ViewProviderPovray   ::init();
    RaytracingGui::Workbench            ::init();

    // register preferences pages
    new Gui::PrefPageProducer<DlgSettingsRayImp> ("Raytracing");

    // add resources and reloads the translators
    loadRaytracingResource();
}

} // extern "C" {
