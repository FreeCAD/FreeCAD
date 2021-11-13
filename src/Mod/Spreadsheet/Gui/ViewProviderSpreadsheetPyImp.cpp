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

PyObject* ViewProviderSpreadsheetPy::select(PyObject* _args)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView* sheetView = vp->getView();

    Py::Sequence args(_args);

    const char* cell;
    const char* topLeft;
    const char* bottomRight;
    int flags = 0;
    if (args.size() == 2 && PyArg_ParseTuple(_args, "si", &cell, &flags)) {
        sheetView->select(App::CellAddress(cell), static_cast<QItemSelectionModel::SelectionFlags>(flags));
    }
    else if (args.size() == 3 && PyArg_ParseTuple(_args, "ssi", &topLeft, &bottomRight, &flags)) {
        sheetView->select(App::CellAddress(topLeft), App::CellAddress(bottomRight), static_cast<QItemSelectionModel::SelectionFlags>(flags));
    }
    else {
        if (args.size() == 2)
            throw Base::TypeError("Expects the arguments to be a cell name (e.g. 'A1') and QItemSelectionModel.SelectionFlags");
        else if (args.size() == 3)
            throw Base::TypeError("Expects the arguments to be a cell name (e.g. 'A1'), a second cell name (e.g. 'B5'), and QItemSelectionModel.SelectionFlags");
        else
            throw Base::TypeError("Wrong arguments to select: specify either a cell, or two cells (for a range), and QItemSelectionModel.SelectionFlags");
    }
    return Py_None;
}

PyObject* ViewProviderSpreadsheetPy::currentIndex(PyObject* /*_args*/)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView* sheetView = vp->getView();
    auto index = sheetView->currentIndex();
    PyObject* py_str = PyUnicode_FromString(
        App::CellAddress(index.row(), index.column()).toString().c_str());
    return py_str;
}

PyObject* ViewProviderSpreadsheetPy::setCurrentIndex(PyObject* args)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView* sheetView = vp->getView();

    const char* cell;
    if (PyArg_ParseTuple(args, "s", &cell)) {
        sheetView->setCurrentIndex(App::CellAddress(cell));
    }
    return Py_None;
}

PyObject *ViewProviderSpreadsheetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}


int ViewProviderSpreadsheetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
