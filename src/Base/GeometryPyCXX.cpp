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
#endif

#include "GeometryPyCXX.h"
#include "VectorPy.h"


int Py::Vector::Vector_TypeCheck(PyObject * obj)
{
    return PyObject_TypeCheck(obj, &(Base::VectorPy::Type));
}

bool Py::Vector::accepts (PyObject *obj) const
{
    if (obj && Vector_TypeCheck (obj)) {
        return true;
    }
    else if (PyTuple_Check(obj)) {
        return (PyTuple_Size(obj) == 3);
    }

    return false;
}

Py::Vector::Vector (const Base::Vector3d& v)
{
    set(new Base::VectorPy(v), true);
    validate();
}

Py::Vector::Vector (const Base::Vector3f& v)
{
    set(new Base::VectorPy(v), true);
    validate();
}

Py::Vector& Py::Vector::operator= (PyObject* rhsp)
{
    if(ptr() == rhsp) return *this;
    set (rhsp, false);
    return *this;
}

Py::Vector& Py::Vector::operator= (const Base::Vector3d& v)
{
    set (new Base::VectorPy(v), true);
    return *this;
}

Py::Vector& Py::Vector::operator= (const Base::Vector3f& v)
{
    set (new Base::VectorPy(v), true);
    return *this;
}

Base::Vector3d Py::Vector::toVector() const
{
    if (Vector_TypeCheck (ptr())) {
        return static_cast<Base::VectorPy*>(ptr())->value();
    }
    else {
        return Base::getVectorFromTuple<double>(ptr());
    }
}
