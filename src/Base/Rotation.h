// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include "Vector3D.h"
#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Base
{

// forward declarations
class Matrix4D;

class BaseExport Rotation
{
public:
    /** Construction. */
    //@{
    Rotation();
    Rotation(const Vector3d& axis, double fAngle);
    Rotation(const Matrix4D& matrix);
    Rotation(const double q[4]);
    Rotation(double q0, double q1, double q2, double q3);
    Rotation(const Vector3d& rotateFrom, const Vector3d& rotateTo);
    Rotation(const Rotation& rot) = default;
    Rotation(Rotation&& rot) = default;
    ~Rotation() = default;

    enum EulerSequence
    {
        Invalid,

        //! Classic Euler angles, alias to Intrinsic_ZXZ
        EulerAngles,

        //! Yaw Pitch Roll (or nautical) angles, alias to Intrinsic_ZYX
        YawPitchRoll,

        // Tait-Bryan angles (using three different axes)
        Extrinsic_XYZ,
        Extrinsic_XZY,
        Extrinsic_YZX,
        Extrinsic_YXZ,
        Extrinsic_ZXY,
        Extrinsic_ZYX,

        Intrinsic_XYZ,
        Intrinsic_XZY,
        Intrinsic_YZX,
        Intrinsic_YXZ,
        Intrinsic_ZXY,
        Intrinsic_ZYX,

        // Proper Euler angles (using two different axes, first and third the same)
        Extrinsic_XYX,
        Extrinsic_XZX,
        Extrinsic_YZY,
        Extrinsic_YXY,
        Extrinsic_ZYZ,
        Extrinsic_ZXZ,

        Intrinsic_XYX,
        Intrinsic_XZX,
        Intrinsic_YZY,
        Intrinsic_YXY,
        Intrinsic_ZXZ,
        Intrinsic_ZYZ,

        EulerSequenceLast,
    };

    /// Utility function to create Rotation based on direction / normal vector
    /// Z base vector is assumed to represent the normal vector
    static Rotation fromNormalVector(const Vector3d& normal);
    /// Utility function to create Rotation based on euler angles
    static Rotation fromEulerAngles(EulerSequence theOrder, double alpha, double beta, double gamma);
    //@}

    /** Methods to get or set rotations. */
    //@{
    const double* getValue() const;
    void getValue(double& q0, double& q1, double& q2, double& q3) const;
    void setValue(double q0, double q1, double q2, double q3);
    /// If not a null quaternion then \a axis will be normalized
    void getValue(Vector3d& axis, double& rfAngle) const;
    /// Does the same as the method above unless normalizing the axis.
    void getRawValue(Vector3d& axis, double& rfAngle) const;
    void getValue(Matrix4D& matrix) const;
    void setValue(const double q[4]);
    void setValue(const Matrix4D& matrix);
    void setValue(const Vector3d& axis, double fAngle);
    void setValue(const Vector3d& rotateFrom, const Vector3d& rotateTo);
    /// Euler angles in yaw,pitch,roll notation
    void setYawPitchRoll(double y, double p, double r);
    /// Euler angles in yaw,pitch,roll notation
    void getYawPitchRoll(double& y, double& p, double& r) const;

    static const char* eulerSequenceName(EulerSequence seq);
    static EulerSequence eulerSequenceFromName(const char* name);
    void getEulerAngles(EulerSequence theOrder, double& alpha, double& beta, double& gamma) const;
    void setEulerAngles(EulerSequence theOrder, double alpha, double beta, double gamma);
    bool isIdentity() const;
    bool isIdentity(double tol) const;
    bool isNull() const;
    bool isSame(const Rotation&) const;
    bool isSame(const Rotation&, double tol) const;
    //@}

    /** Invert rotations. */
    //@{
    Rotation& invert();
    Rotation inverse() const;
    //@}

    /** Operators. */
    //@{
    Rotation& operator*=(const Rotation& q);
    Rotation operator*(const Rotation& q) const;
    bool operator==(const Rotation& q) const;
    bool operator!=(const Rotation& q) const;
    double& operator[](unsigned short usIndex)
    {
        return quat[usIndex];
    }
    const double& operator[](unsigned short usIndex) const
    {
        return quat[usIndex];
    }
    Rotation& operator=(const Rotation&) = default;
    Rotation& operator=(Rotation&&) = default;

    Rotation& multRight(const Base::Rotation& q);
    Rotation& multLeft(const Base::Rotation& q);

    void multVec(const Vector3d& src, Vector3d& dst) const;
    Vector3d multVec(const Vector3d& src) const;
    void multVec(const Vector3f& src, Vector3f& dst) const;
    Vector3f multVec(const Vector3f& src) const;
    void scaleAngle(double scaleFactor);
    //@}

    /** Specialty constructors */
    static Rotation slerp(const Rotation& q0, const Rotation& q1, double t);
    static Rotation identity();

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
    static Rotation makeRotationByAxes(
        Vector3d xdir,
        Vector3d ydir,
        Vector3d zdir,
        const char* priorityOrder = "ZXY"
    );

private:
    void normalize();
    void evaluateVector();
    double quat[4];
    Vector3d _axis;  // the axis kept not to lose direction when angle is 0
    double _angle;   // this angle to keep the angle chosen by the user
};

}  // namespace Base
