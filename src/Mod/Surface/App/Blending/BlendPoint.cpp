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
#include <Base/Persistence.h>
#include <Base/Vector3D.h>
#include <Precision.hxx>
#include <Standard_Version.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#endif
#include "Blending/BlendPoint.h"
#include "Blending/BlendPointPy.h"


using namespace Surface;

BlendPoint::BlendPoint(std::vector<Base::Vector3d> vectorList)
{
    for (size_t i = 0; i < vectorList.size(); i++) {
        vectors.emplace_back(vectorList[i]);
    } 
}

BlendPoint::BlendPoint()
{
    vectors.emplace_back(Base::Vector3d(0, 0, 0));   
}

BlendPoint::~BlendPoint()
{
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

PyObject *BlendPoint::getPyObject(void)
{
    return new BlendPointPy(new BlendPoint(vectors));
}

void BlendPoint::Save(Base::Writer & /*writer*/) const
{
    throw Base::NotImplementedError("BlendPoint::Save");
}

void BlendPoint::Restore(Base::XMLReader & /*reader*/)
{
    throw Base::NotImplementedError("BlendPoint::Restore");
}

unsigned int BlendPoint::getMemSize(void) const
{
    // do we need to loop on the vectors list ?
    return sizeof(vectors) * sizeof(vectors.front());
}
