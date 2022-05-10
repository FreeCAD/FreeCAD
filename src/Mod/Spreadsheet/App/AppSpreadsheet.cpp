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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include "Sheet.h"


namespace Spreadsheet {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Spreadsheet")
    {
        initialize("This module is the Spreadsheet module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}
} // namespace Spreadsheet

/* Python entry */
PyMOD_INIT_FUNC(Spreadsheet)
{
    Spreadsheet::PropertySpreadsheetQuantity::init();
    Spreadsheet::PropertyColumnWidths::init();
    Spreadsheet::PropertyRowHeights::init();
    Spreadsheet::PropertySheet::init();

    Spreadsheet::Sheet::init();
    Spreadsheet::SheetPython::init();

    PyObject* mod = Spreadsheet::initModule();
    Base::Console().Log("Loading Spreadsheet module... done\n");
    PyMOD_Return(mod);
}
