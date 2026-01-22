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


#pragma once

#include <CXX/Extensions.hxx>

class QTimer;

namespace Gui
{
class PythonConsole;

/**
 * Python class for redirection of stdout to FreeCAD's Python
 * console window. This allows one to show all Python messages in
 * the same window where the commands are performed.
 * @see PythonStderr
 * @see PythonConsole
 * @author Werner Mayer
 */
class PythonStdout: public Py::PythonExtension<PythonStdout>
{
private:
    PythonConsole* pyConsole;

public:
    static void init_type();  // announce properties and methods

    explicit PythonStdout(PythonConsole* pc);
    ~PythonStdout() override;

    Py::Object getattr(const char* name) override;
    Py::Object repr() override;
    Py::Object write(const Py::Tuple&);
    Py::Object flush(const Py::Tuple&);
    Py::Object isatty();
};

/**
 * Python class for redirection of stderr to FreeCAD's Python
 * console window. This allows one to show all Python messages in
 * the same window where the commands are performed.
 * @see PythonStdout
 * @see PythonConsole
 * @author Werner Mayer
 */
class PythonStderr: public Py::PythonExtension<PythonStderr>
{
private:
    PythonConsole* pyConsole;

public:
    static void init_type();  // announce properties and methods

    explicit PythonStderr(PythonConsole* pc);
    ~PythonStderr() override;

    Py::Object getattr(const char* name) override;
    Py::Object repr() override;
    Py::Object write(const Py::Tuple&);
    Py::Object flush(const Py::Tuple&);
    Py::Object isatty();
};

/**
 * Python class for redirection of stdout to FreeCAD's output
 * console window. This allows one to report all Python output to
 * the report view which simplifies debugging scripts.
 * @see PythonStdout
 * @see PythonStderr
 * @author Werner Mayer
 */
class OutputStdout: public Py::PythonExtension<OutputStdout>
{
public:
    static void init_type();  // announce properties and methods

    OutputStdout();
    ~OutputStdout() override;

    Py::Object getattr(const char* name) override;
    Py::Object repr() override;
    Py::Object write(const Py::Tuple&);
    Py::Object flush(const Py::Tuple&);
    Py::Object isatty();
};

/**
 * Python class for redirection of stderr to FreeCAD's output
 * console window. This allows one to report all Python errors to
 * the report view which simplifies error tracking.
 * @see PythonStdout
 * @see PythonStderr
 * @author Werner Mayer
 */
class OutputStderr: public Py::PythonExtension<OutputStderr>
{
public:
    static void init_type();  // announce properties and methods

    OutputStderr();
    ~OutputStderr() override;

    Py::Object getattr(const char* name) override;
    Py::Object repr() override;
    Py::Object write(const Py::Tuple&);
    Py::Object flush(const Py::Tuple&);
    Py::Object isatty();
};

/**
 * Python class for redirection of stdin to an input dialog of Qt.
 * @author Werner Mayer
 */
class PythonStdin: public Py::PythonExtension<PythonStdin>
{
private:
    PythonConsole* pyConsole;

public:
    static void init_type();  // announce properties and methods

    explicit PythonStdin(PythonConsole* pc);
    ~PythonStdin() override;

    Py::Object repr() override;
    Py::Object getattr(const char* name) override;
    Py::Object readline(const Py::Tuple&);
};

}  // namespace Gui
