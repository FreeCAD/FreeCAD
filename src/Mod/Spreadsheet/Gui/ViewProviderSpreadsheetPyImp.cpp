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


PyObject* ViewProviderSpreadsheetPy::selectedRanges(PyObject* /*obj*/)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView *sheetView = vp->getView();
    std::vector<App::Range> ranges = sheetView->selectedRanges();
    Py::List list;
    for (const auto &range : ranges)
    {
        list.append(Py::String(range.rangeString()));
    }
    
    return Py::new_reference_to(list);
}

PyObject* ViewProviderSpreadsheetPy::selectedCells(PyObject* /*obj*/)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView *sheetView = vp->getView();
    QModelIndexList cells = sheetView->selectedIndexes();
    Py::List list;
    for (const auto &cell : cells) {
        list.append(Py::String(App::CellAddress(cell.row(), cell.column()).toString()));
    }

    return Py::new_reference_to(list);
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
    Py_RETURN_NONE;
}

PyObject* ViewProviderSpreadsheetPy::currentIndex(PyObject* /*_args*/)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView* sheetView = vp->getView();
    auto index = sheetView->currentIndex();
    Py::String str(App::CellAddress(index.row(), index.column()).toString());
    return Py::new_reference_to(str);
}

PyObject* ViewProviderSpreadsheetPy::setCurrentIndex(PyObject* args)
{
    ViewProviderSheet* vp = this->getViewProviderSheetPtr();
    SheetView* sheetView = vp->getView();

    const char* cell;
    if (PyArg_ParseTuple(args, "s", &cell)) {
        sheetView->setCurrentIndex(App::CellAddress(cell));
    }
    Py_RETURN_NONE;
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
