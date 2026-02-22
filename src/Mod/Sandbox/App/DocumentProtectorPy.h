// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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

namespace App {
class DocumentPy;
class DocumentObject;
class DocumentObjectPy;
}

namespace Sandbox {

class DocumentProtector;
class DocumentProtectorPy : public Py::PythonExtension<DocumentProtectorPy>
{
public:
    static void init_type(void);    // announce properties and methods

    DocumentProtectorPy(App::DocumentPy *doc);
    ~DocumentProtectorPy();

    Py::Object repr();
    Py::Object getattr(const char *);
    int setattr(const char *, const Py::Object &);

    Py::Object addObject(const Py::Tuple&);
    Py::Object recompute(const Py::Tuple&);

private:
    using method_varargs_handler = PyObject* (*)(PyObject *_self, PyObject *_args);
    static method_varargs_handler pycxx_handler;
    static PyObject *method_varargs_ext_handler(PyObject *_self, PyObject *_args);

private:
    DocumentProtector* _dp;
    friend class DocumentProtector;
};

class DocumentObjectProtector;
class DocumentObjectProtectorPy : public Py::PythonExtension<DocumentObjectProtectorPy>
{
public:
    static void init_type(void);    // announce properties and methods

    DocumentObjectProtectorPy(App::DocumentObject *obj);
    DocumentObjectProtectorPy(App::DocumentObjectPy *obj);
    ~DocumentObjectProtectorPy();

    Py::Object repr();
    Py::Object getattr(const char *);
    Py::Object getObject() const;
    int setattr(const char *, const Py::Object &);
    Py::Object purgeTouched(const Py::Tuple&);

private:
    DocumentObjectProtector* _dp;
    friend class DocumentObjectProtector;
};

}