#include "SearchableComboBoxPy.h"
#include "SearchableComboBoxPy.cpp"

using namespace Gui;

PyObject* SearchableComboBoxPy::PyMake(struct _typeobject*, PyObject*, PyObject*)
{
    return new SearchableComboBoxPy(new SearchableComboBox);
}

int SearchableComboBoxPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

std::string SearchableComboBoxPy::representation() const
{
    return "<SearchableComboBox>";
}

PyObject* SearchableComboBoxPy::isSearchable(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        return PyBool_FromLong(getSearchableComboBoxPtr()->isSearchable());
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::setSearchable(PyObject* args, PyObject* kwds)
{
    PyObject* value = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", nullptr, &value)) {
        return nullptr;
    }

    PY_TRY
    {
        if (!PyBool_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "searchable must be a bool");
            return nullptr;
        }

        getSearchableComboBoxPtr()->setSearchable(PyObject_IsTrue(value));

        Py_Return;
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::isGrid(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        return PyBool_FromLong(getSearchableComboBoxPtr()->isGrid());
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::setGrid(PyObject* args, PyObject* kwds)
{
    PyObject* value = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", nullptr, &value)) {
        return nullptr;
    }

    PY_TRY
    {
        if (!PyBool_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "grid must be a bool");
            return nullptr;
        }

        getSearchableComboBoxPtr()->setGrid(PyObject_IsTrue(value));

        Py_Return;
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::popupMaximumHeight(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        return PyLong_FromLong(getSearchableComboBoxPtr()->popupMaximumHeight());
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::setPopupMaximumHeight(PyObject* args, PyObject* kwds)
{
    int height = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", nullptr, &height)) {
        return nullptr;
    }

    PY_TRY
    {
        getSearchableComboBoxPtr()->setPopupMaximumHeight(height);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::gridFixedColumns(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        return PyLong_FromLong(getSearchableComboBoxPtr()->gridFixedColumns());
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::setGridFixedColumns(PyObject* args, PyObject* kwds)
{
    int columns = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", nullptr, &columns)) {
        return nullptr;
    }

    PY_TRY
    {
        getSearchableComboBoxPtr()->setGridFixedColumns(columns);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::popupScrollBar(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        return PyBool_FromLong(getSearchableComboBoxPtr()->popupScrollBar());
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::setPopupScrollBar(PyObject* args, PyObject* kwds)
{
    PyObject* value = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", nullptr, &value)) {
        return nullptr;
    }

    PY_TRY
    {
        if (!PyBool_Check(value)) {
            PyErr_SetString(PyExc_TypeError, "popupScrollBar must be a bool");
            return nullptr;
        }

        getSearchableComboBoxPtr()->setPopupScrollBar(PyObject_IsTrue(value));

        Py_Return;
    }
    PY_CATCH;
}

PyObject* SearchableComboBoxPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int SearchableComboBoxPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
