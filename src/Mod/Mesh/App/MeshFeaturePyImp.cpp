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

#include "MeshFeature.h"
// inclusion of the generated files (generated out of MeshFeaturePy.xml)
// clang-format off
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Mesh/App/MeshFeaturePy.h>
#include <Mod/Mesh/App/MeshFeaturePy.cpp>
// clang-format on


using namespace Mesh;


// returns a string which represent the object e.g. when printed in python
std::string MeshFeaturePy::representation() const
{
    std::stringstream str;
    str << getFeaturePtr()->getTypeId().getName() << " object at " << getFeaturePtr();

    return str.str();
}

PyObject* MeshFeaturePy::countPoints(PyObject* /*args*/)
{
    return Py_BuildValue("i", getFeaturePtr()->Mesh.getValue().countPoints());
}

PyObject* MeshFeaturePy::countFacets(PyObject* /*args*/)
{
    return Py_BuildValue("i", getFeaturePtr()->Mesh.getValue().countFacets());
}

PyObject* MeshFeaturePy::harmonizeNormals(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        Mesh::MeshObject* mesh = getFeaturePtr()->Mesh.startEditing();
        mesh->harmonizeNormals();
        getFeaturePtr()->Mesh.finishEditing();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MeshFeaturePy::smooth(PyObject* args)
{
    int iter = 1;
    float d_max = FLOAT_MAX;
    if (!PyArg_ParseTuple(args, "|if", &iter, &d_max)) {
        return nullptr;
    }

    PY_TRY
    {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->smooth(iter, d_max);
        obj->Mesh.finishEditing();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MeshFeaturePy::removeNonManifolds(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    Mesh::Feature* obj = getFeaturePtr();
    MeshObject* kernel = obj->Mesh.startEditing();
    kernel->removeNonManifolds();
    obj->Mesh.finishEditing();
    Py_Return;
}

PyObject* MeshFeaturePy::removeNonManifoldPoints(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    Mesh::Feature* obj = getFeaturePtr();
    MeshObject* kernel = obj->Mesh.startEditing();
    kernel->removeNonManifoldPoints();
    obj->Mesh.finishEditing();
    Py_Return;
}

PyObject* MeshFeaturePy::fixIndices(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->validateIndices();
        obj->Mesh.finishEditing();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MeshFeaturePy::fixDegenerations(PyObject* args)
{
    float fEpsilon = MeshCore::MeshDefinitions::_fMinPointDistanceP2;
    if (!PyArg_ParseTuple(args, "|f", &fEpsilon)) {
        return nullptr;
    }

    PY_TRY
    {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->validateDegenerations(fEpsilon);
        obj->Mesh.finishEditing();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MeshFeaturePy::removeDuplicatedFacets(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->removeDuplicatedFacets();
        obj->Mesh.finishEditing();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MeshFeaturePy::removeDuplicatedPoints(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PY_TRY
    {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->removeDuplicatedPoints();
        obj->Mesh.finishEditing();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MeshFeaturePy::fixSelfIntersections(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    try {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->removeSelfIntersections();
        obj->Mesh.finishEditing();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject* MeshFeaturePy::removeFoldsOnSurface(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    try {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->removeFoldsOnSurface();
        obj->Mesh.finishEditing();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject* MeshFeaturePy::removeInvalidPoints(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    try {
        Mesh::Feature* obj = getFeaturePtr();
        MeshObject* kernel = obj->Mesh.startEditing();
        kernel->removeInvalidPoints();
        obj->Mesh.finishEditing();
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        return nullptr;
    }
    Py_Return;
}

PyObject* MeshFeaturePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MeshFeaturePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
