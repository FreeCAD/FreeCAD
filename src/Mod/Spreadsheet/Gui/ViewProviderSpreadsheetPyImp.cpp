#include "PreCompiled.h"

#include "ViewProviderSpreadsheetPy.h"
#include "ViewProviderSpreadsheetPy.cpp"
#include <CXX/Objects.hxx>

#include "SpreadsheetView.h"

using namespace SpreadsheetGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderSpreadsheetPy::representation(void) const
{
    return std::string("<ViewProviderSpreadsheet object>");
}

PyObject* ViewProviderSpreadsheetPy::getView(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView* sheetView = vp->getView();
    if (sheetView)
        return sheetView->getPyObject();
    Py_RETURN_NONE;
}

PyObject *ViewProviderSpreadsheetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}


int ViewProviderSpreadsheetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
