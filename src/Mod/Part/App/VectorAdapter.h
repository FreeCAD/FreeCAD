// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/PartGlobal.h>

#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Lin.hxx>
#include <Base/Vector3D.h>

namespace Part
{


/*! @brief Convert to vector
 *
 * Used to construct a vector from various input types
 */
class VectorAdapter
{
public:
    /*!default construction isValid is set to false*/
    VectorAdapter();
    /*!Build a vector from a faceIn
     * @param faceIn vector will be normal to plane and equal to cylindrical axis.
     * @param pickedPointIn location of pick. straight conversion from sbvec. not accurate.*/
    VectorAdapter(const TopoDS_Face& faceIn, const gp_Vec& pickedPointIn);
    /*!Build a vector from an edgeIn
     * @param edgeIn vector will be lastPoint - firstPoint.
     * @param pickedPointIn location of pick. straight conversion from sbvec. not accurate.*/
    VectorAdapter(const TopoDS_Edge& edgeIn, const gp_Vec& pickedPointIn);
    /*!Build a vector From 2 vertices.
     *vector will be equal to @param vertex2In - @param vertex1In.*/
    VectorAdapter(const TopoDS_Vertex& vertex1In, const TopoDS_Vertex& vertex2In);
    /*!Build a vector From 2 vectors.
     *vector will be equal to @param vector2 - @param vector1.*/
    VectorAdapter(const gp_Vec& vector1, const gp_Vec& vector2);

    /*!make sure no errors in vector construction.
     * @return true = vector is good. false = vector is NOT good.*/
    bool isValid() const
    {
        return status;
    }
    /*!get the calculated vector.
     * @return the vector. use isValid to ensure correct results.*/
    explicit operator gp_Vec() const
    {
        return vector;
    }
    /*!build occ line used for extrema calculation*/
    explicit operator gp_Lin() const;
    gp_Vec getPickPoint() const
    {
        return origin;
    }

    explicit operator Base::Vector3d() const
    {
        return Base::Vector3d(vector.X(), vector.Y(), vector.Z());
    }

    static gp_Vec convert(const TopoDS_Vertex& vertex);

private:
    void projectOriginOntoVector(const gp_Vec& pickedPointIn);
    bool handleElementarySurface(const TopoDS_Face& faceIn, const gp_Vec& pickedPointIn);
    bool handlePlanarSurface(const TopoDS_Face& faceIn, const gp_Vec& pickedPointIn);
    bool status;
    gp_Vec vector;
    gp_Vec origin;
};


}  // namespace Part
