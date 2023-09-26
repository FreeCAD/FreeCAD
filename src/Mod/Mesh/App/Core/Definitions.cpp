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

#include <cmath>

#include "Definitions.h"
#include <Base/Tools.h>


namespace MeshCore
{

template<>
MeshExport const float Math<float>::PI = (float)(4.0 * atan(1.0));
template<>
MeshExport const double Math<double>::PI = 4.0 * atan(1.0);

float MeshDefinitions::_fMinPointDistance = float(MESH_MIN_PT_DIST);
float MeshDefinitions::_fMinPointDistanceP2 = _fMinPointDistance * _fMinPointDistance;
float MeshDefinitions::_fMinPointDistanceD1 = _fMinPointDistance;
float MeshDefinitions::_fMinEdgeLength = MESH_MIN_EDGE_LEN;
bool MeshDefinitions::_bRemoveMinLength = MESH_REMOVE_MIN_LEN;
float MeshDefinitions::_fMinEdgeAngle = Base::toRadians<float>(MESH_MIN_EDGE_ANGLE);

MeshDefinitions::MeshDefinitions() = default;

void MeshDefinitions::SetMinPointDistance(float fMin)
{
    _fMinPointDistance = fMin;
    _fMinPointDistanceP2 = fMin * fMin;
    _fMinPointDistanceD1 = float(sqrt((fMin * fMin) / 3.0f));
}

}  // namespace MeshCore
