/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com                 *
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
using GeometrySet = std::unordered_set<std::string>;    //queriable unique set of geometrty descriptors

//using ReferenceEntry = std::pair<App::DocumentObject*, std::string>;
//using ReferenceVector = std::vector<ReferenceEntry>;
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

    TechDraw::DrawViewPart*      TechDrawExport getReferencesFromSelection( ReferenceVector& references2d, ReferenceVector& references3d );
    DimensionGeometryType        TechDrawExport validateDimSelection(ReferenceVector references,
                                                              StringVector acceptableGeometry,                     //"Edge", "Vertex", etc
                                                              std::vector<int> minimumCounts,                      //how many of each geometry are needed for a good dimension
                                                              std::vector<DimensionGeometryType> acceptableDimensionGeometrys);          //isVertical, isHorizontal, ...
    DimensionGeometryType        TechDrawExport validateDimSelection3d(TechDraw::DrawViewPart* dvp,
                                                              ReferenceVector references,
                                                              StringVector acceptableGeometry,                     //"Edge", "Vertex", etc
                                                              std::vector<int> minimumCounts,                      //how many of each geometry are needed for a good dimension
                                                              std::vector<DimensionGeometryType> acceptableDimensionGeometrys);          //isVertical, isHorizontal, ...

    bool            TechDrawExport validateSubnameList(StringVector subNames,
                                                       GeometrySet acceptableGeometrySet);

    DimensionGeometryType        TechDrawExport getGeometryConfiguration(TechDraw::ReferenceVector valid2dReferences);
    DimensionGeometryType        TechDrawExport getGeometryConfiguration3d(TechDraw::DrawViewPart* dvp,
                                                                            TechDraw::ReferenceVector valid3dReferences);

    GeomCountMap    TechDrawExport loadRequiredCounts(StringVector& acceptableGeometry,
                                                      std::vector<int>& minimumCouts);
    bool            TechDrawExport checkGeometryOccurences(StringVector subNames,
                                                           GeomCountMap keyedMinimumCounts);

    DimensionGeometryType        TechDrawExport isValidVertexes(TechDraw::ReferenceVector refs);
    DimensionGeometryType        TechDrawExport isValidMultiEdge(TechDraw::ReferenceVector refs);
    DimensionGeometryType        TechDrawExport isValidSingleEdge(TechDraw::ReferenceEntry ref);
    DimensionGeometryType        TechDrawExport isValidHybrid(TechDraw::ReferenceVector refs);

    DimensionGeometryType        TechDrawExport isValidVertexes3d(TechDraw::DrawViewPart* dvp, TechDraw::ReferenceVector refs);
    DimensionGeometryType        TechDrawExport isValidMultiEdge3d(TechDraw::DrawViewPart* dvp, TechDraw::ReferenceVector refs);
    DimensionGeometryType        TechDrawExport isValidSingleEdge3d(TechDraw::DrawViewPart* dvp, TechDraw::ReferenceEntry ref);
    DimensionGeometryType        TechDrawExport isValidHybrid3d(TechDraw::DrawViewPart* dvp, TechDraw::ReferenceVector refs);

    long int                     TechDrawExport mapGeometryTypeToDimType(long int dimType,
                                                           DimensionGeometryType geometry2d,
                                                           DimensionGeometryType geometry3d);
}
#endif //TECHDRAW_DIMENSIONVALIDATORS_H

