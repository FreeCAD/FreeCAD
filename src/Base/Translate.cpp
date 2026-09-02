// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#include "Translate.h"

#include "QtModulePy.h"
#include "Translation.h"

using namespace Base;


Translate::Translate()
    : Py::ExtensionModule<Translate>("__Translate__")
{
    QtModulePy::initialize(*this);
}

Translate::~Translate() = default;

Py::Object Translate::translate(const Py::Tuple& args)
{
    char* context = nullptr;
    char* source = nullptr;
    char* disambiguation = nullptr;
    int num = -1;
    if (!PyArg_ParseTuple(args.ptr(), "ss|zi", &context, &source, &disambiguation, &num)) {
        throw Py::Exception();
    }

    const std::string disambig = disambiguation ? std::string(disambiguation) : std::string();
    const std::string translated = Base::Translation::translate(context, source, disambig, num);
    return Py::asObject(PyUnicode_FromString(translated.c_str()));
}

Py::Object Translate::translateNoop(const Py::Tuple& args)
{
    PyObject* arg1 = nullptr;
    PyObject* arg2 = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "OO", &arg1, &arg2)) {
        throw Py::Exception();
    }
    return Py::Object(arg2);
}

Py::Object Translate::translateNoop3(const Py::Tuple& args)
{
    PyObject* arg1 = nullptr;
    PyObject* arg2 = nullptr;
    PyObject* arg3 = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "OOO", &arg1, &arg2, &arg3)) {
        throw Py::Exception();
    }
    return Py::Object(arg2);
}

Py::Object Translate::trNoop(const Py::Tuple& args)
{
    PyObject* arg1 = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O", &arg1)) {
        throw Py::Exception();
    }
    return Py::Object(arg1);
}

Py::Object Translate::installTranslator(const Py::Tuple& args)
{
    char* Name = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Name)) {
        throw Py::Exception();
    }
    std::string filename(Name);
    PyMem_Free(Name);

    const bool ok = Base::Translation::installTranslator(filename);
    if (ok) {
        translators.push_back(filename);
    }

    return Py::Boolean(ok);  // NOLINT
}

Py::Object Translate::removeTranslators(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), "")) {
        throw Py::Exception();
    }

    if (translators.empty()) {
        return Py::Boolean(true);  // NOLINT
    }

    const bool ok = Base::Translation::removeTranslators(translators);
    if (ok) {
        translators.clear();
    }
    return Py::Boolean(ok);  // NOLINT
}
