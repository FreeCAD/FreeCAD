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
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include "Workbench.h"
#include "ViewProviderImagePlane.h"

// use a different name to CreateCommand()
void CreateImageCommands(void);

void loadImageResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Image);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef ImageGui_Import_methods[];


/* Python entry */
extern "C" {
void ImageGuiExport initImageGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    (void) Py_InitModule("ImageGui", ImageGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Image module... done\n");

    // instantiating the commands
    CreateImageCommands();

    ImageGui::ViewProviderImagePlane::init();
    ImageGui::Workbench::init();

    // add resources and reloads the translators
    loadImageResource();
}

} // extern "C" {
