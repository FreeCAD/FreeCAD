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

#include <Base/Console.h>
#include <Base/Handle.h>

#include "Core/Evaluation.h"
#include "MeshPy.h"
#include "MeshFeature.h"

// inclusion of the generated files (generated out of MeshFeaturePy.xml)
#include "MeshFeaturePy.h"
#include "MeshFeaturePy.cpp"

using namespace Mesh;


// returns a string which represent the object e.g. when printed in python
std::string MeshFeaturePy::representation(void) const
{
    std::stringstream str;
    str << getFeaturePtr()->getTypeId().getName() << " object at " << getFeaturePtr();

    return str.str();
}

PyObject*  MeshFeaturePy::countPoints(PyObject * /*args*/)
{
    return Py_BuildValue("i",getFeaturePtr()->Mesh.getValue().countPoints()); 
}

PyObject*  MeshFeaturePy::countFacets(PyObject * /*args*/)
{
    return Py_BuildValue("i",getFeaturePtr()->Mesh.getValue().countFacets()); 
}

PyObject*  MeshFeaturePy::harmonizeNormals(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        Mesh::MeshObject *mesh = getFeaturePtr()->Mesh.startEditing();
        mesh->harmonizeNormals();
        getFeaturePtr()->Mesh.finishEditing();
    } PY_CATCH;

    Py_Return; 
}

PyObject*  MeshFeaturePy::smooth(PyObject *args)
{
    int iter=1;
    float d_max=FLOAT_MAX;
    if (!PyArg_ParseTuple(args, "|if", &iter,&d_max))
        return NULL;

    PY_TRY {
        getFeaturePtr()->Mesh.smooth(iter, d_max);
    } PY_CATCH;

    Py_Return; 
}

PyObject*  MeshFeaturePy::removeNonManifolds(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    getFeaturePtr()->Mesh.removeNonManifolds();
    Py_Return
}

PyObject*  MeshFeaturePy::fixIndices(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getFeaturePtr()->Mesh.validateIndices();
    } PY_CATCH;

    Py_Return; 
}

PyObject*  MeshFeaturePy::fixDegenerations(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getFeaturePtr()->Mesh.validateDegenerations();
    } PY_CATCH;

    Py_Return; 
}

PyObject*  MeshFeaturePy::removeDuplicatedFacets(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getFeaturePtr()->Mesh.removeDuplicatedFacets();
    } PY_CATCH;

    Py_Return; 
}

PyObject*  MeshFeaturePy::removeDuplicatedPoints(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getFeaturePtr()->Mesh.removeDuplicatedPoints();
    } PY_CATCH;

    Py_Return; 
}

PyObject*  MeshFeaturePy::fixSelfIntersections(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        getFeaturePtr()->Mesh.removeSelfIntersections();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
    Py_Return;
}

PyObject*  MeshFeaturePy::removeFoldsOnSurface(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        getFeaturePtr()->Mesh.removeFoldsOnSurface();
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
    Py_Return;
}

PyObject *MeshFeaturePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int MeshFeaturePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
