/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <App/DocumentObject.h>
#include <Base/Vector3D.h>

#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
// inclusion of the generated files (generated out of DrawProjGroupPy.xml)
#include <Mod/TechDraw/App/DrawProjGroupPy.h>
#include <Mod/TechDraw/App/DrawProjGroupPy.cpp>
#include <Mod/TechDraw/App/DrawProjGroupItemPy.h>
#include <Base/VectorPy.h>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawProjGroupPy::representation() const
{
    return std::string("<DrawProjGroup object>");
}

PyObject* DrawProjGroupPy::addProjection(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    App::DocumentObject* docObj = projGroup->addProjection(projType);
    TechDraw::DrawProjGroupItem* newProj = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );
    if (!newProj) {
        PyErr_SetString(PyExc_TypeError, "wrong type for adding projection");
        return nullptr;
    }

    return new DrawProjGroupItemPy(newProj);
}

PyObject* DrawProjGroupPy::removeProjection(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    int i = projGroup->removeProjection(projType);

    return PyLong_FromLong((long) i);
}

PyObject* DrawProjGroupPy::purgeProjections(PyObject* /*args*/)
{
    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    int i = projGroup->purgeProjections();

    return PyLong_FromLong((long) i);
}

PyObject* DrawProjGroupPy::getItemByLabel(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    App::DocumentObject* docObj = projGroup->getProjObj(projType);
    TechDraw::DrawProjGroupItem* newProj = dynamic_cast<TechDraw::DrawProjGroupItem *>( docObj );
    if (!newProj) {
        PyErr_SetString(PyExc_TypeError, "wrong type for getting item");
        return nullptr;
    }

    return new DrawProjGroupItemPy(newProj);
}

PyObject* DrawProjGroupPy::getXYPosition(PyObject* args)
{
    char* projType;

    if (!PyArg_ParseTuple(args, "s", &projType)) {
        throw Py::Exception();
    }

    DrawProjGroup* projGroup = getDrawProjGroupPtr();
    Base::Vector3d v = projGroup->getXYPosition(projType);
    return new Base::VectorPy(v);
}



PyObject *DrawProjGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawProjGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
