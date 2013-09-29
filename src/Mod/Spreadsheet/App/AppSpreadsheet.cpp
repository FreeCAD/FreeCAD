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
#include "Sheet.h"
#include "Expression.h"


/* registration table  */
static struct PyMethodDef Spreadsheet_methods[] = {
    {NULL, NULL}                   /* end of table marker */
};

/* Python entry */
extern "C" {
void SpreadsheetExport initSpreadsheet() {
    (void) Py_InitModule("Spreadsheet", Spreadsheet_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading Spreadsheet module... done\n");

    Spreadsheet::PropertyColumnWidths::init();
    Spreadsheet::PropertyRowHeights::init();
    Spreadsheet::PropertySheet::init();

    Spreadsheet::Sheet::init();
    Spreadsheet::Expression::init();
    Spreadsheet::UnitExpression::init();
    Spreadsheet::NumberExpression::init();
    Spreadsheet::ConstantExpression::init();
    Spreadsheet::FunctionExpression::init();
    Spreadsheet::OperatorExpression::init();
    Spreadsheet::VariableExpression::init();
    Spreadsheet::ConditionalExpression::init();
    Spreadsheet::StringExpression::init();

    return;
}

} // extern "C"
