/***************************************************************************
 *   Copyright (c) Eivind Kvedalen        (eivind@kvedalen.name)  2013     *
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
#include <Base/VectorPy.h>
#include <Base/Exception.h>
#include <App/Expression.h>

#include <Mod/Spreadsheet/App/Sheet.h>
#include <App/PropertyStandard.h>

// inclusion of the generated files (generated out of SheetPy.xml)
#include "SheetPy.h"
#include "SheetPy.cpp"

using namespace Spreadsheet;
using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string SheetPy::representation(void) const
{
    return std::string("<Sheet object>");
}

PyObject *SheetPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SheetPy and the Twin object
    return new SheetPy(new Sheet());
}

// constructor method
int SheetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

// +++ methodes implementer ++++++++++++++++++++++++++++++++++++++++++++++++

PyObject* SheetPy::set(PyObject *args)
{
    char *address;
    char *contents;

    if (!PyArg_ParseTuple(args, "ss:set", &address, &contents))
        return 0;

    try {
        getSheetPtr()->setCell(address, contents);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::get(PyObject *args)
{
    char *address;

    if (!PyArg_ParseTuple(args, "s:get", &address))
        return 0;

    App::Property * prop = this->getSheetPtr()->getPropertyByName(address);

    if (prop == 0) {
        PyErr_SetString(PyExc_ValueError, "Invalid address or property.");
        return 0;
    }
    return prop->getPyObject();
}

PyObject* SheetPy::getContents(PyObject *args)
{
    char *address;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:getContents", &address))
        return 0;

    try {        
        Sheet::addressToRowCol(address, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    std::string contents = this->getSheetPtr()->getCellString( row, col );
    return Py::new_reference_to( Py::String( contents ) );
}

PyObject* SheetPy::clear(PyObject *args)
{
    const char * address;
    int all = 1;

    if (!PyArg_ParseTuple(args, "s|p:clear", &address, &all))
        return 0;

    try {
        this->getSheetPtr()->clear(address, all);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::clearAll(PyObject *args)
{
    this->getSheetPtr()->clearAll();
    Py_Return;
}

PyObject* SheetPy::importFile(PyObject *args)
{
    const char * filename;
    const char * delimiter = "\t";
    const char * quoteChar = "\"";
    const char * escapeChar = "\\";

    if (!PyArg_ParseTuple(args, "s|sss:importFile", &filename, &delimiter, &quoteChar, &escapeChar))
        return 0;

    if (getSheetPtr()->importFromFile(filename, delimiter[0], quoteChar[0], escapeChar[0]))
        return Py::new_reference_to( Py::Boolean(true) );
    else
        return Py::new_reference_to( Py::Boolean(false) );
}

PyObject* SheetPy::exportFile(PyObject *args)
{
    const char * filename;
    const char * delimiter = "\t";
    const char * quoteChar = "\"";
    const char * escapeChar = "\\";

    if (!PyArg_ParseTuple(args, "s|sss:importFile", &filename, &delimiter, &quoteChar, &escapeChar))
        return 0;

    if (getSheetPtr()->exportToFile(filename, delimiter[0], quoteChar[0], escapeChar[0]))
        return Py::new_reference_to( Py::Boolean(true) );
    else
        return Py::new_reference_to( Py::Boolean(false) );
}

PyObject* SheetPy::mergeCells(PyObject *args)
{
    const char * range;
    std::string from;
    std::string to;

    if (!PyArg_ParseTuple(args, "s:mergeCells", &range))
        return 0;

    if (strchr(range, ':') == NULL) {
        return 0;
    }

    std::string s = range;
    from = s.substr(0, s.find(':'));
    to = s.substr(s.find(':') + 1);

    getSheetPtr()->mergeCells(from, to);
    Py_Return;
}

PyObject* SheetPy::splitCell(PyObject *args)
{
    const char * cell;

    if (!PyArg_ParseTuple(args, "s:splitCell", &cell))
        return 0;

    getSheetPtr()->splitCell(cell);
    Py_Return;
}

PyObject* SheetPy::setStyle(PyObject *args)
{

    int row, col;
    const char * cell;
    PyObject * value;
    std::set<std::string> style;
    const char * options = "replace";

    if (!PyArg_ParseTuple(args, "sO|s:setStyle", &cell, &value, &options))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    if (PySet_Check(value)) {
        PyObject * copy = PySet_New(value);

        while (PySet_Size(copy) > 0) {
            PyObject * item = PySet_Pop(copy);

            // check on the key:
            if (PyString_Check(item))
                style.insert(PyString_AsString(item));
            else {
                std::string error = std::string("type of the set need to be a string, not ") + item->ob_type->tp_name;
                PyErr_SetString(PyExc_TypeError, error.c_str());
                Py_DECREF(copy);
                return 0;
            }
        }
        Py_DECREF(copy);
    }
    else if (PyString_Check(value)) {
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = PyString_AsString(value);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            style.insert(*i);
    }
    else {
        std::string error = std::string("style must be either set or string, not ") + value->ob_type->tp_name;
        PyErr_SetString(PyExc_TypeError, error.c_str());
        return 0;
    }

    if (strcmp(options, "replace") == 0)
        getSheetPtr()->setStyle(row, col, style);
    else if (strcmp(options, "add") == 0) {
        std::set<std::string> oldStyle;

        // Get old styles first
        getSheetPtr()->getStyle(row, col, oldStyle);

        for (std::set<std::string>::const_iterator it = oldStyle.begin(); it != oldStyle.end(); ++it)
            style.insert(*it);

        // Set new style
        getSheetPtr()->setStyle(row, col, style);
    }
    else if (strcmp(options, "remove") == 0) {
        std::set<std::string> oldStyle;

        // Get old styles first
        getSheetPtr()->getStyle(row, col, oldStyle);

        for (std::set<std::string>::const_iterator it = style.begin(); it != style.end(); ++it)
            oldStyle.erase(*it);

        // Set new style
        getSheetPtr()->setStyle(row, col, oldStyle);
    }
    else if (strcmp(options, "invert") == 0) {
        std::set<std::string> oldStyle;
         std::set<std::string> newStyle;

        // Get old styles first
        getSheetPtr()->getStyle(row, col, oldStyle);
        newStyle = oldStyle;

        for (std::set<std::string>::const_iterator i = style.begin(); i != style.end(); ++i) {
            if (oldStyle.find(*i) == oldStyle.end())
                // Not found in oldstyle; add it to newStyle
                newStyle.insert(*i);
            else
                // Found in oldStyle, remove it from newStyle
                newStyle.erase(*i);
        }

        // Set new style
        getSheetPtr()->setStyle(row, col, newStyle);
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Optional parameter must be either 'replace', 'add', 'remove', or 'invert'");
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::getStyle(PyObject *args)
{
    const char * cell;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:getStyle", &cell))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    std::set<std::string> style;
    if (getSheetPtr()->getStyle(row, col, style)) {
        PyObject * s = PySet_New(NULL);

        for (std::set<std::string>::const_iterator i = style.begin(); i != style.end(); ++i)
            PySet_Add(s, PyString_FromString((*i).c_str()));

        return s;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setDisplayUnit(PyObject *args)
{
    int row, col;
    const char * cell;
    const char * value;

    if (!PyArg_ParseTuple(args, "ss:setDisplayUnit", &cell, &value))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);

        getSheetPtr()->setUnit(row, col, value);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::getDisplayUnit(PyObject *args)
{
    const char * cell;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:getDisplayUnit", &cell))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);

        Spreadsheet::Sheet::DisplayUnit unit;

        if ( getSheetPtr()->getUnit(row, col, unit) )
            return Py::new_reference_to( Py::String( unit.stringRep ) );
        else
            Py_Return;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::setAlignment(PyObject *args)
{
    int row, col;
    const char * cell;
    PyObject * value;
    int alignment = 0;
    const char * options = "replace";

    if (!PyArg_ParseTuple(args, "sO|s:setAlignment", &cell, &value, &options))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    if (PySet_Check(value)) {
        // Argument is a set of strings
        PyObject * copy = PySet_New(value);
        int n = PySet_Size(copy);

        while (n-- > 0) {
            PyObject * item = PySet_Pop(copy);

            if (PyString_Check(item))
                alignment = Sheet::decodeAlignment(PyString_AsString(item), alignment);
            else {
                std::string error = std::string("type of the key need to be a string, not") + item->ob_type->tp_name;
                PyErr_SetString(PyExc_TypeError, error.c_str());
                Py_DECREF(copy);
                return 0;
            }
        }

        Py_DECREF(copy);
    }
    else if (PyString_Check(value)) {
        // Argument is a string, combination of alignments, separated by the pipe character
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
        std::string line = PyString_AsString(value);
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            alignment = Sheet::decodeAlignment(*i, alignment);
    }
    else {
        std::string error = std::string("style must be either set or string, not ") + value->ob_type->tp_name;
        PyErr_SetString(PyExc_TypeError, error.c_str());
        return 0;
    }

    // Set alignment depending on 'options' variable
    if (strcmp(options, "replace") == 0)
        getSheetPtr()->setAlignment(row, col, alignment);
    else if (strcmp(options, "keep") == 0) {
        int oldAlignment = 0;

        getSheetPtr()->getAlignment(row, col, oldAlignment);

        if (alignment & 0x70)
            oldAlignment &= ~0x70;
        if (alignment & 0x07)
            oldAlignment &= ~0x07;

        getSheetPtr()->setAlignment(row, col, alignment | oldAlignment);
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Optional parameter must be either 'replace' or 'keep'");
        return 0;
    }
    Py_Return;
}

PyObject* SheetPy::getAlignment(PyObject *args)
{
    const char * cell;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:getAlignment", &cell))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }


    int alignment;
    if (getSheetPtr()->getAlignment(row, col, alignment)) {
        PyObject * s = PySet_New(NULL);

        if (alignment & 0x1)
            PySet_Add(s, PyString_FromString("left"));
        if (alignment & 0x2)
            PySet_Add(s, PyString_FromString("center"));
        if (alignment & 0x4)
            PySet_Add(s, PyString_FromString("right"));
        if (alignment & 0x10)
            PySet_Add(s, PyString_FromString("top"));
        if (alignment & 0x20)
            PySet_Add(s, PyString_FromString("vcenter"));
        if (alignment & 0x40)
            PySet_Add(s, PyString_FromString("bottom"));

        return s;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static float decodeFloat(const PyObject * obj)
{
    if (PyFloat_Check(obj))
        return PyFloat_AsDouble((PyObject *)obj);
    else if (PyInt_Check(obj))
        return PyInt_AsLong((PyObject *)obj);
    throw Base::TypeError("Float or integer expected");
}

static void decodeColor(PyObject * value, Color & c)
{
    if (PyTuple_Check(value)) {
        if (PyTuple_Size(value) < 3 || PyTuple_Size(value) > 4)
            throw Base::TypeError("Tuple must be either of 3 or 4 floats/ints.");

        c.r = decodeFloat(PyTuple_GetItem(value, 0));
        c.g = decodeFloat(PyTuple_GetItem(value, 1));
        c.b = decodeFloat(PyTuple_GetItem(value, 2));
        if (PyTuple_Size(value) == 4) {
            c.a = decodeFloat(PyTuple_GetItem(value, 3));
            return;
        }
        else
            c.a = 1.0;
    }
    else
        throw Base::TypeError("Tuple required.");
}

PyObject* SheetPy::setForeground(PyObject *args)
{
    try {
        int row, col;
        const char * cell;
        PyObject * value;
        Color c;

        if (!PyArg_ParseTuple(args, "sO:Give a value", &cell, &value))
            return 0;

        Sheet::addressToRowCol(cell, row, col);

        decodeColor(value, c);
        getSheetPtr()->setForeground(row, col, c);
        Py_Return;
    }
    catch (const Base::TypeError & e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getForeground(PyObject *args)
{
    const char * cell;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:setStyle", &cell))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Color c;
    if (getSheetPtr()->getForeground(row, col, c)) {
        PyObject * t = PyTuple_New(4);

        PyTuple_SetItem(t, 0, Py::new_reference_to( Py::Float(c.r) ));
        PyTuple_SetItem(t, 1, Py::new_reference_to( Py::Float(c.g) ));
        PyTuple_SetItem(t, 2, Py::new_reference_to( Py::Float(c.b) ));
        PyTuple_SetItem(t, 3, Py::new_reference_to( Py::Float(c.a) ));

        return t;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setBackground(PyObject *args)
{
    try {
        int row, col;
        const char * cell;
        PyObject * value;
        Color c;

        if (!PyArg_ParseTuple(args, "sO:Give a value", &cell, &value))
            return 0;

        Sheet::addressToRowCol(cell, row, col);

        decodeColor(value, c);
        getSheetPtr()->setBackground(row, col, c);
        Py_Return;
    }
    catch (const Base::TypeError & e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return 0;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getBackground(PyObject *args)
{
    const char * cell;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:setStyle", &cell))
        return 0;

    try {
        Sheet::addressToRowCol(cell, row, col);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Color c;
    if (getSheetPtr()->getBackground(row, col, c)) {
        PyObject * t = PyTuple_New(4);

        PyTuple_SetItem(t, 0, Py::new_reference_to( Py::Float(c.r) ));
        PyTuple_SetItem(t, 1, Py::new_reference_to( Py::Float(c.g) ));
        PyTuple_SetItem(t, 2, Py::new_reference_to( Py::Float(c.b) ));
        PyTuple_SetItem(t, 3, Py::new_reference_to( Py::Float(c.a) ));

        return t;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setColumnWidth(PyObject *args)
{
    const char * columnStr;
    int row, col, width;

    if (!PyArg_ParseTuple(args, "si:setColumnWidth", &columnStr, &width))
        return 0;

    try {
        std::string cellAddr = std::string(columnStr) + "1";

        Sheet::addressToRowCol(cellAddr.c_str(), row, col);
        getSheetPtr()->setColumnWidth(col, width);
        Py_Return;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getColumnWidth(PyObject *args)
{
    const char * columnStr;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:getColumnWidth", &columnStr))
        return 0;

    try {
        std::string cellAddr = std::string(columnStr) + "1";

        Sheet::addressToRowCol(cellAddr.c_str(), row, col);
        return Py::new_reference_to( Py::Int( getSheetPtr()->getColumnWidth(col) ) );
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::setRowHeight(PyObject *args)
{
    const char * rowStr;
    int row, col, height;

    if (!PyArg_ParseTuple(args, "si:setRowHeight", &rowStr, &height))
        return 0;

    try {
        std::string cellAddr = "A" + std::string(rowStr);

        Sheet::addressToRowCol(cellAddr.c_str(), row, col);
        getSheetPtr()->setRowHeight(row, height);
        Py_Return;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getRowHeight(PyObject *args)
{
    const char * rowStr;
    int row, col;

    if (!PyArg_ParseTuple(args, "s:getRowHeight", &rowStr))
        return 0;

    try {
        std::string cellAddr = "A" + std::string(rowStr);

        Sheet::addressToRowCol(cellAddr.c_str(), row, col);
        return Py::new_reference_to( Py::Int( getSheetPtr()->getRowHeight(row) ) );
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

// +++ custom attributes implementer ++++++++++++++++++++++++++++++++++++++++

PyObject *SheetPy::getCustomAttributes(const char* attr) const
{
    App::Property * prop = this->getSheetPtr()->getPropertyByName(attr);

    if (prop == 0) {
        PyErr_SetString(PyExc_ValueError, "Invalid address or property.");
        return 0;
    }
    return prop->getPyObject();
}

int SheetPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    int row, col;

    // Parse attr; if it looks like a cell address specifier, it probably is...
    char *contents;

    if (!PyArg_ParseTuple(obj, "s:setCustomAttributes", &contents))
        return 0;

    try {
        Sheet::addressToRowCol(attr, row, col);
        getSheetPtr()->setCell(row, col, contents);
        return 0;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}
