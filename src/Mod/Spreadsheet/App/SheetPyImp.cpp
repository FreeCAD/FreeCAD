/***************************************************************************
 *   Copyright (c) Eivind Kvedalen        (eivind@kvedalen.name)  2015     *
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
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Mod/Spreadsheet/App/Sheet.h>
#include <App/PropertyStandard.h>
#include "Utils.h"
#include <App/Range.h>

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

// +++ methods implementer ++++++++++++++++++++++++++++++++++++++++++++++++

PyObject* SheetPy::set(PyObject *args)
{
    char *address;
    char *contents;


    if (!PyArg_ParseTuple(args, "ss:set", &address, &contents))
        return 0;

    try {
        Sheet * sheet = getSheetPtr();
        std::string cellAddress = sheet->getAddressFromAlias(address);

        /* Check to see if address is really an alias first */
        if (cellAddress.size() > 0)
            sheet->setCell(cellAddress.c_str(), contents);
        else {
            Range rangeIter(address);

            do {
                sheet->setCell(rangeIter.address().c_str(), contents);
            } while (rangeIter.next());
        }
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::get(PyObject *args)
{
    const char *address;
    const char *address2=0;

    if (!PyArg_ParseTuple(args, "s|s:get", &address, &address2))
        return 0;

    PY_TRY {
        if(address2) {
            auto a1 = getSheetPtr()->getAddressFromAlias(address);
            if(a1.empty())
                a1 = address;
            auto a2 = getSheetPtr()->getAddressFromAlias(address2);
            if(a2.empty())
                a2 = address2;
            Range range(a1.c_str(),a2.c_str());
            Py::Tuple tuple(range.size());
            int i=0;
            do {
                App::Property *prop = getSheetPtr()->getPropertyByName(range.address().c_str());
                if(!prop) {
                    PyErr_Format(PyExc_ValueError, "Invalid address '%s' in range %s:%s",
                            range.address().c_str(), address, address2);
                    return 0;
                }
                tuple.setItem(i++,Py::Object(prop->getPyObject(),true));
            }while(range.next());
            return Py::new_reference_to(tuple);
        }
    }PY_CATCH;

    App::Property * prop = this->getSheetPtr()->getPropertyByName(address);

    if (prop == 0) {
        PyErr_Format(PyExc_ValueError, 
                "Invalid cell address or property: %s",address);
        return 0;
    }
    return prop->getPyObject();
}

PyObject* SheetPy::getContents(PyObject *args)
{
    char *strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getContents", &strAddress))
        return 0;

    PY_TRY {
        try {        
            Sheet * sheet = getSheetPtr();
            std::string addr = sheet->getAddressFromAlias(strAddress);

            if (addr.empty())
                address = stringToAddress(strAddress);
            else
                address = stringToAddress(addr.c_str());
        }
        catch (const Base::Exception & e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return 0;
        }

        std::string contents;
        const Cell * cell = this->getSheetPtr()->getCell(address);

        if (cell)
            cell->getStringContent( contents );

        return Py::new_reference_to( Py::String( contents ) );
    } PY_CATCH
}

PyObject* SheetPy::clear(PyObject *args)
{
    const char * strAddress;
    int all = 1;

    if (!PyArg_ParseTuple(args, "s|p:clear", &strAddress, &all))
        return 0;

    try {
        Range rangeIter(strAddress);
        do {
            this->getSheetPtr()->clear(*rangeIter, all);
        } while (rangeIter.next());
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::clearAll(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

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

    if (!PyArg_ParseTuple(args, "s|sss:exportFile", &filename, &delimiter, &quoteChar, &escapeChar))
        return 0;

    if (getSheetPtr()->exportToFile(filename, delimiter[0], quoteChar[0], escapeChar[0]))
        return Py::new_reference_to( Py::Boolean(true) );
    else
        return Py::new_reference_to( Py::Boolean(false) );
}

PyObject* SheetPy::mergeCells(PyObject *args)
{
    const char * range;

    if (!PyArg_ParseTuple(args, "s:mergeCells", &range))
        return 0;

    getSheetPtr()->mergeCells(Range(range));
    Py_Return;
}

PyObject* SheetPy::splitCell(PyObject *args)
{
    const char * strAddress;

    if (!PyArg_ParseTuple(args, "s:splitCell", &strAddress))
        return 0;

    CellAddress address;
    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    getSheetPtr()->splitCell(address);
    Py_Return;
}

PyObject* SheetPy::insertColumns(PyObject *args)
{
    const char * column;
    int count;

    if (!PyArg_ParseTuple(args, "si:insertColumns", &column, &count))
        return 0;

    getSheetPtr()->insertColumns(decodeColumn(column), count);
    Py_Return;
}

PyObject* SheetPy::removeColumns(PyObject *args)
{
    const char * column;
    int count;

    if (!PyArg_ParseTuple(args, "si:removeColumns", &column, &count))
        return 0;

    getSheetPtr()->removeColumns(decodeColumn(column), count);
    Py_Return;
}

PyObject* SheetPy::insertRows(PyObject *args)
{
    const char * row;
    int count;

    if (!PyArg_ParseTuple(args, "si:insertRows", &row, &count))
        return 0;

    getSheetPtr()->insertRows(decodeRow(std::string(row)), count);
    Py_Return;
}

PyObject* SheetPy::removeRows(PyObject *args)
{
    const char * row;
    int count;

    if (!PyArg_ParseTuple(args, "si:removeRows", &row, &count))
        return 0;

    getSheetPtr()->removeRows(decodeRow(std::string(row)), count);
    Py_Return;
}

PyObject* SheetPy::setStyle(PyObject *args)
{
    const char * cell;
    PyObject * value;
    std::set<std::string> style;
    const char * options = "replace";

    if (!PyArg_ParseTuple(args, "sO|s:setStyle", &cell, &value, &options))
        return 0;

    if (PySet_Check(value)) {
        PyObject * copy = PySet_New(value);

        while (PySet_Size(copy) > 0) {
            PyObject * item = PySet_Pop(copy);

            // check on the key:
#if PY_MAJOR_VERSION >= 3
            if (PyUnicode_Check(item))
                style.insert(PyUnicode_AsUTF8(item));
#else
            if (PyString_Check(item))
                style.insert(PyString_AsString(item));
#endif
            else {
                std::string error = std::string("type of the set need to be a string, not ") + item->ob_type->tp_name;
                PyErr_SetString(PyExc_TypeError, error.c_str());
                Py_DECREF(copy);
                return 0;
            }
        }
        Py_DECREF(copy);
    }
#if PY_MAJOR_VERSION >= 3
    else if (PyUnicode_Check(value)) {
#else
    else if (PyString_Check(value)) {
#endif
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
#if PY_MAJOR_VERSION >= 3
        std::string line = PyUnicode_AsUTF8(value);
#else
        std::string line = PyString_AsString(value);
#endif
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i)
            style.insert(*i);
    }
    else {
        std::string error = std::string("style must be either set or string, not ") + value->ob_type->tp_name;
        PyErr_SetString(PyExc_TypeError, error.c_str());
        return 0;
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
            const Cell * cell = getSheetPtr()->getCell(*rangeIter);

            // Get old styles first
            if (cell)
                cell->getStyle(oldStyle);

            for (std::set<std::string>::const_iterator it = oldStyle.begin(); it != oldStyle.end(); ++it)
                style.insert(*it);

            // Set new style
            getSheetPtr()->setStyle(*rangeIter, style);
        } while (rangeIter.next());
    }
    else if (strcmp(options, "remove") == 0) {
        Range rangeIter(cell);

        do {
            std::set<std::string> oldStyle;
            const Cell * cell = getSheetPtr()->getCell(*rangeIter);

            // Get old styles first
            if (cell)
                cell->getStyle(oldStyle);

            for (std::set<std::string>::const_iterator it = style.begin(); it != style.end(); ++it)
                oldStyle.erase(*it);

            // Set new style
            getSheetPtr()->setStyle(*rangeIter, oldStyle);
        } while (rangeIter.next());
    }
    else if (strcmp(options, "invert") == 0) {
        Range rangeIter(cell);

        do {
            std::set<std::string> oldStyle;
            std::set<std::string> newStyle;
            const Cell * cell = getSheetPtr()->getCell(*rangeIter);

            // Get old styles first
            if (cell) {
                cell->getStyle(oldStyle);
                newStyle = oldStyle;
            }

            for (std::set<std::string>::const_iterator i = style.begin(); i != style.end(); ++i) {
                if (oldStyle.find(*i) == oldStyle.end())
                    // Not found in oldstyle; add it to newStyle
                    newStyle.insert(*i);
                else
                    // Found in oldStyle, remove it from newStyle
                    newStyle.erase(*i);
            }

            // Set new style
            getSheetPtr()->setStyle(*rangeIter, newStyle);
        } while (rangeIter.next());
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Optional parameter must be either 'replace', 'add', 'remove', or 'invert'");
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::getStyle(PyObject *args)
{
    const char * strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getStyle", &strAddress))
        return 0;

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    std::set<std::string> style;
    const Cell * cell = getSheetPtr()->getCell(address);

    if (cell && cell->getStyle(style)) {
        PyObject * s = PySet_New(NULL);

        for (std::set<std::string>::const_iterator i = style.begin(); i != style.end(); ++i)
#if PY_MAJOR_VERSION >= 3
            PySet_Add(s, PyUnicode_FromString((*i).c_str()));
#else
            PySet_Add(s, PyString_FromString((*i).c_str()));
#endif

        return s;
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

PyObject* SheetPy::setDisplayUnit(PyObject *args)
{
    const char * cell;
    const char * value;

    if (!PyArg_ParseTuple(args, "ss:setDisplayUnit", &cell, &value))
        return 0;

    try {
        Range rangeIter(cell);

        do {
            getSheetPtr()->setDisplayUnit(*rangeIter, value);
        } while (rangeIter.next());
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* SheetPy::setAlias(PyObject *args)
{
    CellAddress address;
    const char * strAddress;
    PyObject * value;

    if (!PyArg_ParseTuple(args, "sO:setAlias", &strAddress, &value))
        return 0;

    try {
        address = stringToAddress(strAddress);
        if (PyUnicode_Check(value))
#if PY_MAJOR_VERSION >= 3
            getSheetPtr()->setAlias(address, PyUnicode_AsUTF8(value));
#else
        {
            PyObject* unicode = PyUnicode_AsUTF8String(value);
            getSheetPtr()->setAlias(address, PyString_AsString(unicode));
            Py_DECREF(unicode);            
        }
        else if (PyString_Check(value))
            getSheetPtr()->setAlias(address, PyString_AsString(value));
#endif
        else if (value == Py_None)
            getSheetPtr()->setAlias(address, "");
        else
            throw Base::TypeError("String or None expected");

        Py_Return;
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getAlias(PyObject *args)
{
    const char * strAddress;

    if (!PyArg_ParseTuple(args, "s:getAlias", &strAddress))
        return 0;

    try {
        CellAddress address(strAddress);
        const Cell * cell = getSheetPtr()->getCell(address);
        std::string alias;

        if (cell && cell->getAlias(alias))
            return Py::new_reference_to( Py::String( alias ) );
        else {
            Py_INCREF(Py_None);
            return Py_None;
        }
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getCellFromAlias(PyObject *args)
{
    const char * alias;

    if (!PyArg_ParseTuple(args, "s:getAlias", &alias))
        return 0;

    try {
        std::string address = getSheetPtr()->getAddressFromAlias(alias);

        if (address.size() > 0)
            return Py::new_reference_to( Py::String( address ) );
        else {
            Py_INCREF(Py_None);
            return Py_None;
        }
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getDisplayUnit(PyObject *args)
{
    const char * strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getDisplayUnit", &strAddress))
        return 0;

    try {
        address = stringToAddress(strAddress);

        Spreadsheet::DisplayUnit unit;

        const Cell * cell = getSheetPtr()->getCell(address);

        if ( cell && cell->getDisplayUnit(unit) )
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
    const char * cell;
    PyObject * value;
    int alignment = 0;
    const char * options = "replace";

    if (!PyArg_ParseTuple(args, "sO|s:setAlignment", &cell, &value, &options))
        return 0;

    if (PySet_Check(value)) {
        // Argument is a set of strings
        PyObject * copy = PySet_New(value);
        int n = PySet_Size(copy);

        while (n-- > 0) {
            PyObject * item = PySet_Pop(copy);

#if PY_MAJOR_VERSION >= 3
            if (PyUnicode_Check(item))
                alignment = Cell::decodeAlignment(PyUnicode_AsUTF8(item), alignment);
#else
            if (PyString_Check(item))
                alignment = Cell::decodeAlignment(PyString_AsString(item), alignment);
#endif
            else {
                std::string error = std::string("type of the key need to be a string, not") + item->ob_type->tp_name;
                PyErr_SetString(PyExc_TypeError, error.c_str());
                Py_DECREF(copy);
                return 0;
            }
        }

        Py_DECREF(copy);
    }
#if PY_MAJOR_VERSION >= 3
    else if (PyUnicode_Check(value)) {
#else
    else if (PyString_Check(value)) {
#endif
        // Argument is a string, combination of alignments, separated by the pipe character
        using namespace boost;

        escaped_list_separator<char> e('\0', '|', '\0');
#if PY_MAJOR_VERSION >= 3
        std::string line = PyUnicode_AsUTF8(value);
#else
        std::string line = PyString_AsString(value);
#endif
        tokenizer<escaped_list_separator<char> > tok(line, e);

        for(tokenizer<escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end();++i) {
            if(i->size())
                alignment = Cell::decodeAlignment(*i, alignment);
        }
    }
    else {
        std::string error = std::string("style must be either set or string, not ") + value->ob_type->tp_name;
        PyErr_SetString(PyExc_TypeError, error.c_str());
        return 0;
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
            const Cell * cell = getSheetPtr()->getCell(*rangeIter);

            if (cell)
                cell->getAlignment(oldAlignment);

            if (alignment & Cell::ALIGNMENT_VERTICAL)
                oldAlignment &= ~Cell::ALIGNMENT_VERTICAL;
            if (alignment & Cell::ALIGNMENT_HORIZONTAL)
                oldAlignment &= ~Cell::ALIGNMENT_HORIZONTAL;

            getSheetPtr()->setAlignment(*rangeIter, alignment | oldAlignment);
        } while (rangeIter.next());
    }
    else {
        PyErr_SetString(PyExc_ValueError, "Optional parameter must be either 'replace' or 'keep'");
        return 0;
    }
    Py_Return;
}

PyObject* SheetPy::getAlignment(PyObject *args)
{
    const char * strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getAlignment", &strAddress))
        return 0;

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }


    int alignment;
    const  Cell * cell = getSheetPtr()->getCell(address);
    if (cell && cell->getAlignment(alignment)) {
        PyObject * s = PySet_New(NULL);

#if PY_MAJOR_VERSION >= 3
        if (alignment & Cell::ALIGNMENT_LEFT)
            PySet_Add(s, PyUnicode_FromString("left"));
        if (alignment & Cell::ALIGNMENT_HCENTER)
            PySet_Add(s, PyUnicode_FromString("center"));
        if (alignment & Cell::ALIGNMENT_RIGHT)
            PySet_Add(s, PyUnicode_FromString("right"));
        if (alignment & Cell::ALIGNMENT_TOP)
            PySet_Add(s, PyUnicode_FromString("top"));
        if (alignment & Cell::ALIGNMENT_VCENTER)
            PySet_Add(s, PyUnicode_FromString("vcenter"));
        if (alignment & Cell::ALIGNMENT_BOTTOM)
            PySet_Add(s, PyUnicode_FromString("bottom"));
#else
        if (alignment & Cell::ALIGNMENT_LEFT)
            PySet_Add(s, PyString_FromString("left"));
        if (alignment & Cell::ALIGNMENT_HCENTER)
            PySet_Add(s, PyString_FromString("center"));
        if (alignment & Cell::ALIGNMENT_RIGHT)
            PySet_Add(s, PyString_FromString("right"));
        if (alignment & Cell::ALIGNMENT_TOP)
            PySet_Add(s, PyString_FromString("top"));
        if (alignment & Cell::ALIGNMENT_VCENTER)
            PySet_Add(s, PyString_FromString("vcenter"));
        if (alignment & Cell::ALIGNMENT_BOTTOM)
            PySet_Add(s, PyString_FromString("bottom"));
#endif

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
#if PY_MAJOR_VERSION >= 3
    else if (PyLong_Check(obj))
        return PyLong_AsLong((PyObject *)obj);
#else
    else if (PyInt_Check(obj))
        return PyInt_AsLong((PyObject *)obj);
#endif
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
        const char * range;
        PyObject * value;
        Color c;

        if (!PyArg_ParseTuple(args, "sO:setForeground", &range, &value))
            return 0;

        decodeColor(value, c);

        Range rangeIter(range);
        do {
            getSheetPtr()->setForeground(*rangeIter, c);
        } while (rangeIter.next());
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
    const char * strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getForeground", &strAddress))
        return 0;

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Color c;
    const Cell * cell = getSheetPtr()->getCell(address);
    if (cell && cell->getForeground(c)) {
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
        const char * strAddress;
        PyObject * value;
        Color c;

        if (!PyArg_ParseTuple(args, "sO:setBackground", &strAddress, &value))
            return 0;

        decodeColor(value, c);
        Range rangeIter(strAddress);

        do {
            getSheetPtr()->setBackground(*rangeIter, c);
        } while (rangeIter.next());
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
    const char * strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:setStyle", &strAddress))
        return 0;

    try {
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    Color c;
    const Cell * cell = getSheetPtr()->getCell(address);
    if (cell && cell->getBackground(c)) {
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
    int width;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "si:setColumnWidth", &columnStr, &width))
        return 0;

    try {
        std::string cellAddr = std::string(columnStr) + "1";

        address = stringToAddress(cellAddr.c_str());
        getSheetPtr()->setColumnWidth(address.col(), width);
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

    if (!PyArg_ParseTuple(args, "s:getColumnWidth", &columnStr))
        return 0;

    try {
        CellAddress address(std::string(columnStr) + "1");

        return Py::new_reference_to( Py::Long( getSheetPtr()->getColumnWidth(address.col()) ) );
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::setRowHeight(PyObject *args)
{
    const char * rowStr;
    int height;

    if (!PyArg_ParseTuple(args, "si:setRowHeight", &rowStr, &height))
        return 0;

    try {
        CellAddress address("A" + std::string(rowStr));

        getSheetPtr()->setRowHeight(address.row(), height);
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

    if (!PyArg_ParseTuple(args, "s:getRowHeight", &rowStr))
        return 0;

    try {
        CellAddress address("A" + std::string(rowStr));

        return Py::new_reference_to( Py::Long( getSheetPtr()->getRowHeight(address.row()) ) );
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }
}

PyObject* SheetPy::getEditMode(PyObject *args)
{
    char *strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "s:getEditMode", &strAddress))
        return 0;

    try {        
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    std::string contents;
    const Cell * cell = this->getSheetPtr()->getCell(address);

    const char *mode;
    if (cell) {
        switch(cell->getEditMode()) {
        case Cell::EditNormal:
            mode = "normal";
            break;
        case Cell::EditButton:
            mode = "button";
            break;
        case Cell::EditCombo:
            mode = "combo";
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "unknown edit mode");
            return 0;
        }
    }

    return Py::new_reference_to( Py::String( mode ) );
}

PyObject* SheetPy::setEditMode(PyObject *args)
{
    char *mode;
    char *strAddress;
    CellAddress address;

    if (!PyArg_ParseTuple(args, "ss:setEditMode", &strAddress,&mode))
        return 0;

    try {        
        address = stringToAddress(strAddress);
    }
    catch (const Base::Exception & e) {
        PyErr_SetString(PyExc_ValueError, e.what());
        return 0;
    }

    std::string contents;
    Cell * cell = this->getSheetPtr()->getCell(address);

    if (cell) {
        Cell::EditMode m;
        if(strcmp(mode,"normal")==0)
            m = Cell::EditNormal;
        else if(strcmp(mode,"button")==0)
            m = Cell::EditButton;
        else if(strcmp(mode,"combo")==0)
            m = Cell::EditCombo;
        else {
            PyErr_SetString(PyExc_ValueError, "unknown edit mode");
            return 0;
        }
        PY_TRY {
            cell->setEditMode(m);
        }PY_CATCH
    }

    Py_Return;
}

PyObject* SheetPy::row(PyObject *args) {
    int offset=0;
    if (!PyArg_ParseTuple(args, "|i:row", &offset))
        return 0;
    PY_TRY {
        return Py::new_reference_to(Py::String(getSheetPtr()->getRow(offset)));
    }PY_CATCH
}

PyObject* SheetPy::column(PyObject *args) {
    int offset=0;
    if (!PyArg_ParseTuple(args, "|i:column", &offset))
        return 0;
    PY_TRY {
        return Py::new_reference_to(Py::String(getSheetPtr()->getColumn(offset)));
    }PY_CATCH
}

PyObject *SheetPy::touchCells(PyObject *args) {
    const char *address;
    const char *address2=0;

    if (!PyArg_ParseTuple(args, "s|s:touchCells", &address, &address2))
        return 0;

    PY_TRY {
        std::string a1 = getSheetPtr()->getAddressFromAlias(address);
        if(a1.empty())
            a1 = address;

        std::string a2;
        if(!address2) {
            a2 = a1;
        } else {
            a2 = getSheetPtr()->getAddressFromAlias(address2);
            if(a2.empty())
                a2 = address2;
        }
        getSheetPtr()->touchCells(Range(a1.c_str(),a2.c_str()));
        Py_Return;
    }PY_CATCH;
}

// +++ custom attributes implementer ++++++++++++++++++++++++++++++++++++++++

PyObject *SheetPy::getCustomAttributes(const char*) const
{
    return 0;
}

int SheetPy::setCustomAttributes(const char* , PyObject* )
{
    return 0;
}
