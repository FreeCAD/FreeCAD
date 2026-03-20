// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <memory>
#include <CXX/Extensions.hxx>
#include <Base/Sequencer.h>

namespace Base
{
// NOLINTNEXTLINE
class BaseExport ProgressIndicatorPy: public Py::PythonExtension<ProgressIndicatorPy>
{
public:
    static void init_type();  // announce properties and methods
    static Py::PythonType& behaviors();
    static PyTypeObject* type_object();
    static bool check(PyObject* py);

    ProgressIndicatorPy();
    ~ProgressIndicatorPy() override;

    Py::Object repr() override;

    Py::Object start(const Py::Tuple&);
    Py::Object next(const Py::Tuple&);
    Py::Object stop(const Py::Tuple&);

private:
    static PyObject* PyMake(struct _typeobject*, PyObject*, PyObject*);

private:
    std::unique_ptr<SequencerLauncher> _seq;
};

}  // namespace Base
