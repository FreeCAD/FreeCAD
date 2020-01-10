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

#include "GeometryPyCXX.h"
#include <Base/VectorPy.h>


int Py::Vector::Vector_TypeCheck(PyObject * obj)
{
    return PyObject_TypeCheck(obj, &(Base::VectorPy::Type));
}

bool Py::Vector::accepts (PyObject *obj) const
{
    if (obj && Vector_TypeCheck (obj)) {
        return true;
    }
    else if (obj && PySequence_Check(obj)) {
        return (PySequence_Size(obj) == 3);
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

namespace Base {

Vector2dPy::Vector2dPy(Py::PythonClassInstance *self, Py::Tuple &args, Py::Dict &kwds)
    : Py::PythonClass<Vector2dPy>::PythonClass(self, args, kwds)
{
    double x=0,y=0;
    if (!PyArg_ParseTuple(args.ptr(), "|dd", &x, &y)) {
        throw Py::Exception();
    }

    v.x = x;
    v.y = y;
}

Vector2dPy::~Vector2dPy()
{
}

void Vector2dPy::init_type(void)
{
    behaviors().name( "Vector2d" );
    behaviors().doc( "Vector2d class" );
    behaviors().supportGetattro();
    behaviors().supportSetattro();
    behaviors().supportRepr();
    // Call to make the type ready for use
    behaviors().readyType();
}

Py::Object Vector2dPy::repr()
{
    Py::Float x(v.x);
    Py::Float y(v.y);
    std::stringstream str;
    str << "Vector2 (";
    str << (std::string)x.repr() << ", "<< (std::string)y.repr();
    str << ")";

    return Py::String(str.str());
}

Py::Object Vector2dPy::getattro(const Py::String &name_)
{
    // For Py3 either handle __dict__ or implement __dir__ as shown here:
    // https://stackoverflow.com/questions/48609111/how-is-dir-implemented-exactly-and-how-should-i-know-it
    //
    std::string name( name_.as_std_string( "utf-8" ) );

    if (name == "__members__") { // Py2
        Py::List attr;
        attr.append(Py::String("x"));
        attr.append(Py::String("y"));
        return attr;
    }
    else if (name == "__dict__") { // Py3
        Py::Dict attr;
        attr.setItem(Py::String("x"), Py::Float(v.x));
        attr.setItem(Py::String("y"), Py::Float(v.y));
        return attr;
    }
    else if (name == "x") {
        return Py::Float(v.x);
    }
    else if (name == "y") {
        return Py::Float(v.y);
    }
    else {
        return genericGetAttro( name_ );
    }
}

int Vector2dPy::setattro(const Py::String &name_, const Py::Object &value)
{
    std::string name( name_.as_std_string( "utf-8" ) );

    if (name == "x" && !value.isNull()) {
        v.x = static_cast<double>(Py::Float(value));
        return 0;
    }
    else if (name == "y" && !value.isNull()) {
        v.y = static_cast<double>(Py::Float(value));
        return 0;
    }
    else {
        return genericSetAttro( name_, value );
    }
}

}
