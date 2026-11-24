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

#include <Precision.hxx>
#include <Standard_Real.hxx>

#include "Blending/BlendPoint.h"
#include "Blending/BlendPointPy.h"


using namespace Surface;

BlendPoint::BlendPoint(const std::vector<Base::Vector3d>& vectorList)
    : vectors {vectorList}
{}

BlendPoint::BlendPoint()
{
    vectors.emplace_back(Base::Vector3d(0, 0, 0));
}

void BlendPoint::multiply(double f)
{
    for (int i = 0; i < nbVectors(); i++) {
        vectors[i] *= Pow(f, i);
    }
}

void BlendPoint::setSize(double f)
{
    if (nbVectors() > 1) {
        double il = vectors[1].Length();
        if (il > Precision::Confusion()) {
            multiply(f / il);
        }
    }
}

int BlendPoint::getContinuity()
{
    return vectors.size() - 1;
}

int BlendPoint::nbVectors()
{
    return vectors.size();
}
