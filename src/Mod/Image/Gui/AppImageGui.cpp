/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>

#include "ImageView.h"
#include "Workbench.h"
#include "ViewProviderImagePlane.h"


// use a different name to CreateCommand()
void CreateImageCommands();

void loadImageResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Image);
    Gui::Translator::instance()->refresh();
}

namespace ImageGui {
extern PyObject* initModule();
}


/* Python entry */
PyMOD_INIT_FUNC(ImageGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    PyObject* mod = ImageGui::initModule();
    Base::Console().Log("Loading GUI of Image module... done\n");

    // instantiating the commands
    CreateImageCommands();

    ImageGui::ImageView::init();
    ImageGui::ViewProviderImagePlane::init();
    ImageGui::Workbench::init();

    // add resources and reloads the translators
    loadImageResource();

    PyMOD_Return(mod);
}
