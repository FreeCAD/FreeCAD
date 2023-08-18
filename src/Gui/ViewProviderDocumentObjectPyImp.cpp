/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <sstream>
#endif

#include <App/DocumentObjectPy.h>
#include "Document.h"

// inclusion of the generated files (generated out of ViewProviderDocumentObjectPy.xml)
#include "ViewProviderDocumentObjectPy.h"
#include "ViewProviderDocumentObjectPy.cpp"


using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderDocumentObjectPy::representation() const
{
    std::stringstream str;
    str << "<View provider object at " << getViewProviderDocumentObjectPtr() << ">";

    return str.str();
}

PyObject* ViewProviderDocumentObjectPy::update(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    PY_TRY {
        getViewProviderDocumentObjectPtr()->updateView();
        Py_Return;
    } PY_CATCH;
}

Py::Object ViewProviderDocumentObjectPy::getObject() const
{
    App::DocumentObject* obj = getViewProviderDocumentObjectPtr()->getObject();
    return Py::Object(obj->getPyObject(), true); // do not inc'ref twice
}

void ViewProviderDocumentObjectPy::setObject(Py::Object pyobj)
{
    if(!PyObject_TypeCheck(*pyobj,&App::DocumentObjectPy::Type))
        throw Py::TypeError("Expect document object");
    App::DocumentObject* obj = getViewProviderDocumentObjectPtr()->getObject();
    if(obj)
        throw Py::RuntimeError("View object already attached");
    getViewProviderDocumentObjectPtr()->attach(
            static_cast<App::DocumentObjectPy*>(*pyobj)->getDocumentObjectPtr());
}

Py::Boolean ViewProviderDocumentObjectPy::getForceUpdate() const
{
    return {getViewProviderDocumentObjectPtr()->isUpdateForced()};
}

void ViewProviderDocumentObjectPy::setForceUpdate(Py::Boolean arg)
{
    getViewProviderDocumentObjectPtr()->forceUpdate(arg);
}

Py::Object ViewProviderDocumentObjectPy::getDocument() const
{
    Document* doc = getViewProviderDocumentObjectPtr()->getDocument();
    return Py::asObject(doc->getPyObject()); // do not inc'ref twice
}

PyObject *ViewProviderDocumentObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderDocumentObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
