/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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
#include "ViewProj.h"

using namespace Base;

ViewProjMatrix::ViewProjMatrix (const Matrix4D &rclMtx)
  : _clMtx(rclMtx)
{
    double m30 = _clMtx[3][0];
    double m31 = _clMtx[3][1];
    double m32 = _clMtx[3][2];
    double m33 = _clMtx[3][3];
    isOrthographic = (m30 == 0 && m31 == 0 && m32 == 0 && m33 == 1);

    // Only for orthographic projection mode we can compute a single
    // matrix performing all steps.
    // For perspective projection the scaling and translation must
    // be done afterwards because it depends on the input point.
    if (isOrthographic) {
        // Scale from [-1,1] to [0,1]
        // As done in OpenInventor sources (see SbDPViewVolume::projectToScreen)
        _clMtx.scale(0.5, 0.5, 0.5);
        _clMtx.move(0.5, 0.5, 0.5);
    }

    _clMtxInv = _clMtx;
    _clMtxInv.inverseGauss();
}

ViewProjMatrix::~ViewProjMatrix()
{
}

Matrix4D ViewProjMatrix::getProjectionMatrix (void) const
{
    // Return the same matrix as passed to the constructor
    Matrix4D mat(_clMtx);
    if (isOrthographic) {
        mat.move(-0.5, -0.5, -0.5);
        mat.scale(2.0, 2.0, 2.0);
    }
    return mat;
}

template<typename Vec>
void perspectiveTransform(const Base::Matrix4D& mat, Vec& pnt)
{
    double m30 = mat[3][0];
    double m31 = mat[3][1];
    double m32 = mat[3][2];
    double m33 = mat[3][3];
    double w = (pnt.x * m30 + pnt.y * m31 + pnt.z * m32 + m33);

    mat.multVec(pnt, pnt);
    pnt /= w;
}

Vector3f ViewProjMatrix::operator()(const Vector3f& src) const
{
    Vector3f dst;
    if (!isOrthographic) {
        dst = src;
        perspectiveTransform<Vector3f>(_clMtx, dst);
        dst.Set(0.5*dst.x+0.5, 0.5*dst.y+0.5, 0.5*dst.z+0.5);
    }
    else {
        _clMtx.multVec(src, dst);
    }

    return dst;
}

Vector3d ViewProjMatrix::operator()(const Vector3d& src) const
{
    Vector3d dst;
    if (!isOrthographic) {
        dst = src;
        perspectiveTransform<Vector3d>(_clMtx, dst);
        dst.Set(0.5*dst.x+0.5, 0.5*dst.y+0.5, 0.5*dst.z+0.5);
    }
    else {
        _clMtx.multVec(src, dst);
    }

    return dst;
}

Vector3f ViewProjMatrix::inverse (const Vector3f& src) const
{
    Vector3f dst;
    if (!isOrthographic) {
        dst.Set(2.0*src.x-1.0, 2.0*src.y-1.0, 2.0*src.z-1.0);
        perspectiveTransform<Vector3f>(_clMtxInv, dst);
    }
    else {
        _clMtxInv.multVec(src, dst);
    }

    return dst;
}

Vector3d ViewProjMatrix::inverse (const Vector3d& src) const
{
    Vector3d dst;
    if (!isOrthographic) {
        dst.Set(2.0*src.x-1.0, 2.0*src.y-1.0, 2.0*src.z-1.0);
        perspectiveTransform<Vector3d>(_clMtxInv, dst);
    }
    else {
        _clMtxInv.multVec(src, dst);
    }

    return dst;
}
