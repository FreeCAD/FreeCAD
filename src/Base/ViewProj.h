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


#ifndef BASE_VIEWPROJ_H
#define BASE_VIEWPROJ_H

#include "Vector3D.h"
#include "Matrix.h"


namespace Base {

/**
 * Abstract base class for all project methods.
 */
class BaseExport ViewProjMethod
{
public:
    virtual ~ViewProjMethod(){}
    virtual bool isValid() const { return true; }
    /** Convert 3D point to 2D projection plane */
    virtual Vector3f operator()(const Vector3f &rclPt) const = 0;
    /** Convert 3D point to 2D projection plane */
    virtual Vector3d operator()(const Vector3d &rclPt) const = 0;
    /** Convert a 2D point on the projection plane in 3D space */
    virtual Vector3f inverse (const Vector3f &rclPt) const = 0;
    /** Convert a 2D point on the projection plane in 3D space */
    virtual Vector3d inverse (const Vector3d &rclPt) const = 0;
    /** Calculate the projection (+ mapping) matrix */
    virtual Matrix4D getProjectionMatrix (void) const = 0; 

protected:
    ViewProjMethod(){}
};

/**
 * The ViewProjMatrix class returns the result of the multiplication
 * of the 3D vector and the view transformation matrix.
 */
class BaseExport ViewProjMatrix : public ViewProjMethod
{
public:
    ViewProjMatrix (const Matrix4D &rclMtx);
    virtual ~ViewProjMatrix();

    Vector3f operator()(const Vector3f &rclPt) const;
    Vector3d operator()(const Vector3d &rclPt) const;
    Vector3f inverse (const Vector3f &rclPt) const;
    Vector3d inverse (const Vector3d &rclPt) const;

    Matrix4D getProjectionMatrix (void) const;

protected:
    bool isOrthographic;
    Matrix4D _clMtx, _clMtxInv;
};

} // namespace Base

#endif // BASE_VIEWPROJ_H
