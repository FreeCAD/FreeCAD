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

#include <climits>
#include <cmath>
#include "Base/Matrix.h"

// inclusion of the generated files (generated out of MatrixPy.xml)
#include "VectorPy.h"
#include "GeometryPyCXX.h"
#include "QuantityPy.h"
#include "MatrixPy.h"
#include "MatrixPy.cpp"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string MatrixPy::representation(void) const
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
        MatrixPy::PointerType ptr = reinterpret_cast<MatrixPy::PointerType>(_pcTwinPointer);
        (*ptr) = Matrix4D(a11,a12,a13,a14,
                          a21,a22,a23,a24,
                          a31,a32,a33,a34,
                          a41,a42,a43,a44);
        return 0;
    }

    PyErr_Clear();
    PyObject *o;
    if (PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
        MatrixPy::PointerType ptr = reinterpret_cast<MatrixPy::PointerType>(_pcTwinPointer);
        (*ptr) = static_cast<MatrixPy*>(o)->value();
        return 0;
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "matrix or up to 16 floats expected");
    return -1;
}

PyObject* MatrixPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Matrix");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Matrix");
        return 0;
    }
    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    Base::Matrix4D b = static_cast<MatrixPy*>(other)->value();
    return new MatrixPy(a+b);
}

PyObject* MatrixPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Matrix");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Matrix");
        return 0;
    }
    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    Base::Matrix4D b = static_cast<MatrixPy*>(other)->value();
    return new MatrixPy(a-b);
}

PyObject* MatrixPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Matrix");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(MatrixPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Matrix");
        return 0;
    }
    Base::Matrix4D a = static_cast<MatrixPy*>(self)->value();
    Base::Matrix4D b = static_cast<MatrixPy*>(other)->value();
    return new MatrixPy(a*b);
}

PyObject* MatrixPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(MatrixPy::Type)) &&
        PyObject_TypeCheck(w, &(MatrixPy::Type))) {
        Matrix4D m1 = static_cast<MatrixPy*>(v)->value();
        Matrix4D m2 = static_cast<MatrixPy*>(w)->value();

        PyObject *res=0;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Matrix");
            return 0;
        }
        else if (op == Py_EQ) {
            res = (m1 == m2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else {
            res = (m1 != m2) ? Py_True : Py_False;
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
    double x,y,z;
    Base::Vector3d vec;
    PyObject *pcVecObj;

    if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {   // convert args: Python->C
        vec.x = x;
        vec.y = y;
        vec.z = z;
    }
    else if (PyArg_ParseTuple(args, "O!:three floats or a vector is needed", 
        &PyTuple_Type, &pcVecObj)) {
        vec = getVectorFromTuple<double>(pcVecObj);
        // clears the error from the first PyArg_ParseTuple()6
        PyErr_Clear();
    }
    else if (PyArg_ParseTuple(args, "O!:three floats or a vector is needed", 
        &(Base::VectorPy::Type), &pcVecObj)) {
        Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(pcVecObj);
        Base::Vector3d* val = pcObject->getVectorPtr();
        vec.Set(val->x,val->y,val->z);
        // clears the error from the first PyArg_ParseTuple()6
        PyErr_Clear();
    }
    else
        return NULL;

    PY_TRY {
        getMatrixPtr()->move(vec);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::scale(PyObject * args)
{
    double x,y,z;
    Base::Vector3d vec;
    PyObject *pcVecObj;

    if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {   // convert args: Python->C
        vec.x = x;
        vec.y = y;
        vec.z = z;
    }
    else if (PyArg_ParseTuple(args, "O!:three floats or a vector is needed", 
        &PyTuple_Type, &pcVecObj)) {
        vec = getVectorFromTuple<double>(pcVecObj);
        // clears the error from the first PyArg_ParseTuple()6
        PyErr_Clear();
    }
    else if (PyArg_ParseTuple(args, "O!:three floats or a vector is needed", &(Base::VectorPy::Type), &pcVecObj)) {
        // convert args: Python->C
        Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(pcVecObj);
        Base::Vector3d* val = pcObject->getVectorPtr();
        vec.Set(val->x,val->y,val->z);
        // clears the error from the first PyArg_ParseTuple()6
        PyErr_Clear();
    }
    else
        return NULL;

    PY_TRY {
        getMatrixPtr()->scale(vec);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::unity(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C
      return NULL;                             // NULL triggers exception
    PY_TRY {
        getMatrixPtr()->setToUnity();
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::transform(PyObject * args)
{
    Base::Vector3d vec;
    Matrix4D mat;
    PyObject *pcVecObj,*pcMatObj;

    if (PyArg_ParseTuple(args, "O!O!: a transform point (Vector) and a transform matrix (Matrix) is needed",
        &(Base::VectorPy::Type), &pcVecObj, &(MatrixPy::Type), &pcMatObj) ) {   // convert args: Python->C
        Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(pcVecObj);
        Base::Vector3d* val = pcObject->getVectorPtr();
        vec.Set(val->x,val->y,val->z);
        mat = *(static_cast<MatrixPy*>(pcMatObj)->getMatrixPtr());
        // clears the error from the first PyArg_ParseTuple()6
        PyErr_Clear();
    }
    else
        return NULL;                                 // NULL triggers exception

    PY_TRY {
        getMatrixPtr()->transform(vec,mat);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::rotateX(PyObject * args)
{
    double angle = 0;
    do {
        PyObject *object;
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
        return 0;
    }
    while (false);

    PY_TRY {
        getMatrixPtr()->rotX(angle);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::rotateY(PyObject * args)
{
    double angle = 0;
    do {
        PyObject *object;
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
        return 0;
    }
    while (false);

    PY_TRY {
        getMatrixPtr()->rotY(angle);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::rotateZ(PyObject * args)
{
    double angle = 0;
    do {
        PyObject *object;
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
        return 0;
    }
    while (false);

    PY_TRY {
        getMatrixPtr()->rotZ(angle);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::multiply(PyObject * args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(MatrixPy::Type), &o)) {
        Matrix4D mat = (*getMatrixPtr()) * static_cast<Base::MatrixPy*>(o)->value();
        return new MatrixPy(new Matrix4D(mat));
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &o)) {
        Vector3d vec = (*getMatrixPtr()) * static_cast<Base::VectorPy*>(o)->value();
        return new VectorPy(new Vector3d(vec));
    }

    PyErr_SetString(Base::BaseExceptionFreeCADError, "either vector or matrix expected");
    return 0;
}

PyObject* MatrixPy::multVec(PyObject * args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return NULL;
    Base::Vector3d vec(static_cast<VectorPy*>(obj)->value());
    getMatrixPtr()->multVec(vec, vec);
    return new VectorPy(new Vector3d(vec));
}

PyObject* MatrixPy::invert(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        if (fabs(getMatrixPtr()->determinant()) > DBL_EPSILON)
            getMatrixPtr()->inverseGauss();
        else {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Cannot invert singular matrix");
            return 0;
        }
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::inverse(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        if (fabs(getMatrixPtr()->determinant()) > DBL_EPSILON) {
            Base::Matrix4D m = *getMatrixPtr();
            m.inverseGauss();
            return new MatrixPy(m);
        }
        else {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Cannot invert singular matrix");
            return 0;
        }
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::determinant(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return PyFloat_FromDouble(getMatrixPtr()->determinant());
}

PyObject* MatrixPy::submatrix(PyObject * args)
{
    int dim;
    if (!PyArg_ParseTuple(args, "i", &dim))
        return NULL;
    if (dim < 1 || dim > 4) {
        PyErr_SetString(PyExc_IndexError, "Dimension out of range");
        return NULL;
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
        return 0;
    const Base::Matrix4D& mat = *getMatrixPtr();
    Base::Matrix4D trp = mat;
    trp.transpose();
    trp = trp * mat;

    bool ok = true;
    double mult = trp[0][0];
    for (int i=0; i<4 && ok; i++) {
        for (int j=0; j<4 && ok; j++) {
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

PyObject* MatrixPy::transposed(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        Base::Matrix4D m = *getMatrixPtr();
        m.transpose();
        return new MatrixPy(m);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* MatrixPy::transpose(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        getMatrixPtr()->transpose();
        Py_Return;
    }
    PY_CATCH;
}

PyObject* MatrixPy::analyze(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY {
        std::string type = getMatrixPtr()->analyse();
#if PY_MAJOR_VERSION < 3
        return PyString_FromString(type.c_str());
#else
        return PyUnicode_FromString(type.c_str());
#endif
    }
    PY_CATCH;
}

Py::Float MatrixPy::getA11(void) const
{
    double val = (*this->getMatrixPtr())[0][0];
    return Py::Float(val);
}

void  MatrixPy::setA11(Py::Float arg)
{
    (*this->getMatrixPtr())[0][0] = (double)arg;
}

Py::Float MatrixPy::getA12(void) const
{
    double val = (*this->getMatrixPtr())[0][1];
    return Py::Float(val);
}

void  MatrixPy::setA12(Py::Float arg)
{
    (*this->getMatrixPtr())[0][1] = (double)arg;
}

Py::Float MatrixPy::getA13(void) const
{
    double val = (*this->getMatrixPtr())[0][2];
    return Py::Float(val);
}

void  MatrixPy::setA13(Py::Float arg)
{
    (*this->getMatrixPtr())[0][2] = (double)arg;
}

Py::Float MatrixPy::getA14(void) const
{
    double val = (*this->getMatrixPtr())[0][3];
    return Py::Float(val);
}

void  MatrixPy::setA14(Py::Float arg)
{
    (*this->getMatrixPtr())[0][3] = (double)arg;
}

Py::Float MatrixPy::getA21(void) const
{
    double val = (*this->getMatrixPtr())[1][0];
    return Py::Float(val);
}

void  MatrixPy::setA21(Py::Float arg)
{
    (*this->getMatrixPtr())[1][0] = (double)arg;
}

Py::Float MatrixPy::getA22(void) const
{
    double val = (*this->getMatrixPtr())[1][1];
    return Py::Float(val);
}

void  MatrixPy::setA22(Py::Float arg)
{
    (*this->getMatrixPtr())[1][1] = (double)arg;
}

Py::Float MatrixPy::getA23(void) const
{
    double val = (*this->getMatrixPtr())[1][2];
    return Py::Float(val);
}

void  MatrixPy::setA23(Py::Float arg)
{
    (*this->getMatrixPtr())[1][2] = (double)arg;
}

Py::Float MatrixPy::getA24(void) const
{
    double val = (*this->getMatrixPtr())[1][3];
    return Py::Float(val);
}

void  MatrixPy::setA24(Py::Float arg)
{
    (*this->getMatrixPtr())[1][3] = (double)arg;
}

Py::Float MatrixPy::getA31(void) const
{
    double val = (*this->getMatrixPtr())[2][0];
    return Py::Float(val);
}

void  MatrixPy::setA31(Py::Float arg)
{
    (*this->getMatrixPtr())[2][0] = (double)arg;
}

Py::Float MatrixPy::getA32(void) const
{
    double val = (*this->getMatrixPtr())[2][1];
    return Py::Float(val);
}

void  MatrixPy::setA32(Py::Float arg)
{
    (*this->getMatrixPtr())[2][1] = (double)arg;
}

Py::Float MatrixPy::getA33(void) const
{
    double val = (*this->getMatrixPtr())[2][2];
    return Py::Float(val);
}

void  MatrixPy::setA33(Py::Float arg)
{
    (*this->getMatrixPtr())[2][2] = (double)arg;
}

Py::Float MatrixPy::getA34(void) const
{
    double val = (*this->getMatrixPtr())[2][3];
    return Py::Float(val);
}

void  MatrixPy::setA34(Py::Float arg)
{
    (*this->getMatrixPtr())[2][3] = (double)arg;
}

Py::Float MatrixPy::getA41(void) const
{
    double val = (*this->getMatrixPtr())[3][0];
    return Py::Float(val);
}

void  MatrixPy::setA41(Py::Float arg)
{
    (*this->getMatrixPtr())[3][0] = (double)arg;
}

Py::Float MatrixPy::getA42(void) const
{
    double val = (*this->getMatrixPtr())[3][1];
    return Py::Float(val);
}

void  MatrixPy::setA42(Py::Float arg)
{
    (*this->getMatrixPtr())[3][1] = (double)arg;
}

Py::Float MatrixPy::getA43(void) const
{
    double val = (*this->getMatrixPtr())[3][2];
    return Py::Float(val);
}

void  MatrixPy::setA43(Py::Float arg)
{
    (*this->getMatrixPtr())[3][2] = (double)arg;
}

Py::Float MatrixPy::getA44(void) const
{
    double val = (*this->getMatrixPtr())[3][3];
    return Py::Float(val);
}

void  MatrixPy::setA44(Py::Float arg)
{
    (*this->getMatrixPtr())[3][3] = (double)arg;
}

Py::Sequence MatrixPy::getA(void) const
{
    double mat[16];
    this->getMatrixPtr()->getMatrix(mat);
    Py::Tuple tuple(16);
    for (int i=0; i<16; i++) {
        tuple[i] = Py::Float(mat[i]);
    }
    return tuple;
}

void MatrixPy::setA(Py::Sequence arg)
{
    double mat[16];
    this->getMatrixPtr()->getMatrix(mat);

    int index=0;
    for (Py::Sequence::iterator it = arg.begin(); it != arg.end() && index < 16; ++it) {
        mat[index++] = (double)Py::Float(*it);
    }

    this->getMatrixPtr()->setMatrix(mat);
}

PyObject *MatrixPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int MatrixPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

PyObject * MatrixPy::number_divide_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_remainder_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_power_handler (PyObject* /*self*/, PyObject* /*other*/, PyObject* /*arg*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_negative_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_positive_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_absolute_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

int MatrixPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * MatrixPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

#if PY_MAJOR_VERSION < 3
int MatrixPy::number_coerce_handler (PyObject ** /*self*/, PyObject ** /*other*/)
{
    return 1;
}
#endif

PyObject * MatrixPy::number_int_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

#if PY_MAJOR_VERSION < 3
PyObject * MatrixPy::number_long_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}
#endif

PyObject * MatrixPy::number_float_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

#if PY_MAJOR_VERSION < 3
PyObject * MatrixPy::number_oct_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * MatrixPy::number_hex_handler (PyObject * /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}
#endif
