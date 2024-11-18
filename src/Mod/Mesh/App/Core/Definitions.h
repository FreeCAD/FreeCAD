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

#ifndef MESH_DEFINITIONS_H
#define MESH_DEFINITIONS_H

#ifndef MESH_GLOBAL_H
#include <Mod/Mesh/MeshGlobal.h>
#endif

#include <climits>

// default values
#define MESH_MIN_PT_DIST 1.0e-6f
#define MESH_MIN_EDGE_LEN 1.0e-3f
#define MESH_MIN_EDGE_ANGLE 2.0
#define MESH_REMOVE_MIN_LEN true
#define MESH_REMOVE_G3_EDGES true

/*
 * general constant definitions
 */
#define FLOAT_EPS 1.0e-4f

#ifndef FLOAT_MAX
#define FLOAT_MAX 1e30f
#endif

#ifndef DOUBLE_MAX
#define DOUBLE_MAX 1.7976931348623157E+308 /* max decimal value of a "double"*/
#endif

#ifndef DOUBLE_MIN
#define DOUBLE_MIN 2.2250738585072014E-308 /* min decimal value of a "double"*/
#endif

namespace MeshCore
{

// type definitions
using ElementIndex = unsigned long;
const ElementIndex ELEMENT_INDEX_MAX = ULONG_MAX;
using FacetIndex = ElementIndex;
const FacetIndex FACET_INDEX_MAX = ULONG_MAX;
using PointIndex = ElementIndex;
const PointIndex POINT_INDEX_MAX = ULONG_MAX;

template<class Prec>
class Math
{
public:
    MeshExport static const Prec PI;
};

using Mathf = Math<float>;
using Mathd = Math<double>;

/**
 * Global defined tolerances used to compare points
 * for equality.
 */
class MeshExport MeshDefinitions
{
public:
    MeshDefinitions();

    static float _fMinPointDistance;
    static float _fMinPointDistanceP2;
    static float _fMinPointDistanceD1;

    static float _fMinEdgeLength;
    static bool _bRemoveMinLength;

    static float _fMinEdgeAngle;

    static void SetMinPointDistance(float fMin);
};

}  // namespace MeshCore

#endif  // MESH_DEFINITIONS_H
