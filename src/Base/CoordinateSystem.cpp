/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#include "CoordinateSystem.h"
#include "Exception.h"
#include "Matrix.h"
#include "Placement.h"
#include "Rotation.h"


using namespace Base;

CoordinateSystem::CoordinateSystem()
    : axis(Vector3d(), Vector3d(0, 0, 1))
    , xdir(1, 0, 0)
    , ydir(0, 1, 0)
{}

void CoordinateSystem::setAxes(const Axis& vec, const Vector3d& xd)
{
    if (xd.Sqr() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Direction is null vector");
    }
    Vector3d yd = vec.getDirection() % xd;
    if (yd.Sqr() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Direction is parallel to Z direction");
    }
    ydir = yd;
    ydir.Normalize();
    xdir = ydir % vec.getDirection();
    xdir.Normalize();
    axis.setBase(vec.getBase());
    Base::Vector3d zdir = vec.getDirection();
    zdir.Normalize();
    axis.setDirection(zdir);
}

void CoordinateSystem::setAxes(const Vector3d& n, const Vector3d& xd)
{
    if (xd.Sqr() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Direction is null vector");
    }
    Vector3d yd = n % xd;
    if (yd.Sqr() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Direction is parallel to Z direction");
    }
    ydir = yd;
    ydir.Normalize();
    xdir = ydir % n;
    xdir.Normalize();
    Base::Vector3d zdir = n;
    zdir.Normalize();
    axis.setDirection(zdir);
}

void CoordinateSystem::setAxis(const Axis& axis)
{
    setAxes(axis, xdir);
}

void CoordinateSystem::setXDirection(const Vector3d& dir)
{
    Vector3d yd = axis.getDirection() % dir;
    if (yd.Sqr() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Direction is parallel to Z direction");
    }
    ydir = yd;
    ydir.Normalize();
    xdir = ydir % axis.getDirection();
    xdir.Normalize();
}

void CoordinateSystem::setYDirection(const Vector3d& dir)
{
    Vector3d xd = dir % axis.getDirection();
    if (xd.Sqr() < Base::Vector3d::epsilon()) {
        throw Base::ValueError("Direction is parallel to Z direction");
    }
    xdir = xd;
    xdir.Normalize();
    ydir = axis.getDirection() % xdir;
    ydir.Normalize();
}

void CoordinateSystem::setZDirection(const Vector3d& dir)
{
    setAxes(dir, xdir);
}

Placement CoordinateSystem::displacement(const CoordinateSystem& cs) const
{
    // NOLINTBEGIN
    const Base::Vector3d& a = axis.getBase();
    const Base::Vector3d& zdir = axis.getDirection();
    Base::Matrix4D At;
    At[0][0] = xdir.x;
    At[1][0] = ydir.x;
    At[2][0] = zdir.x;
    At[0][1] = xdir.y;
    At[1][1] = ydir.y;
    At[2][1] = zdir.y;
    At[0][2] = xdir.z;
    At[1][2] = ydir.z;
    At[2][2] = zdir.z;
    Base::Vector3d at = At * a;
    At[0][3] = -at.x;
    At[1][3] = -at.y;
    At[2][3] = -at.z;

    const Base::Vector3d& b = cs.axis.getBase();
    const Base::Vector3d& cszdir = cs.axis.getDirection();
    Base::Matrix4D B;
    B[0][0] = cs.xdir.x;
    B[0][1] = cs.ydir.x;
    B[0][2] = cszdir.x;
    B[0][3] = b.x;
    B[1][0] = cs.xdir.y;
    B[1][1] = cs.ydir.y;
    B[1][2] = cszdir.y;
    B[1][3] = b.y;
    B[2][0] = cs.xdir.z;
    B[2][1] = cs.ydir.z;
    B[2][2] = cszdir.z;
    B[2][3] = b.z;

    Placement PAt(At);
    Placement PB(B);
    Placement C = PB * PAt;
    return C;
    // NOLINTEND
}

void CoordinateSystem::transformTo(Vector3d& pnt)
{
    return pnt.TransformToCoordinateSystem(axis.getBase(), xdir, ydir);
}

void CoordinateSystem::transform(const Placement& plm)
{
    axis *= plm;
    plm.getRotation().multVec(this->xdir, this->xdir);
    plm.getRotation().multVec(this->ydir, this->ydir);
}

void CoordinateSystem::transform(const Rotation& rot)
{
    Vector3d zdir = axis.getDirection();
    rot.multVec(zdir, zdir);
    axis.setDirection(zdir);
    rot.multVec(this->xdir, this->xdir);
    rot.multVec(this->ydir, this->ydir);
}

void CoordinateSystem::setPlacement(const Placement& plm)
{
    Vector3d zdir(0, 0, 1);
    plm.getRotation().multVec(zdir, zdir);
    axis.setBase(plm.getPosition());
    axis.setDirection(zdir);

    plm.getRotation().multVec(Vector3d(1, 0, 0), this->xdir);
    plm.getRotation().multVec(Vector3d(0, 1, 0), this->ydir);
}
