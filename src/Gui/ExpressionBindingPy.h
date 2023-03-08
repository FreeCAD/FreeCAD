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

#ifndef EXPRESSIONBINDINGPY_H
#define EXPRESSIONBINDINGPY_H

#include <CXX/Extensions.hxx>

class QWidget;
namespace Gui {
class ExpressionBinding;

class ExpressionBindingPy : public Py::PythonExtension<ExpressionBindingPy>
{
public:
    static void init_type();    // announce properties and methods

    explicit ExpressionBindingPy(ExpressionBinding*);
    ~ExpressionBindingPy() override;

    Py::Object repr() override;

    Py::Object bind(const Py::Tuple&);
    Py::Object isBound(const Py::Tuple&);
    Py::Object apply(const Py::Tuple&);
    Py::Object hasExpression(const Py::Tuple&);
    Py::Object autoApply(const Py::Tuple&);
    Py::Object setAutoApply(const Py::Tuple&);

private:
    static PyObject *PyMake(struct _typeobject *, PyObject *, PyObject *);
    static ExpressionBinding* asBinding(QWidget*);

private:
    ExpressionBinding* expr;
};

}

#endif // EXPRESSIONBINDING_H
