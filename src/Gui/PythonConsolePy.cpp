/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PythonConsolePy.h"
#include "PythonConsole.h"


using namespace Gui;

void PythonStdout::init_type()
{
    behaviors().name("PythonStdout");
    behaviors().doc("Redirection of stdout to FreeCAD's Python console window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonStdout::write,"write()");
    add_varargs_method("flush",&PythonStdout::flush,"flush()");
    add_noargs_method("isatty",&PythonStdout::isatty,"isatty()");
}

PythonStdout::PythonStdout(PythonConsole *pc)
  : pyConsole(pc)
{
}

PythonStdout::~PythonStdout() = default;

Py::Object PythonStdout::getattr(const char *name)
{
    if (strcmp(name, "softspace") == 0) {
        int i=0;
        return Py::Int(i);
    }
    return getattr_methods(name);
}

Py::Object PythonStdout::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonStdout";
    return Py::String(s_out.str());
}

Py::Object PythonStdout::write(const Py::Tuple& args)
{
    PyObject* output;
    if (!PyArg_ParseTuple(args.ptr(), "O!",&PyUnicode_Type, &output))
        throw Py::TypeError("PythonStdout.write() takes exactly one argument of type str");

    PyObject* unicode = PyUnicode_AsEncodedString(output, "utf-8", nullptr);
    if (unicode) {
        const char* string = PyBytes_AsString(unicode);
        int maxlen = qstrlen(string) > 10000 ? 10000 : -1;
        pyConsole->insertPythonOutput(QString::fromUtf8(string, maxlen));
        Py_DECREF(unicode);
    }

    return Py::None();
}

Py::Object PythonStdout::flush(const Py::Tuple&)
{
    return Py::None();
}

Py::Object PythonStdout::isatty()
{
    return Py::False();
}

// -------------------------------------------------------------------------

void PythonStderr::init_type()
{
    behaviors().name("PythonStderr");
    behaviors().doc("Redirection of stdout to FreeCAD's Python console window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonStderr::write,"write()");
    add_varargs_method("flush",&PythonStderr::flush,"flush()");
    add_noargs_method("isatty",&PythonStderr::isatty,"isatty()");
}

PythonStderr::PythonStderr(PythonConsole *pc)
  : pyConsole(pc)
{
}

PythonStderr::~PythonStderr() = default;

Py::Object PythonStderr::getattr(const char *name)
{
    if (strcmp(name, "softspace") == 0) {
        int i=0;
        return Py::Int(i);
    }
    return getattr_methods(name);
}

Py::Object PythonStderr::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonStderr";
    return Py::String(s_out.str());
}

Py::Object PythonStderr::write(const Py::Tuple& args)
{
    PyObject* output;
    if (!PyArg_ParseTuple(args.ptr(), "O!",&PyUnicode_Type, &output))
        throw Py::TypeError("PythonStderr.write() takes exactly one argument of type str");

    PyObject* unicode = PyUnicode_AsEncodedString(output, "utf-8", nullptr);
    if (unicode) {
        const char* string = PyBytes_AsString(unicode);
        int maxlen = qstrlen(string) > 10000 ? 10000 : -1;
        pyConsole->insertPythonError(QString::fromUtf8(string, maxlen));
        Py_DECREF(unicode);
    }

    return Py::None();
}

Py::Object PythonStderr::flush(const Py::Tuple&)
{
    return Py::None();
}

Py::Object PythonStderr::isatty()
{
    return Py::False();
}

// -------------------------------------------------------------------------

void OutputStdout::init_type()
{
    behaviors().name("OutputStdout");
    behaviors().doc("Redirection of stdout to FreeCAD's report view");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&OutputStdout::write,"write()");
    add_varargs_method("flush",&OutputStdout::flush,"flush()");
    add_noargs_method("isatty",&OutputStdout::isatty,"isatty()");
}

OutputStdout::OutputStdout() = default;

OutputStdout::~OutputStdout() = default;

Py::Object OutputStdout::getattr(const char *name)
{
    if (strcmp(name, "softspace") == 0) {
        int i=0;
        return Py::Int(i);
    }
    return getattr_methods(name);
}

Py::Object OutputStdout::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "OutputStdout";
    return Py::String(s_out.str());
}

Py::Object OutputStdout::write(const Py::Tuple& args)
{
    PyObject* output;
    if (!PyArg_ParseTuple(args.ptr(), "O!",&PyUnicode_Type, &output))
        throw Py::TypeError("OutputStdout.write() takes exactly one argument of type str");

    PyObject* unicode = PyUnicode_AsEncodedString(output, "utf-8", nullptr);
    if (unicode) {
        const char* string = PyBytes_AsString(unicode);
        Base::Console().Message("%s",string);
        Py_DECREF(unicode);
    }

    return Py::None();
}

Py::Object OutputStdout::flush(const Py::Tuple&)
{
    return Py::None();
}

Py::Object OutputStdout::isatty()
{
    return Py::False();
}


// -------------------------------------------------------------------------

void OutputStderr::init_type()
{
    behaviors().name("OutputStderr");
    behaviors().doc("Redirection of stdout to FreeCAD's report view");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&OutputStderr::write,"write()");
    add_varargs_method("flush",&OutputStderr::flush,"flush()");
    add_noargs_method("isatty",&OutputStderr::isatty,"isatty()");
}

OutputStderr::OutputStderr() = default;

OutputStderr::~OutputStderr() = default;

Py::Object OutputStderr::getattr(const char *name)
{
    if (strcmp(name, "softspace") == 0) {
        int i=0;
        return Py::Int(i);
    }
    return getattr_methods(name);
}

Py::Object OutputStderr::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "OutputStderr";
    return Py::String(s_out.str());
}

Py::Object OutputStderr::write(const Py::Tuple& args)
{
    PyObject* output;
    if (!PyArg_ParseTuple(args.ptr(), "O!",&PyUnicode_Type, &output))
        throw Py::TypeError("OutputStderr.write() takes exactly one argument of type str");

    PyObject* unicode = PyUnicode_AsEncodedString(output, "utf-8", nullptr);
    if (unicode) {
        const char* string = PyBytes_AsString(unicode);
        Base::Console().Error("%s",string);
        Py_DECREF(unicode);
    }

    return Py::None();
}

Py::Object OutputStderr::flush(const Py::Tuple&)
{
    return Py::None();
}

Py::Object OutputStderr::isatty()
{
    return Py::False();
}


// -------------------------------------------------------------------------

void PythonStdin::init_type()
{
    behaviors().name("PythonStdin");
    behaviors().doc("Redirection of stdin to FreeCAD to open an input dialog");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    add_varargs_method("readline",&PythonStdin::readline,"readline()");
}

PythonStdin::PythonStdin(PythonConsole *pc)
  : pyConsole(pc)
{
}

PythonStdin::~PythonStdin() = default;

Py::Object PythonStdin::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonStdin";
    return Py::String(s_out.str());
}

Py::Object PythonStdin::getattr(const char *name)
{
    if (strcmp(name, "closed") == 0) {
        return Py::Boolean(false);
    }
    return getattr_methods(name);
}

Py::Object PythonStdin::readline(const Py::Tuple& /*args*/)
{
    return Py::String( (const char *)pyConsole->readline().toLatin1() );
}
