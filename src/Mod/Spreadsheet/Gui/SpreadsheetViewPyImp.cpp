#include "PreCompiled.h"

#include "SpreadsheetViewPy.h"
#include "SpreadsheetViewPy.cpp"

#include <Mod/Spreadsheet/App/SheetPy.h>

using namespace SpreadsheetGui;

PyObject*  SpreadsheetViewPy::getSheet(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return new Spreadsheet::SheetPy(getSheetViewPtr()->getSheet());
}

// returns a string which represents the object e.g. when printed in python
std::string SpreadsheetViewPy::representation(void) const
{
    return std::string("<SheetView object>");
}

PyObject *SpreadsheetViewPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SpreadsheetViewPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
