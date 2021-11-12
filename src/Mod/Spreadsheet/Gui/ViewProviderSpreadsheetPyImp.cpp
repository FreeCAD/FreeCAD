#include "PreCompiled.h"

#include "ViewProviderSpreadsheetPy.h"
#include "ViewProviderSpreadsheetPy.cpp"

#include "SpreadsheetView.h"

using namespace SpreadsheetGui;

#if PY_MAJOR_VERSION >= 2
#define PyString_FromString PyUnicode_FromString
#endif

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
    std::vector<App::Range>::const_iterator i = ranges.begin();
    for (; i != ranges.end(); ++i)
    {
        PyObject *py_str = PyString_FromString(i->rangeString().c_str());
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
    for (auto it : cells) {
        PyObject *py_str = PyString_FromString(
            App::CellAddress(it.row(), it.column()).toString().c_str());
        PyList_Append(out, py_str);
    }    

    return out;
}

PyObject *ViewProviderSpreadsheetPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}


int ViewProviderSpreadsheetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
