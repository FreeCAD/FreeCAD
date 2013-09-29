/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jrgen Riegel 2002                                                     *
 *   Eivind Kvedalen 2015                                                  *
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
#include "ViewProviderSpreadsheet.h"
#include "SpreadsheetView.h"
#include "qrc_Spreadsheet.cxx"

// use a different name to CreateCommand()
void CreateSpreadsheetCommands(void);

void loadSpreadsheetResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Spreadsheet);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef SpreadsheetGui_Import_methods[];


/* Python entry */
extern "C" {
void SpreadsheetGuiExport initSpreadsheetGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    (void) Py_InitModule("SpreadsheetGui", SpreadsheetGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Spreadsheet module... done\n");

    // instantiating the commands
    CreateSpreadsheetCommands();

    SpreadsheetGui::ViewProviderSheet::init();
    SpreadsheetGui::Workbench::init();
//    SpreadsheetGui::SheetView::init();

    // add resources and reloads the translators
    loadSpreadsheetResource();
}

} // extern "C" {
