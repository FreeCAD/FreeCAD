#include "SearchableComboBoxPy.h"
#include "SearchableComboBoxPy.cpp"

using namespace Gui;

PyObject* SearchableComboBoxPy::PyMake(PyTypeObject* /*type*/, PyObject* /*args*/, PyObject* /*kwds*/)
{
    return new SearchableComboBoxPy(new SearchableComboBox);
}

// constructor method
int SearchableComboBoxPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


std::string SearchableComboBoxPy::representation() const
{
    return {"<SearchableComboBox object>"};
}


Py::Boolean SearchableComboBoxPy::getsearchable() const
{
    return Py::Boolean(getSearchableComboBoxPtr()->isSearchable());
}

void SearchableComboBoxPy::setsearchable(Py::Boolean arg)
{
    getSearchableComboBoxPtr()->setSearchable(arg);
}

Py::Boolean SearchableComboBoxPy::getgrid() const
{
    return Py::Boolean(getSearchableComboBoxPtr()->isGrid());
}

void SearchableComboBoxPy::setgrid(Py::Boolean arg)
{
    getSearchableComboBoxPtr()->setGrid(arg);
}

Py::Long SearchableComboBoxPy::getpopupMaximumHeight() const
{
    return Py::Long(getSearchableComboBoxPtr()->popupMaximumHeight());
}

void SearchableComboBoxPy::setpopupMaximumHeight(Py::Long arg)
{
    getSearchableComboBoxPtr()->setPopupMaximumHeight(arg);
}

Py::Long SearchableComboBoxPy::getgridFixedColumns() const
{
    return Py::Long(getSearchableComboBoxPtr()->gridFixedColumns());
}

void SearchableComboBoxPy::setgridFixedColumns(Py::Long arg)
{
    getSearchableComboBoxPtr()->setGridFixedColumns(arg);
}

Py::Boolean SearchableComboBoxPy::getpopupScrollBar() const
{
    return Py::Boolean(getSearchableComboBoxPtr()->popupScrollBar());
}

void SearchableComboBoxPy::setpopupScrollBar(Py::Boolean arg)
{
    getSearchableComboBoxPtr()->setPopupScrollBar(arg);
}


PyObject* SearchableComboBoxPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int SearchableComboBoxPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
