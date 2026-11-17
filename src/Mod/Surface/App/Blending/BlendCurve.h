// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Matteo Grellier
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

#include <Geom_BezierCurve.hxx>
#include <Mod/Surface/App/Blending/BlendPoint.h>
#include <Mod/Surface/SurfaceGlobal.h>

namespace Surface
{
/*!
 * Create a BezierCurve interpolating a list of BlendPoints
 */
class SurfaceExport BlendCurve
{
public:
    std::vector<BlendPoint> blendPoints;

    BlendCurve() = default;
    /*!
     *  Constructor
     *\param std::vector<BlendPoint>
     */
    explicit BlendCurve(const std::vector<BlendPoint>& blendPointsList);
    ~BlendCurve() = default;
    /*!
     *  Perform the interpolate algorithm
     *\return the BezierCurve
     */
    Handle(Geom_BezierCurve) compute();
    /*!
     *  Set the size of the first derivative of a BlendPoint
     *\param int index of the BlendPoint to modify
     *\param double new size
     *\param bool interpret new size relative to chordlength
     */
    void setSize(int, double, bool);
};
}  // namespace Surface
