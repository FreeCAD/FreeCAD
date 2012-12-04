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
#include "ViewProviderPocket.h"
#include "ViewProviderPad.h"
#include "ViewProviderChamfer.h"
#include "ViewProviderFillet.h"
#include "ViewProviderDraft.h"
#include "ViewProviderRevolution.h"
#include "ViewProviderGroove.h"
#include "ViewProviderMirrored.h"
#include "ViewProviderLinearPattern.h"
#include "ViewProviderPolarPattern.h"
#include "ViewProviderScaled.h"
#include "ViewProviderMultiTransform.h"

//#include "resources/qrc_PartDesign.cpp"

// use a different name to CreateCommand()
void CreatePartDesignCommands(void);

void loadPartDesignResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(PartDesign);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef PartDesignGui_Import_methods[];


/* Python entry */
extern "C" {
void PartDesignGuiExport initPartDesignGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    try {
        Base::Interpreter().runString("import PartGui");
        Base::Interpreter().runString("import SketcherGui");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    (void) Py_InitModule("PartDesignGui", PartDesignGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of PartDesign module... done\n");

    // instantiating the commands
    CreatePartDesignCommands();

    PartDesignGui::Workbench                 ::init();
    PartDesignGui::ViewProvider              ::init();
    PartDesignGui::ViewProviderPocket        ::init();
    PartDesignGui::ViewProviderPad           ::init();
    PartDesignGui::ViewProviderRevolution    ::init();
    PartDesignGui::ViewProviderGroove        ::init();
    PartDesignGui::ViewProviderChamfer       ::init();
    PartDesignGui::ViewProviderFillet        ::init();
    PartDesignGui::ViewProviderDraft         ::init();
    PartDesignGui::ViewProviderMirrored      ::init();
    PartDesignGui::ViewProviderLinearPattern ::init();
    PartDesignGui::ViewProviderPolarPattern  ::init();
    PartDesignGui::ViewProviderScaled        ::init();
    PartDesignGui::ViewProviderMultiTransform::init();

     // add resources and reloads the translators
    loadPartDesignResource();
}

} // extern "C" {
