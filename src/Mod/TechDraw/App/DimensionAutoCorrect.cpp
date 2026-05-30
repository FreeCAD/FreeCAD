// SPDX-License-Identifier: LGPL-2.0-or-later
/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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
// a class to validate and correct dimension references

// the dimension reference auto correct algo:
//
// when a dimension is created, the shape of each reference is saved, so in addition
// to the dimensionedObject + subElement reference, we also keep a Part::TopoShape copy
// of the reference.
//
// when we later use that dimension, we check its references as follows:
// for each reference:
//    // auto correct phase 1
//    if ref.currentGeometry == ref.savedGeometry:
//        // the reference points to the same geometry as before, so we
//        // so we consider this to be correct. same geometry, same index case.
//        continue
//    else:
//        // search all the source shapes for a subelement with the exact same
//        // geometry.  same geometry, different index case.
//        newRef = searchForExactSameGeometry(ref)  // geometry matcher
//        if newRef:
//            // substitute the reference we just found in place of the old
//            // reference
//            replace(ref, newRef)
//        else:
//            // auto correct phase 2 - to be implemented
//            // we don't have any geometry that is identical to our saved geometry.
//            // finding a match now becomes guess work.  we have to find the most
//            // similar geometry (with at least some level of same-ness) and use
//            // that to rebuild our reference.
//            // we do not have a good algo for searchForMostSimilarGeometry() yet.
//            newRef = searchForMostSimilarGeometry(ref) // geometry guesser
//            if newRef:
//                replace(ref, newRef)
//            else:
//                //we can't fix this


#include <algorithm>
#include <cmath>
#include <limits>

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Measure/App/ShapeFinder.h>

#include "GeometryMatcher.h"
#include "DimensionReferences.h"
#include "DimensionGeometry.h"
#include "DimensionAutoCorrect.h"
#include "DrawUtil.h"
#include "Preferences.h"

using namespace TechDraw;
using namespace Measure;
using DU = DrawUtil;

//! true if references point to valid geometry and the valid geometry matches the
//! corresponding saved geometry.  this method does not correct anything, it just
//! verifies if the references point to the same geometry as when the reference
//! was created.
bool DimensionAutoCorrect::referencesHaveValidGeometry(std::vector<bool>& referenceState) const
{
    // Base::Console().message("DAC::referencesHaveValidGeometry()\n");
    ReferenceVector refsAll = getDimension()->getEffectiveReferences();
    const std::vector<Part::TopoShape> savedGeometry = getDimension()->SavedGeometry.getValues();

    if (savedGeometry.empty() || savedGeometry.size() != refsAll.size()) {
        // this must be an old document without savedGeometry property.  We can not
        // validate the references in this case so we hope they are valid.
        referenceState = std::vector<bool>(refsAll.size(), true);
        return true;
    }

    bool result {true};
    size_t iRef {0};
    for (auto& entry : refsAll) {
        if (entry.hasGeometry()) {
            // entry points to something, is it the correct geom?
            if (isMatchingGeometry(entry, savedGeometry.at(iRef))) {
                referenceState.emplace_back(true);
            }
            else {
                result = false;
                referenceState.emplace_back(false);
            }
        }
        else {
            result = false;
            referenceState.emplace_back(false);
        }
        iRef++;
    }
    return result;
}

//! try to correct references that point to non-existent geometry or when the saved
//! geometry does not match the current geometry the reference points to.
//! referenceState is the output of a previous use of referencesHaveValidGeometry.
bool DimensionAutoCorrect::autocorrectReferences(std::vector<bool>& referenceState,
                                                 ReferenceVector& repairedRefs) const
{
    // Base::Console().message("DAC::autocorrectReferences()\n");
    if (!Preferences::autoCorrectDimRefs()) {
        return false;
    }

    bool result {true};
    ReferenceVector refsAll = getDimension()->getEffectiveReferences();
    const std::vector<Part::TopoShape> savedGeometry = getDimension()->SavedGeometry.getValues();
    if (savedGeometry.empty() || savedGeometry.size() != refsAll.size()) {
        // this must be an old document without savedGeometry property.  We can not
        // validate the references in this case so we have to assume the references are
        // correct.
        return true;
    }

    size_t iRef {0};
    for (const auto& state : referenceState) {
        if (state) {
            // ref points to valid geometry that matches saved geometry
            referenceState.at(iRef) = true;
            repairedRefs.push_back(refsAll.at(iRef));
            iRef++;
            continue;
        }

        const Part::TopoShape& temp = savedGeometry.at(iRef);
        if (temp.isNull()) {
            result = false;
            referenceState.at(iRef) = false;
            repairedRefs.push_back(refsAll.at(iRef));
            iRef++;
            // we could exit here instead of checking all the refs?
            continue;
        }

        // this ref does not point to valid geometry or
        // the geometry it points to does not match the saved geometry
        ReferenceEntry fixedRef = refsAll.at(iRef);

        // first, look for an exact match to the saved geometry
        bool success = fix1GeomExact(fixedRef, savedGeometry.at(iRef).getShape());
        if (success) {
            // we did find a match
            referenceState.at(iRef) = true;
            repairedRefs.push_back(fixedRef);
            iRef++;
            continue;
        }

        // we did not find an exact match, so check for a similar match
        success = fix1GeomSimilar(fixedRef, savedGeometry.at(iRef).getShape());
        if (success) {
            // we did find a similar match
            referenceState.at(iRef) = true;
            repairedRefs.push_back(fixedRef);
            iRef++;
            continue;
        }

        // we did not find a similar match the geometry
        result = false;
        referenceState.at(iRef) = false;
        repairedRefs.push_back(fixedRef);
        iRef++;
        // we could exit here
    }

    return result;
}

//! fix a single reference with an exact match to geomToMatch
bool DimensionAutoCorrect::fix1GeomExact(ReferenceEntry& refToFix, const TopoDS_Shape &geomToMatch) const
{
    // Base::Console().message("DAC::fix1GeomExact()\n");
    ReferenceEntry fixedRef = refToFix;
    Part::TopoShape topoShapeToMatch(geomToMatch);
    bool success {false};
    if (refToFix.is3d()) {
        if (!refToFix.getObject() && m_3dObjectCache.empty()) {
            return false;
        }
        if (geomToMatch.ShapeType() == TopAbs_VERTEX) {
            success = findExactVertex3d(refToFix, topoShapeToMatch);
        }
        else {
            success = findExactEdge3d(refToFix, topoShapeToMatch);
        }
    }
    else {
        if (geomToMatch.ShapeType() == TopAbs_VERTEX) {
            success = findExactVertex2d(refToFix, topoShapeToMatch);
        }
        else {
            success = findExactEdge2d(refToFix, topoShapeToMatch);
        }
    }
    return success;
}


//! fix a single reference with an Similar match to geomToMatch
bool DimensionAutoCorrect::fix1GeomSimilar(ReferenceEntry& refToFix, const TopoDS_Shape &geomToMatch) const
{
    // Base::Console().message("DAC::fix1GeomSimilar()\n");
    Part::TopoShape topoShapeToMatch(geomToMatch);
    bool success {false};
    if (refToFix.is3d()) {
        if (!refToFix.getObject() && m_3dObjectCache.empty()) {
            // can't fix this. nothing to compare.
            return false;
        }
        if (geomToMatch.ShapeType() == TopAbs_VERTEX) {
            success = findSimilarVertex3d(refToFix, topoShapeToMatch);
        }
        else {
            success = findSimilarEdge3d(refToFix, topoShapeToMatch);
        }
    }
    else {
        if (geomToMatch.ShapeType() == TopAbs_VERTEX) {
            success = findSimilarVertex2d(refToFix, topoShapeToMatch);
        }
        else {
            success = findSimilarEdge2d(refToFix, topoShapeToMatch);
        }
    }
    return success;
}


//! search the view for a 2d vertex that is the same as the saved reference geometry
//! and return a reference pointing to the matching vertex
bool DimensionAutoCorrect::findExactVertex2d(ReferenceEntry& refToFix,
                                             const Part::TopoShape& refGeom) const
{
    // Base::Console().message("DAC::findExactVertex2d()\n");
    getMatcher()->setPointTolerance(EWTOLERANCE);
    auto refObj = refToFix.getObject();
    auto refDvp = dynamic_cast<TechDraw::DrawViewPart*>(refObj);
    if (refDvp) {
        ReferenceEntry fixedRef = searchViewForVert(refDvp, refGeom);
        if (fixedRef.getObject()) {
            refToFix = fixedRef;
            return true;
        }
    }
    // no match
    return false;
}


//! search the view for a 2d edge that is the same as the saved reference geometry (from DVD::SavedGeometry)
//! and return a reference pointing to the matching edge.
bool DimensionAutoCorrect::findExactEdge2d(ReferenceEntry& refToFix, const Part::TopoShape& refGeom) const
{
    // Base::Console().message("DAC::findExactEdge2d()\n");
    auto refObj = refToFix.getObject();
    auto refDvp = dynamic_cast<TechDraw::DrawViewPart*>(refObj);
    if (refDvp) {
        ReferenceEntry fixedRef = searchViewForExactEdge(refDvp, refGeom);
        if (fixedRef.getObject()) {
            refToFix = fixedRef;
            return true;
        }
    }

    // no match, return the input reference
    return false;
}

//! search the model for a 3d vertex that is the same as the saved reference geometry
//! and return a reference pointing to the matching vertex
bool DimensionAutoCorrect::findExactVertex3d(ReferenceEntry& refToFix,
                                             const Part::TopoShape& refGeom) const
{
    // Base::Console().message("DAC::findExactVertex3d()\n");
    getMatcher()->setPointTolerance(EWTOLERANCE);

    // try the referenced object
    auto refObj = refToFix.getObject();
    if (refObj) {
        ReferenceEntry fixedRef = searchObjForVert(refObj, refGeom);
        if (fixedRef.getObject()) {
            refToFix = fixedRef;
            return true;
        }
    }

    // not match in refObj (or no refObj!)
    for (auto& objectName : m_3dObjectCache) {
        auto object3d = getDimension()->getDocument()->getObject(objectName.c_str());
        ReferenceEntry fixedRef = searchObjForVert(object3d, refGeom);
        if (fixedRef.getObject()) {
            refToFix = fixedRef;
            return true;
        }
    }

    return false;
}

//! search the model for a 3d edge that is the same as the saved reference geometry
//! and return a reference pointing to the matching edge.
bool DimensionAutoCorrect::findExactEdge3d(ReferenceEntry& refToFix, const Part::TopoShape& refGeom) const
{
    // Base::Console().message("DAC::findExactEdge3d() - cache: %d\n", m_3dObjectCache.size());
    // first, try to find a match in the referenced object
    auto refObj = refToFix.getObject();
    if (refObj) {
        ReferenceEntry fixedRef = searchObjForEdge(refObj, refGeom);
        if (fixedRef.getObject()) {
            refToFix = fixedRef;
            return true;
        }
    }

    // no match in refObj, so now search the cached objects
    for (auto& objectName : m_3dObjectCache) {
        auto object3d = getDimension()->getDocument()->getObject(objectName.c_str());
        auto shape3d = Part::Feature::getShape(object3d, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        auto edgesAll = getDimension()->getEdges(shape3d);
        size_t iEdge {1};
        for (auto& edge : edgesAll) {
            if (getMatcher()->compareGeometry(edge, refGeom)) {
                // found a match!
                refToFix.setObjectName(objectName);
                refToFix.setObject(object3d);
                refToFix.setSubName(std::string("Edge") + std::to_string(iEdge));
                return true;
            }
            iEdge++;
        }
    }
    return false;
}

//! search the view for a vertex that is within a tolerance of the saved reference geometry
//! and return a reference pointing to the matching vertex
bool DimensionAutoCorrect::findSimilarVertex2d(ReferenceEntry& refToFix,
                                               const Part::TopoShape& refGeom) const
{
    auto refDvp = dynamic_cast<TechDraw::DrawViewPart*>(refToFix.getObject());
    if (!refDvp) {
        return false;
    }
    ReferenceEntry fixed = searchViewForSimilarVert(refDvp, refGeom);
    if (fixed.getObject()) {
        refToFix = fixed;
        return true;
    }
    return false;
}

//! search the view for a 2d edge that is similar to the saved reference geometry
//! and return a reference pointing to the similar edge.
bool DimensionAutoCorrect::findSimilarEdge2d(ReferenceEntry& refToFix,
                                             const Part::TopoShape& refGeom) const
{
    auto refDvp = dynamic_cast<TechDraw::DrawViewPart*>(refToFix.getObject());
    if (!refDvp) {
        return false;
    }
    ReferenceEntry fixed = searchViewForSimilarEdge(refDvp, refGeom);
    if (fixed.getObject()) {
        refToFix = fixed;
        return true;
    }
    return false;
}

//! search the referenced 3d object and the object cache for a vertex that is within
//! a tolerance of the saved reference geometry and return a reference pointing
//! to the matching vertex
bool DimensionAutoCorrect::findSimilarVertex3d(ReferenceEntry& refToFix,
                                               const Part::TopoShape& refGeom) const
{
    // Base::Console().message("DAC::findSimilarVertex3d()\n");
    (void)refToFix;
    (void)refGeom;
    // Base::Console().message("DAC::findSimilarVertex3d is not implemented yet\n");
    return false;
}

//! search the referenced 3d object and the object cache for an edge that is
//! similar to the saved reference geometry and return a reference pointing
//! to the similar edge
bool DimensionAutoCorrect::findSimilarEdge3d(ReferenceEntry& refToFix,
                                             const Part::TopoShape& refGeom) const
{
    // Base::Console().message("DAC::findSimilarEdge3d(%s)\n", refToFix.getObjectName().c_str());
    (void)refToFix;
    (void)refGeom;
    // Base::Console().message("DAC::findSimilarEdge3d is not implemented yet\n");
    return false;
}

//! compare the geometry pointed to by a reference to the corresponding saved geometry
bool DimensionAutoCorrect::isMatchingGeometry(const ReferenceEntry& ref,
                                              const Part::TopoShape& savedGeometry) const
{
    Part::TopoShape temp;
    if (ref.is3d()) {
        auto shape3d = ShapeFinder::getLocatedShape(*ref.getObject(), ref.getSubName(true));
        temp = Part::TopoShape(shape3d);
    } else {
        auto shape2d = ref.getGeometry();
        temp = Part::TopoShape(shape2d);
    }


    if (temp.isNull()) {
        // this shouldn't happen as we already know that this ref points to valid geometry
        return false;
    }
    if (getMatcher()->compareGeometry(temp, savedGeometry)) {
        // reference still points to the same geometry
        return true;
    }

    return false;
}

//! search obj (3d object with a shape) for a match to refVertex.  This is always
//! an exact match for phase 1 (GeometryMatcher), but in phase 2 (GeometryGuesser)
//! a similar match will be allowed.
ReferenceEntry DimensionAutoCorrect::searchObjForVert(App::DocumentObject* obj,
                                                      const Part::TopoShape& refVertex,
                                                      bool exact) const
{
    (void)exact;
    auto shape3d = ShapeFinder::getLocatedShape(*obj, "");
    if (shape3d.IsNull()) {
        // how to handle this?
        return {};
    }
    auto vertsAll = getDimension()->getVertexes(shape3d);
    size_t iVert {1};
    for (auto& vert : vertsAll) {
        bool isSame = getMatcher()->compareGeometry(refVertex, vert);
        if (isSame) {
            auto newSubname = std::string("Vertex") + std::to_string(iVert);
            return {obj, newSubname, getDimension()->getDocument()};
        }
        iVert++;
    }
    return {};
}


//! search View (2d part display) for a match to refVertex.  This can be an
//! exact or Similar match depending on the setting of exact.
ReferenceEntry DimensionAutoCorrect::searchViewForVert(DrawViewPart* obj,
                                                       const Part::TopoShape& refVertex,
                                                       bool exact) const
{
    // Base::Console().message("DAC::searchViewForVert()\n");
    (void)exact;
    std::vector<TechDraw::VertexPtr> gVertexAll =
        getDimension()->getViewPart()->getVertexGeometry();
    getMatcher()->setPointTolerance(EWTOLERANCE);
    int iVertex = 0;
    for (auto& vert : gVertexAll) {
        // vert is in display form - scaled and rotated
        Part::TopoShape temp = ReferenceEntry::asCanonicalTopoShape(vert->asTopoShape(), *obj);
        bool isSame = getMatcher()->compareGeometry(temp, refVertex);
        if (isSame) {
            auto newSubname = std::string("Vertex") + std::to_string(iVertex);
            return {obj, newSubname, getDimension()->getDocument()};
        }
        iVertex++;
    }
    return {};
}

//! search View (2d part display) for an exact match to refEdge.
ReferenceEntry DimensionAutoCorrect::searchViewForExactEdge(DrawViewPart* obj,
                                                            const Part::TopoShape& refEdge) const
{
    // Base::Console().message("DAC::searchViewForExactEdge()\n");
    auto gEdgeAll = getDimension()->getViewPart()->getEdgeGeometry();
    int iEdge {0};
    for (auto& edge : gEdgeAll) {
        // edge is scaled and rotated. we need it in the same scale/rotate state as
        // the reference edge in order to match.
        Part::TopoShape temp = ReferenceEntry::asCanonicalTopoShape(edge->asTopoShape(), *obj);
        bool isSame = getMatcher()->compareGeometry(refEdge, temp);
        if (isSame) {
            auto newSubname = std::string("Edge") + std::to_string(iEdge);
            return {obj, newSubname, getDimension()->getDocument()};
        }
        iEdge++;
    }
    return {};
}


//! search View (2d part display) for an edge whose endpoints, length and curve
//! type match refEdge within a tight tolerance. Phase-2 fallback for stale 2D
//! references; rejects on any ambiguity rather than risk a wrong rebind (#19871).
ReferenceEntry DimensionAutoCorrect::searchViewForSimilarEdge(DrawViewPart* obj,
                                                              const Part::TopoShape& refEdge) const
{
    if (!obj || refEdge.isNull()) {
        return {};
    }
    const TopoDS_Shape& refShape = refEdge.getShape();
    if (refShape.IsNull() || refShape.ShapeType() != TopAbs_EDGE) {
        return {};
    }
    // The projected reference edge can be degenerate; OCCT raises Standard_Failure
    // (not a std::exception) which would otherwise escape recompute (#19871).
    GeomAbs_CurveType refType = GeomAbs_OtherCurve;
    double refLength = 0.0;
    gp_Pnt refP0;
    gp_Pnt refP1;
    try {
        TopoDS_Edge refOcc = TopoDS::Edge(refShape);
        BRepAdaptor_Curve refAdapt(refOcc);
        refType = refAdapt.GetType();
        refLength = GCPnts_AbscissaPoint::Length(refAdapt);
        refP0 = refAdapt.Value(refAdapt.FirstParameter());
        refP1 = refAdapt.Value(refAdapt.LastParameter());
    }
    catch (const Standard_Failure&) {
        return {};
    }

    // Match on BOTH endpoints (orientation-independent) plus length, not just the
    // midpoint: an edge with a similar midpoint but different endpoints is a
    // different edge, and binding to it yields a confidently wrong dimension.
    int bestIdx = -1;
    double bestScore = std::numeric_limits<double>::max();
    double runnerUp = std::numeric_limits<double>::max();

    const auto edgesAll = obj->getEdgeGeometry();
    for (size_t i = 0; i < edgesAll.size(); ++i) {
        const auto& bg = edgesAll[i];
        if (!bg) {
            continue;
        }
        Part::TopoShape canon = ReferenceEntry::asCanonicalTopoShape(bg->asTopoShape(), *obj);
        const TopoDS_Shape& cShape = canon.getShape();
        if (cShape.IsNull() || cShape.ShapeType() != TopAbs_EDGE) {
            continue;
        }
        double endpointDist = 0.0;
        try {
            TopoDS_Edge cEdge = TopoDS::Edge(cShape);
            BRepAdaptor_Curve cAdapt(cEdge);
            if (cAdapt.GetType() != refType) {
                continue;
            }
            double cLength = GCPnts_AbscissaPoint::Length(cAdapt);
            double lenMax = std::max(refLength, cLength);
            if (lenMax > 1.0e-7 && std::fabs(refLength - cLength) / lenMax > 0.05) {
                continue;  // length differs by more than 5 % -- not the same edge
            }
            gp_Pnt cP0 = cAdapt.Value(cAdapt.FirstParameter());
            gp_Pnt cP1 = cAdapt.Value(cAdapt.LastParameter());
            // orientation can flip between projections; take the better pairing.
            double sameDir = refP0.Distance(cP0) + refP1.Distance(cP1);
            double flipDir = refP0.Distance(cP1) + refP1.Distance(cP0);
            endpointDist = std::min(sameDir, flipDir);
        }
        catch (const Standard_Failure&) {
            continue;
        }
        if (endpointDist < bestScore) {
            runnerUp = bestScore;
            bestScore = endpointDist;
            bestIdx = static_cast<int>(i);
        }
        else if (endpointDist < runnerUp) {
            runnerUp = endpointDist;
        }
    }

    // Tight accept window: both endpoints must land within ~0.5 mm (model space)
    // of the saved edge, summed, and the best candidate must be clearly better
    // than the runner-up. Otherwise refuse to rebind.
    const double acceptThreshold = 1.0;   // mm, summed over both endpoints
    const double ambiguityMargin = 0.5;   // best must be <= 50 % of runner-up
    if (bestIdx < 0 || bestScore > acceptThreshold) {
        return {};
    }
    if (runnerUp != std::numeric_limits<double>::max()
        && bestScore > (1.0 - ambiguityMargin) * runnerUp) {
        return {};
    }
    auto newSubname = std::string("Edge") + std::to_string(bestIdx);
    return {obj, newSubname, getDimension()->getDocument()};
}

//! search View (2d part display) for a vertex within an accept-threshold of
//! refVertex. Companion to searchViewForSimilarEdge for the phase-2 fallback.
ReferenceEntry DimensionAutoCorrect::searchViewForSimilarVert(DrawViewPart* obj,
                                                              const Part::TopoShape& refVertex) const
{
    if (!obj || refVertex.isNull()) {
        return {};
    }
    const TopoDS_Shape& refShape = refVertex.getShape();
    if (refShape.IsNull() || refShape.ShapeType() != TopAbs_VERTEX) {
        return {};
    }
    gp_Pnt refPnt;
    try {
        refPnt = BRep_Tool::Pnt(TopoDS::Vertex(refShape));
    }
    catch (const Standard_Failure&) {
        return {};
    }

    int bestIdx = -1;
    double bestDist = std::numeric_limits<double>::max();
    double runnerUp = std::numeric_limits<double>::max();

    const auto vertsAll = obj->getVertexGeometry();
    for (size_t i = 0; i < vertsAll.size(); ++i) {
        if (!vertsAll[i]) {
            continue;
        }
        Part::TopoShape canon =
            ReferenceEntry::asCanonicalTopoShape(vertsAll[i]->asTopoShape(), *obj);
        const TopoDS_Shape& cShape = canon.getShape();
        if (cShape.IsNull() || cShape.ShapeType() != TopAbs_VERTEX) {
            continue;
        }
        gp_Pnt cPnt;
        try {
            cPnt = BRep_Tool::Pnt(TopoDS::Vertex(cShape));
        }
        catch (const Standard_Failure&) {
            continue;
        }
        double d = refPnt.Distance(cPnt);
        if (d < bestDist) {
            runnerUp = bestDist;
            bestDist = d;
            bestIdx = static_cast<int>(i);
        }
        else if (d < runnerUp) {
            runnerUp = d;
        }
    }

    const double acceptThreshold = 0.5;   // mm in canonical view space
    const double ambiguityMargin = 0.5;
    if (bestIdx < 0 || bestDist > acceptThreshold) {
        return {};
    }
    if (runnerUp != std::numeric_limits<double>::max()
        && bestDist > (1.0 - ambiguityMargin) * runnerUp) {
        return {};
    }
    auto newSubname = std::string("Vertex") + std::to_string(bestIdx);
    return {obj, newSubname, getDimension()->getDocument()};
}

//! search model for for a 3d edge that is a match to refEdge
//! note that only the exact match is implemented in phase 1
ReferenceEntry DimensionAutoCorrect::searchObjForEdge(App::DocumentObject* obj,
                                                      const Part::TopoShape& refEdge,
                                                      bool exact) const
{
    // Base::Console().message("DAC::searchObjForEdge(%s)\n", obj->Label.getValue());
    (void)exact;
    auto shape3d = Part::Feature::getShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    if (shape3d.IsNull()) {
        // how to handle this?
        return {};
    }
    auto edgesAll = getDimension()->getEdges(shape3d);
    size_t iEdge {1};
    for (auto& edge : edgesAll) {
        bool isSame = getMatcher()->compareGeometry(edge, refEdge);
        if (isSame) {
            auto newSubname = std::string("Edge") + std::to_string(iEdge);
            return {obj, newSubname, getDimension()->getDocument()};
        }
        iEdge++;
    }
    return {};
}

//! rebuild 3d references from saved geometry.  returns true is all references
//! have been repaired
bool DimensionAutoCorrect::fixBrokenReferences(ReferenceVector& fixedReferences) const
{
    // Base::Console().message("DAC::fixBrokenReferences()\n");
    bool success {true};
    const std::vector<Part::TopoShape> savedGeometry = getDimension()->SavedGeometry.getValues();
    int iGeom {0};
    for (auto& geom : savedGeometry) {
        if (fixedReferences.at(iGeom).hasGeometry()) {
            iGeom++;
            continue;
        }
        //
        const TopoDS_Shape& geomShape = geom.getShape();
        for (auto& objectName : m_3dObjectCache) {
            auto object3d = getDimension()->getDocument()->getObject(objectName.c_str());
            if (!object3d) {
                // cached object has been deleted
                continue;
            }
            // TODO: do we need to check for Similar matches here too?
            ReferenceEntry newRef;
            if (geomShape.ShapeType() == TopAbs_VERTEX) {
                newRef = searchObjForVert(object3d, geomShape);
                fixedReferences.at(iGeom) = newRef;
            }
            else {
                newRef = searchObjForEdge(object3d, geomShape);
                fixedReferences.at(iGeom) = newRef;
            }
            fixedReferences.at(iGeom) = newRef;
            if (!newRef.getObject()) {
                success = false;
            }
        }
    }
    return success;
}


GeometryMatcher* DimensionAutoCorrect::getMatcher() const
{
    return getDimension()->getMatcher();
}

