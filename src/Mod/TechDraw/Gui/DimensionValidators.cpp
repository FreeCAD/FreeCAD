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
#include <limits>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#endif//#ifndef _PreComp_

#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Selection/Selection.h>
#include <Mod/Measure/App/ShapeFinder.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/ShapeExtractor.h>

#include "DimensionValidators.h"


using namespace TechDraw;
using namespace Measure;
using DU = DrawUtil;
using DimensionGeometry = TechDraw::DimensionGeometry;

TechDraw::DrawViewPart* TechDraw::getReferencesFromSelection(ReferenceVector& references2d,
                                                             ReferenceVector& references3d)
{
    TechDraw::DrawViewPart* dvp(nullptr);
    TechDraw::DrawViewDimension* dim(nullptr);
    constexpr bool allowOnlySingle{false};
    std::vector<Gui::SelectionObject> selectionAll =
            Gui::Selection().getSelectionEx("*",
                                            App::DocumentObject::getClassTypeId(),
                                            Gui::ResolveMode::NoResolve,
                                            allowOnlySingle);

    for (auto& selItem : selectionAll) {
        if (selItem.getObject()->isDerivedFrom<TechDraw::DrawViewDimension>()) {
            //we are probably repairing a dimension, but we will check later
            dim = static_cast<TechDraw::DrawViewDimension*>(selItem.getObject());  //NOLINT cppcoreguidelines-pro-type-static-cast-downcast
        } else if (selItem.getObject()->isDerivedFrom<TechDraw::DrawViewPart>()) {
            //this could be a 2d geometry selection or just a DrawViewPart for context in
            //a 3d selection
            dvp = static_cast<TechDraw::DrawViewPart*>(selItem.getObject());  //NOLINT cppcoreguidelines-pro-type-static-cast-downcast
            if (selItem.getSubNames().empty()) {
                //there are no subNames, so we think this is a 3d case,
                //and we only need to select the view. We set the reference
                //subName to a null string to avoid later misunderstandings.
                ReferenceEntry ref(dvp, std::string());
                references2d.push_back(ref);
                continue;
            }
            for (auto& sub : selItem.getSubNames()) {
                // plain ordinary 2d view + geometry reference

                ReferenceEntry ref(dvp, ShapeFinder::getLastTerm(sub));
                references2d.push_back(ref);
            }
        } else if (!selItem.getObject()->isDerivedFrom<TechDraw::DrawView>()) {
            App::DocumentObject* obj3d = selItem.getObject();
            // this is a regular 3d reference in form obj + long subelement
            for (auto& sub3d : selItem.getSubNames()) {
                ReferenceEntry ref(obj3d, sub3d);
                references3d.push_back(ref);
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
DimensionGeometry TechDraw::validateDimSelection(
    const ReferenceVector& references,     //[(dvp*, std::string),...,(dvp*, std::string)]
    const StringVector& acceptableGeometry,//"Edge", "Vertex", etc
    const std::vector<int>& minimumCounts, //how many of each geometry are needed for a good dimension
    const std::vector<DimensionGeometry>& acceptableDimensionGeometrys)//isVertical, isHorizontal, ...
{
    StringVector subNames;
    TechDraw::DrawViewPart* dvpSave(nullptr);
    for (auto& ref : references) {
        auto dvp = dynamic_cast<TechDraw::DrawViewPart*>(ref.getObject());
        if (dvp) {
            dvpSave = dvp;
            if (!ref.getSubName().empty()) {
                subNames.push_back(ref.getSubName());
            }
        }
    }
    if (!dvpSave) {
        //must have 1 DVP in selection
        return DimensionGeometry::isInvalid;
    }

    if (subNames.empty()) {
        //no geometry referenced. can not make a dim from this mess. We are being called to validate
        //a selection for a 3d reference
        return DimensionGeometry::isViewReference;
    }

    if (subNames.front().empty()) {
        //can this still happen?
        return DimensionGeometry::isViewReference;
    }

    //check for invalid geometry descriptors in the subNames
    std::unordered_set<std::string> acceptableGeometrySet(acceptableGeometry.begin(),
                                                          acceptableGeometry.end());
    if (!TechDraw::validateSubnameList(subNames, acceptableGeometrySet)) {
        //can not make a dimension from this
        return DimensionGeometry::isInvalid;
    }

    //check for wrong number of geometry
    GeomCountVector foundCounts;
    GeomCountMap minimumCountMap = loadRequiredCounts(acceptableGeometry, minimumCounts);
    if (!checkGeometryOccurrences(subNames, minimumCountMap)) {
        //too many or too few geometry descriptors.
        return DimensionGeometry::isInvalid;
    }

    //we have a (potentially valid collection of 2d geometry
    ReferenceVector valid2dReferences;
    for (auto& sub : subNames) {
        ReferenceEntry validEntry(dvpSave, sub);
        valid2dReferences.push_back(validEntry);
    }

    DimensionGeometry foundGeometry = getGeometryConfiguration(valid2dReferences);
    if (acceptableDimensionGeometrys.empty()) {
        //if the list is empty, we are accepting anything
        return foundGeometry;
    }
    for (auto& acceptable : acceptableDimensionGeometrys) {
        if (foundGeometry == acceptable) {
            return foundGeometry;
        }
    }

    return DimensionGeometry::isInvalid;
}

//! verify that the proposed references contains valid geometries from non-TechDraw objects.
DimensionGeometry TechDraw::validateDimSelection3d(
    TechDraw::DrawViewPart* dvp,
    const ReferenceVector& references,     //[(dvp*, std::string),...,(dvp*, std::string)]
    const StringVector& acceptableGeometry,//"Edge", "Vertex", etc
    const std::vector<int>& minimumCounts, //how many of each geometry are needed for a good dimension
    const std::vector<DimensionGeometry>& acceptableDimensionGeometrys)//isVertical, isHorizontal, ...
{
    StringVector subNames;
    for (auto& ref : references) {
        if (!ref.getSubName().empty()) {
            subNames.push_back(ref.getSubName(true));
        }
    }

    //check for invalid geometry descriptors in the subNames
    std::unordered_set<std::string> acceptableGeometrySet(acceptableGeometry.begin(),
                                                          acceptableGeometry.end());
    if (!TechDraw::validateSubnameList(subNames, acceptableGeometrySet)) {
        //can not make a dimension from this
        return DimensionGeometry::isInvalid;
    }

    //check for wrong number of geometry
    GeomCountMap minimumCountMap = loadRequiredCounts(acceptableGeometry, minimumCounts);
    if (!checkGeometryOccurrences(subNames, minimumCountMap)) {
        //too many or too few geometry descriptors.
        return DimensionGeometry::isInvalid;
    }

    //we have a (potentially valid collection of 3d geometry
    DimensionGeometry foundGeometry = getGeometryConfiguration3d(dvp, references);
    if (acceptableDimensionGeometrys.empty()) {
        //if the list is empty, we are accepting anything
        return foundGeometry;
    }
    for (auto& acceptable : acceptableDimensionGeometrys) {
        if (foundGeometry == acceptable) {
            return foundGeometry;
        }
    }

    return DimensionGeometry::isInvalid;
}
bool TechDraw::validateSubnameList(const StringVector& subNames, const GeometrySet& acceptableGeometrySet)
{
    for (auto& sub : subNames) {    // NOLINT (std::ranges::all_of())
        std::string geometryType = DrawUtil::getGeomTypeFromName(ShapeFinder::getLastTerm(sub));
        if (!acceptableGeometrySet.contains(geometryType)) {
            //this geometry type is not allowed
            return false;
        }
    }
    return true;
}

//count how many of each "Edge", "Vertex, etc and compare totals to required minimum
bool TechDraw::checkGeometryOccurrences(const StringVector& subNames, GeomCountMap keyedMinimumCounts)
{
    //how many of each geometry descriptor are input
    GeomCountMap foundCounts;
    for (auto& sub : subNames) {
        std::string geometryType = DrawUtil::getGeomTypeFromName(ShapeFinder::getLastTerm(sub));
        auto it0(foundCounts.find(geometryType));
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
DimensionGeometry TechDraw::getGeometryConfiguration(ReferenceVector valid2dReferences)
{
    DimensionGeometry config = isValidHybrid(valid2dReferences);
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }

    config = isValidMultiEdge(valid2dReferences);
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidVertexes(valid2dReferences);
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidSingleEdge(valid2dReferences.front());
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidSingleFace(valid2dReferences.front());
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }

    // no valid configuration found
    return DimensionGeometry::isInvalid;
}

//return the first valid configuration contained in the already validated references
DimensionGeometry TechDraw::getGeometryConfiguration3d(DrawViewPart* dvp,
                                                       const ReferenceVector& valid3dReferences)
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
        return DimensionGeometry::isMultiEdge;
    }
    if (!wholeObjectRefs.empty()) {
        //mix of whole object and subelement refs
        return DimensionGeometry::isMultiEdge;//??? correct ???
    }

    //only have subelement refs
    DimensionGeometry config = isValidMultiEdge3d(dvp, valid3dReferences);
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidVertexes3d(dvp, valid3dReferences);
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidSingleEdge3d(dvp, valid3dReferences.front());
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidSingleFace3d(dvp, valid3dReferences.front());
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }
    config = isValidHybrid3d(dvp, valid3dReferences);
    if (config > DimensionGeometry::isInvalid) {
        return config;
    }

    // no valid configuration found
    return DimensionGeometry::isInvalid;
}

//fill the GeomCountMap with pairs made from corresponding items in acceptableGeometry
//and minimumCounts
GeomCountMap TechDraw::loadRequiredCounts(const StringVector& acceptableGeometry,
                                          const std::vector<int>& minimumCounts)
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
DimensionGeometry TechDraw::isValidSingleEdge(const ReferenceEntry& ref)
{
    auto objFeat(dynamic_cast<TechDraw::DrawViewPart*>(ref.getObject()));
    if (!objFeat) {
        return DimensionGeometry::isInvalid;
    }

    //the Name starts with "Edge"
    std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName());
    if (geomName != "Edge") {
        return DimensionGeometry::isInvalid;
    }

    //the geometry exists (redundant?)
    int GeoId(TechDraw::DrawUtil::getIndexFromName(ref.getSubName()));
    TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
    if (!geom) {
        return DimensionGeometry::isInvalid;
    }

    if (geom->getGeomType() == GeomType::GENERIC) {
        TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom);
        if (gen1->points.size() < 2) {
            return DimensionGeometry::isInvalid;
        }
        Base::Vector3d line = gen1->points.at(1) - gen1->points.at(0);
        if (fabs(line.y) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isVertical;
        }
        if (fabs(line.x) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isHorizontal;
        }
        return DimensionGeometry::isDiagonal;
    }

    if (geom->getGeomType() == GeomType::CIRCLE || geom->getGeomType() == GeomType::ARCOFCIRCLE) {
        return DimensionGeometry::isCircle;
    }

    if (geom->getGeomType() == GeomType::ELLIPSE || geom->getGeomType() == GeomType::ARCOFELLIPSE) {
        return DimensionGeometry::isEllipse;
    }

    if (geom->getGeomType() == GeomType::BSPLINE) {
        TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline>(geom);
        if (spline->isCircle()) {
            return DimensionGeometry::isBSplineCircle;
        }
        return DimensionGeometry::isBSpline;
    }

    return DimensionGeometry::isInvalid;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
DimensionGeometry TechDraw::isValidSingleEdge3d(DrawViewPart* dvp, const ReferenceEntry& ref)
{
    (void)dvp;
    //the Name starts with "Edge"
    std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName());
    if (geomName != "Edge") {
        return DimensionGeometry::isInvalid;
    }

    TopoDS_Shape refShape = ref.getGeometry();
    if (refShape.IsNull() || refShape.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for reference is not an edge.");
    }

    TopoDS_Edge occEdge = TopoDS::Edge(refShape);
    BRepAdaptor_Curve adapt(occEdge);
    if (adapt.GetType() == GeomAbs_Line) {
        auto point0 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopExp::FirstVertex(occEdge)));
        point0 = dvp->projectPoint(point0);
        auto point1 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopExp::LastVertex(occEdge)));
        point1 = dvp->projectPoint(point1);
        Base::Vector3d line = point1 - point0;
        if (fabs(line.y) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isVertical;
        }

        if (fabs(line.x) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isHorizontal;
        }

        // we don't support Z direction dimensions
        //        else if (fabs(line.z) < std::numeric_limits<float>::epsilon()) {
        //            return TechDraw::isZLimited;
        //        }

        return DimensionGeometry::isDiagonal;
    }

    if (adapt.GetType() == GeomAbs_Circle) {
        return DimensionGeometry::isCircle;
    }

    if (adapt.GetType() == GeomAbs_Ellipse) {
        return DimensionGeometry::isEllipse;
    }
     else if (adapt.GetType() == GeomAbs_BSplineCurve) {
        if (GeometryUtils::isCircle(occEdge)) {
            return DimensionGeometry::isBSplineCircle;
        }
        return DimensionGeometry::isBSpline;
    }

    return DimensionGeometry::isInvalid;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
DimensionGeometry TechDraw::isValidSingleFace(const ReferenceEntry& ref)
{
    auto objFeat(dynamic_cast<TechDraw::DrawViewPart*>(ref.getObject()));
    if (!objFeat) {
        return DimensionGeometry::isInvalid;
    }

    //the Name starts with "Edge"
    std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName());
    if (geomName != "Face") {
        return DimensionGeometry::isInvalid;
    }

    auto geom = objFeat->getFace(ref.getSubName());
    if (!geom) {
        return DimensionGeometry::isInvalid;
    }

    return DimensionGeometry::isFace;
}

//! verify that Selection contains a valid Geometry for a single Edge Dimension
DimensionGeometry TechDraw::isValidSingleFace3d(DrawViewPart* dvp, const ReferenceEntry& ref)
{
    (void)dvp;
    //the Name starts with "Edge"
    std::string geomName = DrawUtil::getGeomTypeFromName(ref.getSubName());
    if (geomName != "Face") {
        return DimensionGeometry::isInvalid;
    }

    TopoDS_Shape refShape = ref.getGeometry();
    if (refShape.IsNull() || refShape.ShapeType() != TopAbs_FACE) {
        Base::Console().warning("Geometry for reference is not a face.\n");
        return DimensionGeometry::isInvalid;
    }

    return DimensionGeometry::isFace;
}

//! verify that the edge references can make a dimension. Currently only extent
//! dimensions support more than 2 edges
DimensionGeometry TechDraw::isValidMultiEdge(const ReferenceVector& refs)
{
    //there has to be at least 2
    if (refs.size() < 2) {
        return DimensionGeometry::isInvalid;
    }

    //they all must start with "Edge"
    const std::string matchToken{"Edge"};
    if (!refsMatchToken(refs, matchToken)) {
        return DimensionGeometry::isInvalid;
    }

    auto objFeat0(dynamic_cast<TechDraw::DrawViewPart*>(refs.at(0).getObject()));
    if (!objFeat0) {
        //probably redundant
        throw Base::RuntimeError("Logic error in isValidMultiEdge");
    }

    if (refs.size() > 2) {
        //many edges, must be an extent?
        return DimensionGeometry::isMultiEdge;
    }

    //exactly 2 edges. could be angle, could be distance
    int GeoId0(TechDraw::DrawUtil::getIndexFromName(refs.at(0).getSubName()));
    int GeoId1(TechDraw::DrawUtil::getIndexFromName(refs.at(1).getSubName()));
    TechDraw::BaseGeomPtr geom0 = objFeat0->getGeomByIndex(GeoId0);
    TechDraw::BaseGeomPtr geom1 = objFeat0->getGeomByIndex(GeoId1);

    if (geom0->getGeomType() == GeomType::GENERIC && geom1->getGeomType() == GeomType::GENERIC) {
        TechDraw::GenericPtr gen0 = std::static_pointer_cast<TechDraw::Generic>(geom0);
        TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
        if (gen0->points.size() > 2 || gen1->points.size() > 2) {//the edge is a polyline
            return DimensionGeometry::isInvalid;                                    //not supported yet
        }
        Base::Vector3d line0 = gen0->points.at(1) - gen0->points.at(0);
        line0.Normalize();
        Base::Vector3d line1 = gen1->points.at(1) - gen1->points.at(0);
        line1.Normalize();
        auto lineDot{line0.Dot(line1)};
        if (lineDot >= 1 ||
            lineDot <= -1) {
            // the edges are parallel
            return DimensionGeometry::isDiagonal;    //distance || line
        }
        return DimensionGeometry::isAngle;       //angle or distance
    }
    return DimensionGeometry::isDiagonal;  //two edges, not both straight lines
}

//! verify that the edge references can make a dimension. Currently only extent
//! dimensions support more than 2 edges
DimensionGeometry TechDraw::isValidMultiEdge3d(DrawViewPart* dvp, const ReferenceVector& refs)
{
    (void)dvp;
    //there has to be at least 2
    if (refs.size() < 2) {
        return DimensionGeometry::isInvalid;
    }

    //they all must start with "Edge"
    const std::string matchToken{"Edge"};
    if (!refsMatchToken(refs, matchToken)) {
        return DimensionGeometry::isInvalid;
    }

    std::vector<TopoDS_Edge> edges;
    for (auto& ref : refs) {
        std::vector<TopoDS_Shape> shapesAll = ShapeExtractor::getShapesFromObject(ref.getObject());
        if (shapesAll.empty()) {
            //reference has no geometry
            return DimensionGeometry::isInvalid;
        }
    }
    std::vector<TopoDS_Edge> edgesAll;
    std::vector<int> typeAll;
    for (auto& ref : refs) {
        TopoDS_Shape geometry = ref.getGeometry();
        if (geometry.ShapeType() != TopAbs_EDGE) {
            return DimensionGeometry::isInvalid;
        }
        TopoDS_Edge edge = TopoDS::Edge(geometry);
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() != GeomAbs_Line) {
            //not a line, so this must be an extent dim?
            return DimensionGeometry::isMultiEdge;
        }
        edgesAll.push_back(edge);
    }
    if (edgesAll.size() > 2) {
        //must be an extent dimension of lines?
        return DimensionGeometry::isMultiEdge;
    }

    if (edgesAll.size() == 2) {
        auto first0 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopExp::FirstVertex(edgesAll.at(0))));
        auto last0 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopExp::LastVertex(edgesAll.at(1))));
        Base::Vector3d line0 = last0 - first0;
        auto first1 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopExp::FirstVertex(edgesAll.at(0))));
        auto last1 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopExp::LastVertex(edgesAll.at(1))));
        Base::Vector3d line1 = last1 - first1;
        line0.Normalize();
        line1.Normalize();
        auto lineDot{std::fabs(line0.Dot(line1))};
        double localTolerance{std::numeric_limits<float>::epsilon()};  // this is as close as we can reliably measure
        if (DU::fpCompare(lineDot, 1, localTolerance)) {
            //lines are parallel, must be distance dim
            return DimensionGeometry::isDiagonal;
        }
        //lines are skew, could be angle, could be distance?
        return DimensionGeometry::isAngle;
    }

    return DimensionGeometry::isInvalid;
}

//! verify that the vertex references can make a dimension
DimensionGeometry TechDraw::isValidVertexes(const ReferenceVector& refs)
{
    auto* dvp(dynamic_cast<TechDraw::DrawViewPart*>(refs.front().getObject()));
    if (!dvp) {
        //probably redundant
        throw Base::RuntimeError("Logic error in isValidVertexes");
    }

    const std::string matchToken{"Vertex"};
    if (!refsMatchToken(refs, matchToken)) {
        return DimensionGeometry::isInvalid;
    }

    if (refs.size() == 2) {
        //2 vertices can only make a distance dimension
        TechDraw::VertexPtr v0 = dvp->getVertex(refs.at(0).getSubName());
        TechDraw::VertexPtr v1 = dvp->getVertex(refs.at(1).getSubName());
        Base::Vector3d line = v1->point() - v0->point();
        if (fabs(line.y) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isHorizontal;
        }

        if (fabs(line.x) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isVertical;
        }
        return DimensionGeometry::isDiagonal;

    } else if (refs.size() == 3) {
        //three vertices make an angle dimension
        return DimensionGeometry::isAngle3Pt;
    }

    // did not find a valid configuration
    return DimensionGeometry::isInvalid;
}

//! verify that the vertex references can make a dimension
DimensionGeometry TechDraw::isValidVertexes3d(DrawViewPart* dvp, const ReferenceVector& refs)
{
    (void)dvp;
    const std::string matchToken{"Vertex"};
    if (!refsMatchToken(refs, matchToken)) {
        return DimensionGeometry::isInvalid;
    }

    if (refs.size() == 2) {
        //2 vertices can only make a distance dimension
        TopoDS_Shape geometry0 = refs.at(0).getGeometry();
        TopoDS_Shape geometry1 = refs.at(1).getGeometry();
        if (geometry0.IsNull() || geometry1.IsNull() || geometry0.ShapeType() != TopAbs_VERTEX
            || geometry1.ShapeType() != TopAbs_VERTEX) {
            return DimensionGeometry::isInvalid;
        }
        auto point0 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopoDS::Vertex(geometry0)));
        point0 = dvp->projectPoint(point0);
        auto point1 = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopoDS::Vertex(geometry1)));
        point1 = dvp->projectPoint(point1);
        Base::Vector3d line = point1 - point0;
        if (fabs(line.y) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isVertical;
        }
        if (fabs(line.x) < std::numeric_limits<float>::epsilon()) {
            return DimensionGeometry::isHorizontal;
            //        } else if(fabs(line.z) < std::numeric_limits<float>::epsilon()) {
            //            return isZLimited;
        }
        return DimensionGeometry::isDiagonal;

    } else if (refs.size() == 3) {
        //three vertices make an angle dimension
        //we could check here that all the geometries are Vertex
        return DimensionGeometry::isAngle3Pt;
    }

    // did not find a valid configuration
    return DimensionGeometry::isInvalid;
}

//! verify that the mixed bag (ex Vertex-Edge) of references can make a dimension
DimensionGeometry TechDraw::isValidHybrid(const ReferenceVector& refs)
{
    if (refs.empty()) {
        return DimensionGeometry::isInvalid;
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
        //must be a diagonal dim? could it be DimensionGeometry::isHorizontal or isVertical?
        return DimensionGeometry::isHybrid;
    }

    return DimensionGeometry::isInvalid;
}

//! verify that the mixed bag (ex Vertex-Edge) of references can make a dimension
DimensionGeometry TechDraw::isValidHybrid3d(DrawViewPart* dvp, const ReferenceVector& refs)
{
    (void)dvp;
    //we can reuse the 2d check here.
    return isValidHybrid(refs);
}

//handle situations where revised geometry type is valid but not suitable for existing dimType
long int TechDraw::mapGeometryTypeToDimType(long int dimType, DimensionGeometry geometry2d,
                                            DimensionGeometry geometry3d)
{
    if (geometry2d == DimensionGeometry::isInvalid && geometry3d == DimensionGeometry::isInvalid) {
        //probably an error, but we can't do anything with this
        return dimType;
    }

    if (geometry2d == DimensionGeometry::isViewReference && geometry3d != DimensionGeometry::isInvalid) {
        switch (geometry3d) {
            case DimensionGeometry::isDiagonal:
                return DrawViewDimension::Distance;
            case DimensionGeometry::isHorizontal:
                return DrawViewDimension::DistanceX;
            case DimensionGeometry::isVertical:
                return DrawViewDimension::DistanceY;
            case DimensionGeometry::isAngle:
                return DrawViewDimension::Angle;
            case DimensionGeometry::isAngle3Pt:
                return DrawViewDimension::Angle3Pt;
            default:
                return dimType;
        }
    }

    if (geometry2d != DimensionGeometry::isViewReference) {
        switch (geometry2d) {
            case DimensionGeometry::isDiagonal:
                return DrawViewDimension::Distance;
            case DimensionGeometry::isHorizontal:
                return DrawViewDimension::DistanceX;
            case DimensionGeometry::isVertical:
                return DrawViewDimension::DistanceY;
            case DimensionGeometry::isAngle:
                return DrawViewDimension::Angle;
            case DimensionGeometry::isAngle3Pt:
                return DrawViewDimension::Angle3Pt;
            default:
                break;  // For all other cases, return dimType
        }
    }

    return dimType;
}

//! true if all the input references have subelements that match the geometry
//! type token.
bool  TechDraw::refsMatchToken(const ReferenceVector& refs, const std::string& matchToken)
{
    //NOLINTNEXTLINE
    for (auto& entry : refs) {
        std::string entryToken = DU::getGeomTypeFromName(entry.getSubName(false));
        if (entryToken != matchToken) {
            return false;
        }
    }
    return true;
}
