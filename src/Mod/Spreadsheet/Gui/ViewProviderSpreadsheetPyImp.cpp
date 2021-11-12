#include "PreCompiled.h"

#include "ViewProviderSpreadsheetPy.h"
#include "ViewProviderSpreadsheetPy.cpp"

#include "SpreadsheetView.h"

using namespace SpreadsheetGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderSpreadsheetPy::representation(void) const
{
    return std::string("<ViewProviderSpreadsheet object>");
}


PyObject* ViewProviderSpreadsheetPy::selectedRanges(PyObject* /*obj*/)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView *sheetView = vp->getView();
    std::vector<App::Range> ranges = sheetView->selectedRanges();
    PyObject *out = PyList_New(0);
    for (const auto &range : ranges)
    {
        PyObject *py_str = PyUnicode_FromString(range.rangeString().c_str());
        PyList_Append(out, py_str);
    }
    
    return out;
}

PyObject* ViewProviderSpreadsheetPy::selectedCells(PyObject* /*obj*/)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView *sheetView = vp->getView();
    QModelIndexList cells = sheetView->selectedIndexes();
    PyObject *out = PyList_New(0);
    for (const auto &cell : cells) {
        PyObject *py_str = PyUnicode_FromString(
            App::CellAddress(cell.row(), cell.column()).toString().c_str());
        PyList_Append(out, py_str);
    }    

    return out;
}

PyObject *ViewProviderSpreadsheetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}


int ViewProviderSpreadsheetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
