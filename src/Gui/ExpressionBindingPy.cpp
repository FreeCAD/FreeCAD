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

ExpressionBindingPy::ExpressionBindingPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
  : Py::PythonClass<ExpressionBindingPy>::PythonClass(self, args, kwds)
{
    PyObject* pyObj;
    if (!PyArg_ParseTuple(args.ptr(), "O", &pyObj)) {
        throw Py::Exception();
    }

    PythonWrapper wrap;
    wrap.loadWidgetsModule();

    QWidget* obj = dynamic_cast<QWidget*>(wrap.toQObject(Py::Object(pyObj)));
    expr = asBinding(obj);

    if (!expr) {
        throw Py::Exception(PyExc_TypeError, "Wrong type");
    }
}

ExpressionBindingPy::~ExpressionBindingPy() = default;

ExpressionBinding* ExpressionBindingPy::asBinding(QWidget* obj)
{
    ExpressionBinding* expr = nullptr;
    if (obj) {
        do {
            auto qsb = qobject_cast<QuantitySpinBox*>(obj);
            if (qsb) {
                expr = qsb;
                break;
            }
            auto usp = qobject_cast<UIntSpinBox*>(obj);
            if (usp) {
                expr = usp;
                break;
            }
            auto isp = qobject_cast<IntSpinBox*>(obj);
            if (isp) {
                expr = isp;
                break;
            }
            auto dsp = qobject_cast<DoubleSpinBox*>(obj);
            if (dsp) {
                expr = dsp;
                break;
            }
            auto exp = qobject_cast<ExpLineEdit*>(obj);
            if (exp) {
                expr = exp;
                break;
            }
            auto inp = qobject_cast<InputField*>(obj);
            if (inp) {
                expr = inp;
                break;
            }
        }
        while (false);
    }

    return expr;
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
    if (!PyArg_ParseTuple(args.ptr(), "O!s", &App::DocumentObjectPy::Type, &py, &str)) {
        throw Py::Exception();
    }

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
PYCXX_VARARGS_METHOD_DECL(ExpressionBindingPy, bind)

Py::Object ExpressionBindingPy::isBound()
{
    return Py::Boolean(expr->isBound());
}
PYCXX_NOARGS_METHOD_DECL(ExpressionBindingPy, isBound)

Py::Object ExpressionBindingPy::apply(const Py::Tuple& args)
{
    const char* str;
    if (!PyArg_ParseTuple(args.ptr(), "s", &str)) {
        throw Py::Exception();
    }

    return Py::Boolean(expr->apply(str));
}
PYCXX_VARARGS_METHOD_DECL(ExpressionBindingPy, apply)

Py::Object ExpressionBindingPy::hasExpression()
{
    return Py::Boolean(expr->hasExpression());
}
PYCXX_NOARGS_METHOD_DECL(ExpressionBindingPy, hasExpression)

Py::Object ExpressionBindingPy::autoApply()
{
    return Py::Boolean(expr->autoApply());
}
PYCXX_NOARGS_METHOD_DECL(ExpressionBindingPy, autoApply)

Py::Object ExpressionBindingPy::setAutoApply(const Py::Tuple& args)
{
    PyObject* b;
    if (!PyArg_ParseTuple(args.ptr(), "O!", &PyBool_Type, &b)) {
        throw Py::Exception();
    }

    bool value = Base::asBoolean(b);
    expr->setAutoApply(value);
    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(ExpressionBindingPy, setAutoApply)

void ExpressionBindingPy::init_type()
{
    behaviors().name("Gui.ExpressionBinding");
    behaviors().doc("Python interface class for ExpressionBinding");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattro();
    behaviors().supportSetattro();

    PYCXX_ADD_VARARGS_METHOD(bind, bind, "Bind with an expression");
    PYCXX_ADD_NOARGS_METHOD(isBound, isBound, "Check if already bound with an expression");
    PYCXX_ADD_VARARGS_METHOD(apply, apply, "apply");
    PYCXX_ADD_NOARGS_METHOD(hasExpression, hasExpression, "hasExpression");
    PYCXX_ADD_NOARGS_METHOD(autoApply, autoApply, "autoApply");
    PYCXX_ADD_VARARGS_METHOD(setAutoApply, setAutoApply, "setAutoApply");

    behaviors().readyType();
}
