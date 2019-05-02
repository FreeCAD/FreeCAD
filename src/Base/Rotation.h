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


#ifndef BASE_ROTATION_H
#define BASE_ROTATION_H

#include "Vector3D.h"

namespace Base {

// forward declarations
class Matrix4D;

class BaseExport Rotation
{
public:
    /** Construction. */
    //@{
    Rotation();
    Rotation(const Vector3d& axis, const double fAngle);
    Rotation(const Matrix4D& matrix);
    Rotation(const double q[4]);
    Rotation(const double q0, const double q1, const double q2, const double q3);
    Rotation(const Vector3d& rotateFrom, const Vector3d& rotateTo);
    Rotation(const Rotation& rot);
    //@}

    /** Methods to get or set rotations. */
    //@{
    const double * getValue(void) const;
    void getValue(double & q0, double & q1, double & q2, double & q3) const;
    void setValue(const double q0, const double q1, const double q2, const double q3);
    /// If not a null quaternion then \a axis will be normalized
    void getValue(Vector3d & axis, double & rfAngle) const;
    /// Does the same as the method above unless normalizing the axis.
    void getRawValue(Vector3d & axis, double & rfAngle) const;
    void getValue(Matrix4D & matrix) const;
    void setValue(const double q[4]);
    void setValue(const Matrix4D& matrix);
    void setValue(const Vector3d & axis, const double fAngle);
    void setValue(const Vector3d & rotateFrom, const Vector3d & rotateTo);
    /// Euler angles in yaw,pitch,roll notation
    void setYawPitchRoll(double y, double p, double r);
    /// Euler angles in yaw,pitch,roll notation
    void getYawPitchRoll(double& y, double& p, double& r) const;
    bool isIdentity() const;
    bool isNull() const;
    //@}

    /** Invert rotations. */
    //@{
    Rotation & invert(void);
    Rotation inverse(void) const;
    //@}

    /** Operators. */
    //@{
    Rotation & operator*=(const Rotation & q);
    Rotation operator *(const Rotation & q) const;
    bool operator==(const Rotation & q) const;
    bool operator!=(const Rotation & q) const;
    double & operator [] (unsigned short usIndex){return quat[usIndex];}
    const double & operator [] (unsigned short usIndex) const{return quat[usIndex];}

    void multVec(const Vector3d & src, Vector3d & dst) const;
    Vector3d multVec(const Vector3d & src) const;
    void scaleAngle(const double scaleFactor);
    bool isSame(const Rotation&) const;
    bool isSame(const Rotation&, double tol) const;
    //@}

    /** Specialty constructors */
    static Rotation slerp(const Rotation & rot0, const Rotation & rot1, double t);
    static Rotation identity(void);

    /**
     * @brief makeRotationByAxes(xdir, ydir, zdir, priorityOrder): creates a rotation
     * that converts a vector in local cs with axes given as arguments, into a
     * vector in global cs.
     * @param xdir is wanted direction of local X axis
     * @param ydir ...
     * @param zdir
     * @param priorityOrder sets which directions are followed. It is a string
     * like "ZXY". This means, Z direction is followed precisely; X direction is
     * corrected to be perpendicular to Z direction, and used; Y direction
     * argument is ignored altogether (Y direction is generated from Z and X).
     *
     * If only one vector provided is nonzero, the other two directions are picked automatically.
     */
    static Rotation makeRotationByAxes(Vector3d xdir, Vector3d ydir, Vector3d zdir, const char* priorityOrder = "ZXY");

private:
    void normalize();
    void evaluateVector ();
    double quat[4];
    Vector3d _axis; // the axis kept not to lose direction when angle is 0
    double _angle; // this angle to keep the angle chosen by the user
};

}

#endif // BASE_ROTATION_H
