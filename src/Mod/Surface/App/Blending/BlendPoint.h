// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Matteo Grellier
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once


#include <Base/Vector3D.h>
#include <Mod/Surface/SurfaceGlobal.h>
#include <vector>


namespace Surface
{

/*!
 *   Create a list of vectors formed by a point and some derivatives
 *   obtained from a curve or surface
 */
class SurfaceExport BlendPoint
{
public:
    std::vector<Base::Vector3d> vectors;

    BlendPoint();
    /*!
     *  Constructor
     *\param std::vector<Base::Vector3d>
     */
    explicit BlendPoint(const std::vector<Base::Vector3d>& vectorList);
    ~BlendPoint() = default;
    /*!
     *  Scale the blendpoint vectors
     *\param double scaling factor
     */
    void multiply(double f);
    /*!
     * Resize the blendpoint vectors
     * by setting the size of the first derivative
     *\param double new size
     */
    void setSize(double f);
    /*!
     *\return continuity of this BlendPoint
     */
    int getContinuity();
    /*!
     *\return Number of vectors of this BlendPoint
     */
    int nbVectors();

private:
};
}  // namespace Surface
