/***************************************************************************
 *   Copyright (c) 2025 [Teu Nome]                                         *
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
#endif

#include <Base/VectorPy.h>

#include "NotePy.h"
#include "NotePy.cpp"
#include "OCCError.h"

using namespace Part;

std::string NotePy::representation() const
{
    std::stringstream str;
    Base::Vector3d pos = getGeomNotePtr()->getPosition();
    str << "<Note position=(" << pos.x << ", " << pos.y << ", " << pos.z << "), text=\"" 
        << getGeomNotePtr()->getText() << "\" >";
    return str.str();
}

PyObject* NotePy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    return new NotePy(new GeomNote);
}

int NotePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();

    PyObject* pVecOnly = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pVecOnly)) {
        Base::Vector3d vec = static_cast<Base::VectorPy*>(pVecOnly)->value();
        getGeomNotePtr()->setPosition(vec);
        return 0;
    }

    PyErr_Clear();

    PyObject* pVec = nullptr;
    const char* txt = nullptr;
    if (PyArg_ParseTuple(args, "O!s", &(Base::VectorPy::Type), &pVec, &txt)) {
        Base::Vector3d vec = static_cast<Base::VectorPy*>(pVec)->value();
        getGeomNotePtr()->setPosition(vec);
        getGeomNotePtr()->setText(txt);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Note constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Coordinates vector\n"
        "-- Coordinates vector and a string (position, text)");
    return -1;
}

Py::Float NotePy::getX() const
{
    return Py::Float(getGeomNotePtr()->getPosition().x);
}

void NotePy::setX(Py::Float x)
{
    auto pos = getGeomNotePtr()->getPosition();
    pos.x = double(x);
    getGeomNotePtr()->setPosition(pos);
}

Py::Float NotePy::getY() const
{
    return Py::Float(getGeomNotePtr()->getPosition().y);
}

void NotePy::setY(Py::Float y)
{
    auto pos = getGeomNotePtr()->getPosition();
    pos.y = double(y);
    getGeomNotePtr()->setPosition(pos);
}

Py::Float NotePy::getZ() const
{
    return Py::Float(getGeomNotePtr()->getPosition().z);
}

void NotePy::setZ(Py::Float z)
{
    auto pos = getGeomNotePtr()->getPosition();
    pos.z = double(z);
    getGeomNotePtr()->setPosition(pos);
}

Py::String NotePy::getText() const
{
    return Py::String(getGeomNotePtr()->getText());
}

void NotePy::setText(Py::String arg)
{
    std::string s = arg;
    getGeomNotePtr()->setText(s);
}

Py::Float NotePy::getFontSize() const
{
    return Py::Float(getGeomNotePtr()->getFontSize());
}

void NotePy::setFontSize(Py::Float fs)
{
    getGeomNotePtr()->setFontSize(double(fs));
}

Py::Tuple NotePy::getColor() const
{
    const float* c = getGeomNotePtr()->getColor();
    Py::Tuple tuple(4);
    for (int i=0; i<4; ++i) {
        tuple[i] = Py::Float(c[i]);
    }
    return tuple;
}

void NotePy::setColor(Py::Tuple arg)
{
    if (arg.length() != 4) {
        throw Py::TypeError("Color must be a tuple of 4 floats");
    }
    float rgba[4];
    for (int i = 0; i < 4; ++i) {
        Py::Object item = arg[i];
        rgba[i] = static_cast<float>(Py::Float(item));
    }
    getGeomNotePtr()->setColor(rgba);
}

PyObject* NotePy::getCustomAttributes(const char* attr) const
{
    return 0; 
}

int NotePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return 0; 
}
