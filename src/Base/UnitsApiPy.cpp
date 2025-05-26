/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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
#ifndef _PreComp_
#include <memory>
#endif

#include <CXX/Objects.hxx>

#include "UnitsApi.h"
#include "Quantity.h"
#include "QuantityPy.h"


using namespace Base;

//**************************************************************************
// Python stuff of UnitsApi

PyMethodDef UnitsApi::Methods[] = {
    {"parseQuantity",
     sParseQuantity,
     METH_VARARGS,
     "parseQuantity(string) -> Base.Quantity()\n\n"
     "calculate a mathematical expression with units to a quantity object. \n"
     "can be used for simple unit translation like: \n"
     "parseQuantity('10m')\n"
     "or for more complex espressions:\n"
     "parseQuantity('sin(pi)/50.0 m/s^2')\n"},
    {"listSchemas",
     sListSchemas,
     METH_VARARGS,
     "listSchemas() -> a tuple of schemas\n\n"
     "listSchemas(int) -> description of the given schema\n\n"},
    {"getSchema",
     sGetSchema,
     METH_VARARGS,
     "getSchema() -> int\n\n"
     "The int is the position of the tuple returned by listSchemas"},
    {"setSchema",
     sSetSchema,
     METH_VARARGS,
     "setSchema(int) -> None\n\n"
     "Sets the current schema to the given number, if possible"},
    {"schemaTranslate",
     sSchemaTranslate,
     METH_VARARGS,
     "schemaTranslate(Quantity, int) -> tuple\n\n"
     "Translate a quantity to a given schema"},
    {"toNumber",
     sToNumber,
     METH_VARARGS,
     "toNumber(Quantity or float, [format='g', decimals=-1]) -> str\n\n"
     "Convert a quantity or float to a string"},

    {nullptr, nullptr, 0, nullptr} /* Sentinel */
};

PyObject* UnitsApi::sParseQuantity(PyObject* /*self*/, PyObject* args)
{
    char* pstr {};
    if (!PyArg_ParseTuple(args, "et", "utf-8", &pstr)) {
        return nullptr;
    }

    const std::string str {pstr};
    PyMem_Free(pstr);
    try {
        return new QuantityPy(new Quantity(Quantity::parse(str)));
    }
    catch (const ParserError&) {
        PyErr_Format(PyExc_ValueError, "invalid unit expression: '%s'\n", str.c_str());
        return nullptr;
    }
}

PyObject* UnitsApi::sListSchemas(PyObject* /*self*/, PyObject* args)
{
    auto names = UnitsApi::getNames();
    const int num = static_cast<int>(names.size());

    if (PyArg_ParseTuple(args, "")) {
        Py::Tuple tuple {num};

        auto addItem = [&, i {0}](const std::string& name) mutable {
            tuple.setItem(i++, Py::String {name.c_str()});
        };

        std::for_each(names.begin(), names.end(), addItem);

        return Py::new_reference_to(tuple);
    }

    PyErr_Clear();
    int index {};
    if (PyArg_ParseTuple(args, "i", &index)) {
        if (index < 0 || index >= num) {
            PyErr_SetString(PyExc_ValueError, "invalid schema value");
            return nullptr;
        }

        const auto description = schemas->descriptions().at(index);
        return Py_BuildValue("s", description.c_str());
    }

    PyErr_SetString(PyExc_TypeError, "int or empty argument list expected");
    return nullptr;
}

PyObject* UnitsApi::sGetSchema(PyObject* /*self*/, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    return Py_BuildValue("i", schemas->currentSchema()->getNum());
}

PyObject* UnitsApi::sSetSchema(PyObject* /*self*/, PyObject* args)
{
    PyErr_Clear();
    int index {};
    if (PyArg_ParseTuple(args, "i", &index) != 0) {

        if (index < 0 || index >= static_cast<int>(count())) {
            PyErr_SetString(PyExc_ValueError, "invalid schema value");
            return nullptr;
        }

        schemas->select(index);
    }
    Py_Return;
}

PyObject* UnitsApi::sSchemaTranslate(PyObject* /*self*/, PyObject* args)
{
    PyObject* py {};
    int index {};
    if (!PyArg_ParseTuple(args, "O!i", &QuantityPy::Type, &py, &index)) {
        return nullptr;
    }

    if (index < 0 || index >= static_cast<int>(count())) {
        PyErr_SetString(PyExc_ValueError,
                        std::string {"invalid schema index: " + std::to_string(index)}.c_str());
        return nullptr;
    }

    const Quantity quant {*static_cast<QuantityPy*>(py)->getQuantityPtr()};

    double factor {};
    std::string unitStr;
    auto schema = std::make_unique<UnitsSchema>(schemas->spec(index));
    const std::string unitStrLocalised = schema->translate(quant, factor, unitStr);

    Py::Tuple res {3};
    res[0] = Py::String {unitStrLocalised, "utf-8"};
    res[1] = Py::Float {factor};
    res[2] = Py::String {unitStr, "utf-8"};

    return Py::new_reference_to(res);
}

PyObject* UnitsApi::sToNumber(PyObject* /*self*/, PyObject* args)
{
    double value {};
    const char* format = "g";
    int decimals {};
    do {
        PyObject* py {};
        if (PyArg_ParseTuple(args, "O!|si", &(QuantityPy::Type), &py, &format, &decimals)) {
            value = static_cast<QuantityPy*>(py)->getQuantityPtr()->getValue();
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args, "d|si", &value, &format, &decimals)) {
            break;
        }

        PyErr_SetString(PyExc_TypeError, "toNumber(Quantity or float, [format='g', decimals=-1])");
        return nullptr;
    } while (false);

    if (strlen(format) != 1) {
        PyErr_SetString(PyExc_ValueError, "Format string hasn't length of 1");
        return nullptr;
    }

    bool ok {};
    QuantityFormat qf;
    qf.format = QuantityFormat::toFormat(format[0], &ok);
    qf.precision = decimals;

    if (!ok) {
        PyErr_SetString(PyExc_ValueError, "Invalid format string");
        return nullptr;
    }

    return Py::new_reference_to(Py::String(toNumber(value, qf)));
}
