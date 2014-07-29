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
#ifndef _PreComp_
# include <cfloat>
#endif

#include "CoordinateSystem.h"
#include "Exception.h"

using namespace Base;

CoordinateSystem::CoordinateSystem()
  : axis(Vector3d(), Vector3d(0,0,1)), xdir(1,0,0), ydir(0,1,0)
{
}

CoordinateSystem::~CoordinateSystem()
{
}

void CoordinateSystem::setAxes(const Axis& v, const Vector3d& xd)
{
    if (xd.Sqr() < FLT_EPSILON)
        throw Base::Exception("Direction is null vector");
    Vector3d yd = v.getDirection() % xd;
    if (yd.Sqr() < FLT_EPSILON)
        throw Base::Exception("Direction is parallel to Z direction");
    ydir = yd;
    xdir = ydir % v.getDirection();
    axis = v;
}

void CoordinateSystem::setAxes(const Vector3d& n, const Vector3d& xd)
{
    if (xd.Sqr() < FLT_EPSILON)
        throw Base::Exception("Direction is null vector");
    Vector3d yd = n % xd;
    if (yd.Sqr() < FLT_EPSILON)
        throw Base::Exception("Direction is parallel to Z direction");
    ydir = yd;
    xdir = ydir % n;
    axis.setDirection(n);
}

void CoordinateSystem::setAxis(const Axis& v)
{
    setAxes(v, xdir);
}

void CoordinateSystem::setXDirection(const Vector3d& dir)
{
    Vector3d yd = axis.getDirection() % dir;
    if (yd.Sqr() < FLT_EPSILON)
        throw Base::Exception("Direction is parallel to Z direction");
    ydir = yd;
    xdir = ydir % axis.getDirection();
}

void CoordinateSystem::setYDirection(const Vector3d& dir)
{
    Vector3d xd = dir & axis.getDirection();
    if (xd.Sqr() < FLT_EPSILON)
        throw Base::Exception("Direction is parallel to Z direction");
    xdir = xd;
    ydir = axis.getDirection() % xdir;
}

void CoordinateSystem::setZDirection(const Vector3d& dir)
{
    setAxes(dir, xdir);
}

Placement CoordinateSystem::displacement(const CoordinateSystem& cs) const
{
    // align the Z axes
    Base::Rotation rotZ(getZDirection(), cs.getZDirection());

    // align the X axes
    Base::Vector3d xd = xdir;
    rotZ.multVec(xd,xd);
    Base::Rotation rotX(xd, cs.getXDirection());

    // the transformed base point
    Vector3d mov = axis.getBase();
    rotZ.multVec(mov,mov);
    rotX.multVec(mov,mov);
    mov = cs.getPosition() - mov;

    Base::Rotation rot;
    rot = rotX * rotZ;

    return Placement(mov, rot);
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
