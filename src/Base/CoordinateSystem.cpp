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


using namespace Base;

CoordinateSystem::CoordinateSystem()
  : axis(Vector3d(), Vector3d(0,0,1)), xdir(1,0,0), ydir(0,1,0)
{
}

CoordinateSystem::~CoordinateSystem() = default;

void CoordinateSystem::setAxes(const Axis& v, const Vector3d& xd)
{
    if (xd.Sqr() < Base::Vector3d::epsilon())
        throw Base::ValueError("Direction is null vector");
    Vector3d yd = v.getDirection() % xd;
    if (yd.Sqr() < Base::Vector3d::epsilon())
        throw Base::ValueError("Direction is parallel to Z direction");
    ydir = yd;
    ydir.Normalize();
    xdir = ydir % v.getDirection();
    xdir.Normalize();
    axis.setBase(v.getBase());
    Base::Vector3d zdir = v.getDirection();
    zdir.Normalize();
    axis.setDirection(zdir);
}

void CoordinateSystem::setAxes(const Vector3d& n, const Vector3d& xd)
{
    if (xd.Sqr() < Base::Vector3d::epsilon())
        throw Base::ValueError("Direction is null vector");
    Vector3d yd = n % xd;
    if (yd.Sqr() < Base::Vector3d::epsilon())
        throw Base::ValueError("Direction is parallel to Z direction");
    ydir = yd;
    ydir.Normalize();
    xdir = ydir % n;
    xdir.Normalize();
    Base::Vector3d zdir = n;
    zdir.Normalize();
    axis.setDirection(zdir);
}

void CoordinateSystem::setAxis(const Axis& v)
{
    setAxes(v, xdir);
}

void CoordinateSystem::setXDirection(const Vector3d& dir)
{
    Vector3d yd = axis.getDirection() % dir;
    if (yd.Sqr() < Base::Vector3d::epsilon())
        throw Base::ValueError("Direction is parallel to Z direction");
    ydir = yd;
    ydir.Normalize();
    xdir = ydir % axis.getDirection();
    xdir.Normalize();
}

void CoordinateSystem::setYDirection(const Vector3d& dir)
{
    Vector3d xd = dir % axis.getDirection();
    if (xd.Sqr() < Base::Vector3d::epsilon())
        throw Base::ValueError("Direction is parallel to Z direction");
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
    const Base::Vector3d& a = axis.getBase();
    const Base::Vector3d& zdir = axis.getDirection();
    Base::Matrix4D At;
    At[0][0] = xdir.x; At[1][0] = ydir.x; At[2][0] = zdir.x;
    At[0][1] = xdir.y; At[1][1] = ydir.y; At[2][1] = zdir.y;
    At[0][2] = xdir.z; At[1][2] = ydir.z; At[2][2] = zdir.z;
    Base::Vector3d at = At * a;
    At[0][3] = -at.x;  At[1][3] = -at.y;  At[2][3] = -at.z;

    const Base::Vector3d& b = cs.axis.getBase();
    const Base::Vector3d& cszdir = cs.axis.getDirection();
    Base::Matrix4D B;
    B[0][0] = cs.xdir.x; B[0][1] = cs.ydir.x; B[0][2] = cszdir.x; B[0][3] = b.x;
    B[1][0] = cs.xdir.y; B[1][1] = cs.ydir.y; B[1][2] = cszdir.y; B[1][3] = b.y;
    B[2][0] = cs.xdir.z; B[2][1] = cs.ydir.z; B[2][2] = cszdir.z; B[2][3] = b.z;

    Placement PAt(At);
    Placement PB(B);
    Placement C = PB * PAt;
    return C;
}

void CoordinateSystem::transformTo(Vector3d& p)
{
    return p.TransformToCoordinateSystem(axis.getBase(), xdir, ydir);
}

void CoordinateSystem::transform(const Placement& p)
{
    axis *= p;
    p.getRotation().multVec(this->xdir, this->xdir);
    p.getRotation().multVec(this->ydir, this->ydir);
}

void CoordinateSystem::transform(const Rotation& r)
{
    Vector3d zdir = axis.getDirection();
    r.multVec(zdir, zdir);
    axis.setDirection(zdir);
    r.multVec(this->xdir, this->xdir);
    r.multVec(this->ydir, this->ydir);
}

void CoordinateSystem::setPlacement(const Placement& p)
{
    Vector3d zdir(0,0,1);
    p.getRotation().multVec(zdir, zdir);
    axis.setBase(p.getPosition());
    axis.setDirection(zdir);

    p.getRotation().multVec(Vector3d(1,0,0), this->xdir);
    p.getRotation().multVec(Vector3d(0,1,0), this->ydir);
}
