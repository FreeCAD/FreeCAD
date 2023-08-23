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

ViewProjMethod::ViewProjMethod() = default;

bool ViewProjMethod::isValid() const
{
    return true;
}

/*! Calculate the composed projection matrix which is a product of
 * projection matrix multiplied with input transformation matrix.
 */
Matrix4D ViewProjMethod::getComposedProjectionMatrix () const
{
    Matrix4D mat = getProjectionMatrix();

    // Compose the object transform, if defined
    if (hasTransform) {
        mat = mat * transform;
    }

    return mat;
}

/*!
 * \brief This method applies an additional transformation to the input points
 * passed with the () operator.
 * \param mat
 */
void ViewProjMethod::setTransform(const Base::Matrix4D& mat)
{
    transform = mat;
    hasTransform = (mat != Base::Matrix4D());
}

void ViewProjMethod::transformInput(const Base::Vector3f& src, Base::Vector3f& dst) const
{
    dst = src;
    if (hasTransform) {
        transform.multVec(dst, dst);
    }
}

void ViewProjMethod::transformInput(const Base::Vector3d& src, Base::Vector3d& dst) const
{
    dst = src;
    if (hasTransform) {
        transform.multVec(dst, dst);
    }
}

//-----------------------------------------------------------------------------

ViewProjMatrix::ViewProjMatrix (const Matrix4D &rclMtx)
  : _clMtx(rclMtx)
{
    double m30 = _clMtx[3][0];
    double m31 = _clMtx[3][1];
    double m32 = _clMtx[3][2];
    double m33 = _clMtx[3][3];
    isOrthographic = (m30 == 0.0 && m31 == 0.0 && m32 == 0.0 && m33 == 1.0);

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

Matrix4D ViewProjMatrix::getProjectionMatrix () const
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
    double w = (static_cast<double>(pnt.x) * m30 +
                static_cast<double>(pnt.y) * m31 +
                static_cast<double>(pnt.z) * m32 + m33);

    mat.multVec(pnt, pnt);
    pnt /= static_cast<typename Vec::num_type>(w);
}

Vector3f ViewProjMatrix::operator()(const Vector3f& inp) const
{
    Vector3f src;
    transformInput(inp, src);

    Vector3f dst;
    if (!isOrthographic) {
        dst = src;
        perspectiveTransform<Vector3f>(_clMtx, dst);
        dst.Set(0.5f*dst.x+0.5f, 0.5f*dst.y+0.5f, 0.5f*dst.z+0.5f);
    }
    else {
        _clMtx.multVec(src, dst);
    }

    return dst;
}

Vector3d ViewProjMatrix::operator()(const Vector3d& inp) const
{
    Vector3d src;
    transformInput(inp, src);

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
        dst.Set(2.0f*src.x-1.0f, 2.0f*src.y-1.0f, 2.0f*src.z-1.0f);
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

// ----------------------------------------------------------------------------

ViewOrthoProjMatrix::ViewOrthoProjMatrix (const Matrix4D &rclMtx)
    : _clMtx(rclMtx)
{
    _clMtxInv = _clMtx;
    _clMtxInv.inverse();
}

Matrix4D ViewOrthoProjMatrix::getProjectionMatrix () const
{
    return _clMtx;
}

Vector3f ViewOrthoProjMatrix::operator()(const Vector3f &rclPt) const
{
    return Vector3f(_clMtx * rclPt);
}

Vector3d ViewOrthoProjMatrix::operator()(const Vector3d &rclPt) const
{
    return Vector3d(_clMtx * rclPt);
}

Vector3f ViewOrthoProjMatrix::inverse (const Vector3f &rclPt) const
{
    return Vector3f(_clMtxInv * rclPt);
}

Vector3d ViewOrthoProjMatrix::inverse (const Vector3d &rclPt) const
{
    return Vector3d(_clMtxInv * rclPt);
}
