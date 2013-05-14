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
#ifndef _PreComp_
# include <QByteArray>
# include <QInputDialog>
# include <QEventLoop>
# include <QTimer>
#endif

#include "PythonConsolePy.h"
#include "PythonConsole.h"
#include "MainWindow.h"

#include <Base/Console.h>
#include <Base/Exception.h>

using namespace Gui;

void PythonStdout::init_type()
{
    behaviors().name("PythonStdout");
    behaviors().doc("Redirection of stdout to FreeCAD's Python console window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonStdout::write,"write()");
    add_varargs_method("flush",&PythonStdout::flush,"flush()");
}

PythonStdout::PythonStdout(PythonConsole *pc)
  : pyConsole(pc)
{
}

PythonStdout::~PythonStdout()
{
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
    try {
        Py::Object output(args[0]);
        if (PyUnicode_Check(output.ptr())) {
            PyObject* unicode = PyUnicode_AsEncodedObject(output.ptr(), "utf-8", "strict");
            if (unicode) {
                const char* string = PyString_AsString(unicode);
                int maxlen = qstrlen(string) > 10000 ? 10000 : -1;
                pyConsole->insertPythonOutput(QString::fromUtf8(string, maxlen));
                Py_DECREF(unicode);
            }
        }
        else {
            Py::String text(args[0]);
            std::string string = (std::string)text;
            int maxlen = string.size() > 10000 ? 10000 : -1;
            pyConsole->insertPythonOutput(QString::fromUtf8(string.c_str(), maxlen));
        }
    }
    catch (Py::Exception& e) {
        // Do not provoke error messages 
        e.clear();
    }

    return Py::None();
}

Py::Object PythonStdout::flush(const Py::Tuple&)
{
    return Py::None();
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
}

PythonStderr::PythonStderr(PythonConsole *pc)
  : pyConsole(pc)
{
}

PythonStderr::~PythonStderr()
{
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
    try {
        Py::Object output(args[0]);
        if (PyUnicode_Check(output.ptr())) {
            PyObject* unicode = PyUnicode_AsEncodedObject(output.ptr(), "utf-8", "strict");
            if (unicode) {
                const char* string = PyString_AsString(unicode);
                int maxlen = qstrlen(string) > 10000 ? 10000 : -1;
                pyConsole->insertPythonError(QString::fromUtf8(string, maxlen));
                Py_DECREF(unicode);
            }
        }
        else {
            Py::String text(args[0]);
            std::string string = (std::string)text;
            int maxlen = string.size() > 10000 ? 10000 : -1;
            pyConsole->insertPythonError(QString::fromUtf8(string.c_str(), maxlen));
        }
    }
    catch (Py::Exception& e) {
        // Do not provoke error messages
        e.clear();
    }

    return Py::None();
}

Py::Object PythonStderr::flush(const Py::Tuple&)
{
    return Py::None();
}

// -------------------------------------------------------------------------

void OutputStdout::init_type()
{
    behaviors().name("OutputStdout");
    behaviors().doc("Redirection of stdout to FreeCAD's output window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&OutputStdout::write,"write()");
    add_varargs_method("flush",&OutputStdout::flush,"flush()");
}

OutputStdout::OutputStdout()
{
}

OutputStdout::~OutputStdout()
{
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
    try {
        Py::Object output(args[0]);
        if (PyUnicode_Check(output.ptr())) {
            PyObject* unicode = PyUnicode_AsEncodedObject(output.ptr(), "utf-8", "strict");
            if (unicode) {
                const char* string = PyString_AsString(unicode);
                Base::Console().Message("%s",string);
                Py_DECREF(unicode);
            }
        }
        else {
            Py::String text(args[0]);
            std::string string = (std::string)text;
            Base::Console().Message("%s",string.c_str());
        }
    }
    catch (Py::Exception& e) {
        // Do not provoke error messages
        e.clear();
    }

    return Py::None();
}

Py::Object OutputStdout::flush(const Py::Tuple&)
{
    return Py::None();
}

// -------------------------------------------------------------------------

void OutputStderr::init_type()
{
    behaviors().name("OutputStderr");
    behaviors().doc("Redirection of stdout to FreeCAD's output window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&OutputStderr::write,"write()");
    add_varargs_method("flush",&OutputStderr::flush,"flush()");
}

OutputStderr::OutputStderr()
{
}

OutputStderr::~OutputStderr()
{
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
    try {
        Py::Object output(args[0]);
        if (PyUnicode_Check(output.ptr())) {
            PyObject* unicode = PyUnicode_AsEncodedObject(output.ptr(), "utf-8", "strict");
            if (unicode) {
                const char* string = PyString_AsString(unicode);
                Base::Console().Error("%s",string);
                Py_DECREF(unicode);
            }
        }
        else {
            Py::String text(args[0]);
            std::string string = (std::string)text;
            Base::Console().Error("%s",string.c_str());
        }
    }
    catch (Py::Exception& e) {
        // Do not provoke error messages
        e.clear();
    }

    return Py::None();
}

Py::Object OutputStderr::flush(const Py::Tuple&)
{
    return Py::None();
}

// -------------------------------------------------------------------------

void PythonStdin::init_type()
{
    behaviors().name("PythonStdin");
    behaviors().doc("Redirection of stdin to FreeCAD to open an input dialog");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("readline",&PythonStdin::readline,"readline()");
}

PythonStdin::PythonStdin(PythonConsole *pc)
  : pyConsole(pc)
{
    console = getMainWindow()->findChild<PythonConsole*>();
}

PythonStdin::~PythonStdin()
{
}

Py::Object PythonStdin::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonStdin";
    return Py::String(s_out.str());
}

Py::Object PythonStdin::readline(const Py::Tuple& args)
{
    return Py::String( (const char *)console->readline().toAscii() );
}
