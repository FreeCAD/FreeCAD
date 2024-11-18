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
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <sstream>
#endif

#include "ViewProvider.h"
// inclusion of the generated files (generated out of ViewProviderMeshPy.xml)
#include "ViewProviderMeshPy.h"
#include "ViewProviderMeshPy.cpp"


using namespace MeshGui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderMeshPy::representation() const
{
    std::stringstream str;
    str << "<View provider object at " << getViewProviderDocumentObjectPtr() << ">";

    return str.str();
}

PyObject* ViewProviderMeshPy::setSelection(PyObject* args)
{
    PyObject* obj {};
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }

    Py::Sequence list(obj);
    std::vector<Mesh::FacetIndex> selection;
    selection.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Py::Long index(*it);
        Mesh::FacetIndex value = static_cast<Mesh::FacetIndex>(index);
        selection.push_back(value);
    }

    ViewProviderMesh* vp = getViewProviderMeshPtr();
    vp->setSelection(selection);
    Py_Return;
}

PyObject* ViewProviderMeshPy::addSelection(PyObject* args)
{
    PyObject* obj {};
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }

    Py::Sequence list(obj);
    std::vector<Mesh::FacetIndex> selection;
    selection.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Py::Long index(*it);
        Mesh::FacetIndex value = static_cast<Mesh::FacetIndex>(index);
        selection.push_back(value);
    }

    ViewProviderMesh* vp = getViewProviderMeshPtr();
    vp->addSelection(selection);
    Py_Return;
}

PyObject* ViewProviderMeshPy::removeSelection(PyObject* args)
{
    PyObject* obj {};
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }

    Py::Sequence list(obj);
    std::vector<Mesh::FacetIndex> selection;
    selection.reserve(list.size());
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        Py::Long index(*it);
        Mesh::FacetIndex value = static_cast<Mesh::FacetIndex>(index);
        selection.push_back(value);
    }

    ViewProviderMesh* vp = getViewProviderMeshPtr();
    vp->removeSelection(selection);
    Py_Return;
}

PyObject* ViewProviderMeshPy::invertSelection(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    ViewProviderMesh* vp = getViewProviderMeshPtr();
    vp->invertSelection();
    Py_Return;
}

PyObject* ViewProviderMeshPy::clearSelection(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    ViewProviderMesh* vp = getViewProviderMeshPtr();
    vp->clearSelection();
    Py_Return;
}

PyObject* ViewProviderMeshPy::highlightSegments(PyObject* args)
{
    PyObject* list {};
    if (!PyArg_ParseTuple(args, "O", &list)) {
        return nullptr;
    }

    App::PropertyColorList colors;
    colors.setPyObject(list);

    ViewProviderMesh* vp = getViewProviderMeshPtr();
    vp->highlightSegments(colors.getValues());
    Py_Return;
}

PyObject* ViewProviderMeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderMeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
