// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2002 Juergen Riegel <juergen.riegel@web.de>                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "Sheet.h"


namespace Spreadsheet
{
class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Spreadsheet")
    {
        initialize("This module is the Spreadsheet module.");  // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}
}  // namespace Spreadsheet

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
    Base::Console().log("Loading Spreadsheet module... done\n");
    PyMOD_Return(mod);
}
