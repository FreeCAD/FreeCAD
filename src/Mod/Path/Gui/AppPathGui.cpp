/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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

#include <Gui/WidgetFactory.h>
#include <Gui/Language/Translator.h>

#include "DlgSettingsPathColor.h"
#include "ViewProviderArea.h"
#include "ViewProviderPath.h"
#include "ViewProviderPathCompound.h"
#include "ViewProviderPathShape.h"
#include <Gui/Application.h>


// use a different name to CreateCommand()
void CreatePathCommands();

void loadPathResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Path);
    Q_INIT_RESOURCE(Path_translation);
    Gui::Translator::instance()->refresh();
}

namespace PathGui {
extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(PathGui)
{
     if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }
    try {
        Base::Interpreter().runString("import PartGui");
        Base::Interpreter().runString("import Path");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = PathGui::initModule();
    Base::Console().Log("Loading GUI of Path module... done\n");

    // instantiating the commands
    CreatePathCommands();

    // addition objects
    PathGui::ViewProviderPath               ::init();
    PathGui::ViewProviderPathCompound       ::init();
    PathGui::ViewProviderPathCompoundPython ::init();
    PathGui::ViewProviderPathShape          ::init();
    PathGui::ViewProviderPathPython         ::init();
    PathGui::ViewProviderArea               ::init();
    PathGui::ViewProviderAreaPython         ::init();
    PathGui::ViewProviderAreaView           ::init();
    PathGui::ViewProviderAreaViewPython     ::init();

     // add resources and reloads the translators
    loadPathResource();

    // register preferences pages
    new Gui::PrefPageProducer<PathGui::DlgSettingsPathColor> (QT_TRANSLATE_NOOP("QObject","Path"));

    PyMOD_Return(mod);
}
