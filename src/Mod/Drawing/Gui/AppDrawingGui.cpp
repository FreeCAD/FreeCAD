/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>

#include "ViewProviderPage.h"
#include "ViewProviderView.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateDrawingCommands(void);

void loadDrawingResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Drawing);
    Q_INIT_RESOURCE(Drawing_translation);
    Gui::Translator::instance()->refresh();
}

namespace DrawingGui
{
extern PyObject* initModule();
}


/* Python entry */
PyMOD_INIT_FUNC(DrawingGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    PyObject* mod = DrawingGui::initModule();
    Base::Console().Log("Loading GUI of Drawing module... done\n");

    // instantiating the commands
    CreateDrawingCommands();
    DrawingGui::Workbench::init();

    DrawingGui::ViewProviderDrawingPage::init();
    DrawingGui::ViewProviderDrawingView::init();
    DrawingGui::ViewProviderDrawingViewPython::init();
    DrawingGui::ViewProviderDrawingClip::init();

    // add resources and reloads the translators
    loadDrawingResource();
    PyMOD_Return(mod);
}
