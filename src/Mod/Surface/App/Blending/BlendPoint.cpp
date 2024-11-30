/***************************************************************************
 *   Copyright (c) 2022 Matteo Grellier <matteogrellier@gmail.com>         *
 *                                                                         *
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
#ifndef _PreComp_
#include <Precision.hxx>
#include <Standard_Real.hxx>
#endif
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
