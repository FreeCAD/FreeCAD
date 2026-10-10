#include "LargeComboBoxPy.h"
#include "LargeComboBoxPy.cpp"

using namespace Gui;

PyObject* LargeComboBoxPy::PyMake(PyTypeObject* /*type*/, PyObject* /*args*/, PyObject* /*kwds*/)
{
    return new LargeComboBoxPy(new LargeComboBox);
}

// constructor method
int LargeComboBoxPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


std::string LargeComboBoxPy::representation() const
{
    return {"<LargeComboBox object>"};
}


Py::Boolean LargeComboBoxPy::getsearchable() const
{
    return Py::Boolean(getLargeComboBoxPtr()->isSearchable());
}

void LargeComboBoxPy::setsearchable(Py::Boolean arg)
{
    getLargeComboBoxPtr()->setSearchable(arg);
}

Py::Boolean LargeComboBoxPy::getgrid() const
{
    return Py::Boolean(getLargeComboBoxPtr()->isGrid());
}

void LargeComboBoxPy::setgrid(Py::Boolean arg)
{
    getLargeComboBoxPtr()->setGrid(arg);
}

Py::Long LargeComboBoxPy::getpopupMaximumHeight() const
{
    return Py::Long(getLargeComboBoxPtr()->popupMaximumHeight());
}

void LargeComboBoxPy::setpopupMaximumHeight(Py::Long arg)
{
    getLargeComboBoxPtr()->setPopupMaximumHeight(arg);
}

Py::Long LargeComboBoxPy::getgridFixedColumns() const
{
    return Py::Long(getLargeComboBoxPtr()->gridFixedColumns());
}

void LargeComboBoxPy::setgridFixedColumns(Py::Long arg)
{
    getLargeComboBoxPtr()->setGridFixedColumns(arg);
}

Py::Long LargeComboBoxPy::getgridRowCount() const
{
    return Py::Long(getLargeComboBoxPtr()->gridRowCount());
}

void LargeComboBoxPy::setgridRowCount(Py::Long arg)
{
    getLargeComboBoxPtr()->setGridRowCount(arg);
}

Py::Boolean LargeComboBoxPy::getpopupScrollBar() const
{
    return Py::Boolean(getLargeComboBoxPtr()->popupScrollBar());
}

void LargeComboBoxPy::setpopupScrollBar(Py::Boolean arg)
{
    getLargeComboBoxPtr()->setPopupScrollBar(arg);
}


PyObject* LargeComboBoxPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int LargeComboBoxPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
