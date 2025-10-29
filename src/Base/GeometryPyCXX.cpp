// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <sstream>

#include "GeometryPyCXX.h"
#include "VectorPy.h"


// NOLINTBEGIN(readability-identifier-length)
int Py::Vector::Vector_TypeCheck(PyObject* obj)
{
    return PyObject_TypeCheck(obj, &(Base::VectorPy::Type));
}

bool Py::Vector::accepts(PyObject* obj) const
{
    if (obj && Vector_TypeCheck(obj)) {
        return true;
    }
    if (obj && PySequence_Check(obj)) {
        return (PySequence_Size(obj) == 3);
    }

    return false;
}

Py::Vector::Vector(const Base::Vector3d& vec)
{
    set(new Base::VectorPy(vec), true);
    validate();
}

Py::Vector::Vector(const Base::Vector3f& vec)
{
    set(new Base::VectorPy(vec), true);
    validate();
}

Py::Vector& Py::Vector::operator=(PyObject* rhsp)
{
    if (ptr() == rhsp) {
        return *this;
    }
    set(rhsp, false);
    return *this;
}

Py::Vector& Py::Vector::operator=(const Base::Vector3d& vec)
{
    set(new Base::VectorPy(vec), true);
    return *this;
}

Py::Vector& Py::Vector::operator=(const Base::Vector3f& vec)
{
    set(new Base::VectorPy(vec), true);
    return *this;
}

Base::Vector3d Py::Vector::toVector() const
{
    if (Vector_TypeCheck(ptr())) {
        return static_cast<Base::VectorPy*>(ptr())->value();
    }

    return Base::getVectorFromTuple<double>(ptr());
}

namespace Base
{

Py::PythonType& Vector2dPy::behaviors()
{
    return Py::PythonClass<Vector2dPy>::behaviors();
}

PyTypeObject* Vector2dPy::type_object()
{
    return Py::PythonClass<Vector2dPy>::type_object();
}

bool Vector2dPy::check(PyObject* py)
{
    return Py::PythonClass<Vector2dPy>::check(py);
}

Py::PythonClassObject<Vector2dPy> Vector2dPy::create(const Vector2d& vec)
{
    return create(vec.x, vec.y);
}

Py::PythonClassObject<Vector2dPy> Vector2dPy::create(double vx, double vy)
{
    Py::Callable class_type(type());
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(vx));
    arg.setItem(1, Py::Float(vy));
    Py::PythonClassObject<Vector2dPy> py =
        Py::PythonClassObject<Vector2dPy>(class_type.apply(arg, Py::Dict()));
    return py;
}

Vector2dPy::Vector2dPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds)
    : Py::PythonClass<Vector2dPy>::PythonClass(self, args, kwds)
{
    double vx = 0;
    double vy = 0;
    if (!PyArg_ParseTuple(args.ptr(), "|dd", &vx, &vy)) {
        throw Py::Exception();
    }

    v.x = vx;
    v.y = vy;
}

Vector2dPy::~Vector2dPy() = default;

Py::Object Vector2dPy::repr()
{
    Py::Float vx(v.x);
    Py::Float vy(v.y);
    std::stringstream str;
    str << "Vector2 (";
    str << static_cast<std::string>(vx.repr()) << ", " << static_cast<std::string>(vy.repr());
    str << ")";

    return Py::String(str.str());  // NOLINT
}

Py::Object Vector2dPy::getattro(const Py::String& name_)
{
    // For Py3 either handle __dict__ or implement __dir__ as shown here:
    // https://stackoverflow.com/questions/48609111/how-is-dir-implemented-exactly-and-how-should-i-know-it
    //
    std::string name(name_.as_std_string("utf-8"));

    if (name == "__dict__") {
        Py::Dict attr;
        attr.setItem(Py::String("x"), Py::Float(v.x));
        attr.setItem(Py::String("y"), Py::Float(v.y));
        return attr;  // NOLINT
    }
    if (name == "x") {
        return Py::Float(v.x);  // NOLINT
    }
    if (name == "y") {
        return Py::Float(v.y);  // NOLINT
    }

    return genericGetAttro(name_);
}

int Vector2dPy::setattro(const Py::String& name_, const Py::Object& value)
{
    std::string name(name_.as_std_string("utf-8"));

    if (name == "x" && !value.isNull()) {
        v.x = static_cast<double>(Py::Float(value));
        return 0;
    }
    if (name == "y" && !value.isNull()) {
        v.y = static_cast<double>(Py::Float(value));
        return 0;
    }

    return genericSetAttro(name_, value);
}

Py::Object Vector2dPy::number_negative()
{
    return create(-v.x, -v.y);  // NOLINT
}

Py::Object Vector2dPy::number_positive()
{
    return create(v.x, v.y);  // NOLINT
}

Py::Object Vector2dPy::number_absolute()
{
    return create(fabs(v.x), fabs(v.y));  // NOLINT
}

Py::Object Vector2dPy::number_invert()
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_int()
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_float()
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_add(const Py::Object& py)
{
    Vector2d vec(Py::toVector2d(py));
    vec = v + vec;
    return create(vec);  // NOLINT
}

Py::Object Vector2dPy::number_subtract(const Py::Object& py)
{
    Vector2d vec(Py::toVector2d(py));
    vec = v - vec;
    return create(vec);  // NOLINT
}

Py::Object Vector2dPy::number_multiply(const Py::Object& py)
{
    if (PyObject_TypeCheck(py.ptr(), Vector2dPy::type_object())) {
        Vector2d vec(Py::toVector2d(py));
        double scalar = v * vec;
        return Py::Float(scalar);  // NOLINT
    }
    if (py.isNumeric()) {
        double scale = static_cast<double>(Py::Float(py));
        return create(v * scale);  // NOLINT
    }

    throw Py::TypeError("Argument must be Vector2d or Float");
}

Py::Object Vector2dPy::number_remainder(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_divmod(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_lshift(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_rshift(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_and(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_xor(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_or(const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::number_power(const Py::Object&, const Py::Object&)
{
    throw Py::TypeError("Not defined");
}

Py::Object Vector2dPy::isNull(const Py::Tuple& args)
{
    double tol = 0.0;
    if (args.size() > 0) {
        tol = static_cast<double>(Py::Float(args[0]));
    }
    return Py::Boolean(v.IsNull(tol));  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, isNull)

Py::Object Vector2dPy::length(const Py::Tuple&)
{
    return Py::Float(v.Length());  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, length)

Py::Object Vector2dPy::atan2(const Py::Tuple&)
{
    return Py::Float(v.Angle());  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, atan2)

Py::Object Vector2dPy::square(const Py::Tuple&)
{
    return Py::Float(v.Sqr());  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, square)

Py::Object Vector2dPy::scale(const Py::Tuple& args)
{
    double value = static_cast<double>(Py::Float(args[0]));
    v.Scale(value);
    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, scale)

Py::Object Vector2dPy::rotate(const Py::Tuple& args)
{
    double value = static_cast<double>(Py::Float(args[0]));
    v.Rotate(value);
    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, rotate)

Py::Object Vector2dPy::normalize(const Py::Tuple&)
{
    v.Normalize();
    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, normalize)

Py::Object Vector2dPy::perpendicular(const Py::Tuple& args)
{
    bool value = static_cast<bool>(Py::Boolean(args[0]));
    Base::Vector2d pnt = v.Perpendicular(value);
    return create(pnt);  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, perpendicular)

Py::Object Vector2dPy::distance(const Py::Tuple& args)
{
    Base::Vector2d pnt = Py::toVector2d(args[0]);
    return Py::Float(pnt.Distance(v));  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, distance)

Py::Object Vector2dPy::isEqual(const Py::Tuple& args)
{
    Base::Vector2d pnt = Py::toVector2d(args[0]);
    double tol = static_cast<double>(Py::Float(args[1]));
    return Py::Boolean(v.IsEqual(pnt, tol));  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, isEqual)

Py::Object Vector2dPy::getAngle(const Py::Tuple& args)
{
    Base::Vector2d vec = Py::toVector2d(args[0]);
    return Py::Float(v.GetAngle(vec));  // NOLINT
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, getAngle)

Py::Object Vector2dPy::projectToLine(const Py::Tuple& args)
{
    Base::Vector2d pnt1 = Py::toVector2d(args[0]);
    Base::Vector2d pnt2 = Py::toVector2d(args[1]);
    v.ProjectToLine(pnt1, pnt2);
    return Py::None();
}
PYCXX_VARARGS_METHOD_DECL(Vector2dPy, projectToLine)
// NOLINTEND(readability-identifier-length)

void Vector2dPy::init_type()
{
    behaviors().name("Vector2d");
    behaviors().doc("Vector2d class");
    behaviors().supportGetattro();
    behaviors().supportSetattro();
    behaviors().supportRepr();
    behaviors().supportNumberType();

    PYCXX_ADD_VARARGS_METHOD(isNull, isNull, "isNull()");
    PYCXX_ADD_VARARGS_METHOD(length, length, "length()");
    PYCXX_ADD_VARARGS_METHOD(atan2, atan2, "atan2()");
    PYCXX_ADD_VARARGS_METHOD(square, square, "square()");
    PYCXX_ADD_VARARGS_METHOD(scale, scale, "scale()");
    PYCXX_ADD_VARARGS_METHOD(rotate, rotate, "rotate()");
    PYCXX_ADD_VARARGS_METHOD(normalize, normalize, "normalize()");
    PYCXX_ADD_VARARGS_METHOD(perpendicular, perpendicular, "perpendicular()");
    PYCXX_ADD_VARARGS_METHOD(distance, distance, "distance()");
    PYCXX_ADD_VARARGS_METHOD(isEqual, isEqual, "isEqual()");
    PYCXX_ADD_VARARGS_METHOD(getAngle, getAngle, "getAngle()");
    PYCXX_ADD_VARARGS_METHOD(projectToLine, projectToLine, "projectToLine()");

    // Call to make the type ready for use
    behaviors().readyType();
}

}  // namespace Base
