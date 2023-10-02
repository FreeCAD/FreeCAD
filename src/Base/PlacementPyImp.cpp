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

#include "GeometryPyCXX.h"
#include <Base/PyWrapParseTupleAndKeywords.h>

// inclusion of the generated files (generated out of PlacementPy.xml)
#include "PlacementPy.h"
#include "PlacementPy.cpp"
#include "Matrix.h"
#include "MatrixPy.h"
#include "RotationPy.h"
#include "VectorPy.h"
#include "Tools.h"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string PlacementPy::representation() const
{
    double yaw{}, pitch{}, roll{};
    PlacementPy::PointerType ptr = getPlacementPtr();
    std::stringstream str;
    ptr->getRotation().getYawPitchRoll(yaw, pitch, roll);

    str << "Placement [Pos=(";
    str << ptr->getPosition().x << ","<< ptr->getPosition().y << "," << ptr->getPosition().z;
    str << "), Yaw-Pitch-Roll=(" << yaw << "," << pitch << "," << roll << ")]";

    return str.str();
}

PyObject *PlacementPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PlacementPy and the Twin object
    return new PlacementPy(new Placement);
}

// constructor method
int PlacementPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o{};
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
        try {
            Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
            getPlacementPtr()->fromMatrix(mat);
            return 0;
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(e.getPyExceptionType(), e.what());
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &o)) {
        Base::Placement *plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
        *(getPlacementPtr()) = *plm;
        return 0;
    }

    PyErr_Clear();
    PyObject* d{};
    double angle{};
    if (PyArg_ParseTuple(args, "O!O!d", &(Base::VectorPy::Type), &o,
                                        &(Base::VectorPy::Type), &d, &angle)) {
        // NOTE: The first parameter defines the translation, the second the rotation axis
        // and the last parameter defines the rotation angle in degree.
        Base::Rotation rot(static_cast<Base::VectorPy*>(d)->value(), angle/180.0*D_PI);
        *getPlacementPtr() = Base::Placement(static_cast<Base::VectorPy*>(o)->value(),rot);
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type), &o,
                                       &(Base::RotationPy::Type), &d)) {
        Base::Vector3d *pos = static_cast<Base::VectorPy*>(o)->getVectorPtr();
        getPlacementPtr()->setPosition(*pos);
        Base::Rotation *rot = static_cast<Base::RotationPy*>(d)->getRotationPtr();
        getPlacementPtr()->setRotation(*rot);
        return 0;
    }

    PyErr_Clear();
    PyObject* c{};
    if (PyArg_ParseTuple(args, "O!O!O!", &(Base::VectorPy::Type), &o,
                                         &(Base::RotationPy::Type), &d,
                                         &(Base::VectorPy::Type), &c)) {
        Base::Vector3d *pos = static_cast<Base::VectorPy*>(o)->getVectorPtr();
        Base::Rotation *rot = static_cast<Base::RotationPy*>(d)->getRotationPtr();
        Base::Vector3d *cnt = static_cast<Base::VectorPy*>(c)->getVectorPtr();
        Base::Placement p(*pos,*rot,*cnt);
        getPlacementPtr()->operator = (p);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "empty parameter list, matrix or placement expected");
    return -1;
}

PyObject* PlacementPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(PlacementPy::Type)) &&
        PyObject_TypeCheck(w, &(PlacementPy::Type))) {
        Base::Placement p1 = *static_cast<PlacementPy*>(v)->getPlacementPtr();
        Base::Placement p2 = *static_cast<PlacementPy*>(w)->getPlacementPtr();

        PyObject *res=nullptr;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Placement");
            return nullptr;
        }
        else if (op == Py_EQ) {
            res = (p1 == p2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else {
            res = (p1 != p2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
    }
    else {
        // This always returns False
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
}

PyObject* PlacementPy::move(PyObject * args)
{
    PyObject *vec{};
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &vec))
        return nullptr;
    getPlacementPtr()->move(static_cast<VectorPy*>(vec)->value());
    Py_Return;
}

PyObject* PlacementPy::translate(PyObject * args)
{
    return move(args);
}

PyObject* PlacementPy::rotate(PyObject *args, PyObject *kwds)
{
    double angle{};
    static const std::array<const char *, 6> kwlist { "center", "axis", "angle", "comp", nullptr };
    Vector3d center;
    Vector3d axis;
    PyObject* pyComp = Py_False; // NOLINT

    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "(ddd)(ddd)d|O!", kwlist, &center.x, &center.y, &center.z,
                                             &axis.x, &axis.y, &axis.z, &angle, &PyBool_Type, &pyComp)) {
        return nullptr;
    }

    try {
        /*
         * if comp is False, we retain the original behaviour that - contrary to the (old) documentation - generates
         * Placements different from TopoShape.rotate() to ensure compatibility for existing code
         */
        bool comp = Base::asBoolean(pyComp);

        if (!comp) {
            getPlacementPtr()->multRight(Placement(Vector3d(), Rotation(axis, toRadians<double>(angle)), center));
        }
        else {
            // multiply new Placement the same way TopoShape.rotate() does
            getPlacementPtr()->multLeft(Placement(Vector3d(), Rotation(axis, toRadians<double>(angle)), center));
        }

        Py_Return;
    }
    catch (const Py::Exception&) {
        return nullptr;
    }
}

PyObject* PlacementPy::multiply(PyObject * args)
{
    PyObject *plm{};
    if (!PyArg_ParseTuple(args, "O!", &(PlacementPy::Type), &plm))
        return nullptr;
    Placement mult = (*getPlacementPtr()) * (*static_cast<PlacementPy*>(plm)->getPlacementPtr());
    return new PlacementPy(new Placement(mult));
}

PyObject* PlacementPy::multVec(PyObject * args)
{
    PyObject *vec{};
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &vec))
        return nullptr;
    Base::Vector3d pnt(static_cast<VectorPy*>(vec)->value());
    getPlacementPtr()->multVec(pnt, pnt);
    return new VectorPy(new Vector3d(pnt));
}

PyObject* PlacementPy::copy(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return new PlacementPy(new Placement(*getPlacementPtr()));
}

PyObject* PlacementPy::toMatrix(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Base::Matrix4D mat = getPlacementPtr()->toMatrix();
    return new MatrixPy(new Matrix4D(mat));
}

PyObject* PlacementPy::inverse(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Base::Placement p = getPlacementPtr()->inverse();
    return new PlacementPy(new Placement(p));
}

PyObject* PlacementPy::pow(PyObject* args)
{
    double t{};
    PyObject* shorten = Py_True;
    if (!PyArg_ParseTuple(args, "d|O!", &t, &(PyBool_Type), &shorten))
        return nullptr;
    Base::Placement ret = getPlacementPtr()->pow(t, Base::asBoolean(shorten));
    return new PlacementPy(new Placement(ret));
}


PyObject* PlacementPy::sclerp(PyObject* args)
{
    PyObject* pyplm2{};
    double t{};
    PyObject* shorten = Py_True;
    if (!PyArg_ParseTuple(args, "O!d|O!", &(PlacementPy::Type), &pyplm2, &t, &(PyBool_Type), &shorten))
        return nullptr;
    Base::Placement plm2 = static_cast<Base::PlacementPy*>(pyplm2)->value();
    Base::Placement ret = Base::Placement::sclerp(*getPlacementPtr(), plm2, t, Base::asBoolean(shorten));
    return new PlacementPy(new Placement(ret));
}

PyObject* PlacementPy::slerp(PyObject* args)
{
    PyObject* pyplm2{};
    double t{};
    if (!PyArg_ParseTuple(args, "O!d", &(PlacementPy::Type), &pyplm2, &t))
        return nullptr;
    Base::Placement plm2 = static_cast<Base::PlacementPy*>(pyplm2)->value();
    Base::Placement ret = Base::Placement::slerp(*getPlacementPtr(), plm2, t);
    return new PlacementPy(new Placement(ret));
}

PyObject* PlacementPy::isIdentity(PyObject *args)
{
    double tol = 0.0;
    if (!PyArg_ParseTuple(args, "|d", &tol))
        return nullptr;
    bool none = tol > 0 ? getPlacementPtr()->isIdentity(tol)
                        : getPlacementPtr()->isIdentity();
    return Py_BuildValue("O", (none ? Py_True : Py_False));
}

PyObject* PlacementPy::isSame(PyObject *args)
{
    PyObject* plm{};
    double tol = 0.0;
    if (!PyArg_ParseTuple(args, "O!|d", &PlacementPy::Type, &plm, &tol))
        return nullptr;

    Base::Placement plm1 = * getPlacementPtr();
    Base::Placement plm2 = * static_cast<PlacementPy*>(plm)->getPlacementPtr();
    bool same = tol > 0.0 ? plm1.isSame(plm2, tol) : plm1.isSame(plm2);
    return Py_BuildValue("O", (same ? Py_True : Py_False));
}

Py::Object PlacementPy::getBase() const
{
    return Py::Vector(getPlacementPtr()->getPosition()); // NOLINT
}

void PlacementPy::setBase(Py::Object arg)
{
    getPlacementPtr()->setPosition(Py::Vector(arg).toVector());
}

Py::Object PlacementPy::getRotation() const
{
    return Py::Rotation(getPlacementPtr()->getRotation());
}

void PlacementPy::setRotation(Py::Object arg)
{
    Py::Rotation rot;
    if (rot.accepts(arg.ptr())) {
        getPlacementPtr()->setRotation(static_cast<Base::Rotation>(Py::Rotation(arg)));
        return;
    }
    Py::Tuple tuple;
    if (tuple.accepts(arg.ptr())) {
        tuple = arg;
        getPlacementPtr()->setRotation(Base::Rotation(static_cast<double>(Py::Float(tuple[0])),
                                                      static_cast<double>(Py::Float(tuple[1])),
                                                      static_cast<double>(Py::Float(tuple[2])),
                                                      static_cast<double>(Py::Float(tuple[3]))
                                                     ));
        return;
    }

    throw Py::TypeError("either Rotation or tuple of four floats expected");
}

Py::Object PlacementPy::getMatrix() const
{
    return Py::Matrix(getPlacementPtr()->toMatrix());
}

void PlacementPy::setMatrix(Py::Object arg)
{
    Py::Matrix mat;
    if (!mat.accepts(arg.ptr()))
        throw Py::TypeError("Expect type Matrix");
    try {
        mat = arg;
        getPlacementPtr()->fromMatrix(mat);
    }
    catch (const Base::ValueError& e) {
        throw Py::ValueError(e.what());
    }
}

PyObject *PlacementPy::getCustomAttributes(const char* attr) const
{
    // for backward compatibility
    if (strcmp(attr, "isNull") == 0) {
        PyObject *w{}, *res{};
        w = PyUnicode_InternFromString("isIdentity");
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        res = PyObject_GenericGetAttr(const_cast<PlacementPy *>(this), w);
        Py_XDECREF(w);
        return res;
    }
    return nullptr;
}

int PlacementPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* PlacementPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (PyObject_TypeCheck(self, &(PlacementPy::Type))) {
        Base::Placement a = static_cast<PlacementPy*>(self)->value();

        if (PyObject_TypeCheck(other, &(VectorPy::Type))) {
            Vector3d res;
            a.multVec(static_cast<VectorPy*>(other)->value(),res);
            return new VectorPy(res);
        }

        if (PyObject_TypeCheck(other, &(RotationPy::Type))) {
            Placement b(Vector3d(),static_cast<RotationPy*>(other)->value());
            return new PlacementPy(a*b);
        }

        if (PyObject_TypeCheck(other, &(PlacementPy::Type))) {
            const auto &b = static_cast<PlacementPy*>(other)->value();
            return new PlacementPy(a*b);
        }

        if (PyObject_TypeCheck(other, &(MatrixPy::Type))) {
            const auto &b = static_cast<MatrixPy*>(other)->value();
            return new MatrixPy(a.toMatrix()*b);
        }
    }

    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_power_handler (PyObject* self, PyObject* other, PyObject* arg)
{
    Py::Object pw(other);
    Py::Tuple tup(1);
    tup[0] = pw;

    double pw_v{};
    if (!PyArg_ParseTuple(tup.ptr(), "d", &pw_v)){
        return nullptr;
    }
    if (!PyObject_TypeCheck(self, &(PlacementPy::Type))
        || arg != Py_None)
    {
        PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
        return nullptr;
    }

    Placement a = static_cast<PlacementPy*>(self)->value();
    return new PlacementPy(a.pow(pw_v));
}

PyObject* PlacementPy::number_add_handler(PyObject * /*self*/, PyObject * /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject* PlacementPy::number_subtract_handler(PyObject * /*self*/, PyObject * /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_divide_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_remainder_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_negative_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_positive_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_absolute_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

int PlacementPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * PlacementPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_int_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * PlacementPy::number_float_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

