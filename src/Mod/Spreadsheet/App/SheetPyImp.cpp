/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <boost/tokenizer.hpp>

#include <App/Range.h>
#include <Base/Exception.h>

#include "Sheet.h"
// inclusion of the generated files (generated out of SheetPy.xml)
// clang-format off
#include "SheetPy.h"
#include "SheetPy.cpp"
// clang-format on


using namespace Spreadsheet;
using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string SheetPy::representation() const
{
    return {"<Sheet object>"};
}

PyObject* SheetPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of SheetPy and the Twin object
    return new SheetPy(new Sheet());
}

// constructor method
int SheetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

// +++ methods implementer ++++++++++++++++++++++++++++++++++++++++++++++++

PyObject* SheetPy::set(PyObject* args)
{
    char* address;
    char* contents;


    if (!PyArg_ParseTuple(args, "ss:set", &address, &contents)) {
        return nullptr;
    }

    try {
        Sheet* sheet = getSheetPtr();
        std::string cellAddress = sheet->getAddressFromAlias(address).c_str();

        /* Check to see if address is really an alias first */
        if (!cellAddress.empty()) {
            sheet->setCell(cellAddress.c_str(), contents);
        }
        else {
            Range rangeIter(address);

            do {
                sheet->setCell(rangeIter.address().c_str(), contents);
            } while (rangeIter.next());
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Py_Return;
}

PyObject* SheetPy::get(PyObject* args)
{
    const char* address;
    const char* address2 = nullptr;

    if (!PyArg_ParseTuple(args, "s|s:get", &address, &address2)) {
        return nullptr;
    }

    PY_TRY
    {
        if (address2) {
            auto a1 = getSheetPtr()->getAddressFromAlias(address);
            if (a1.empty()) {
                a1 = address;
            }
            auto a2 = getSheetPtr()->getAddressFromAlias(address2);
            if (a2.empty()) {
                a2 = address2;
            }
            Range range(a1.c_str(), a2.c_str());
            Py::Tuple tuple(range.size());
            int i = 0;
            do {
                App::Property* prop = getSheetPtr()->getPropertyByName(range.address().c_str());
                if (!prop) {
                    PyErr_Format(PyExc_ValueError,
                                 "Invalid address '%s' in range %s:%s",
                                 range.address().c_str(),
                                 address,
                                 address2);
                    return nullptr;
                }
                tuple.setItem(i++, Py::Object(prop->getPyObject(), true));
            } while (range.next());
            return Py::new_reference_to(tuple);
        }
    }
    PY_CATCH;

    App::Property* prop = this->getSheetPtr()->getPropertyByName(address);

    if (!prop) {
        PyErr_Format(PyExc_ValueError, "Invalid cell address or property: %s", address);
        return nullptr;
    }
    return prop->getPyObject();
}

PyObject* SheetPy::getContents(PyObject* args)
{
    char* strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getContents", &strAddress)) {
        return nullptr;
    }

    PY_TRY
    {
        try {
            Sheet* sheet = getSheetPtr();
            std::string addr = sheet->getAddressFromAlias(strAddress);

            if (addr.empty()) {
                address = stringToAddress(strAddress);
            }
            else {
                address = stringToAddress(addr.c_str());
            }
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return nullptr;
        }

        std::string contents;
        const Cell* cell = this->getSheetPtr()->getCell(address);

        if (cell) {
            cell->getStringContent(contents);
        }

        return Py::new_reference_to(Py::String(contents));
    }
    PY_CATCH
}

PyObject* SheetPy::clear(PyObject* args)
{
    const char* strAddress;
    int all = 1;

    if (!PyArg_ParseTuple(args, "s|p:clear", &strAddress, &all)) {
        return nullptr;
    }

    try {
        Range rangeIter(strAddress);
        do {
            this->getSheetPtr()->clear(*rangeIter, all);
        } while (rangeIter.next());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Py_Return;
}

PyObject* SheetPy::clearAll(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    this->getSheetPtr()->clearAll();
    Py_Return;
}

PyObject* SheetPy::importFile(PyObject* args)
{
    const char* filename;
    const char* delimiter = "\t";
    const char* quoteChar = "\"";
    const char* escapeChar = "\\";

    if (!PyArg_ParseTuple(args,
                          "s|sss:importFile",
                          &filename,
                          &delimiter,
                          &quoteChar,
                          &escapeChar)) {
        return nullptr;
    }

    if (getSheetPtr()->importFromFile(filename, delimiter[0], quoteChar[0], escapeChar[0])) {
        return Py::new_reference_to(Py::Boolean(true));
    }
    else {
        return Py::new_reference_to(Py::Boolean(false));
    }
}

PyObject* SheetPy::exportFile(PyObject* args)
{
    const char* filename;
    const char* delimiter = "\t";
    const char* quoteChar = "\"";
    const char* escapeChar = "\\";

    if (!PyArg_ParseTuple(args,
                          "s|sss:exportFile",
                          &filename,
                          &delimiter,
                          &quoteChar,
                          &escapeChar)) {
        return nullptr;
    }

    if (getSheetPtr()->exportToFile(filename, delimiter[0], quoteChar[0], escapeChar[0])) {
        return Py::new_reference_to(Py::Boolean(true));
    }
    else {
        return Py::new_reference_to(Py::Boolean(false));
    }
}

PyObject* SheetPy::mergeCells(PyObject* args)
{
    const char* range;

    if (!PyArg_ParseTuple(args, "s:mergeCells", &range)) {
        return nullptr;
    }

    getSheetPtr()->mergeCells(Range(range));
    Py_Return;
}

PyObject* SheetPy::splitCell(PyObject* args)
{
    const char* strAddress;

    if (!PyArg_ParseTuple(args, "s:splitCell", &strAddress)) {
        return nullptr;
    }

    CellAddress address;
    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    getSheetPtr()->splitCell(address);
    Py_Return;
}

PyObject* SheetPy::insertColumns(PyObject* args)
{
    const char* column;
    int count;

    if (!PyArg_ParseTuple(args, "si:insertColumns", &column, &count)) {
        return nullptr;
    }

    getSheetPtr()->insertColumns(decodeColumn(column), count);
    Py_Return;
}

PyObject* SheetPy::removeColumns(PyObject* args)
{
    const char* column;
    int count;

    if (!PyArg_ParseTuple(args, "si:removeColumns", &column, &count)) {
        return nullptr;
    }

    getSheetPtr()->removeColumns(decodeColumn(column), count);
    Py_Return;
}

PyObject* SheetPy::insertRows(PyObject* args)
{
    const char* row;
    int count;

    if (!PyArg_ParseTuple(args, "si:insertRows", &row, &count)) {
        return nullptr;
    }

    getSheetPtr()->insertRows(decodeRow(std::string(row)), count);
    Py_Return;
}

PyObject* SheetPy::removeRows(PyObject* args)
{
    const char* row;
    int count;

    if (!PyArg_ParseTuple(args, "si:removeRows", &row, &count)) {
        return nullptr;
    }

    getSheetPtr()->removeRows(decodeRow(std::string(row)), count);
    Py_Return;
}

PyObject* SheetPy::setStyle(PyObject* args)
{
    const char* cell;
    PyObject* value;
    std::set<std::string> style;
    const char* options = "replace";

    if (!PyArg_ParseTuple(args, "sO|s:setStyle", &cell, &value, &options)) {
        return nullptr;
    }

    if (PySet_Check(value)) {
        PyObject* copy = PySet_New(value);

        while (PySet_Size(copy) > 0) {
            PyObject* item = PySet_Pop(copy);

            // check on the key:
            if (PyUnicode_Check(item)) {
                style.insert(PyUnicode_AsUTF8(item));
            }
            else {
                std::string error = std::string("type of the set need to be a string, not ")
                    + item->ob_type->tp_name;
                PyErr_SetString(PyExc_TypeError, error.c_str());
                Py_DECREF(copy);
                return nullptr;
            }
        }
        Py_DECREF(copy);
    }
    else if (PyUnicode_Check(value)) {
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = PyUnicode_AsUTF8(value);
        tokenizer<escaped_list_separator<char>> tok(line, e);

        for (tokenizer<escaped_list_separator<char>>::iterator i = tok.begin(); i != tok.end();
             ++i) {
            style.insert(*i);
        }
    }
    else {
        std::string error =
            std::string("style must be either set or string, not ") + value->ob_type->tp_name;
        PyErr_SetString(PyExc_TypeError, error.c_str());
        return nullptr;
    }

    if (strcmp(options, "replace") == 0) {
        Range rangeIter(cell);
        do {
            getSheetPtr()->setStyle(*rangeIter, style);
        } while (rangeIter.next());
    }
    else if (strcmp(options, "add") == 0) {
        Range rangeIter(cell);

        do {
            std::set<std::string> oldStyle;
            const Cell* cell = getSheetPtr()->getCell(*rangeIter);

            // Get old styles first
            if (cell) {
                cell->getStyle(oldStyle);
            }

            for (const auto& it : oldStyle) {
                style.insert(it);
            }

            // Set new style
            getSheetPtr()->setStyle(*rangeIter, style);
        } while (rangeIter.next());
    }
    else if (strcmp(options, "remove") == 0) {
        Range rangeIter(cell);

        do {
            std::set<std::string> oldStyle;
            const Cell* cell = getSheetPtr()->getCell(*rangeIter);

            // Get old styles first
            if (cell) {
                cell->getStyle(oldStyle);
            }

            for (const auto& it : style) {
                oldStyle.erase(it);
            }

            // Set new style
            getSheetPtr()->setStyle(*rangeIter, oldStyle);
        } while (rangeIter.next());
    }
    else if (strcmp(options, "invert") == 0) {
        Range rangeIter(cell);

        do {
            std::set<std::string> oldStyle;
            std::set<std::string> newStyle;
            const Cell* cell = getSheetPtr()->getCell(*rangeIter);

            // Get old styles first
            if (cell) {
                cell->getStyle(oldStyle);
                newStyle = oldStyle;
            }

            for (const auto& i : style) {
                if (oldStyle.find(i) == oldStyle.end()) {
                    // Not found in oldstyle; add it to newStyle
                    newStyle.insert(i);
                }
                else {
                    // Found in oldStyle, remove it from newStyle
                    newStyle.erase(i);
                }
            }

            // Set new style
            getSheetPtr()->setStyle(*rangeIter, newStyle);
        } while (rangeIter.next());
    }
    else {
        PyErr_SetString(
            PyExc_ValueError,
            "Optional parameter must be either 'replace', 'add', 'remove', or 'invert'");
        return nullptr;
    }

    Py_Return;
}

PyObject* SheetPy::getStyle(PyObject* args)
{
    const char* strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getStyle", &strAddress)) {
        return nullptr;
    }

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    std::set<std::string> style;
    const Cell* cell = getSheetPtr()->getCell(address);

    if (cell && cell->getStyle(style)) {
        PyObject* s = PySet_New(nullptr);

        for (const auto& i : style) {
            PySet_Add(s, PyUnicode_FromString(i.c_str()));
        }

        return s;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setDisplayUnit(PyObject* args)
{
    const char* cell;
    const char* value;

    if (!PyArg_ParseTuple(args, "ss:setDisplayUnit", &cell, &value)) {
        return nullptr;
    }

    try {
        Range rangeIter(cell);

        do {
            getSheetPtr()->setDisplayUnit(*rangeIter, value);
        } while (rangeIter.next());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Py_Return;
}

PyObject* SheetPy::setAlias(PyObject* args)
{
    CellAddress address;
    const char* strAddress;
    PyObject* value;

    if (!PyArg_ParseTuple(args, "sO:setAlias", &strAddress, &value)) {
        return nullptr;
    }

    try {
        address = stringToAddress(strAddress);
        Base::PyTypeCheck(&value, &PyUnicode_Type, "String or None expected");
        getSheetPtr()->setAlias(address, value ? PyUnicode_AsUTF8(value) : "");
        Py_Return;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getAlias(PyObject* args)
{
    const char* strAddress;

    if (!PyArg_ParseTuple(args, "s:getAlias", &strAddress)) {
        return nullptr;
    }

    try {
        CellAddress address(strAddress);
        const Cell* cell = getSheetPtr()->getCell(address);
        std::string alias;

        if (cell && cell->getAlias(alias)) {
            return Py::new_reference_to(Py::String(alias));
        }
        else {
            Py_Return;
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getCellFromAlias(PyObject* args)
{
    const char* alias;

    if (!PyArg_ParseTuple(args, "s:getAlias", &alias)) {
        return nullptr;
    }

    try {
        std::string address = getSheetPtr()->getAddressFromAlias(alias);

        if (!address.empty()) {
            return Py::new_reference_to(Py::String(address));
        }
        else {
            Py_Return;
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getDisplayUnit(PyObject* args)
{
    const char* strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getDisplayUnit", &strAddress)) {
        return nullptr;
    }

    try {
        address = stringToAddress(strAddress);

        Spreadsheet::DisplayUnit unit;

        const Cell* cell = getSheetPtr()->getCell(address);

        if (cell && cell->getDisplayUnit(unit)) {
            return Py::new_reference_to(Py::String(unit.stringRep));
        }
        else {
            Py_Return;
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::setAlignment(PyObject* args)
{
    const char* cell;
    PyObject* value;
    int alignment = 0;
    const char* options = "replace";

    if (!PyArg_ParseTuple(args, "sO|s:setAlignment", &cell, &value, &options)) {
        return nullptr;
    }

    if (PySet_Check(value)) {
        // Argument is a set of strings
        PyObject* copy = PySet_New(value);
        int n = PySet_Size(copy);

        while (n-- > 0) {
            PyObject* item = PySet_Pop(copy);

            if (PyUnicode_Check(item)) {
                alignment = Cell::decodeAlignment(PyUnicode_AsUTF8(item), alignment);
            }
            else {
                std::string error = std::string("type of the key need to be a string, not")
                    + item->ob_type->tp_name;
                PyErr_SetString(PyExc_TypeError, error.c_str());
                Py_DECREF(copy);
                return nullptr;
            }
        }

        Py_DECREF(copy);
    }
    else if (PyUnicode_Check(value)) {
        // Argument is a string, combination of alignments, separated by the pipe character
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = PyUnicode_AsUTF8(value);
        tokenizer<escaped_list_separator<char>> tok(line, e);

        for (tokenizer<escaped_list_separator<char>>::iterator i = tok.begin(); i != tok.end();
             ++i) {
            if (!i->empty()) {
                alignment = Cell::decodeAlignment(*i, alignment);
            }
        }
    }
    else {
        std::string error =
            std::string("style must be either set or string, not ") + value->ob_type->tp_name;
        PyErr_SetString(PyExc_TypeError, error.c_str());
        return nullptr;
    }

    // Set alignment depending on 'options' variable
    if (strcmp(options, "replace") == 0) {
        Range rangeIter(cell);

        do {
            getSheetPtr()->setAlignment(*rangeIter, alignment);
        } while (rangeIter.next());
    }
    else if (strcmp(options, "keep") == 0) {
        Range rangeIter(cell);

        do {
            int oldAlignment = 0;
            const Cell* cell = getSheetPtr()->getCell(*rangeIter);

            if (cell) {
                cell->getAlignment(oldAlignment);
            }

            if (alignment & Cell::ALIGNMENT_VERTICAL) {
                oldAlignment &= ~Cell::ALIGNMENT_VERTICAL;
            }
            if (alignment & Cell::ALIGNMENT_HORIZONTAL) {
                oldAlignment &= ~Cell::ALIGNMENT_HORIZONTAL;
            }

            getSheetPtr()->setAlignment(*rangeIter, alignment | oldAlignment);
        } while (rangeIter.next());
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Optional parameter must be either 'replace' or 'keep'");
        return nullptr;
    }
    Py_Return;
}

PyObject* SheetPy::getAlignment(PyObject* args)
{
    const char* strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getAlignment", &strAddress)) {
        return nullptr;
    }

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }


    int alignment;
    const Cell* cell = getSheetPtr()->getCell(address);
    if (cell && cell->getAlignment(alignment)) {
        PyObject* s = PySet_New(nullptr);

        if (alignment & Cell::ALIGNMENT_LEFT) {
            PySet_Add(s, PyUnicode_FromString("left"));
        }
        if (alignment & Cell::ALIGNMENT_HCENTER) {
            PySet_Add(s, PyUnicode_FromString("center"));
        }
        if (alignment & Cell::ALIGNMENT_RIGHT) {
            PySet_Add(s, PyUnicode_FromString("right"));
        }
        if (alignment & Cell::ALIGNMENT_TOP) {
            PySet_Add(s, PyUnicode_FromString("top"));
        }
        if (alignment & Cell::ALIGNMENT_VCENTER) {
            PySet_Add(s, PyUnicode_FromString("vcenter"));
        }
        if (alignment & Cell::ALIGNMENT_BOTTOM) {
            PySet_Add(s, PyUnicode_FromString("bottom"));
        }

        return s;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static float decodeFloat(const PyObject* obj)
{
    if (PyFloat_Check(obj)) {
        return PyFloat_AsDouble((PyObject*)obj);
    }
    else if (PyLong_Check(obj)) {
        return PyLong_AsLong((PyObject*)obj);
    }
    throw Base::TypeError("Float or integer expected");
}

static void decodeColor(PyObject* value, Color& c)
{
    if (PyTuple_Check(value)) {
        if (PyTuple_Size(value) < 3 || PyTuple_Size(value) > 4) {
            throw Base::TypeError("Tuple must be either of 3 or 4 floats/ints.");
        }

        c.r = decodeFloat(PyTuple_GetItem(value, 0));
        c.g = decodeFloat(PyTuple_GetItem(value, 1));
        c.b = decodeFloat(PyTuple_GetItem(value, 2));
        if (PyTuple_Size(value) == 4) {
            c.a = decodeFloat(PyTuple_GetItem(value, 3));
            return;
        }
        else {
            c.a = 1.0;
        }
    }
    else {
        throw Base::TypeError("Tuple required.");
    }
}

PyObject* SheetPy::setForeground(PyObject* args)
{
    try {
        const char* range;
        PyObject* value;
        Color c;

        if (!PyArg_ParseTuple(args, "sO:setForeground", &range, &value)) {
            return nullptr;
        }

        decodeColor(value, c);

        Range rangeIter(range);
        do {
            getSheetPtr()->setForeground(*rangeIter, c);
        } while (rangeIter.next());
        Py_Return;
    }
    catch (const Base::TypeError& e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return nullptr;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getForeground(PyObject* args)
{
    const char* strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getForeground", &strAddress)) {
        return nullptr;
    }

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Color c;
    const Cell* cell = getSheetPtr()->getCell(address);
    if (cell && cell->getForeground(c)) {
        PyObject* t = PyTuple_New(4);

        PyTuple_SetItem(t, 0, Py::new_reference_to(Py::Float(c.r)));
        PyTuple_SetItem(t, 1, Py::new_reference_to(Py::Float(c.g)));
        PyTuple_SetItem(t, 2, Py::new_reference_to(Py::Float(c.b)));
        PyTuple_SetItem(t, 3, Py::new_reference_to(Py::Float(c.a)));

        return t;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setBackground(PyObject* args)
{
    try {
        const char* strAddress;
        PyObject* value;
        Color c;

        if (!PyArg_ParseTuple(args, "sO:setBackground", &strAddress, &value)) {
            return nullptr;
        }

        decodeColor(value, c);
        Range rangeIter(strAddress);

        do {
            getSheetPtr()->setBackground(*rangeIter, c);
        } while (rangeIter.next());
        Py_Return;
    }
    catch (const Base::TypeError& e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return nullptr;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getBackground(PyObject* args)
{
    const char* strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:setStyle", &strAddress)) {
        return nullptr;
    }

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }

    Color c;
    const Cell* cell = getSheetPtr()->getCell(address);
    if (cell && cell->getBackground(c)) {
        PyObject* t = PyTuple_New(4);

        PyTuple_SetItem(t, 0, Py::new_reference_to(Py::Float(c.r)));
        PyTuple_SetItem(t, 1, Py::new_reference_to(Py::Float(c.g)));
        PyTuple_SetItem(t, 2, Py::new_reference_to(Py::Float(c.b)));
        PyTuple_SetItem(t, 3, Py::new_reference_to(Py::Float(c.a)));

        return t;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setColumnWidth(PyObject* args)
{
    const char* columnStr;
    int width;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "si:setColumnWidth", &columnStr, &width)) {
        return nullptr;
    }

    try {
        std::string cellAddr = std::string(columnStr) + "1";

        address = stringToAddress(cellAddr.c_str());
        getSheetPtr()->setColumnWidth(address.col(), width);
        Py_Return;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getColumnWidth(PyObject* args)
{
    const char* columnStr;

    if (!PyArg_ParseTuple(args, "s:getColumnWidth", &columnStr)) {
        return nullptr;
    }

    try {
        CellAddress address(std::string(columnStr) + "1");

        return Py::new_reference_to(Py::Long(getSheetPtr()->getColumnWidth(address.col())));
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::setRowHeight(PyObject* args)
{
    const char* rowStr;
    int height;

    if (!PyArg_ParseTuple(args, "si:setRowHeight", &rowStr, &height)) {
        return nullptr;
    }

    try {
        CellAddress address("A" + std::string(rowStr));

        getSheetPtr()->setRowHeight(address.row(), height);
        Py_Return;
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::getRowHeight(PyObject* args)
{
    const char* rowStr;

    if (!PyArg_ParseTuple(args, "s:getRowHeight", &rowStr)) {
        return nullptr;
    }

    try {
        CellAddress address("A" + std::string(rowStr));

        return Py::new_reference_to(Py::Long(getSheetPtr()->getRowHeight(address.row())));
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return nullptr;
    }
}

PyObject* SheetPy::touchCells(PyObject* args)
{
    const char* address;
    const char* address2 = nullptr;

    if (!PyArg_ParseTuple(args, "s|s:touchCells", &address, &address2)) {
        return nullptr;
    }

    PY_TRY
    {
        std::string a1 = getSheetPtr()->getAddressFromAlias(address);
        if (a1.empty()) {
            a1 = address;
        }

        std::string a2;
        if (!address2) {
            a2 = a1;
        }
        else {
            a2 = getSheetPtr()->getAddressFromAlias(address2);
            if (a2.empty()) {
                a2 = address2;
            }
        }
        getSheetPtr()->touchCells(Range(a1.c_str(), a2.c_str()));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* SheetPy::recomputeCells(PyObject* args)
{
    const char* address;
    const char* address2 = nullptr;

    if (!PyArg_ParseTuple(args, "s|s:touchCells", &address, &address2)) {
        return nullptr;
    }

    PY_TRY
    {
        std::string a1 = getSheetPtr()->getAddressFromAlias(address);
        if (a1.empty()) {
            a1 = address;
        }

        std::string a2;
        if (!address2) {
            a2 = a1;
        }
        else {
            a2 = getSheetPtr()->getAddressFromAlias(address2);
            if (a2.empty()) {
                a2 = address2;
            }
        }
        getSheetPtr()->recomputeCells(Range(a1.c_str(), a2.c_str()));
        Py_Return;
    }
    PY_CATCH;
}

PyObject* SheetPy::getUsedCells(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    auto usedCells = getSheetPtr()->getCells()->getUsedCells();
    Py::List pyCellList;
    for (const auto& cell : usedCells) {
        pyCellList.append(Py::String(cell.toString()));
    }
    return Py::new_reference_to(pyCellList);
}

PyObject* SheetPy::getUsedRange(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    auto usedRange = getSheetPtr()->getCells()->getUsedRange();
    Py::Tuple pyTuple(2);
    pyTuple[0] = Py::String(std::get<0>(usedRange).toString());
    pyTuple[1] = Py::String(std::get<1>(usedRange).toString());
    return Py::new_reference_to(pyTuple);
}

PyObject* SheetPy::getNonEmptyCells(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    auto nonEmptyCells = getSheetPtr()->getCells()->getNonEmptyCells();
    Py::List pyCellList;
    for (const auto& cell : nonEmptyCells) {
        pyCellList.append(Py::String(cell.toString()));
    }
    return Py::new_reference_to(pyCellList);
}

PyObject* SheetPy::getNonEmptyRange(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    auto nonEmptyRange = getSheetPtr()->getCells()->getNonEmptyRange();
    Py::Tuple pyTuple(2);
    pyTuple[0] = Py::String(std::get<0>(nonEmptyRange).toString());
    pyTuple[1] = Py::String(std::get<1>(nonEmptyRange).toString());
    return Py::new_reference_to(pyTuple);
}


// +++ custom attributes implementer ++++++++++++++++++++++++++++++++++++++++

PyObject* SheetPy::getCustomAttributes(const char*) const
{
    return nullptr;
}

int SheetPy::setCustomAttributes(const char*, PyObject*)
{
    return 0;
}
