/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include <CXX/Extensions.hxx>

class QWidget;
namespace Gui
{
class ExpressionBinding;

class ExpressionBindingPy: public Py::PythonClass<ExpressionBindingPy>
{
public:
    static void init_type();  // announce properties and methods

    ExpressionBindingPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds);
    ~ExpressionBindingPy() override;

    Py::Object repr() override;

    Py::Object bind(const Py::Tuple&);
    Py::Object isBound();
    Py::Object apply(const Py::Tuple&);
    Py::Object hasExpression();
    Py::Object autoApply();
    Py::Object setAutoApply(const Py::Tuple&);

private:
    static ExpressionBinding* asBinding(QWidget*);

private:
    ExpressionBinding* expr;
};

}  // namespace Gui
