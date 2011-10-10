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
#ifndef _PreComp_
# include <cmath>
# include <climits>
#endif

#include "Rotation.h"
#include "Matrix.h"

using namespace Base;

Rotation::Rotation()
{
    quat[0]=quat[1]=quat[2]=0.0f;quat[3]=1.0f;
}

/** Construct a rotation by rotation axis and angle */
Rotation::Rotation(const Vector3d& axis, const double fAngle)
{
    this->setValue(axis, fAngle);
}

Rotation::Rotation(const Matrix4D& matrix)
{
    this->setValue(matrix);
}

/** Construct a rotation initialized with the given quaternion components:
 * q[0] = x, q[1] = y, q[2] = z and q[3] = w,
 * where the quaternion is specified by q=w+xi+yj+zk.
 */
Rotation::Rotation(const double q[4])
{
    this->setValue(q);
}

/** Construct a rotation initialized with the given quaternion components:
 * q0 = x, q1 = y, q2 = z and q3 = w,
 * where the quaternion is specified by q=w+xi+yj+zk.
 */
Rotation::Rotation(const double q0, const double q1, const double q2, const double q3)
{
    this->setValue(q0, q1, q2, q3);
}

Rotation::Rotation(const Vector3d & rotateFrom, const Vector3d & rotateTo)
{
    this->setValue(rotateFrom, rotateTo);
}

Rotation::Rotation(const Rotation& rot)
{
    this->quat[0] = rot.quat[0];
    this->quat[1] = rot.quat[1];
    this->quat[2] = rot.quat[2];
    this->quat[3] = rot.quat[3];
}

const double * Rotation::getValue(void) const
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

void Rotation::setValue(const double q0, const double q1, const double q2, const double q3)
{
    this->quat[0] = q0;
    this->quat[1] = q1;
    this->quat[2] = q2;
    this->quat[3] = q3;
    this->normalize();
}

void Rotation::getValue(Vector3d & axis, double & rfAngle) const
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    //
    // Note: -1 < w < +1 (|w| == 1 not allowed, with w:=quat[3]) 
    if((this->quat[3] > -1.0f) && (this->quat[3] < 1.0f)) {
        rfAngle = double(acos(this->quat[3])) * 2.0f;
        double scale = (double)sin(rfAngle / 2.0f);
        // Get a normalized vector 
        axis.x = this->quat[0] / scale;
        axis.y = this->quat[1] / scale;
        axis.z = this->quat[2] / scale;
    }
    else {
        // The quaternion doesn't describe a rotation, so we can setup any value we want 
        axis.Set(0.0f, 0.0f, 1.0f);
        rfAngle = 0.0f;
    }
}

/**
 * Returns this rotation in form of a matrix.
 */
void Rotation::getValue(Matrix4D & matrix) const
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    //
    const double x = this->quat[0];
    const double y = this->quat[1];
    const double z = this->quat[2];
    const double w = this->quat[3];

    matrix[0][0] = 1.0f-2.0f*(y*y+z*z);
    matrix[0][1] = 2.0f*(x*y-z*w);
    matrix[0][2] = 2.0f*(x*z+y*w);
    matrix[0][3] = 0.0f;

    matrix[1][0] = 2.0f*(x*y+z*w);
    matrix[1][1] = 1.0f-2.0f*(x*x+z*z);
    matrix[1][2] = 2.0f*(y*z-x*w);
    matrix[1][3] = 0.0f;

    matrix[2][0] = 2.0f*(x*z-y*w);
    matrix[2][1] = 2.0f*(y*z+x*w);
    matrix[2][2] = 1.0f-2.0f*(x*x+y*y);
    matrix[2][3] = 0.0f;

    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;
    matrix[3][3] = 1.0f;
}

void Rotation::setValue(const double q[4])
{
    this->quat[0] = q[0];
    this->quat[1] = q[1];
    this->quat[2] = q[2];
    this->quat[3] = q[3];
    this->normalize();
}

void Rotation::setValue(const Matrix4D & m)
{
    double trace = (double)(m[0][0] + m[1][1] + m[2][2]);
    if (trace > 0.0f) {
        double s = (double)sqrt(1.0f+trace);
        this->quat[3] = 0.5f * s;
        s = 0.5f / s;
        this->quat[0] = (double)((m[2][1] - m[1][2]) * s);
        this->quat[1] = (double)((m[0][2] - m[2][0]) * s);
        this->quat[2] = (double)((m[1][0] - m[0][1]) * s);
    }
    else {
        // Described in RotationIssues.pdf from <http://www.geometrictools.com>
        //
        // Get the max. element of the trace
        int i = 0;
        if (m[1][1] > m[0][0]) i = 1;
        if (m[2][2] > m[i][i]) i = 2;

        int j = (i+1)%3;
        int k = (i+2)%3;

        double s = (double)sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.0f);
        this->quat[i] = s * 0.5f;
        s = 0.5f / s;
        this->quat[3] = (double)((m[k][j] - m[j][k]) * s);
        this->quat[j] = (double)((m[j][i] + m[i][j]) * s);
        this->quat[k] = (double)((m[k][i] + m[i][k]) * s);
    }
}

void Rotation::setValue(const Vector3d & axis, const double fAngle)
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    //
    this->quat[3] = (double)cos(fAngle/2.0);
    Vector3d norm = axis;
    norm.Normalize();
    double scale = (double)sin(fAngle/2.0);
    this->quat[0] = norm.x * scale;
    this->quat[1] = norm.y * scale;
    this->quat[2] = norm.z * scale;
}

void Rotation::setValue(const Vector3d & rotateFrom, const Vector3d & rotateTo)
{
    Vector3d u(rotateFrom); u.Normalize();
    Vector3d v(rotateTo); v.Normalize();

    // The vector from x to is the roatation axis because it's the normal of the plane defined by (0,u,v) 
    const double dot = u * v;
    Vector3d w = u % v;
    const double wlen = w.Length();

    if (wlen == 0.0f) { // Parallel vectors
        // Check if they are pointing in the same direction.
        if (dot > 0.0f) {
            this->setValue(0.0f, 0.0f, 0.0f, 1.0f);
        }
        else {
            // We can use any axis perpendicular to u (and v)
            Vector3d t = u % Vector3d(1.0f, 0.0f, 0.0f);
            if(t.Length() < FLT_EPSILON) 
                t = u % Vector3d(0.0f, 1.0f, 0.0f);
            this->setValue(t.x, t.y, t.z, 0.0f);
        }
    }
    else { // Vectors are not parallel
        // Note: A quaternion is not well-defined by specifying a point and its transformed point.
        // Every quaternion with a rotation axis having the same angle to the vectors of both points is okay.
        double angle = (double)acos(dot);
        this->setValue(w, angle);
    }
}

void Rotation::normalize()
{
    double len = (double)sqrt(this->quat[0]*this->quat[0]+
                              this->quat[1]*this->quat[1]+
                              this->quat[2]*this->quat[2]+
                              this->quat[3]*this->quat[3]);
    this->quat[0] /= len;
    this->quat[1] /= len;
    this->quat[2] /= len;
    this->quat[3] /= len;
}

Rotation & Rotation::invert(void)
{
    this->quat[0] = -this->quat[0];
    this->quat[1] = -this->quat[1];
    this->quat[2] = -this->quat[2];
    this->quat[3] =  this->quat[3];
    return *this;
}

Rotation Rotation::inverse(void) const
{
    Rotation rot;
    rot.quat[0] = -this->quat[0];
    rot.quat[1] = -this->quat[1];
    rot.quat[2] = -this->quat[2];
    rot.quat[3] =  this->quat[3];
    return rot;
}

Rotation & Rotation::operator*=(const Rotation & q)
{
    // Taken from <http://de.wikipedia.org/wiki/Quaternionen>
    double x0, y0, z0, w0;
    this->getValue(x0, y0, z0, w0);
    double x1, y1, z1, w1;
    q.getValue(x1, y1, z1, w1);

    this->setValue(w0*x1 + x0*w1 + y0*z1 - z0*y1,
                   w0*y1 - x0*z1 + y0*w1 + z0*x1,
                   w0*z1 + x0*y1 - y0*x1 + z0*w1,
                   w0*w1 - x0*x1 - y0*y1 - z0*z1);
    return *this;
}

Rotation Rotation::operator*(const Rotation & q) const
{
    Rotation quat(*this);
    quat *= q;
    return quat;
}

bool Rotation::operator==(const Rotation & q) const
{
    bool equal = true;
    for (int i=0; i<4;i++)
        equal &= (fabs(this->quat[i] - q.quat[i]) < 0.005 );
    return equal;
}

bool Rotation::operator!=(const Rotation & q) const
{
    return !(*this == q);
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

    double dx = (x2+w2-y2-z2)*src.x + 2.0f*(x*y-z*w)*src.y + 2.0f*(x*z+y*w)*src.z;
    double dy = 2.0f*(x*y+z*w)*src.x + (w2-x2+y2-z2)*src.y + 2.0f*(y*z-x*w)*src.z;
    double dz = 2.0f*(x*z-y*w)*src.x + 2.0f*(x*w+y*z)*src.y + (w2-x2-y2+z2)*src.z;
    dst.x = dx;
    dst.y = dy;
    dst.z = dz;
}

void Rotation::scaleAngle(const double scaleFactor)
{
    Vector3d axis;
    double fAngle;
    this->getValue(axis, fAngle);
    this->setValue(axis, fAngle * scaleFactor);
}

Rotation Rotation::slerp(const Rotation & q0, const Rotation & q1, double t)
{
    // Taken from <http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/>
    // q = [q0*sin((1-t)*theta)+q1*sin(t*theta)]/sin(theta), 0<=t<=1
    if (t<0.0f) t=0.0f;
    else if (t>1.0f) t=1.0f;
    //return q0;

    double scale0 = 1.0f - t;
    double scale1 = t;
    double dot = q0.quat[0]*q1.quat[0]+q0.quat[1]*q1.quat[1]+q0.quat[2]*q1.quat[2]+q0.quat[3]*q1.quat[3];
    bool neg=false;
    if(dot < 0.0f) {
        dot = -dot;
        neg = true;
    }

    if ((1.0f - dot) > FLT_EPSILON) {
        double angle = (double)acos(dot);
        double sinangle = (double)sin(angle);
        // If possible calculate spherical interpolation, otherwise use linear interpolation
        if (sinangle > FLT_EPSILON) {
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
    return Rotation(x, y, z, w);
}

Rotation Rotation::identity(void)
{
    return Rotation(0.0f, 0.0f, 0.0f, 1.0f);
}

void Rotation::setYawPitchRoll(double y, double p, double r)
{
    // taken from http://www.resonancepub.com/quaterni.htm
    // The Euler angles (yaw,pitch,roll) are in ZY'X''-notation
    double c1 = cos(((y/180.0)*D_PI)/2.0);
    double s1 = sin(((y/180.0)*D_PI)/2.0);
    double c2 = cos(((p/180.0)*D_PI)/2.0);
    double s2 = sin(((p/180.0)*D_PI)/2.0);
    double c3 = cos(((r/180.0)*D_PI)/2.0);
    double s3 = sin(((r/180.0)*D_PI)/2.0);

    quat[0] = c1*c2*s3 - s1*s2*c3;
    quat[1] = c1*s2*c3 + s1*c2*s3;
    quat[2] = s1*c2*c3 - c1*s2*s3;
    quat[3] = c1*c2*c3 + s1*s2*s3;
}

void Rotation::getYawPitchRoll(double& y, double& p, double& r) const
{
    // taken from http://www.resonancepub.com/quaterni.htm
    // see also http://willperone.net/Code/quaternion.php
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

    y = atan2(2.0*(q01+q23),(q00+q33)-(q11+q22));
    p = qd2 > 1.0 ? D_PI/2.0 : (qd2 < -1.0 ? -D_PI/2.0 : asin (qd2));
    r = atan2(2.0*(q12+q03),(q22+q33)-(q00+q11));

    // convert to degree
    y = (y/D_PI)*180;
    p = (p/D_PI)*180;
    r = (r/D_PI)*180;
}
