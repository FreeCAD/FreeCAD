/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include "Application.h"

#include <App/ExpressionPy.h>
#include <App/ExpressionPy.cpp>
#include <App/DocumentObject.h>

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string ExpressionPy::representation(void) const
{
    if (!pyOwner->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Owner document object expired");
        return NULL;
    }
    return std::string("=") + getExpressionPtr()->toString();
}

static PyObject *ExpressionPy_Call( PyObject *self, PyObject *args, PyObject *kw ) {
    assert(PyObject_TypeCheck(self,&ExpressionPy::Type));
    return static_cast<ExpressionPy*>(self)->__call__(args,kw);
}

int ExpressionPy::initialization() {
    if(!Type.tp_call) 
        Type.tp_call = ExpressionPy_Call;
    pyOwner = static_cast<PyObjectBase*>(getExpressionPtr()->getOwner()->getPyObject());
    return 1;
}

int ExpressionPy::finalization() {
    Py_DECREF(pyOwner);
    delete getExpressionPtr();
    return 1;
}

PyObject *ExpressionPy::__call__(PyObject *args, PyObject *kwds) {
    if (!pyOwner->isValid()){
        PyErr_Format(PyExc_ReferenceError, "Owner document object expired");
        return NULL;
    }
    PY_TRY {
        auto expr = dynamic_cast<CallableExpression*>(getExpressionPtr());
        if(expr)
            return Py::new_reference_to(expr->evaluate(args,kwds));
        Py_Return;
    }PY_CATCH
}

PyObject *ExpressionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ExpressionPy::setCustomAttributes(const char* /*attr*/, PyObject * /*obj*/)
{
    return 0;
}
