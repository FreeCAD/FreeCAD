/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_DIMENSIONVALIDATORS_H
#define TECHDRAW_DIMENSIONVALIDATORS_H

#include <Mod/TechDraw/TechDrawGlobal.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>


namespace App
{
class Document;
class DocumentObject;
}

using StringVector = std::vector<std::string>;
using GeomCount = std::pair<std::string, int>;           //geometry descriptor ("Edge") + counter pair
using GeomCountVector = std::vector<GeomCount>;
using GeomCountMap = std::map<std::string, int>;
using GeometrySet = std::unordered_set<std::string>;    //queryable unique set of geometrty descriptors

using DimensionGeometryType = int;

namespace TechDraw
{

class DrawViewPart;

enum DimensionGeometryEnum {
        isInvalid,
        isHorizontal,
        isVertical,
        isDiagonal,
        isCircle,
        isEllipse,
        isBSplineCircle,
        isBSpline,
        isAngle,
        isAngle3Pt,
        isMultiEdge,
        isZLimited,
        isViewReference         //never needs to be specified in the acceptable list
    };

DrawViewPart* getReferencesFromSelection(ReferenceVector& references2d,
                                         ReferenceVector& references3d);
DimensionGeometryType validateDimSelection(
    ReferenceVector references,
    StringVector acceptableGeometry,//"Edge", "Vertex", etc
    std::vector<int> minimumCounts, //how many of each geometry are needed for a good dimension
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys);//isVertical, isHorizontal, ...
DimensionGeometryType validateDimSelection3d(
    DrawViewPart* dvp, ReferenceVector references,
    StringVector acceptableGeometry,//"Edge", "Vertex", etc
    std::vector<int> minimumCounts, //how many of each geometry are needed for a good dimension
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys);//isVertical, isHorizontal, ...

bool validateSubnameList(StringVector subNames, GeometrySet acceptableGeometrySet);

DimensionGeometryType getGeometryConfiguration(ReferenceVector valid2dReferences);
DimensionGeometryType getGeometryConfiguration3d(DrawViewPart* dvp,
                                                                ReferenceVector valid3dReferences);

GeomCountMap loadRequiredCounts(StringVector& acceptableGeometry,
                                               std::vector<int>& minimumCouts);
bool checkGeometryOccurences(StringVector subNames, GeomCountMap keyedMinimumCounts);

DimensionGeometryType isValidVertexes(ReferenceVector refs);
DimensionGeometryType isValidMultiEdge(ReferenceVector refs);
DimensionGeometryType isValidSingleEdge(ReferenceEntry ref);
DimensionGeometryType isValidHybrid(ReferenceVector refs);

DimensionGeometryType isValidVertexes3d(DrawViewPart* dvp, ReferenceVector refs);
DimensionGeometryType isValidMultiEdge3d(DrawViewPart* dvp, ReferenceVector refs);
DimensionGeometryType isValidSingleEdge3d(DrawViewPart* dvp, ReferenceEntry ref);
DimensionGeometryType isValidHybrid3d(DrawViewPart* dvp, ReferenceVector refs);

long int mapGeometryTypeToDimType(long int dimType, DimensionGeometryType geometry2d,
                                                 DimensionGeometryType geometry3d);
}
#endif //TECHDRAW_DIMENSIONVALIDATORS_H

