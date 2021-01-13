/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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

#include "SelectionObject.h"
#include "Selection.h"
#include <Base/GeometryPyCXX.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Application.h>

// inclusion of the generated files (generated out of SelectionObjectPy.xml)
#include "SelectionObjectPy.h"
#include "SelectionObjectPy.cpp"

using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string SelectionObjectPy::representation(void) const
{
    return "<SelectionObject>";
}

PyObject* SelectionObjectPy::remove(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Selection().rmvSelection(getSelectionObjectPtr()->getDocName(),
                             getSelectionObjectPtr()->getFeatName());
    Py_Return;
}

PyObject* SelectionObjectPy::isObjectTypeOf(PyObject * args)
{
    char* type;
    if (!PyArg_ParseTuple(args, "s", &type))
        return 0;
    Base::Type id = Base::Type::fromName(type);
    if (id.isBad()) {
        PyErr_SetString(PyExc_TypeError, "Not a valid type");
        return 0;
    }

    bool ok = getSelectionObjectPtr()->isObjectTypeOf(id);

    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

Py::String SelectionObjectPy::getObjectName(void) const
{
    return Py::String(getSelectionObjectPtr()->getFeatName());
}

Py::Tuple SelectionObjectPy::getSubElementNames(void) const
{
    std::vector<std::string> objs = getSelectionObjectPtr()->getSubNames();

    Py::Tuple temp(objs.size());
    Py::sequence_index_type index = 0;
    for(std::vector<std::string>::const_iterator it= objs.begin();it!=objs.end();++it)
        temp.setItem(index++, Py::String(*it));

    return temp;
}

Py::String SelectionObjectPy::getFullName(void) const
{
    return Py::String(getSelectionObjectPtr()->getAsPropertyLinkSubString());
}

Py::String SelectionObjectPy::getTypeName(void) const
{
    return Py::String(getSelectionObjectPtr()->getTypeName());
}

Py::String SelectionObjectPy::getDocumentName(void) const
{
    return Py::String(getSelectionObjectPtr()->getDocName());
}

Py::Object SelectionObjectPy::getDocument(void) const
{
    App::DocumentObject *obj = getSelectionObjectPtr()->getObject();
    if (!obj)
        throw Py::RuntimeError("Cannot get document of deleted object");
    return Py::Object(obj->getDocument()->getPyObject(), true);
}

Py::Object SelectionObjectPy::getObject(void) const
{
    App::DocumentObject *obj = getSelectionObjectPtr()->getObject();
    if (!obj)
        throw Py::RuntimeError("Object already deleted");
    return Py::Object(obj->getPyObject(), true);
}

Py::Tuple SelectionObjectPy::getSubObjects(void) const
{
    std::vector<PyObject *> objs;

    for(const auto &subname : getSelectionObjectPtr()->getSubNames()) {
        PyObject *pyObj=0;
        Base::Matrix4D mat;
        getSelectionObjectPtr()->getObject()->getSubObject(subname.c_str(),&pyObj,&mat);
        if(pyObj) objs.push_back(pyObj);
    }

    Py::Tuple temp(objs.size());
    Py::sequence_index_type index = 0;
    for(std::vector<PyObject *>::const_iterator it= objs.begin();it!=objs.end();++it)
        temp.setItem(index++, Py::asObject(*it));

    return temp;
}

Py::Boolean SelectionObjectPy::getHasSubObjects(void) const
{
    return Py::Boolean(getSelectionObjectPtr()->hasSubNames());
}

Py::Tuple SelectionObjectPy::getPickedPoints(void) const
{
    const std::vector<Base::Vector3d>& points = getSelectionObjectPtr()->getPickedPoints();

    Py::Tuple temp(points.size());
    Py::sequence_index_type index = 0;
    for(std::vector<Base::Vector3d>::const_iterator it= points.begin();it!=points.end();++it)
        temp.setItem(index++, Py::Vector(*it));

    return temp;
}

PyObject *SelectionObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SelectionObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
