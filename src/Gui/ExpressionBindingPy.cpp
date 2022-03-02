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

#include "PreCompiled.h"

#include <App/DocumentObjectPy.h>

#include "ExpressionBindingPy.h"
#include "InputField.h"
#include "PythonWrapper.h"
#include "QuantitySpinBox.h"


using namespace Gui;

void ExpressionBindingPy::init_type()
{
    behaviors().name("ExpressionBinding");
    behaviors().doc("Python interface class for ExpressionBinding");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().set_tp_new(PyMake);
    behaviors().readyType();

    add_varargs_method("bind",&ExpressionBindingPy::bind,"Bind with an expression");
    add_varargs_method("isBound",&ExpressionBindingPy::isBound,"Check if already bound with an expression");
    add_varargs_method("apply",&ExpressionBindingPy::apply,"apply");
    add_varargs_method("hasExpression",&ExpressionBindingPy::hasExpression,"hasExpression");
    add_varargs_method("autoApply",&ExpressionBindingPy::autoApply,"autoApply");
    add_varargs_method("setAutoApply",&ExpressionBindingPy::setAutoApply,"setAutoApply");
}

PyObject *ExpressionBindingPy::PyMake(struct _typeobject *, PyObject * args, PyObject *)
{
    Py::Tuple tuple(args);

    ExpressionBinding* expr = nullptr;
    PythonWrapper wrap;
    wrap.loadWidgetsModule();

    QWidget* obj = dynamic_cast<QWidget*>(wrap.toQObject(tuple.getItem(0)));
    if (obj) {
        do {
            QuantitySpinBox* sb = qobject_cast<QuantitySpinBox*>(obj);
            if (sb) {
                expr = sb;
                break;
            }
            InputField* le = qobject_cast<InputField*>(obj);
            if (le) {
                expr = le;
                break;
            }
        }
        while(false);
    }

    if (!expr) {
        PyErr_SetString(PyExc_TypeError, "Wrong type");
        return nullptr;
    }

    return new ExpressionBindingPy(expr);
}

ExpressionBindingPy::ExpressionBindingPy(ExpressionBinding* expr)
  : expr(expr)
{
}

ExpressionBindingPy::~ExpressionBindingPy()
{
}

Py::Object ExpressionBindingPy::repr()
{
    std::stringstream s;
    s << "<ExpressionBinding at " << this << ">";
    return Py::String(s.str());
}

Py::Object ExpressionBindingPy::bind(const Py::Tuple& args)
{
    PyObject* py;
    const char* str;
    if (!PyArg_ParseTuple(args.ptr(), "O!s", &App::DocumentObjectPy::Type, &py, &str))
        throw Py::Exception();

    try {
        App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(py)->getDocumentObjectPtr();
        App::ObjectIdentifier id(App::ObjectIdentifier::parse(obj, str));
        if (!id.getProperty()) {
            throw Base::AttributeError("Wrong property");
        }

        expr->bind(id);
        return Py::None();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
    catch (...) {
        throw Py::RuntimeError("Cannot bind to object");
    }
}

Py::Object ExpressionBindingPy::isBound(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Boolean(expr->isBound());
}

Py::Object ExpressionBindingPy::apply(const Py::Tuple& args)
{
    const char* str;
    if (!PyArg_ParseTuple(args.ptr(), "s", &str))
        throw Py::Exception();

    return Py::Boolean(expr->apply(str));
}

Py::Object ExpressionBindingPy::hasExpression(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Boolean(expr->hasExpression());
}

Py::Object ExpressionBindingPy::autoApply(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    return Py::Boolean(expr->autoApply());
}

Py::Object ExpressionBindingPy::setAutoApply(const Py::Tuple& args)
{
    PyObject* b;
    if (!PyArg_ParseTuple(args.ptr(), "O!", &PyBool_Type, &b))
        throw Py::Exception();

    bool value = PyObject_IsTrue(b) ? true : false;
    expr->setAutoApply(value);
    return Py::None();
}
