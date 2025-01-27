/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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
#include <App/DocumentObjectPy.h>

#include "SelectionFilterPy.h"


using namespace Gui;

SelectionFilterPy::SelectionFilterPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
    : Py::PythonClass<SelectionFilterPy>::PythonClass(self, args, kwds), filter("")
{
    const char* str;
    if (!PyArg_ParseTuple(args.ptr(), "s", &str)) {
        throw Py::Exception();
    }
    try {
        filter = SelectionFilter(str);
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(PyExc_SyntaxError, e.what());
    }
}

SelectionFilterPy::~SelectionFilterPy() = default;

Py::Object SelectionFilterPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "SelectionFilter";
    return Py::String(s_out.str());
}

Py::Object SelectionFilterPy::match()
{
    return Py::Boolean(filter.match());
}
PYCXX_NOARGS_METHOD_DECL(SelectionFilterPy, match)

Py::Object SelectionFilterPy::test(const Py::Tuple& args)
{
    PyObject * pcObj;
    char* text=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O!|s",
        &(App::DocumentObjectPy::Type), &pcObj, &text)) {
        throw Py::Exception();
    }

    auto docObj = static_cast<App::DocumentObjectPy*>(pcObj);

    return Py::Boolean(filter.test(docObj->getDocumentObjectPtr(),text));
}
PYCXX_VARARGS_METHOD_DECL(SelectionFilterPy, test)

Py::Object SelectionFilterPy::result()
{
    Py::List list;
    for (const auto& vec : filter.Result) {
        Py::Tuple tuple(vec.size());
        int index=0;
        for (auto sel : vec) {
            tuple[index++] = Py::asObject(sel.getPyObject());
        }
        list.append(tuple);
    }

    return list;
}
PYCXX_NOARGS_METHOD_DECL(SelectionFilterPy, result)

Py::Object SelectionFilterPy::setFilter(const Py::Tuple& args)
{
    char* text=nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s", &text)) {
        throw Py::Exception();
    }

    try {
        filter.setFilter(text);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(PyExc_SyntaxError, e.what());
    }
}
PYCXX_VARARGS_METHOD_DECL(SelectionFilterPy, setFilter)

Py::Object SelectionFilterPy::getFilter()
{
    return Py::String(filter.getFilter());
}
PYCXX_NOARGS_METHOD_DECL(SelectionFilterPy, getFilter)

void SelectionFilterPy::init_type()
{
    behaviors().name("Gui.SelectionFilter");
    behaviors().doc("Filter for certain selection\n"
        "Example strings are:\n"
        "\"SELECT Part::Feature SUBELEMENT Edge\",\n"
        "\"SELECT Part::Feature\", \n"
        "\"SELECT Part::Feature COUNT 1..5\"\n");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattro();
    behaviors().supportSetattro();
    PYCXX_ADD_NOARGS_METHOD(match, match,
        "Check if the current selection matches the filter");
    PYCXX_ADD_NOARGS_METHOD(result, result,
        "If match() returns True then with result() you get a list of the matching objects");
    PYCXX_ADD_VARARGS_METHOD(test, test,
        "test(Feature, SubName='')\n"
        "Test if a given object is described in the filter.\n"
        "If SubName is not empty the sub-element gets also tested.");
    PYCXX_ADD_VARARGS_METHOD(setFilter, setFilter,
        "Set a new selection filter from a string");
    PYCXX_ADD_NOARGS_METHOD(getFilter, getFilter,
        "Get the selection filter string");

    behaviors().readyType();
}
