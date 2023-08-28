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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#endif//#ifndef _PreComp_

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/ShapeExtractor.h>

#include "DimensionValidators.h"


using namespace TechDraw;
using DU = DrawUtil;

TechDraw::DrawViewPart* TechDraw::getReferencesFromSelection(ReferenceVector& references2d,
                                                             ReferenceVector& references3d)
{
    TechDraw::DrawViewPart* dvp(nullptr);
    TechDraw::DrawViewDimension* dim(nullptr);
    std::vector<Gui::SelectionObject> selectionAll = Gui::Selection().getSelectionEx();
    for (auto& selItem : selectionAll) {
        if (selItem.getObject()->isDerivedFrom(TechDraw::DrawViewDimension::getClassTypeId())) {
            //we are probably repairing a dimension, but we will check later
            dim = static_cast<TechDraw::DrawViewDimension*>(selItem.getObject());
        } else if (selItem.getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            //this could be a 2d geometry selection or just a DrawViewPart for context in
            //a 3d selection
            dvp = static_cast<TechDraw::DrawViewPart*>(selItem.getObject());
            if (selItem.getSubNames().empty()) {
                //there are no subNames, so we think this is a 3d case,
                //and we only need to select the view. We set the reference
                //subName to a null string to avoid later misunderstandings.
                ReferenceEntry ref(dvp, std::string());
                references2d.push_back(ref);
            }
            for (auto& sub : selItem.getSubNames()) {
                ReferenceEntry ref(dvp, sub);
                references2d.push_back(ref);
            }
        } else if (!selItem.getObject()->isDerivedFrom(TechDraw::DrawView::getClassTypeId())) {
            //this is not a TechDraw object, so we check to see if it has 3d geometry
            std::vector<App::DocumentObject*> links;
            links.push_back(selItem.getObject());
            if (!ShapeExtractor::getShapes(links).IsNull()) {
                //this item has 3d geometry so we are interested
                App::DocumentObject* obj3d = selItem.getObject();
                if (selItem.getSubNames().empty()) {
                    if (ShapeExtractor::isPointType(obj3d)) {
                        //a point object may not have a subName when selected,
                        //so we need to perform some special handling.
                        ReferenceEntry ref(obj3d, "Vertex1");
                        references3d.push_back(ref);
                        continue;
                    } else {
                        //this is a whole object reference, probably for an extent dimension
                        ReferenceEntry ref(obj3d, std::string());
                        references3d.push_back(ref);
                        continue;
                    }
                }
                //this is a regular reference in form obj+subelement
                for (auto& sub3d : selItem.getSubNames()) {
                    ReferenceEntry ref(obj3d, sub3d);
                    references3d.push_back(ref);
                }
            } else {
                Base::Console().Message("DV::getRefsFromSel - %s has no shape!\n",
                                        selItem.getObject()->getNameInDocument());
            }
        }
    }
    if (dim) {
        if (!dvp) {
            ReferenceEntry ref(dim->getViewPart(), std::string());
            references2d.push_back(ref);
            return dim->getViewPart();
        }
    }
    return dvp;
}

//! verify that the proposed references contains valid geometries from a 2d DrawViewPart.
DimensionGeometryType TechDraw::validateDimSelection(
    ReferenceVector references,     //[(dvp*, std::string),...,(dvp*, std::string)]
    StringVector acceptableGeometry,//"Edge", "Vertex", etc
    std::vector<int> minimumCounts, //how many of each geometry are needed for a good dimension
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys)//isVertical, isHorizontal, ...
{
    StringVector subNames;
    TechDraw::DrawViewPart* dvpSave(nullptr);
    for (auto& ref : references) {
        TechDraw::DrawViewPart* dvp = dynamic_cast<TechDraw::DrawViewPart*>(ref.getObject());
        if (dvp) {
            dvpSave = dvp;
            if (!ref.getSubName().empty()) {
                subNames.push_back(ref.getSubName());
            }
        }
    }
    if (!dvpSave) {
        //must have 1 DVP in selection
        return isInvalid;
    }

    if (subNames.empty()) {
        //no geometry referenced. can not make a dim from this mess. We are being called to validate
        //a selection for a 3d reference
        return isViewReference;
    }

    if (subNames.front().empty()) {
        //can this still happen?
        return isViewReference;
    }

    //check for invalid geometry descriptors in the subNames
    std::unordered_set<std::string> acceptableGeometrySet(acceptableGeometry.begin(),
                                                          acceptableGeometry.end());
    if (!TechDraw::validateSubnameList(subNames, acceptableGeometrySet)) {
        //can not make a dimension from this
        return isInvalid;
    }

    //check for wrong number of geometry
    GeomCountVector foundCounts;
    GeomCountMap minimumCountMap = loadRequiredCounts(acceptableGeometry, minimumCounts);
    if (!checkGeometryOccurences(subNames, minimumCountMap)) {
        //too many or too few geometry descriptors.
        return isInvalid;
    }

    //we have a (potentially valid collection of 2d geometry
    ReferenceVector valid2dReferences;
    for (auto& sub : subNames) {
        ReferenceEntry validEntry(dvpSave, sub);
        valid2dReferences.push_back(validEntry);
    }

    DimensionGeometryType foundGeometry = getGeometryConfiguration(valid2dReferences);
    if (acceptableDimensionGeometrys.empty()) {
        //if the list is empty, we are accepting anything
        return foundGeometry;
    }
    for (auto& acceptable : acceptableDimensionGeometrys) {
        if (foundGeometry == acceptable) {
            return foundGeometry;
        }
    }

    return isInvalid;
}

//! verify that the proposed references contains valid geometries from non-TechDraw objects.
DimensionGeometryType TechDraw::validateDimSelection3d(
    TechDraw::DrawViewPart* dvp,
    ReferenceVector references,     //[(dvp*, std::string),...,(dvp*, std::string)]
    StringVector acceptableGeometry,//"Edge", "Vertex", etc
    std::vector<int> minimumCounts, //how many of each geometry are needed for a good dimension
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys)//isVertical, isHorizontal, ...
{
    StringVector subNames;
    for (auto& ref : references) {
        if (!ref.getSubName().empty()) {
            subNames.push_back(ref.getSubName());
        }
    }


    //check for invalid geometry descriptors in the subNames
    std::unordered_set<std::string> acceptableGeometrySet(acceptableGeometry.begin(),
                                                          acceptableGeometry.end());
    if (!TechDraw::validateSubnameList(subNames, acceptableGeometrySet)) {
        //can not make a dimension from this
        return isInvalid;
    }

    //check for wrong number of geometry
    GeomCountMap minimumCountMap = loadRequiredCounts(acceptableGeometry, minimumCounts);
    if (!checkGeometryOccurences(subNames, minimumCountMap)) {
        //too many or too few geometry descriptors.
        return isInvalid;
    }

    //we have a (potentially valid collection of 3d geometry
    DimensionGeometryType foundGeometry = getGeometryConfiguration3d(dvp, references);
    if (acceptableDimensionGeometrys.empty()) {
        //if the list is empty, we are accepting anything
        return foundGeometry;
    }
    for (auto& acceptable : acceptableDimensionGeometrys) {
        if (foundGeometry == acceptable) {
            return foundGeometry;
        }
    }

    return isInvalid;
}
bool TechDraw::validateSubnameList(StringVector subNames, GeometrySet acceptableGeometrySet)
{
    for (auto& sub : subNames) {
        std::string geometryType = DrawUtil::getGeomTypeFromName(sub);
        if (acceptableGeometrySet.count(geometryType) == 0) {
            //this geometry type is not allowed
            return false;
        }
    }
    return true;
}

//count how many of each "Edge", "Vertex, etc and compare totals to required minimum
bool TechDraw::checkGeometryOccurences(StringVector subNames, GeomCountMap keyedMinimumCounts)
{
    //how many of each geometry descriptor are input
    GeomCountMap foundCounts;
    for (auto& sub : subNames) {
        std::string geometryType = DrawUtil::getGeomTypeFromName(sub);
        std::map<std::string, int>::iterator it0(foundCounts.find(geometryType));
        if (it0 == foundCounts.end()) {
            //first occurrence of this geometryType
            foundCounts[geometryType] = 1;
        } else {
            //already have this geometryType
            it0->second++;
        }
    }

    //hybrid dims (vertex-edge) can skip this check
    if (foundCounts.size() > 1) {
        //this is a hybrid dimension
        return true;
    }

    //check found geometry counts against required counts
    for (auto& foundItem : foundCounts) {
        std::string currentKey = foundItem.first;
        int foundCount = foundItem.second;
        auto itAccept = keyedMinimumCounts.find(currentKey);
        if (itAccept == keyedMinimumCounts.end()) {
            //not supposed to happen by this point
            throw Base::IndexError("Dimension validation counts and geometry do not match");
        }
        if (foundCount < keyedMinimumCounts[currentKey]) {
            //not enough of this type of geom to make a good dimension - ex 1 Vertex
            return false;
        }
    }
    //we have no complaints about the input
    return true;
}

//return the first valid configuration contained in the already validated references
DimensionGeometryType TechDraw::getGeometryConfiguration(ReferenceVector valid2dReferences)
{
    DimensionGeometryType config = isValidMultiEdge(valid2dReferences);
    if (config > isInvalid) {
        return config;
    }
    config = isValidVertexes(valid2dReferences);
    if (config > isInvalid) {
        return config;
    }
    config = isValidSingleEdge(valid2dReferences.front());
    if (config > isInvalid) {
        return config;
    }
    config = isValidHybrid(valid2dReferences);
    if (config > isInvalid) {
        return config;
    }

    // no valid configuration found
    return isInvalid;
}

//return the first valid configuration contained in the already validated references
DimensionGeometryType TechDraw::getGeometryConfiguration3d(DrawViewPart* dvp,
                                                           ReferenceVector valid3dReferences)
{
    //first we check for whole object references
    ReferenceVector wholeObjectRefs;
    ReferenceVector subElementRefs;
    for (auto& ref : valid3dReferences) {
        if (ref.isWholeObject()) {
            wholeObjectRefs.push_back(ref);
        } else {
            subElementRefs.push_back(ref);
        }
    }
    if (subElementRefs.empty()) {
        //only whole object references
        return isMultiEdge;
    }
    if (!wholeObjectRefs.empty()) {
        //mix of whole object and subelement refs
        return isMultiEdge;//??? correct ???
    }

    //only have subelement refs
    DimensionGeometryType config = isValidMultiEdge3d(dvp, valid3dReferences);
    if (config > isInvalid) {
        return config;
    }
    config = isValidVertexes3d(dvp, valid3dReferences);
    if (config > isInvalid) {
        return config;
    }
    config = isValidSingleEdge3d(dvp, valid3dReferences.front());
    if (config > isInvalid) {
        return config;
    }
    config = isValidHybrid3d(dvp, valid3dReferences);
    if (config > isInvalid) {
        return config;
    }

    // no valid configuration found
    return isInvalid;
}

//fill the GeomCountMap with pairs made from corresponding items in acceptableGeometry
//and minimumCounts
GeomCountMap TechDraw::loadRequiredCounts(StringVector& acceptableGeometry,
                                          std::vector<int>& minimumCounts)
{
    if (acceptableGeometry.size() != minimumCounts.size()) {
        throw Base::IndexError("acceptableGeometry and minimum counts have different sizes.");
    }

    GeomCountMap result;
    int iCount = 0;
    for (auto& acceptableItem : acceptableGeometry) {
        result[acceptableItem] = minimumCounts.at(iCount);
        iCount++;
    }
    return result;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
DimensionGeometryType TechDraw::isValidSingleEdge(ReferenceEntry ref)
{
    auto objFeat(dynamic_cast<TechDraw::DrawViewPart*>(ref.getObject()));
    if (!objFeat) {
        return isInvalid;
    }

    //the Name starts with "Edge"
    std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName());
    if (geomName != "Edge") {
        return isInvalid;
    }

    //the geometry exists (redundant?)
    int GeoId(TechDraw::DrawUtil::getIndexFromName(ref.getSubName()));
    TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
    if (!geom) {
        return isInvalid;
    }

    if (geom->getGeomType() == TechDraw::GENERIC) {
        TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom);
        if (gen1->points.size() < 2) {
            return isInvalid;
        }
        Base::Vector3d line = gen1->points.at(1) - gen1->points.at(0);
        if (fabs(line.y) < FLT_EPSILON) {
            return TechDraw::isHorizontal;
        } else if (fabs(line.x) < FLT_EPSILON) {
            return TechDraw::isVertical;
        } else {
            return TechDraw::isDiagonal;
        }
    } else if (geom->getGeomType() == TechDraw::CIRCLE || geom->getGeomType() == TechDraw::ARCOFCIRCLE) {
        return isCircle;
    } else if (geom->getGeomType() == TechDraw::ELLIPSE || geom->getGeomType() == TechDraw::ARCOFELLIPSE) {
        return isEllipse;
    } else if (geom->getGeomType() == TechDraw::BSPLINE) {
        TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline>(geom);
        if (spline->isCircle()) {
            return isBSplineCircle;
        } else {
            return isBSpline;
        }
    }
    return isInvalid;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
DimensionGeometryType TechDraw::isValidSingleEdge3d(DrawViewPart* dvp, ReferenceEntry ref)
{
    (void)dvp;
    //the Name starts with "Edge"
    std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName());
    if (geomName != "Edge") {
        return isInvalid;
    }

    TopoDS_Shape refShape = ref.getGeometry();
    if (refShape.IsNull() || refShape.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for reference is not an edge.");
    }

    TopoDS_Edge occEdge = TopoDS::Edge(refShape);
    BRepAdaptor_Curve adapt(occEdge);
    if (adapt.GetType() == GeomAbs_Line) {
        Base::Vector3d point0 = DU::toVector3d(BRep_Tool::Pnt(TopExp::FirstVertex(occEdge)));
        point0 = dvp->projectPoint(point0);
        Base::Vector3d point1 = DU::toVector3d(BRep_Tool::Pnt(TopExp::LastVertex(occEdge)));
        point1 = dvp->projectPoint(point1);
        Base::Vector3d line = point1 - point0;
        if (fabs(line.y) < FLT_EPSILON) {
            return TechDraw::isHorizontal;
        } else if (fabs(line.x) < FLT_EPSILON) {
            return TechDraw::isVertical;
        }
        //        else if (fabs(line.z) < FLT_EPSILON) {
        //            return TechDraw::isZLimited;
        //        }
        else {
            return TechDraw::isDiagonal;
        }
    } else if (adapt.GetType() == GeomAbs_Circle) {
        return isCircle;
    } else if (adapt.GetType() == GeomAbs_Ellipse) {
        return isEllipse;
    } else if (adapt.GetType() == GeomAbs_BSplineCurve) {
        if (GeometryUtils::isCircle(occEdge)) {
            return isBSplineCircle;
        } else {
            return isBSpline;
        }
    }

    return isInvalid;
}

//! verify that the edge references can make a dimension. Currently only extent
//! dimensions support more than 2 edges
DimensionGeometryType TechDraw::isValidMultiEdge(ReferenceVector refs)
{
    //there has to be at least 2
    if (refs.size() < 2) {
        return isInvalid;
    }

    auto objFeat0(dynamic_cast<TechDraw::DrawViewPart*>(refs.at(0).getObject()));
    if (!objFeat0) {
        //probably redundant
        throw Base::RuntimeError("Logic error in isValidMultiEdge");
    }

    //they all must start with "Edge"
    for (auto& ref : refs) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(ref.getSubName()) != "Edge") {
            return isInvalid;
        }
    }

    if (refs.size() > 2) {
        //many edges, must be an extent?
        return isMultiEdge;
    }

    //exactly 2 edges. could be angle, could be distance
    int GeoId0(TechDraw::DrawUtil::getIndexFromName(refs.at(0).getSubName()));
    int GeoId1(TechDraw::DrawUtil::getIndexFromName(refs.at(1).getSubName()));
    TechDraw::BaseGeomPtr geom0 = objFeat0->getGeomByIndex(GeoId0);
    TechDraw::BaseGeomPtr geom1 = objFeat0->getGeomByIndex(GeoId1);

    if (geom0->getGeomType() == TechDraw::GENERIC && geom1->getGeomType() == TechDraw::GENERIC) {
        TechDraw::GenericPtr gen0 = std::static_pointer_cast<TechDraw::Generic>(geom0);
        TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
        if (gen0->points.size() > 2 || gen1->points.size() > 2) {//the edge is a polyline
            return isInvalid;                                    //not supported yet
        }
        Base::Vector3d line0 = gen0->points.at(1) - gen0->points.at(0);
        Base::Vector3d line1 = gen1->points.at(1) - gen1->points.at(0);
        double xprod = fabs(line0.x * line1.y - line0.y * line1.x);
        if (xprod > FLT_EPSILON) {//edges are not parallel
            return isAngle;       //angle or distance
        } else {
            return isDiagonal;//distance || line
        }
    } else {
        return isDiagonal;//two edges, not both straight lines
    }

    return isInvalid;
}

//! verify that the edge references can make a dimension. Currently only extent
//! dimensions support more than 2 edges
DimensionGeometryType TechDraw::isValidMultiEdge3d(DrawViewPart* dvp, ReferenceVector refs)
{
    (void)dvp;
    //there has to be at least 2
    if (refs.size() < 2) {
        return isInvalid;
    }

    std::vector<TopoDS_Edge> edges;
    for (auto& ref : refs) {
        std::vector<TopoDS_Shape> shapesAll = ShapeExtractor::getShapesFromObject(ref.getObject());
        if (shapesAll.empty()) {
            //reference has no geometry
            return isInvalid;
        }

        //the Name starts with "Edge"
        std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName().c_str());
        if (geomName != "Edge") {
            return isInvalid;
        }
    }
    std::vector<TopoDS_Edge> edgesAll;
    std::vector<int> typeAll;
    for (auto& ref : refs) {
        TopoDS_Shape geometry = ref.getGeometry();
        if (geometry.ShapeType() != TopAbs_EDGE) {
            return isInvalid;
        }
        TopoDS_Edge edge = TopoDS::Edge(geometry);
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() != GeomAbs_Line) {
            //not a line, so this must be an extent dim?
            return isMultiEdge;
        }
        edgesAll.push_back(edge);
    }
    if (edgesAll.size() > 2) {
        //must be an extent dimension of lines?
        return isMultiEdge;
    } else if (edgesAll.size() == 2) {
        Base::Vector3d first0 = DU::toVector3d(BRep_Tool::Pnt(TopExp::FirstVertex(edgesAll.at(0))));
        Base::Vector3d last0 = DU::toVector3d(BRep_Tool::Pnt(TopExp::LastVertex(edgesAll.at(1))));
        Base::Vector3d line0 = last0 - first0;
        Base::Vector3d first1 = DU::toVector3d(BRep_Tool::Pnt(TopExp::FirstVertex(edgesAll.at(0))));
        Base::Vector3d last1 = DU::toVector3d(BRep_Tool::Pnt(TopExp::LastVertex(edgesAll.at(1))));
        Base::Vector3d line1 = last1 - first1;
        if (DU::fpCompare(fabs(line0.Dot(line1)), 1)) {
            //lines are parallel, must be distance dim
            return isDiagonal;
        } else {
            //lines are skew, could be angle, could be distance?
            return isAngle;
        }
    }

    return isInvalid;
}

//! verify that the vertex references can make a dimension
DimensionGeometryType TechDraw::isValidVertexes(ReferenceVector refs)
{
    TechDraw::DrawViewPart* dvp(dynamic_cast<TechDraw::DrawViewPart*>(refs.front().getObject()));
    if (!dvp) {
        //probably redundant
        throw Base::RuntimeError("Logic error in isValidMultiEdge");
    }

    if (refs.size() == 2) {
        //2 vertices can only make a distance dimension
        TechDraw::VertexPtr v0 = dvp->getVertex(refs.at(0).getSubName());
        TechDraw::VertexPtr v1 = dvp->getVertex(refs.at(1).getSubName());
        Base::Vector3d line = v1->point() - v0->point();
        if (fabs(line.y) < FLT_EPSILON) {
            return isHorizontal;
        } else if (fabs(line.x) < FLT_EPSILON) {
            return isVertical;
        } else {
            return isDiagonal;
        }
    } else if (refs.size() == 3) {
        //three vertices make an angle dimension
        return isAngle3Pt;
    }

    // did not find a valid configuration
    return isInvalid;
}

//! verify that the vertex references can make a dimension
DimensionGeometryType TechDraw::isValidVertexes3d(DrawViewPart* dvp, ReferenceVector refs)
{
    (void)dvp;
    if (refs.size() == 2) {
        //2 vertices can only make a distance dimension
        TopoDS_Shape geometry0 = refs.at(0).getGeometry();
        TopoDS_Shape geometry1 = refs.at(1).getGeometry();
        if (geometry0.IsNull() || geometry1.IsNull() || geometry0.ShapeType() != TopAbs_VERTEX
            || geometry1.ShapeType() != TopAbs_VERTEX) {
            return isInvalid;
        }
        Base::Vector3d point0 = DU::toVector3d(BRep_Tool::Pnt(TopoDS::Vertex(geometry0)));
        point0 = dvp->projectPoint(point0);
        Base::Vector3d point1 = DU::toVector3d(BRep_Tool::Pnt(TopoDS::Vertex(geometry1)));
        point1 = dvp->projectPoint(point1);
        Base::Vector3d line = point1 - point0;
        if (fabs(line.y) < FLT_EPSILON) {
            return isHorizontal;
        } else if (fabs(line.x) < FLT_EPSILON) {
            return isVertical;
            //        } else if(fabs(line.z) < FLT_EPSILON) {
            //            return isZLimited;
        } else {
            return isDiagonal;
        }
    } else if (refs.size() == 3) {
        //three vertices make an angle dimension
        //we could check here that all the geometries are Vertex
        return isAngle3Pt;
    }

    // did not find a valid configuration
    return isInvalid;
}

//! verify that the mixed bag (ex Vertex-Edge) of references can make a dimension
DimensionGeometryType TechDraw::isValidHybrid(ReferenceVector refs)
{
    if (refs.empty()) {
        return isInvalid;
    }

    int vertexCount(0);
    int edgeCount(0);
    for (auto& ref : refs) {
        if (DU::getGeomTypeFromName(ref.getSubName()) == "Vertex") {
            vertexCount++;
        }
        if (DU::getGeomTypeFromName(ref.getSubName()) == "Edge") {
            edgeCount++;
        }
    }
    if (vertexCount > 0 && edgeCount > 0) {
        //must be a diagonal dim? could it be isHorizontal or isVertical?
        return isDiagonal;
    }

    return isInvalid;
}

//! verify that the mixed bag (ex Vertex-Edge) of references can make a dimension
DimensionGeometryType TechDraw::isValidHybrid3d(DrawViewPart* dvp, ReferenceVector refs)
{
    (void)dvp;
    //we don't have a special check for 3d in this case
    return isValidHybrid(refs);
}

//handle situations where revised geometry type is valid but not suitable for existing dimType
long int TechDraw::mapGeometryTypeToDimType(long int dimType, DimensionGeometryType geometry2d,
                                            DimensionGeometryType geometry3d)
{
    if (geometry2d == isInvalid && geometry3d == isInvalid) {
        //probably an error, but we can't do anything with this
        return dimType;
    }

    if (geometry2d == isViewReference && geometry3d != isInvalid) {
        switch (geometry3d) {
            case isDiagonal:
                return DrawViewDimension::Distance;
            case isHorizontal:
                return DrawViewDimension::DistanceX;
            case isVertical:
                return DrawViewDimension::DistanceY;
            case isAngle:
                return DrawViewDimension::Angle;
            case isAngle3Pt:
                return DrawViewDimension::Angle3Pt;
        }
    } else if (geometry2d != isViewReference) {
        switch (geometry2d) {
            case isDiagonal:
                return DrawViewDimension::Distance;
            case isHorizontal:
                return DrawViewDimension::DistanceX;
            case isVertical:
                return DrawViewDimension::DistanceY;
            case isAngle:
                return DrawViewDimension::Angle;
            case isAngle3Pt:
                return DrawViewDimension::Angle3Pt;
        }
    }
    return dimType;
}
