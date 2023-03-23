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
//#include <array>

// inclusion of the generated files (generated out of MatrixPy.xml)
#include "RotationPy.h"
#include "VectorPy.h"
#include "GeometryPyCXX.h"
#include "QuantityPy.h"
#include "MatrixPy.h"
#include "MatrixPy.cpp"


using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string MatrixPy::representation() const
{
    const Base::Matrix4D& m = *(this->getMatrixPtr());
    std::stringstream str;
    str << "Matrix (";
    str << "(" << m[0][0] << ","<< m[0][1] << ","<< m[0][2] << ","<< m[0][3] << ")" << ",";
    str << "(" << m[1][0] << ","<< m[1][1] << ","<< m[1][2] << ","<< m[1][3] << ")"<< ",";
    str << "(" << m[2][0] << ","<< m[2][1] << ","<< m[2][2] << ","<< m[2][3] << ")"<< ",";
    str << "(" << m[3][0] << ","<< m[3][1] << ","<< m[3][2] << ","<< m[3][3] << ")";
    str << ")";

    return str.str();
}

PyObject *MatrixPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of MatrixPy and the Twin object
    return new MatrixPy(new Matrix4D);
}

// constructor method
int MatrixPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    double a11=1.0, a12=0.0, a13=0.0, a14=0.0;
    double a21=0.0, a22=1.0, a23=0.0, a24=0.0;
    double a31=0.0, a32=0.0, a33=1.0, a34=0.0;
    double a41=0.0, a42=0.0, a43=0.0, a44=1.0;

    if (PyArg_ParseTuple(args, "|dddddddddddddddd",
                          &a11,&a12,&a13,&a14,
                          &a21,&a22,&a23,&a24,
                          &a31,&a32,&a33,&a34,
                          &a41,&a42,&a43,&a44)) {
        MatrixPy::PointerType ptr = getMatrixPtr();
        (*ptr) = Matrix4D(a11,a12,a13,a14,
                          a21,a22,a23,a24,
                          a31,a32,a33,a34,
                          a41,a42,a43,a44);
        return 0;
    }

    PyErr_Clear();
    PyObject *o{};
    if (PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
        MatrixPy::PointerType ptr = getMatrixPtr();
        (*ptr) = static_cast<MatrixPy*>(o)->value();
        return 0;
    }

    PyErr_Clear();
    PyObject *o1{};
    PyObject *o2{};
    PyObject *o3{};
    PyObject *o4{};
    if (PyArg_ParseTuple(args, "O!O!O!|O!", &(Base::VectorPy::Type), &o1
                                          , &(Base::VectorPy::Type), &o2
                                          , &(Base::VectorPy::Type), &o3
                                          , &(Base::VectorPy::Type), &o4)) {
        Base::Vector3d v1 = Py::Vector(o1, false).toVector();
        Base::Vector3d v2 = Py::Vector(o2, false).toVector();
        Base::Vector3d v3 = Py::Vector(o3, false).toVector();
        Base::Vector3d v4;
        if (o4)
            v4 = Py::Vector(o4, false).toVector();
        MatrixPy::PointerType ptr = this->getMatrixPtr();

        (*ptr)[0][0] = v1.x;
        (*ptr)[1][0] = v1.y;
        (*ptr)[2][0] = v1.z;

        (*ptr)[0][1] = v2.x;
        (*ptr)[1][1] = v2.y;
        (*ptr)[2][1] = v2.z;

        (*ptr)[0][2] = v3.x;
        (*ptr)[1][2] = v3.y;
        (*ptr)[2][2] = v3.z;

        (*ptr)[0][3] = v4.x;
        (*ptr)[1][3] = v4.y;
        (*ptr)[2][3] = v4.z;

        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Base.Matrix, four Base.Vector or up to 16 floats expected");
    return -1;
}

PyObject* MatrixPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Matrix");
        return nullptr;
    }
    if (!PyObject_TypeCheck(other, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Matrix");
        return nullptr;
    }
    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    Base::Matrix4D b = static_cast<MatrixPy*>(other)->value();
    return new MatrixPy(a+b);
}

PyObject* MatrixPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Matrix");
        return nullptr;
    }
    if (!PyObject_TypeCheck(other, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Matrix");
        return nullptr;
    }
    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    Base::Matrix4D b = static_cast<MatrixPy*>(other)->value();
    return new MatrixPy(a-b);
}

PyObject* MatrixPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();

        if (PyObject_TypeCheck(other, &(VectorPy::Type))) {
            auto b = static_cast<VectorPy*>(other)->value();
            return new VectorPy(a*b);
        }

        if (PyObject_TypeCheck(other, &(RotationPy::Type))) {
            auto r = static_cast<RotationPy*>(other)->value();
            Matrix4D b;
            r.getValue(b);
            return new MatrixPy(a*b);
        }

        if (PyObject_TypeCheck(other, &(PlacementPy::Type))) {
            auto b = static_cast<PlacementPy*>(other)->value();
            return new MatrixPy(a*b.toMatrix());
        }

        if (PyObject_TypeCheck(other, &(MatrixPy::Type))) {
            Base::Matrix4D b = static_cast<MatrixPy*>(other)->value();
            return new MatrixPy(a*b);
        }

        if (PyNumber_Check(other)) {
            double v = PyFloat_AsDouble(other);
            return new MatrixPy(a * v);
        }
    }

    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_power_handler (PyObject* self, PyObject* other, PyObject* arg)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type)) ||

            !PyLong_Check(other)
            || arg != Py_None
       )
    {
        PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
        return nullptr;
    }

    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();

    long b = Py::Int(other);
    if (b == 0)
        return new MatrixPy(Matrix4D());

    if (b < 0) {
        if (fabs(a.determinant()) > DBL_EPSILON)
            a.inverseGauss();
        else {
            PyErr_SetString(PyExc_RuntimeError, "Cannot invert singular matrix");
            return nullptr;
        }
        b = -b;
    }

    auto res = a;
    for (--b;b;--b)
        res *= a;
    return new MatrixPy(res);
}

PyObject* MatrixPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(MatrixPy::Type)) &&
        PyObject_TypeCheck(w, &(MatrixPy::Type))) {
        Matrix4D m1 = static_cast<MatrixPy*>(v)->value();
        Matrix4D m2 = static_cast<MatrixPy*>(w)->value();

        PyObject *res=nullptr;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Matrix");
            return nullptr;
        }
        else if (op == Py_EQ) {
            res = (m1 == m2) ? Py_True : Py_False; //NOLINT
            Py_INCREF(res);
            return res;
        }
        else {
            res = (m1 != m2) ? Py_True : Py_False; //NOLINT
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

PyObject* MatrixPy::move(PyObject * args)
{
    double x{},y{},z{};
    Base::Vector3d vec;
    PyObject *pcVecObj{};

    do { // dummy do..while for cascaded if
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            vec.x = x;
            vec.y = y;
            vec.z = z;
            break;
        }
        // clears the error from previous PyArg_ParseTuple()
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!",
        &PyTuple_Type, &pcVecObj)) {
            vec = getVectorFromTuple<double>(pcVecObj);
            break;
        }
        // clears the error from previous PyArg_ParseTuple()
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!;three floats, or a tuple, or a vector is needed",
        &(Base::VectorPy::Type), &pcVecObj)) {
            Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(pcVecObj);
            Base::Vector3d* val = pcObject->getVectorPtr();
            vec.Set(val->x,val->y,val->z);
            break;
        }
        return nullptr;
    }
    while(false);

    PY_TRY {
        getMatrixPtr()->move(vec);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::scale(PyObject * args)
{
    double x{},y{},z{};
    Base::Vector3d vec;
    PyObject *pcVecObj{};

    do { // dummy do..while for cascaded if
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            vec.x = x;
            vec.y = y;
            vec.z = z;
            break;
        }
        // clears the error from previous PyArg_ParseTuple()
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "d", &x)) {
            vec.x = vec.y = vec.z = x;
            break;
        }
        // clears the error from previous PyArg_ParseTuple()
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!", &PyTuple_Type, &pcVecObj)) {
            vec = getVectorFromTuple<double>(pcVecObj);
            break;
        }
        // clears the error from previous PyArg_ParseTuple()
        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!;one or three floats, or a tuple, or a vector is needed",
        &(Base::VectorPy::Type), &pcVecObj)) {
            Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(pcVecObj);
            Base::Vector3d* val = pcObject->getVectorPtr();
            vec.Set(val->x,val->y,val->z);
            break;
        }
        return nullptr;
    }
    while(false);

    PY_TRY {
        getMatrixPtr()->scale(vec);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::hasScale(PyObject * args)
{
    double tol=0;
    if (!PyArg_ParseTuple(args, "|d", &tol))
        return nullptr;

    ScaleType type = getMatrixPtr()->hasScale(tol);
    Py::Module mod("FreeCAD");
    return Py::new_reference_to(mod.callMemberFunction("ScaleType", Py::TupleN(Py::Int(static_cast<int>(type)))));
}
PyObject* MatrixPy::decompose(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
      return nullptr;

    auto ms = getMatrixPtr()->decompose();
    Py::Tuple tuple(4);
    for (int i=0; i<4; i++) {
        tuple.setItem(i, Py::Matrix(ms[i]));
    }
    return Py::new_reference_to(tuple);
}

PyObject* MatrixPy::nullify()
{
    PY_TRY {
        getMatrixPtr()->nullify();
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::isNull()
{
    PY_TRY {
        bool ok = getMatrixPtr()->isNull();
        return Py::new_reference_to(Py::Boolean(ok));
    }
    PY_CATCH;
}

PyObject* MatrixPy::unity()
{
    PY_TRY {
        getMatrixPtr()->setToUnity();
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::isUnity()
{
    PY_TRY {
        bool ok = getMatrixPtr()->isUnity();
        return Py::new_reference_to(Py::Boolean(ok));
    }
    PY_CATCH;
}

PyObject* MatrixPy::transform(PyObject * args)
{
    Base::Vector3d vec;
    Matrix4D mat;
    PyObject *pcVecObj{}, *pcMatObj{};

    if (!PyArg_ParseTuple(args, "O!O!: a transform point (Vector) and a transform matrix (Matrix) is needed",
        &(Base::VectorPy::Type), &pcVecObj, &(MatrixPy::Type), &pcMatObj))
        return nullptr;

    Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(pcVecObj);
    Base::Vector3d* val = pcObject->getVectorPtr();
    vec.Set(val->x,val->y,val->z);
    mat = *(static_cast<MatrixPy*>(pcMatObj)->getMatrixPtr());

    getMatrixPtr()->transform(vec,mat);
    Py_Return;
}

PyObject* MatrixPy::col(PyObject * args)
{
    int index{};
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    if (index < 0 || index > 3) {
        PyErr_SetString(PyExc_ValueError, "Index expected in the range [0, 3]");
        return nullptr;
    }

    Matrix4D* mat = getMatrixPtr();
    Base::Vector3d v = mat->getCol(index);
    return Py::new_reference_to(Py::Vector(v));
}

PyObject* MatrixPy::setCol(PyObject * args)
{
    int index{};
    PyObject* o{};
    if (!PyArg_ParseTuple(args, "iO!", &index, &(VectorPy::Type), &o))
        return nullptr;

    if (index < 0 || index > 3) {
        PyErr_SetString(PyExc_ValueError, "Index expected in the range [0, 3]");
        return nullptr;
    }

    Base::Vector3d v = Py::Vector(o, false).toVector();
    Matrix4D* mat = getMatrixPtr();
    mat->setCol(index, v);
    Py_Return;
}

PyObject* MatrixPy::row(PyObject * args)
{
    int index{};
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    if (index < 0 || index > 3) {
        PyErr_SetString(PyExc_ValueError, "Index expected in the range [0, 3]");
        return nullptr;
    }

    Matrix4D* mat = getMatrixPtr();
    Base::Vector3d v = mat->getRow(index);
    return Py::new_reference_to(Py::Vector(v));
}

PyObject* MatrixPy::setRow(PyObject * args)
{
    int index{};
    PyObject* o{};
    if (!PyArg_ParseTuple(args, "iO!", &index, &(VectorPy::Type), &o))
        return nullptr;

    if (index < 0 || index > 3) {
        PyErr_SetString(PyExc_ValueError, "Index expected in the range [0, 3]");
        return nullptr;
    }

    Base::Vector3d v = Py::Vector(o, false).toVector();
    Matrix4D* mat = getMatrixPtr();
    mat->setRow(index, v);
    Py_Return;
}

PyObject* MatrixPy::diagonal()
{
    Matrix4D* mat = getMatrixPtr();
    Base::Vector3d v = mat->diagonal();
    return Py::new_reference_to(Py::Vector(v));
}

PyObject* MatrixPy::setDiagonal(PyObject * args)
{
    PyObject* o{};
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &o))
        return nullptr;

    Base::Vector3d v = Py::Vector(o, false).toVector();
    Matrix4D* mat = getMatrixPtr();
    mat->setDiagonal(v);
    Py_Return;
}

PyObject* MatrixPy::rotateX(PyObject * args)
{
    double angle = 0;
    do {
        PyObject *object{};
        if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
            Quantity *q = static_cast<Base::QuantityPy*>(object)->getQuantityPtr();
            if (q->getUnit() == Base::Unit::Angle) {
                angle = q->getValueAs(Base::Quantity::Radian);
                break;
            }
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args, "d: angle to rotate (double) needed", &angle)) {
            break;
        }

        PyErr_SetString(PyExc_TypeError, "For angle either float or Quantity expected");
        return nullptr;
    }
    while (false);

    PY_TRY {
        getMatrixPtr()->rotX(angle);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::rotateY(PyObject * args)
{
    double angle = 0;
    do {
        PyObject *object{};
        if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
            Quantity *q = static_cast<Base::QuantityPy*>(object)->getQuantityPtr();
            if (q->getUnit() == Base::Unit::Angle) {
                angle = q->getValueAs(Base::Quantity::Radian);
                break;
            }
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args, "d: angle to rotate (double) needed", &angle)) {
            break;
        }

        PyErr_SetString(PyExc_TypeError, "For angle either float or Quantity expected");
        return nullptr;
    }
    while (false);

    PY_TRY {
        getMatrixPtr()->rotY(angle);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::rotateZ(PyObject * args)
{
    double angle = 0;
    do {
        PyObject *object{};
        if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
            Quantity *q = static_cast<Base::QuantityPy*>(object)->getQuantityPtr();
            if (q->getUnit() == Base::Unit::Angle) {
                angle = q->getValueAs(Base::Quantity::Radian);
                break;
            }
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args, "d: angle to rotate (double) needed", &angle)) {
            break;
        }

        PyErr_SetString(PyExc_TypeError, "For angle either float or Quantity expected");
        return nullptr;
    }
    while (false);

    PY_TRY {
        getMatrixPtr()->rotZ(angle);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::multiply(PyObject * args)
{
    PyObject* o{};
    if (PyArg_ParseTuple(args, "O!", &(MatrixPy::Type), &o)) {
        Matrix4D mat = (*getMatrixPtr()) * static_cast<Base::MatrixPy*>(o)->value();
        return new MatrixPy(new Matrix4D(mat));
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &o)) {
        Vector3d vec = (*getMatrixPtr()) * static_cast<Base::VectorPy*>(o)->value();
        return new VectorPy(new Vector3d(vec));
    }

    PyErr_SetString(PyExc_TypeError, "either vector or matrix expected");
    return nullptr;
}

PyObject* MatrixPy::multVec(PyObject * args)
{
    PyObject *obj{};
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return nullptr;

    Base::Vector3d vec(static_cast<VectorPy*>(obj)->value());
    getMatrixPtr()->multVec(vec, vec);
    return new VectorPy(new Vector3d(vec));
}

PyObject* MatrixPy::invert()
{
    PY_TRY {
        if (fabs(getMatrixPtr()->determinant()) > DBL_EPSILON) {
            getMatrixPtr()->inverseGauss();
            Py_Return;
        }
        else {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot invert singular matrix");
            return nullptr;
        }
    }
    PY_CATCH;
}

PyObject* MatrixPy::inverse()
{
    PY_TRY {
        if (fabs(getMatrixPtr()->determinant()) > DBL_EPSILON) {
            Base::Matrix4D m = *getMatrixPtr();
            m.inverseGauss();
            return new MatrixPy(m);
        }
        else {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot invert singular matrix");
            return nullptr;
        }
    }
    PY_CATCH;
}

PyObject* MatrixPy::determinant()
{
    return PyFloat_FromDouble(getMatrixPtr()->determinant());
}

PyObject* MatrixPy::submatrix(PyObject * args)
{
    int dim{};
    if (!PyArg_ParseTuple(args, "i", &dim))
        return nullptr;

    if (dim < 1 || dim > 4) {
        PyErr_SetString(PyExc_IndexError, "Dimension out of range");
        return nullptr;
    }

    const Base::Matrix4D& mat = *getMatrixPtr();
    Base::Matrix4D sub;
    switch (dim)
    {
    case 1:
        sub[0][0] = mat[0][0];
        break;
    case 2:
        sub[0][0] = mat[0][0]; sub[0][1] = mat[0][1];
        sub[1][0] = mat[1][0]; sub[1][1] = mat[1][1];
        break;
    case 3:
        sub[0][0] = mat[0][0]; sub[0][1] = mat[0][1]; sub[0][2] = mat[0][2];
        sub[1][0] = mat[1][0]; sub[1][1] = mat[1][1]; sub[1][2] = mat[1][2];
        sub[2][0] = mat[2][0]; sub[2][1] = mat[2][1]; sub[2][2] = mat[2][2];
        break;
    default:
        sub = mat;
        break;
    }

    return new MatrixPy(sub);
}

PyObject* MatrixPy::isOrthogonal(PyObject * args)
{
    double eps=1.0e-06;
    if (!PyArg_ParseTuple(args, "|d",&eps))
        return nullptr;

    const Base::Matrix4D& mat = *getMatrixPtr();
    Base::Matrix4D trp = mat;
    trp.transpose();
    trp = trp * mat;

    bool ok = true;
    double mult = trp[0][0];
    for (unsigned short i=0; i<4 && ok; i++) {
        for (unsigned short j=0; j<4 && ok; j++) {
            if (i != j) {
                if (fabs(trp[i][j]) > eps) {
                    ok = false;
                    break;
                }
            }
            else { // the main diagonal
                if (fabs(trp[i][j]-mult) > eps) {
                    ok = false;
                    break;
                }
            }
        }
    }

    return Py::new_reference_to(Py::Float(ok ? mult : 0.0));
}

PyObject* MatrixPy::transposed()
{
    PY_TRY {
        Base::Matrix4D m = *getMatrixPtr();
        m.transpose();
        return new MatrixPy(m);
    }
    PY_CATCH;
}

PyObject* MatrixPy::transpose()
{
    PY_TRY {
        getMatrixPtr()->transpose();
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::analyze()
{
    PY_TRY {
        std::string type = getMatrixPtr()->analyse();
        return PyUnicode_FromString(type.c_str());
    }
    PY_CATCH;
}

Py::Float MatrixPy::getA11() const
{
    double val = (*this->getMatrixPtr())[0][0];
    return Py::Float(val);
}

void  MatrixPy::setA11(Py::Float arg)
{
    (*this->getMatrixPtr())[0][0] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA12() const
{
    double val = (*this->getMatrixPtr())[0][1];
    return Py::Float(val);
}

void  MatrixPy::setA12(Py::Float arg)
{
    (*this->getMatrixPtr())[0][1] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA13() const
{
    double val = (*this->getMatrixPtr())[0][2];
    return Py::Float(val);
}

void  MatrixPy::setA13(Py::Float arg)
{
    (*this->getMatrixPtr())[0][2] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA14() const
{
    double val = (*this->getMatrixPtr())[0][3];
    return Py::Float(val);
}

void  MatrixPy::setA14(Py::Float arg)
{
    (*this->getMatrixPtr())[0][3] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA21() const
{
    double val = (*this->getMatrixPtr())[1][0];
    return Py::Float(val);
}

void  MatrixPy::setA21(Py::Float arg)
{
    (*this->getMatrixPtr())[1][0] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA22() const
{
    double val = (*this->getMatrixPtr())[1][1];
    return Py::Float(val);
}

void  MatrixPy::setA22(Py::Float arg)
{
    (*this->getMatrixPtr())[1][1] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA23() const
{
    double val = (*this->getMatrixPtr())[1][2];
    return Py::Float(val);
}

void  MatrixPy::setA23(Py::Float arg)
{
    (*this->getMatrixPtr())[1][2] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA24() const
{
    double val = (*this->getMatrixPtr())[1][3];
    return Py::Float(val);
}

void  MatrixPy::setA24(Py::Float arg)
{
    (*this->getMatrixPtr())[1][3] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA31() const
{
    double val = (*this->getMatrixPtr())[2][0];
    return Py::Float(val);
}

void  MatrixPy::setA31(Py::Float arg)
{
    (*this->getMatrixPtr())[2][0] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA32() const
{
    double val = (*this->getMatrixPtr())[2][1];
    return Py::Float(val);
}

void  MatrixPy::setA32(Py::Float arg)
{
    (*this->getMatrixPtr())[2][1] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA33() const
{
    double val = (*this->getMatrixPtr())[2][2];
    return Py::Float(val);
}

void  MatrixPy::setA33(Py::Float arg)
{
    (*this->getMatrixPtr())[2][2] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA34() const
{
    double val = (*this->getMatrixPtr())[2][3];
    return Py::Float(val);
}

void  MatrixPy::setA34(Py::Float arg)
{
    (*this->getMatrixPtr())[2][3] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA41() const
{
    double val = (*this->getMatrixPtr())[3][0];
    return Py::Float(val);
}

void  MatrixPy::setA41(Py::Float arg)
{
    (*this->getMatrixPtr())[3][0] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA42() const
{
    double val = (*this->getMatrixPtr())[3][1];
    return Py::Float(val);
}

void  MatrixPy::setA42(Py::Float arg)
{
    (*this->getMatrixPtr())[3][1] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA43() const
{
    double val = (*this->getMatrixPtr())[3][2];
    return Py::Float(val);
}

void  MatrixPy::setA43(Py::Float arg)
{
    (*this->getMatrixPtr())[3][2] = static_cast<double>(arg);
}

Py::Float MatrixPy::getA44() const
{
    double val = (*this->getMatrixPtr())[3][3];
    return Py::Float(val);
}

void  MatrixPy::setA44(Py::Float arg)
{
    (*this->getMatrixPtr())[3][3] = static_cast<double>(arg);
}

Py::Sequence MatrixPy::getA() const
{
    double mat[16];
    this->getMatrixPtr()->getMatrix(mat);
    Py::Tuple tuple(16);
    for (int i=0; i<16; i++) {
        tuple[i] = Py::Float(mat[i]);
    }
    return std::move(tuple);
}

void MatrixPy::setA(Py::Sequence arg)
{
    double mat[16];
    this->getMatrixPtr()->getMatrix(mat);

    int index=0;
    for (Py::Sequence::iterator it = arg.begin(); it != arg.end() && index < 16; ++it) {
        mat[index++] = static_cast<double>(Py::Float(*it));
    }

    this->getMatrixPtr()->setMatrix(mat);
}

PyObject *MatrixPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MatrixPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject * MatrixPy::number_divide_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_remainder_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_negative_handler (PyObject* self)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "arg must be Matrix");
        return nullptr;
    }

    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    return new MatrixPy(a * -1);
}

PyObject * MatrixPy::number_positive_handler (PyObject* self)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "arg must be Matrix");
        return nullptr;
    }

    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    return new MatrixPy(a);
}

PyObject * MatrixPy::number_absolute_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

int MatrixPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * MatrixPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_int_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * MatrixPy::number_float_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}
