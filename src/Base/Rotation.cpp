/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <array>

#include <boost/algorithm/string/predicate.hpp>
#include "Base/Exception.h"

#include "Rotation.h"
#include "Matrix.h"
#include "Precision.h"


using namespace Base;

Rotation::Rotation()
    : quat{0.0,0.0,0.0,1.0}
    , _axis{0.0,0.0,1.0}
    , _angle{0.0}
{
}

/** Construct a rotation by rotation axis and angle */
Rotation::Rotation(const Vector3d& axis, const double fAngle)
    : Rotation()
{
    // set to (0,0,1) as fallback in case the passed axis is the null vector
    _axis.Set(0.0, 0.0, 1.0);
    this->setValue(axis, fAngle);
}

Rotation::Rotation(const Matrix4D& matrix)
    : Rotation()
{
    this->setValue(matrix);
}

/** Construct a rotation initialized with the given quaternion components:
 * q[0] = x, q[1] = y, q[2] = z and q[3] = w,
 * where the quaternion is specified by q=w+xi+yj+zk.
 */
Rotation::Rotation(const double q[4])
    : Rotation()
{
    this->setValue(q);
}

/** Construct a rotation initialized with the given quaternion components:
 * q0 = x, q1 = y, q2 = z and q3 = w,
 * where the quaternion is specified by q=w+xi+yj+zk.
 */
Rotation::Rotation(const double q0, const double q1, const double q2, const double q3)
    : Rotation()
{
    this->setValue(q0, q1, q2, q3);
}

Rotation::Rotation(const Vector3d & rotateFrom, const Vector3d & rotateTo)
    : Rotation()
{
    this->setValue(rotateFrom, rotateTo);
}

Rotation::Rotation(const Rotation& rot)
    : Rotation()
{
    this->quat[0] = rot.quat[0];
    this->quat[1] = rot.quat[1];
    this->quat[2] = rot.quat[2];
    this->quat[3] = rot.quat[3];

    this->_axis[0] = rot._axis[0];
    this->_axis[1] = rot._axis[1];
    this->_axis[2] = rot._axis[2];
    this->_angle   = rot._angle;
}

void Rotation::operator = (const Rotation& rot)
{
    this->quat[0] = rot.quat[0];
    this->quat[1] = rot.quat[1];
    this->quat[2] = rot.quat[2];
    this->quat[3] = rot.quat[3];

    this->_axis[0] = rot._axis[0];
    this->_axis[1] = rot._axis[1];
    this->_axis[2] = rot._axis[2];
    this->_angle   = rot._angle;
}

const double * Rotation::getValue() const
{
    return &this->quat[0];
}

void Rotation::getValue(double & q0, double & q1, double & q2, double & q3) const
{
    q0 = this->quat[0];
    q1 = this->quat[1];
    q2 = this->quat[2];
    q3 = this->quat[3];
}

void Rotation::evaluateVector()
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    //
    // Note: -1 < w < +1 (|w| == 1 not allowed, with w:=quat[3])
    if ((this->quat[3] > -1.0) && (this->quat[3] < 1.0)) {
        double rfAngle = acos(this->quat[3]) * 2.0;
        double scale = sin(rfAngle / 2.0);
        // Get a normalized vector
        double l = this->_axis.Length();
        if (l < Base::Vector3d::epsilon()) l = 1;
        this->_axis.x = this->quat[0] * l / scale;
        this->_axis.y = this->quat[1] * l / scale;
        this->_axis.z = this->quat[2] * l / scale;

        _angle = rfAngle;
    }
    else {
        _axis.Set(0.0, 0.0, 1.0);
        _angle = 0.0;
    }
}

void Rotation::setValue(const double q0, const double q1, const double q2, const double q3)
{
    this->quat[0] = q0;
    this->quat[1] = q1;
    this->quat[2] = q2;
    this->quat[3] = q3;
    this->normalize();
    this->evaluateVector();
}

void Rotation::getValue(Vector3d & axis, double & rfAngle) const
{
    rfAngle = _angle;
    axis.x = _axis.x;
    axis.y = _axis.y;
    axis.z = _axis.z;
    axis.Normalize();
}

void Rotation::getRawValue(Vector3d & axis, double & rfAngle) const
{
    rfAngle = _angle;
    axis.x = _axis.x;
    axis.y = _axis.y;
    axis.z = _axis.z;
}

/**
 * Returns this rotation in form of a matrix.
 */
void Rotation::getValue(Matrix4D & matrix) const
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    //
    const double l = sqrt(this->quat[0] * this->quat[0] +
                          this->quat[1] * this->quat[1] +
                          this->quat[2] * this->quat[2] +
                          this->quat[3] * this->quat[3]);
    const double x = this->quat[0] / l;
    const double y = this->quat[1] / l;
    const double z = this->quat[2] / l;
    const double w = this->quat[3] / l;

    matrix[0][0] = 1.0-2.0*(y*y+z*z);
    matrix[0][1] = 2.0*(x*y-z*w);
    matrix[0][2] = 2.0*(x*z+y*w);
    matrix[0][3] = 0.0;

    matrix[1][0] = 2.0*(x*y+z*w);
    matrix[1][1] = 1.0-2.0*(x*x+z*z);
    matrix[1][2] = 2.0*(y*z-x*w);
    matrix[1][3] = 0.0;

    matrix[2][0] = 2.0*(x*z-y*w);
    matrix[2][1] = 2.0*(y*z+x*w);
    matrix[2][2] = 1.0-2.0*(x*x+y*y);
    matrix[2][3] = 0.0;

    matrix[3][0] = 0.0;
    matrix[3][1] = 0.0;
    matrix[3][2] = 0.0;
    matrix[3][3] = 1.0;
}

void Rotation::setValue(const double q[4])
{
    this->quat[0] = q[0];
    this->quat[1] = q[1];
    this->quat[2] = q[2];
    this->quat[3] = q[3];
    this->normalize();
    this->evaluateVector();
}

void Rotation::setValue(const Matrix4D & m)
{
    // Get the rotation part matrix
    Matrix4D mc = m.decompose()[2];
    // Extract quaternion
    double trace = (mc[0][0] + mc[1][1] + mc[2][2]);
    if (trace > 0.0) {
        double s = sqrt(1.0+trace);
        this->quat[3] = 0.5 * s;
        s = 0.5 / s;
        this->quat[0] = ((mc[2][1] - mc[1][2]) * s);
        this->quat[1] = ((mc[0][2] - mc[2][0]) * s);
        this->quat[2] = ((mc[1][0] - mc[0][1]) * s);
    }
    else {
        // Described in RotationIssues.pdf from <http://www.geometrictools.com>
        //
        // Get the max. element of the trace
        unsigned short i = 0;
        if (mc[1][1] > mc[0][0]) i = 1;
        if (mc[2][2] > mc[i][i]) i = 2;

        unsigned short j = (i+1)%3;
        unsigned short k = (i+2)%3;

        double s = sqrt((mc[i][i] - (mc[j][j] + mc[k][k])) + 1.0);
        this->quat[i] = s * 0.5;
        s = 0.5 / s;
        this->quat[3] = ((mc[k][j] - mc[j][k]) * s);
        this->quat[j] = ((mc[j][i] + mc[i][j]) * s);
        this->quat[k] = ((mc[k][i] + mc[i][k]) * s);
    }

    this->evaluateVector();
}

void Rotation::setValue(const Vector3d & axis, const double fAngle)
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    //
    // normalization of the angle to be in [0, 2pi[
    _angle = fAngle;
    double theAngle = fAngle - floor(fAngle / (2.0 * D_PI))*(2.0 * D_PI);
    this->quat[3] = cos(theAngle/2.0);

    Vector3d norm = axis;
    norm.Normalize();
    double l = norm.Length();
    // Keep old axis in case the new axis is the null vector
    if (l > 0.5) {
        this->_axis = axis;
    }
    else {
        norm = _axis;
        norm.Normalize();
    }

    double scale = sin(theAngle/2.0);
    this->quat[0] = norm.x * scale;
    this->quat[1] = norm.y * scale;
    this->quat[2] = norm.z * scale;
}

void Rotation::setValue(const Vector3d & rotateFrom, const Vector3d & rotateTo)
{
    Vector3d u(rotateFrom); u.Normalize();
    Vector3d v(rotateTo); v.Normalize();

    // The vector from x to is the rotation axis because it's the normal of the plane defined by (0,u,v)
    const double dot = u * v;
    Vector3d w = u % v;
    const double wlen = w.Length();

    if (wlen == 0.0) { // Parallel vectors
        // Check if they are pointing in the same direction.
        if (dot > 0.0) {
            this->setValue(0.0, 0.0, 0.0, 1.0);
        }
        else {
            // We can use any axis perpendicular to u (and v)
            Vector3d t = u % Vector3d(1.0, 0.0, 0.0);
            if (t.Length() < Base::Vector3d::epsilon())
                t = u % Vector3d(0.0, 1.0, 0.0);
            this->setValue(t.x, t.y, t.z, 0.0);
        }
    }
    else { // Vectors are not parallel
        // Note: A quaternion is not well-defined by specifying a point and its transformed point.
        // Every quaternion with a rotation axis having the same angle to the vectors of both points is okay.
        double angle = acos(dot);
        this->setValue(w, angle);
    }
}

void Rotation::normalize()
{
    double len = sqrt(this->quat[0]*this->quat[0]+
                      this->quat[1]*this->quat[1]+
                      this->quat[2]*this->quat[2]+
                      this->quat[3]*this->quat[3]);
    if (len > 0.0) {
        this->quat[0] /= len;
        this->quat[1] /= len;
        this->quat[2] /= len;
        this->quat[3] /= len;
    }
}

Rotation & Rotation::invert()
{
    this->quat[0] = -this->quat[0];
    this->quat[1] = -this->quat[1];
    this->quat[2] = -this->quat[2];

    this->_axis.x = -this->_axis.x;
    this->_axis.y = -this->_axis.y;
    this->_axis.z = -this->_axis.z;

    return *this;
}

Rotation Rotation::inverse() const
{
    Rotation rot;
    rot.quat[0] = -this->quat[0];
    rot.quat[1] = -this->quat[1];
    rot.quat[2] = -this->quat[2];
    rot.quat[3] =  this->quat[3];

    rot._axis[0] = -this->_axis[0];
    rot._axis[1] = -this->_axis[1];
    rot._axis[2] = -this->_axis[2];
    rot._angle = this->_angle;
    return rot;
}

/*!
  Let this rotation be right-multiplied by \a q. Returns reference to
  self.

  \sa multRight()
*/
Rotation & Rotation::operator*=(const Rotation & q)
{
    return multRight(q);
}

Rotation Rotation::operator*(const Rotation & q) const
{
    Rotation quat(*this);
    quat *= q;
    return quat;
}

/*!
  Let this rotation be right-multiplied by \a q. Returns reference to
  self.

  \sa multLeft()
*/
Rotation& Rotation::multRight(const Base::Rotation& q)
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    double x0{}, y0{}, z0{}, w0{};
    this->getValue(x0, y0, z0, w0);
    double x1{}, y1{}, z1{}, w1{};
    q.getValue(x1, y1, z1, w1);

    this->setValue(w0*x1 + x0*w1 + y0*z1 - z0*y1,
                   w0*y1 - x0*z1 + y0*w1 + z0*x1,
                   w0*z1 + x0*y1 - y0*x1 + z0*w1,
                   w0*w1 - x0*x1 - y0*y1 - z0*z1);
    return *this;
}

/*!
  Let this rotation be left-multiplied by \a q. Returns reference to
  self.

  \sa multRight()
*/
Rotation& Rotation::multLeft(const Base::Rotation& q)
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    double x0{}, y0{}, z0{}, w0{};
    q.getValue(x0, y0, z0, w0);
    double x1{}, y1{}, z1{}, w1{};
    this->getValue(x1, y1, z1, w1);

    this->setValue(w0*x1 + x0*w1 + y0*z1 - z0*y1,
                   w0*y1 - x0*z1 + y0*w1 + z0*x1,
                   w0*z1 + x0*y1 - y0*x1 + z0*w1,
                   w0*w1 - x0*x1 - y0*y1 - z0*z1);
    return *this;
}

bool Rotation::operator==(const Rotation & q) const
{
    if ((this->quat[0] == q.quat[0] &&
         this->quat[1] == q.quat[1] &&
         this->quat[2] == q.quat[2] &&
         this->quat[3] == q.quat[3]) ||
        (this->quat[0] == -q.quat[0] &&
         this->quat[1] == -q.quat[1] &&
         this->quat[2] == -q.quat[2] &&
         this->quat[3] == -q.quat[3]))
        return true;
    return false;
}

bool Rotation::operator!=(const Rotation & q) const
{
    return !(*this == q);
}

Vector3d Rotation::multVec(const Vector3d & src) const
{
    Vector3d dst;
    multVec(src,dst);
    return dst;
}

void Rotation::multVec(const Vector3d & src, Vector3d & dst) const
{
    double x = this->quat[0];
    double y = this->quat[1];
    double z = this->quat[2];
    double w = this->quat[3];
    double x2 = x * x;
    double y2 = y * y;
    double z2 = z * z;
    double w2 = w * w;

    double dx = (x2+w2-y2-z2)*src.x + 2.0*(x*y-z*w)*src.y + 2.0*(x*z+y*w)*src.z;
    double dy = 2.0*(x*y+z*w)*src.x + (w2-x2+y2-z2)*src.y + 2.0*(y*z-x*w)*src.z;
    double dz = 2.0*(x*z-y*w)*src.x + 2.0*(x*w+y*z)*src.y + (w2-x2-y2+z2)*src.z;
    dst.x = dx;
    dst.y = dy;
    dst.z = dz;
}

void Rotation::multVec(const Vector3f & src, Vector3f & dst) const
{
    Base::Vector3d srcd = Base::toVector<double>(src);
    multVec(srcd, srcd);
    dst = Base::toVector<float>(srcd);
}

Vector3f Rotation::multVec(const Vector3f & src) const
{
    Vector3f dst;
    multVec(src,dst);
    return dst;
}

void Rotation::scaleAngle(const double scaleFactor)
{
    Vector3d axis;
    double fAngle{};
    this->getValue(axis, fAngle);
    this->setValue(axis, fAngle * scaleFactor);
}

Rotation Rotation::slerp(const Rotation & q0, const Rotation & q1, double t)
{
    // Taken from <http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/>
    if (t<0.0) t=0.0;
    else if (t>1.0) t=1.0;

    double scale0 = 1.0 - t;
    double scale1 = t;
    double dot = q0.quat[0]*q1.quat[0]+q0.quat[1]*q1.quat[1]+q0.quat[2]*q1.quat[2]+q0.quat[3]*q1.quat[3];
    bool neg=false;
    if (dot < 0.0) {
        dot = -dot;
        neg = true;
    }

    if ((1.0 - dot) > Base::Vector3d::epsilon()) {
        double angle = acos(dot);
        double sinangle = sin(angle);
        // If possible calculate spherical interpolation, otherwise use linear interpolation
        if (sinangle > Base::Vector3d::epsilon()) {
            scale0 = double(sin((1.0 - t) * angle)) / sinangle;
            scale1 = double(sin(t * angle)) / sinangle;
        }
    }

    if (neg)
        scale1 = -scale1;

    double x = scale0 * q0.quat[0] + scale1 * q1.quat[0];
    double y = scale0 * q0.quat[1] + scale1 * q1.quat[1];
    double z = scale0 * q0.quat[2] + scale1 * q1.quat[2];
    double w = scale0 * q0.quat[3] + scale1 * q1.quat[3];
    return {x, y, z, w};
}

Rotation Rotation::identity()
{
    return {0.0, 0.0, 0.0, 1.0};
}

Rotation Rotation::makeRotationByAxes(Vector3d xdir, Vector3d ydir, Vector3d zdir, const char* priorityOrder)
{
    const double tol = Precision::Confusion();
    enum dirIndex {
        X,
        Y,
        Z
    };

    //convert priorityOrder string into a sequence of ints.
    if (strlen(priorityOrder)!=3)
        THROWM(ValueError, "makeRotationByAxes: length of priorityOrder is not 3");
    int order[3];
    for (int i = 0; i < 3; ++i){
        order[i] = priorityOrder[i] - 'X';
        if (order[i] < 0 || order[i] > 2)
            THROWM(ValueError, "makeRotationByAxes: characters in priorityOrder must be uppercase X, Y, or Z. Some other character encountered.")
    }

    //ensure every axis is listed in priority list
    if( order[0] == order[1] ||
        order[1] == order[2] ||
        order[2] == order[0])
        THROWM(ValueError,"makeRotationByAxes: not all axes are listed in priorityOrder");


    //group up dirs into an array, to access them by indexes stored in @order.
    std::vector<Vector3d*> dirs = {&xdir, &ydir, &zdir};


    auto dropPriority = [&order](int index){
        int tmp{};
        if (index == 0){
            tmp = order[0];
            order[0] = order[1];
            order[1] = order[2];
            order[2] = tmp;
        } else if (index == 1) {
            tmp = order[1];
            order[1] = order[2];
            order[2] = tmp;
        } //else if index == 2 do nothing
    };

    //pick up the strict direction
    Vector3d mainDir;
    for (int i = 0; i < 3; ++i){
        mainDir = *(dirs[size_t(order[0])]);
        if (mainDir.Length() > tol)
            break;
        else
            dropPriority(0);
        if (i == 2)
            THROWM(ValueError, "makeRotationByAxes: all directions supplied are zero");
    }
    mainDir.Normalize();

    //pick up the 2nd priority direction, "hint" direction.
    Vector3d hintDir;
    for (int i = 0; i < 2; ++i){
        hintDir = *(dirs[size_t(order[1])]);
        if ((hintDir.Cross(mainDir)).Length() > tol)
            break;
        else
            dropPriority(1);
        if (i == 1)
            hintDir = Vector3d(); //no vector can be used as hint direction. Zero it out, to indicate that a guess is needed.
    }
    if (hintDir.Length() == 0.0){
        switch (order[0]){
        case X: { //xdir is main
            //align zdir to OZ
            order[1] = Z;
            order[2] = Y;
            hintDir = Vector3d(0,0,1);
            if ((hintDir.Cross(mainDir)).Length() <= tol){
                //aligning to OZ is impossible, align to ydir to OY. Why so? I don't know, just feels right =)
                hintDir = Vector3d(0,1,0);
                order[1] = Y;
                order[2] = Z;
            }
        } break;
        case Y: { //ydir is main
            //align zdir to OZ
            order[1] = Z;
            order[2] = X;
            hintDir = mainDir.z > -tol ? Vector3d(0,0,1) : Vector3d(0,0,-1);
            if ((hintDir.Cross(mainDir)).Length() <= tol){
                //aligning zdir to OZ is impossible, align xdir to OX then.
                hintDir = Vector3d(1,0,0);
                order[1] = X;
                order[2] = Z;
            }
        } break;
        case Z: { //zdir is main
            //align ydir to OZ
            order[1] = Y;
            order[2] = X;
            hintDir =  Vector3d(0,0,1);
            if ((hintDir.Cross(mainDir)).Length() <= tol){
                //aligning ydir to OZ is impossible, align xdir to OX then.
                hintDir = Vector3d(1,0,0);
                order[1] = X;
                order[2] = Y;
            }
        } break;
        }//switch ordet[0]
    }

    //ensure every axis is listed in priority list
    assert(order[0] != order[1]);
    assert(order[1] != order[2]);
    assert(order[2] != order[0]);

    hintDir.Normalize();
    //make hintDir perpendicular to mainDir. For that, we cross-product the two to obtain the third axis direction, and then recover back the hint axis by doing another cross product.
    Vector3d lastDir = mainDir.Cross(hintDir);
    lastDir.Normalize();
    hintDir = lastDir.Cross(mainDir);
    hintDir.Normalize(); //redundant?

    Vector3d finaldirs[3];
    finaldirs[order[0]] = mainDir;
    finaldirs[order[1]] = hintDir;
    finaldirs[order[2]] = lastDir;

    //fix handedness
    if (finaldirs[X].Cross(finaldirs[Y]) * finaldirs[Z] < 0.0)
        //handedness is wrong. Switch the direction of the least important axis
        finaldirs[order[2]] = finaldirs[order[2]] * (-1.0);

    //build the rotation, by constructing a matrix first.
    Matrix4D m;
    m.setToUnity();
    for (int i = 0; i < 3; ++i){
        //matrix indexing: [row][col]
        m[0][i] = finaldirs[i].x;
        m[1][i] = finaldirs[i].y;
        m[2][i] = finaldirs[i].z;
    }

    return {m};
}

void Rotation::setYawPitchRoll(double y, double p, double r)
{
    // The Euler angles (yaw,pitch,roll) are in XY'Z''-notation
    // convert to radians
    y = (y/180.0)*D_PI;
    p = (p/180.0)*D_PI;
    r = (r/180.0)*D_PI;

    double c1 = cos(y/2.0);
    double s1 = sin(y/2.0);
    double c2 = cos(p/2.0);
    double s2 = sin(p/2.0);
    double c3 = cos(r/2.0);
    double s3 = sin(r/2.0);

    this->setValue (
      c1*c2*s3 - s1*s2*c3,
      c1*s2*c3 + s1*c2*s3,
      s1*c2*c3 - c1*s2*s3,
      c1*c2*c3 + s1*s2*s3
    );
}

void Rotation::getYawPitchRoll(double& y, double& p, double& r) const
{
    double q00 = quat[0]*quat[0];
    double q11 = quat[1]*quat[1];
    double q22 = quat[2]*quat[2];
    double q33 = quat[3]*quat[3];
    double q01 = quat[0]*quat[1];
    double q02 = quat[0]*quat[2];
    double q03 = quat[0]*quat[3];
    double q12 = quat[1]*quat[2];
    double q13 = quat[1]*quat[3];
    double q23 = quat[2]*quat[3];
    double qd2 = 2.0*(q13-q02);

    // handle gimbal lock
    if (fabs(qd2-1.0) <= 16 * DBL_EPSILON) { // Tolerance copied from OCC "gp_Quaternion.cxx"
        // north pole
        y = 0.0;
        p = D_PI/2.0;
        r = 2.0 * atan2(quat[0],quat[3]);
    }
    else if (fabs(qd2+1.0) <= 16 * DBL_EPSILON) { // Tolerance copied from OCC "gp_Quaternion.cxx"
        // south pole
        y = 0.0;
        p = -D_PI/2.0;
        r = 2.0 * atan2(quat[0],quat[3]);
    }
    else {
        y = atan2(2.0*(q01+q23),(q00+q33)-(q11+q22));
        p = qd2 > 1.0 ? D_PI/2.0 : (qd2 < -1.0 ? -D_PI/2.0 : asin (qd2));
        r = atan2(2.0*(q12+q03),(q22+q33)-(q00+q11));
    }

    // convert to degree
    y = (y/D_PI)*180;
    p = (p/D_PI)*180;
    r = (r/D_PI)*180;
}

bool Rotation::isSame(const Rotation& q) const
{
    if ((this->quat[0] == q.quat[0] &&
         this->quat[1] == q.quat[1] &&
         this->quat[2] == q.quat[2] &&
         this->quat[3] == q.quat[3]) ||
        (this->quat[0] == -q.quat[0] &&
         this->quat[1] == -q.quat[1] &&
         this->quat[2] == -q.quat[2] &&
         this->quat[3] == -q.quat[3]))
        return true;
    return false;
}

bool Rotation::isSame(const Rotation& q, double tol) const
{
    // This follows the implementation of Coin3d where the norm
    // (x1-y1)**2 + ... + (x4-y4)**2 is computed.
    // This term can be simplified to
    // 2 - 2*(x1*y1 + ... + x4*y4) so that for the equality we have to check
    // 1 - tol/2 <= x1*y1 + ... + x4*y4
    // This simplification only work if both quats are normalized
    // Is it safe to assume that?
    // Because a quaternion (x1,x2,x3,x4) is equal to (-x1,-x2,-x3,-x4) we use the
    // absolute value of the scalar product
    double dot = q.quat[0]*quat[0]+q.quat[1]*quat[1]+q.quat[2]*quat[2]+q.quat[3]*quat[3];
    return fabs(dot) >= 1.0 - tol/2;
}

bool Rotation::isIdentity() const
{
    return ((this->quat[0] == 0.0  &&
             this->quat[1] == 0.0  &&
             this->quat[2] == 0.0) &&
            (this->quat[3] == 1.0 ||
             this->quat[3] == -1.0));
}

bool Rotation::isIdentity(double tol) const
{
    return isSame(Rotation(), tol);
}

bool Rotation::isNull() const
{
    return (this->quat[0] == 0.0 &&
            this->quat[1] == 0.0 &&
            this->quat[2] == 0.0 &&
            this->quat[3] == 0.0);
}

//=======================================================================
// The following code is borrowed from OCCT gp/gp_Quaternion.cxx

namespace { // anonymous namespace
//=======================================================================
//function : translateEulerSequence
//purpose  : 
// Code supporting conversion between quaternion and generalized 
// Euler angles (sequence of three rotations) is based on
// algorithm by Ken Shoemake, published in Graphics Gems IV, p. 222-22
// http://tog.acm.org/resources/GraphicsGems/gemsiv/euler_angle/EulerAngles.c
//=======================================================================

struct EulerSequence_Parameters
{
    int i;           // first rotation axis
    int j;           // next axis of rotation
    int k;           // third axis
    bool isOdd;       // true if order of two first rotation axes is odd permutation, e.g. XZ
    bool isTwoAxes;   // true if third rotation is about the same axis as first 
    bool isExtrinsic; // true if rotations are made around fixed axes

    EulerSequence_Parameters (int theAx1, 
                              bool theisOdd, 
                              bool theisTwoAxes,
                              bool theisExtrinsic)
        : i(theAx1), 
        j(1 + (theAx1 + (theisOdd ? 1 : 0)) % 3), 
        k(1 + (theAx1 + (theisOdd ? 0 : 1)) % 3), 
        isOdd(theisOdd), 
        isTwoAxes(theisTwoAxes), 
        isExtrinsic(theisExtrinsic)
        {}
};

EulerSequence_Parameters translateEulerSequence (const Rotation::EulerSequence theSeq)
{
    const bool F = false;
    const bool T = true;

    switch (theSeq)
    {
    case Rotation::Extrinsic_XYZ: return {1, F, F, T};
    case Rotation::Extrinsic_XZY: return {1, T, F, T};
    case Rotation::Extrinsic_YZX: return {2, F, F, T};
    case Rotation::Extrinsic_YXZ: return {2, T, F, T};
    case Rotation::Extrinsic_ZXY: return {3, F, F, T};
    case Rotation::Extrinsic_ZYX: return {3, T, F, T};

    // Conversion of intrinsic angles is made by the same code as for extrinsic,
    // using equivalence rule: intrinsic rotation is equivalent to extrinsic
    // rotation by the same angles but with inverted order of elemental rotations.
    // Swapping of angles (Alpha <-> Gamma) is done inside conversion procedure;
    // sequence of axes is inverted by setting appropriate parameters here.
    // Note that proper Euler angles (last block below) are symmetric for sequence of axes.
    case Rotation::Intrinsic_XYZ: return {3, T, F, F};
    case Rotation::Intrinsic_XZY: return {2, F, F, F};
    case Rotation::Intrinsic_YZX: return {1, T, F, F};
    case Rotation::Intrinsic_YXZ: return {3, F, F, F};
    case Rotation::Intrinsic_ZXY: return {2, T, F, F};
    case Rotation::Intrinsic_ZYX: return {1, F, F, F};

    case Rotation::Extrinsic_XYX: return {1, F, T, T};
    case Rotation::Extrinsic_XZX: return {1, T, T, T};
    case Rotation::Extrinsic_YZY: return {2, F, T, T};
    case Rotation::Extrinsic_YXY: return {2, T, T, T};
    case Rotation::Extrinsic_ZXZ: return {3, F, T, T};
    case Rotation::Extrinsic_ZYZ: return {3, T, T, T};

    case Rotation::Intrinsic_XYX: return {1, F, T, F};
    case Rotation::Intrinsic_XZX: return {1, T, T, F};
    case Rotation::Intrinsic_YZY: return {2, F, T, F};
    case Rotation::Intrinsic_YXY: return {2, T, T, F};
    case Rotation::Intrinsic_ZXZ: return {3, F, T, F};
    case Rotation::Intrinsic_ZYZ: return {3, T, T, F};

    default:
    case Rotation::EulerAngles : return {3, F, T, F}; // = Intrinsic_ZXZ
    case Rotation::YawPitchRoll: return {1, F, F, F}; // = Intrinsic_ZYX
    };
}

class Mat : public Base::Matrix4D
{
public:
    double operator()(int i, int j) const {
        return this->operator[](i-1)[j-1];
    }
    double & operator()(int i, int j) {
        return this->operator[](i-1)[j-1];
    }
};

const char *EulerSequenceNames[] = {
    //! Classic Euler angles, alias to Intrinsic_ZXZ
    "Euler",

    //! Yaw Pitch Roll (or nautical) angles, alias to Intrinsic_ZYX
    "YawPitchRoll",

    // Tait-Bryan angles (using three different axes)
    "XYZ",
    "XZY",
    "YZX",
    "YXZ",
    "ZXY",
    "ZYX",

    "IXYZ",
    "IXZY",
    "IYZX",
    "IYXZ",
    "IZXY",
    "IZYX",

    // Proper Euler angles (using two different axes, first and third the same)
    "XYX",
    "XZX",
    "YZY",
    "YXY",
    "ZYZ",
    "ZXZ",

    "IXYX",
    "IXZX",
    "IYZY",
    "IYXY",
    "IZXZ",
    "IZYZ",
};

} // anonymous namespace

const char * Rotation::eulerSequenceName(EulerSequence seq)
{
    if (seq == Invalid || seq >= EulerSequenceLast)
        return nullptr;
    return EulerSequenceNames[seq-1];
}

Rotation::EulerSequence Rotation::eulerSequenceFromName(const char *name)
{
    if (name) {
        for (unsigned i=0; i<sizeof(EulerSequenceNames)/sizeof(EulerSequenceNames[0]); ++i) {
            if (boost::iequals(name, EulerSequenceNames[i]))
                return static_cast<EulerSequence>(i+1);
        }
    }
    return Invalid;
}

void Rotation::setEulerAngles(EulerSequence theOrder,
                              double theAlpha,
                              double theBeta,
                              double theGamma)
{
    if (theOrder == Invalid || theOrder >= EulerSequenceLast)
        throw Base::ValueError("invalid euler sequence");

    EulerSequence_Parameters o = translateEulerSequence (theOrder);

    theAlpha *= D_PI/180.0;
    theBeta *= D_PI/180.0;
    theGamma *= D_PI/180.0;

    double a = theAlpha, b = theBeta, c = theGamma;
    if ( ! o.isExtrinsic )
        std::swap(a, c);

    if ( o.isOdd )
        b = -b;

    double ti = 0.5 * a; 
    double tj = 0.5 * b; 
    double th = 0.5 * c;
    double ci = cos (ti);  
    double cj = cos (tj);  
    double ch = cos (th);
    double si = sin (ti);
    double sj = sin (tj);
    double sh = sin (th);
    double cc = ci * ch; 
    double cs = ci * sh; 
    double sc = si * ch; 
    double ss = si * sh;

    double values[4]; // w, x, y, z
    if ( o.isTwoAxes ) 
    {
        values[o.i] = cj * (cs + sc);
        values[o.j] = sj * (cc + ss);
        values[o.k] = sj * (cs - sc);
        values[0]   = cj * (cc - ss);
    } 
    else 
    {
        values[o.i] = cj * sc - sj * cs;
        values[o.j] = cj * ss + sj * cc;
        values[o.k] = cj * cs - sj * sc;
        values[0]   = cj * cc + sj * ss;
    }
    if ( o.isOdd ) 
        values[o.j] = -values[o.j];

    quat[0] = values[1];
    quat[1] = values[2];
    quat[2] = values[3];
    quat[3] = values[0];
}

void Rotation::getEulerAngles(EulerSequence theOrder,
                              double& theAlpha,
                              double& theBeta,
                              double& theGamma) const
{
    Mat M;
    getValue(M);

    EulerSequence_Parameters o = translateEulerSequence (theOrder);
    if ( o.isTwoAxes ) 
    {
        double sy = sqrt (M(o.i, o.j) * M(o.i, o.j) + M(o.i, o.k) * M(o.i, o.k));
        if (sy > 16 * DBL_EPSILON) 
        {
            theAlpha = atan2 (M(o.i, o.j),  M(o.i, o.k));
            theGamma = atan2 (M(o.j, o.i), -M(o.k, o.i));
        } 
        else 
        {
            theAlpha = atan2 (-M(o.j, o.k), M(o.j, o.j));
            theGamma = 0.;
        }
        theBeta = atan2 (sy, M(o.i, o.i));
    } 
    else 
    {
        double cy = sqrt (M(o.i, o.i) * M(o.i, o.i) + M(o.j, o.i) * M(o.j, o.i));
        if (cy > 16 * DBL_EPSILON) 
        {
            theAlpha = atan2 (M(o.k, o.j), M(o.k, o.k));
            theGamma = atan2 (M(o.j, o.i), M(o.i, o.i));
        } 
        else 
        {
            theAlpha = atan2 (-M(o.j, o.k), M(o.j, o.j));
            theGamma = 0.;
        }
        theBeta = atan2 (-M(o.k, o.i), cy);
    }
    if ( o.isOdd ) 
    {
        theAlpha = -theAlpha;
        theBeta  = -theBeta;
        theGamma = -theGamma;
    }
    if ( ! o.isExtrinsic ) 
    { 
        double aFirst = theAlpha; 
        theAlpha = theGamma;
        theGamma = aFirst;
    }

    theAlpha *= 180.0/D_PI;
    theBeta *= 180.0/D_PI;
    theGamma *= 180.0/D_PI;
}
