// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <algorithm>
#include <cmath>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>

#include <memory>

#include "GeoEnum.h"
#include "SketchObject.h"


#undef DEBUG
// #define DEBUG

// clang-format off
using namespace Sketcher;
using namespace Base;

int SketchObject::moveGeometries(const std::vector<GeoElementId>& geoEltIds, const Base::Vector3d& toPoint, bool relative,
                            bool updateGeoBeforeMoving)
{

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // if we are moving a point at SketchObject level, we need to start from a solved sketch
    // if we have conflicts we can forget about moving. However, there is the possibility that we
    // need to do programmatically moves of new geometry that has not been solved yet and that
    // because they were programmatically generated won't generate a conflict. This is the case of
    // Fillet for example. This is why exceptionally, it may be required to update the sketch
    // geometry to that of of SketchObject upon moving. => use updateGeometry parameter = true then


    if (updateGeoBeforeMoving || solverNeedsUpdate) {
        lastDoF = solvedSketch.setUpSketch(
            getCompleteGeometry(), Constraints.getValues(), getExternalGeometryCount());

        retrieveSolverDiagnostics();

        solverNeedsUpdate = false;
    }

    if (lastDoF < 0)// over-constrained sketch
        return -1;
    if (lastHasConflict)// conflicting constraints
        return -1;

    // move the point and solve
    lastSolverStatus = solvedSketch.moveGeometries(geoEltIds, toPoint, relative);

    // moving the point can not result in a conflict that we did not have
    // or a redundancy that we did not have before, or a change of DoF

    if (lastSolverStatus == 0) {
        std::vector<Part::Geometry*> geomlist = solvedSketch.extractGeometry();
        Geometry.setValues(geomlist);
        for (auto* geo :  geomlist) {
            if (geo){
                delete geo;
            }
        }
    }

    solvedSketch.resetInitMove();// reset solver point moving mechanism

    return lastSolverStatus;
}

int SketchObject::moveGeometry(int geoId, PointPos pos, const Base::Vector3d& toPoint, bool relative,
    bool updateGeoBeforeMoving)
{
    std::vector<GeoElementId> geoEltIds = { GeoElementId(geoId, pos) };
    return moveGeometries(geoEltIds, toPoint, relative, updateGeoBeforeMoving);
}

int SketchObject::delGeometries(const std::vector<int>& GeoIds, DeleteOptions options)
{
    return delGeometries(GeoIds.begin(), GeoIds.end(), options);
}

template <class InputIt>
int SketchObject::delGeometries(InputIt first, InputIt last, DeleteOptions options)
{
    std::vector<int> sGeoIds;
    std::vector<int> negativeGeoIds;

    // Separate GeoIds into negative (external) and non-negative GeoIds
    for (auto it = first; it != last; ++it) {
        int geoId = *it;
        if (geoId < 0 && geoId <= GeoEnum::RefExt) {
            negativeGeoIds.push_back(geoId);
        }
        else if (geoId >= 0){
            sGeoIds.push_back(geoId);
        }
    }

    // Handle negative GeoIds by calling delExternal
    if (!negativeGeoIds.empty()) {
        int result = delExternal(negativeGeoIds);
        if (result != 0) {
            return result; // Return if deletion of external geometries failed
        }
    }

    // Proceed with non-negative GeoIds
    if (sGeoIds.empty()) {
        return 0; // No positive GeoIds to delete
    }

    // if a GeoId has internal geometry, it must delete internal geometries too
    for (auto c : Constraints.getValues()) {
        if (c->Type == InternalAlignment) {
            auto pos = std::ranges::find(sGeoIds, c->Second);

            if (pos != sGeoIds.end()) {
                sGeoIds.push_back(c->First);
            }
        }
    }

    std::ranges::sort(sGeoIds);
    // eliminate duplicates
    auto newend = std::unique(sGeoIds.begin(), sGeoIds.end());
    sGeoIds.resize(std::distance(sGeoIds.begin(), newend));

    return delGeometriesExclusiveList(sGeoIds, options);
}

void SketchObject::replaceGeometries(std::vector<int> oldGeoIds,
                                     std::vector<Part::Geometry*>& newGeos)
{
    auto& vals = getInternalGeometry();
    auto newVals(vals);

    if (std::ranges::any_of(oldGeoIds, [](auto geoId) {
            return geoId < 0;
        })) {
        THROWM(ValueError, "Cannot replace external geometries and axes.");
    }

    auto oldGeoIdIter = oldGeoIds.begin();
    auto newGeoIter = newGeos.begin();

    for (; oldGeoIdIter != oldGeoIds.end() && newGeoIter != newGeos.end();
         ++oldGeoIdIter, ++newGeoIter) {
        GeometryFacade::copyId(getGeometry(*oldGeoIdIter), *newGeoIter);
        newVals[*oldGeoIdIter] = *newGeoIter;
    }

    if (newGeoIter != newGeos.end()) {
        for (; newGeoIter != newGeos.end(); ++newGeoIter) {
            generateId(*newGeoIter);
            newVals.push_back(*newGeoIter);
        }
    }
    else {
        delGeometries(oldGeoIdIter, oldGeoIds.end());
    }

    Geometry.setValues(std::move(newVals));
}
// clang-format off

std::vector<int> SketchObject::chooseFilletsEdges(const std::vector<int>& GeoIdList) const
{
    if (GeoIdList.size() == 2) {
        return GeoIdList;
    }

    std::vector<int> dst;
    for (auto id : GeoIdList) {
        if (!GeometryFacade::getFacade(getGeometry(id))->getConstruction()) {
            dst.push_back(id);

            if (dst.size() > 2) {
                return {};
            }
        }
    }
    return dst;
}
int SketchObject::fillet(int GeoId, PointPos PosId, double radius, bool trim, bool createCorner, bool chamfer)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    // Find the other geometry Id associated with the coincident point
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);

    GeoIdList = chooseFilletsEdges(GeoIdList);

    // only coincident points between two (non-external) edges can be filleted
    if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
        const Part::Geometry* geo1 = getGeometry(GeoIdList[0]);
        const Part::Geometry* geo2 = getGeometry(GeoIdList[1]);
        if (geo1->is<Part::GeomLineSegment>()
            && geo2->is<Part::GeomLineSegment>()) {
            auto* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
            auto* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

            Base::Vector3d midPnt1 = (lineSeg1->getStartPoint() + lineSeg1->getEndPoint()) / 2;
            Base::Vector3d midPnt2 = (lineSeg2->getStartPoint() + lineSeg2->getEndPoint()) / 2;
            return fillet(GeoIdList[0], GeoIdList[1], midPnt1, midPnt2, radius, trim, createCorner, chamfer);
        }
    }

    return -1;
}

int SketchObject::fillet(int GeoId1, int GeoId2, const Base::Vector3d& refPnt1,
                         const Base::Vector3d& refPnt2, double radius, bool trim, bool createCorner, bool chamfer)
{
    if (GeoId1 < 0 || GeoId1 > getHighestCurveIndex() || GeoId2 < 0 || GeoId2 > getHighestCurveIndex()) {
        return -1;
    }

    // If either of the two input lines are locked, don't try to trim since it won't work anyway
    const Part::Geometry* geo1 = getGeometry(GeoId1);
    const Part::Geometry* geo2 = getGeometry(GeoId2);
    if (trim && (GeometryFacade::getBlocked(geo1) || GeometryFacade::getBlocked(geo2))) {
        trim = false;
    }

    int pos1 = 0;
    int pos2 = 0;
    bool reverse = false;
    std::unique_ptr<Part::GeomArcOfCircle> arc(createFilletGeometry(geo1, geo2, refPnt1, refPnt2, radius, pos1, pos2, reverse));
    if (!arc) {
        return -1;
    }

    int filletId = addGeometry(arc.get());
    if (filletId < 0) {
        return -1;
    }

    PointPos PosId1 = static_cast<PointPos>(pos1);
    PointPos PosId2= static_cast<PointPos>(pos2);
    PointPos filletPosId1 = PointPos::none;
    PointPos filletPosId2 = PointPos::none;

    Base::Vector3d p1 = arc->getStartPoint(true);
    Base::Vector3d p2 = arc->getEndPoint(true);

    if (trim) {
        if (createCorner && geo1->is<Part::GeomLineSegment>() && geo2->is<Part::GeomLineSegment>()) {
            transferFilletConstraints(GeoId1, PosId1, GeoId2, PosId2);
        }
        else {
            delConstraintOnPoint(GeoId1, PosId1, false);
            delConstraintOnPoint(GeoId2, PosId2, false);
        }

        if (reverse) {
            filletPosId1 = PointPos::start;
            filletPosId2 = PointPos::end;
            moveGeometry(GeoId1, PosId1, p1, false, true);
            moveGeometry(GeoId2, PosId2, p2, false, true);
        }
        else {
            filletPosId1 = PointPos::end;
            filletPosId2 = PointPos::start;
            moveGeometry(GeoId1, PosId1, p2, false, true);
            moveGeometry(GeoId2, PosId2, p1, false, true);
        }

        auto tangent1 = std::make_unique<Sketcher::Constraint>();
        auto tangent2 = std::make_unique<Sketcher::Constraint>();

        tangent1->Type = Sketcher::Tangent;
        tangent1->First = GeoId1;
        tangent1->FirstPos = PosId1;
        tangent1->Second = filletId;
        tangent1->SecondPos = filletPosId1;

        tangent2->Type = Sketcher::Tangent;
        tangent2->First = GeoId2;
        tangent2->FirstPos = PosId2;
        tangent2->Second = filletId;
        tangent2->SecondPos = filletPosId2;

        addConstraint(std::move(tangent1));
        addConstraint(std::move(tangent2));
    }

    if (chamfer) {
        auto line = std::make_unique<Part::GeomLineSegment>();
        line->setPoints(p1, p2);
        int lineGeoId = addGeometry(line.get());


        auto coinc1 = std::make_unique<Sketcher::Constraint>();
        auto coinc2 = std::make_unique<Sketcher::Constraint>();

        coinc1->Type = Sketcher::Coincident;
        coinc1->First = lineGeoId;
        coinc1->FirstPos = filletPosId1;

        coinc2->Type = Sketcher::Coincident;
        coinc2->First = lineGeoId;
        coinc2->FirstPos = filletPosId2;

        if (trim) {
            coinc1->Second = GeoId1;
            coinc1->SecondPos = PosId1;
            coinc2->Second = GeoId2;
            coinc2->SecondPos = PosId2;
        }
        else {
            coinc1->Second = filletId;
            coinc1->SecondPos = PointPos::start;
            coinc2->Second = filletId;
            coinc2->SecondPos = PointPos::end;
        }

        addConstraint(std::move(coinc1));
        addConstraint(std::move(coinc2));

        setConstruction(filletId, true);
    }

    // if we do not have a recompute after the geometry creation, the sketch must be solved to
    // update the DoF of the solver
    if (noRecomputes) {
        solve();
    }

    return 0;
}

int SketchObject::extend(int GeoId, double increment, PointPos endpoint)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const std::vector<Part::Geometry*>& geomList = getInternalGeometry();
    Part::Geometry* geom = geomList[GeoId];
    int retcode = -1;
    if (geom->is<Part::GeomLineSegment>()) {
        Part::GeomLineSegment* seg = static_cast<Part::GeomLineSegment*>(geom);
        Base::Vector3d startVec = seg->getStartPoint();
        Base::Vector3d endVec = seg->getEndPoint();
        if (endpoint == PointPos::start) {
            Base::Vector3d newPoint = startVec - endVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + endVec;
            retcode = moveGeometry(GeoId, Sketcher::PointPos::start, newPoint, false, true);
        }
        else if (endpoint == PointPos::end) {
            Base::Vector3d newPoint = endVec - startVec;
            double scaleFactor = newPoint.Length() + increment;
            newPoint.Normalize();
            newPoint.Scale(scaleFactor, scaleFactor, scaleFactor);
            newPoint = newPoint + startVec;
            retcode = moveGeometry(GeoId, Sketcher::PointPos::end, newPoint, false, true);
        }
    }
    else if (geom->is<Part::GeomArcOfCircle>()) {
        Part::GeomArcOfCircle* arc = static_cast<Part::GeomArcOfCircle*>(geom);
        double startArc, endArc;
        arc->getRange(startArc, endArc, true);
        if (endpoint == PointPos::start) {
            arc->setRange(startArc - increment, endArc, true);
            retcode = 0;
        }
        else if (endpoint == PointPos::end) {
            arc->setRange(startArc, endArc + increment, true);
            retcode = 0;
        }
    }
    if (retcode == 0 && noRecomputes) {
        solve();
    }
    return retcode;
}

// clang-format on
bool SketchObject::seekTrimPoints(
    int GeoId,
    const Base::Vector3d& point,
    int& GeoId1,
    Base::Vector3d& intersect1,
    int& GeoId2,
    Base::Vector3d& intersect2
)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return false;
    }

    auto geos = getCompleteGeometry();  // this includes the axes too

    geos.resize(geos.size() - 2);  // remove the axes to avoid intersections with the axes

    int localindex1, localindex2;

    // Not found in will be returned as -1, not as GeoUndef, Part WB is agnostic to the concept of
    // GeoUndef
    if (!Part2DObject::seekTrimPoints(geos, GeoId, point, localindex1, intersect1, localindex2, intersect2)) {
        return false;
    }

    // invalid complete geometry indices are mapped to GeoUndef
    GeoId1 = getGeoIdFromCompleteGeometryIndex(localindex1);
    GeoId2 = getGeoIdFromCompleteGeometryIndex(localindex2);

    return true;
}

// given a geometry and a point, returns the corresponding parameter of the geometry point
// closest to the point. Wrapped around a try-catch so the calling operation can fail without
// throwing an exception.
bool getIntersectionParameter(const Part::Geometry* geo, const Base::Vector3d point, double& pointParam)
{
    const auto* curve = static_cast<const Part::GeomCurve*>(geo);

    try {
        curve->closestParameter(point, pointParam);
    }
    catch (Base::CADKernelError& e) {
        e.reportException();
        return false;
    }

    return true;
}

bool arePointsWithinPrecision(const Base::Vector3d& point1, const Base::Vector3d& point2)
{
    // From testing: 500x (or 0.000050) is needed in order to not falsely distinguish points
    // calculated with seekTrimPoints
    return ((point1 - point2).Length() < 500 * Precision::Confusion());
}

bool areParamsWithinApproximation(double param1, double param2)
{
    // From testing: 500x (or 0.000050) is needed in order to not falsely distinguish points
    // calculated with seekTrimPoints
    return (std::abs(param1 - param2) < Precision::PApproximation());
}

// returns true if the point defined by (GeoId1, pos1) can be considered to be coincident with
// point.
bool isPointAtPosition(const SketchObject* obj, int GeoId1, PointPos pos1, const Base::Vector3d& point)
{
    Base::Vector3d pp = obj->getPoint(GeoId1, pos1);

    return arePointsWithinPrecision(point, pp);
}

// Checks whether preexisting constraints must be converted to new constraints.
// Preexisting point on object constraints get converted to coincidents.
// Returns:
//     - The constraint that should be used to constraint GeoId and cuttingGeoId
std::unique_ptr<Constraint> transformPreexistingConstraintForTrim(
    const SketchObject* obj,
    const Constraint* constr,
    int GeoId,
    int cuttingGeoId,
    const Base::Vector3d& cutPointVec,
    int newGeoId,
    PointPos newPosId
)
{
    /* TODO: It is possible that the trimming entity has both a PointOnObject constraint to the
     * trimmed entity, and a simple Tangent constraint to the trimmed entity. In this case we
     * want to change to a single end-to-end tangency, i.e we want to ensure that constrType1
     * is set to Sketcher::Tangent, that the secondPos1 is captured from the PointOnObject,
     * and also make sure that the PointOnObject constraint is deleted.
     */
    // TODO: Symmetric and distance constraints (sometimes together) can be changed to something
    std::unique_ptr<Constraint> newConstr;
    if (cuttingGeoId == GeoEnum::GeoUndef || !constr->involvesGeoId(cuttingGeoId)
        || !constr->involvesGeoIdAndPosId(GeoId, PointPos::none)) {
        return newConstr;
    }
    switch (constr->Type) {
        case PointOnObject: {
            // we might want to transform this (and the new point-on-object constraints) into a
            // coincidence At this stage of the check the point has to be an end of `cuttingGeoId`
            // on the edge of `GeoId`.
            if (isPointAtPosition(obj, constr->First, constr->FirstPos, cutPointVec)) {
                // We already know the point-on-object is on the whole of GeoId
                newConstr.reset(constr->copy());
                newConstr->Type = Sketcher::Coincident;
                newConstr->Second = newGeoId;
                newConstr->SecondPos = newPosId;
            }
            break;
        }
        case Tangent:
        case Perpendicular: {
            // These may have to be turned into endpoint-to-endpoint or endpoint-to-edge
            // TODO: could there be tangent/perpendicular constraints not involving the trim that
            // are modified below?
            newConstr.reset(constr->copy());
            newConstr->substituteIndexAndPos(GeoId, PointPos::none, newGeoId, newPosId);
            // make sure the first position is a point
            if (newConstr->FirstPos == PointPos::none) {
                std::swap(newConstr->First, newConstr->Second);
                std::swap(newConstr->FirstPos, newConstr->SecondPos);
            }
            // there is no need for the third point if it exists
            newConstr->Third = GeoEnum::GeoUndef;
            newConstr->ThirdPos = PointPos::none;
            break;
        }
        default:
            break;
    }
    return newConstr;
}

std::unique_ptr<Constraint> getNewConstraintAtTrimCut(
    const SketchObject* obj,
    int cuttingGeoId,
    int cutGeoId,
    PointPos cutPosId,
    const Base::Vector3d& cutPointVec
)
{
    auto newConstr = std::make_unique<Sketcher::Constraint>();
    newConstr->First = cutGeoId;
    newConstr->FirstPos = cutPosId;
    newConstr->Second = cuttingGeoId;
    if (isPointAtPosition(obj, cuttingGeoId, PointPos::start, cutPointVec)) {
        newConstr->Type = Sketcher::Coincident;
        newConstr->SecondPos = PointPos::start;
    }
    else if (isPointAtPosition(obj, cuttingGeoId, PointPos::end, cutPointVec)) {
        newConstr->Type = Sketcher::Coincident;
        newConstr->SecondPos = PointPos::end;
    }
    else {
        // Points are sufficiently far apart: use point-on-object
        newConstr->Type = Sketcher::PointOnObject;
        newConstr->SecondPos = PointPos::none;
    }
    return newConstr;
}

bool isGeoIdAllowedForTrim(const SketchObject* obj, int GeoId)
{
    const auto* geo = obj->getGeometry(GeoId);

    return GeoId >= 0 && GeoId <= obj->getHighestCurveIndex()
        && GeometryFacade::isInternalType(geo, InternalType::None);
}

bool getParamLimitsOfNewGeosForTrim(
    const SketchObject* obj,
    int GeoId,
    std::array<int, 2>& cuttingGeoIds,
    std::array<Base::Vector3d, 2>& cutPoints,
    std::vector<std::pair<double, double>>& paramsOfNewGeos
)
{
    const auto* geoAsCurve = obj->getGeometry<Part::GeomCurve>(GeoId);
    double firstParam = geoAsCurve->getFirstParameter();
    double lastParam = geoAsCurve->getLastParameter();
    double cut0Param {firstParam}, cut1Param {lastParam};

    bool allParamsFound = getIntersectionParameter(geoAsCurve, cutPoints[0], cut0Param)
        && getIntersectionParameter(geoAsCurve, cutPoints[1], cut1Param);
    if (!allParamsFound) {
        return false;
    }

    if (!obj->isClosedCurve(geoAsCurve) && areParamsWithinApproximation(firstParam, cut0Param)) {
        cuttingGeoIds[0] = GeoEnum::GeoUndef;
    }

    if (!obj->isClosedCurve(geoAsCurve) && areParamsWithinApproximation(lastParam, cut1Param)) {
        cuttingGeoIds[1] = GeoEnum::GeoUndef;
    }

    size_t numUndefs = std::count(cuttingGeoIds.begin(), cuttingGeoIds.end(), GeoEnum::GeoUndef);

    if (numUndefs == 0 && arePointsWithinPrecision(cutPoints[0], cutPoints[1])) {
        // If both points are detected and are coincident, deletion is the only option.
        paramsOfNewGeos.clear();
        return true;
    }

    paramsOfNewGeos.assign(2 - numUndefs, {firstParam, lastParam});

    if (paramsOfNewGeos.empty()) {
        return true;
    }

    if (obj->isClosedCurve(geoAsCurve)) {
        paramsOfNewGeos.pop_back();
    }

    if (cuttingGeoIds[0] != GeoEnum::GeoUndef) {
        paramsOfNewGeos.front().second = cut0Param;
    }
    if (cuttingGeoIds[1] != GeoEnum::GeoUndef) {
        paramsOfNewGeos.back().first = cut1Param;
    }

    return true;
}

void createArcsFromGeoWithLimits(
    const Part::GeomCurve* geo,
    const std::vector<std::pair<double, double>>& paramsOfNewGeos,
    std::vector<Part::Geometry*>& newGeos
)
{
    for (auto& [u1, u2] : paramsOfNewGeos) {
        auto newGeo = static_cast<const Part::GeomCurve*>(geo)->createArc(u1, u2);
        assert(newGeo);
        newGeos.emplace_back(newGeo);
    }
}

void createNewConstraintsForTrim(
    const SketchObject* obj,
    const int GeoId,
    const std::array<int, 2>& cuttingGeoIds,
    const std::array<Base::Vector3d, 2>& cutPoints,
    const std::vector<int>& newIds,
    const std::vector<const Part::Geometry*> newGeos,
    std::vector<int>& idsOfOldConstraints,
    std::vector<Constraint*>& newConstraints,
    std::set<int, std::greater<>>& geoIdsToBeDeleted
)
{
    const auto& allConstraints = obj->Constraints.getValues();

    bool isPoint1ConstrainedOnGeoId1 = false;
    bool isPoint2ConstrainedOnGeoId2 = false;

    for (const auto& oldConstrId : idsOfOldConstraints) {
        // trim-specific changes first
        const Constraint* con = allConstraints[oldConstrId];
        if (con->Type == InternalAlignment) {
            geoIdsToBeDeleted.insert(con->First);
            continue;
        }
        if (auto newConstr = transformPreexistingConstraintForTrim(
                obj,
                con,
                GeoId,
                cuttingGeoIds[0],
                cutPoints[0],
                newIds.front(),
                PointPos::end
            )) {
            newConstraints.push_back(newConstr.release());
            isPoint1ConstrainedOnGeoId1 = true;
            continue;
        }
        if (auto newConstr = transformPreexistingConstraintForTrim(
                obj,
                con,
                GeoId,
                cuttingGeoIds[1],
                cutPoints[1],
                newIds.back(),
                PointPos::start
            )) {
            newConstraints.push_back(newConstr.release());
            isPoint2ConstrainedOnGeoId2 = true;
            continue;
        }
        // We have already transferred all constraints on endpoints to the new pieces.
        // If there is still any left, this means one of the remaining pieces was degenerate.
        if (!con->involvesGeoIdAndPosId(GeoId, PointPos::none)) {
            continue;
        }
        // constraint has not yet been changed
        obj->deriveConstraintsForPieces(GeoId, newIds, newGeos, con, newConstraints);
    }

    // Add point-on-object/coincidence constraints with the newly exposed points.
    // This will need to account for the constraints that were already converted
    // to coincident or end-to-end tangency/perpendicularity.
    // TODO: Tangent/perpendicular not yet covered

    if (cuttingGeoIds[0] != GeoEnum::GeoUndef && !isPoint1ConstrainedOnGeoId1) {
        newConstraints.emplace_back(
            getNewConstraintAtTrimCut(obj, cuttingGeoIds[0], newIds.front(), PointPos::end, cutPoints[0])
                .release()
        );
    }

    if (cuttingGeoIds[1] != GeoEnum::GeoUndef && !isPoint2ConstrainedOnGeoId2) {
        newConstraints.emplace_back(
            getNewConstraintAtTrimCut(obj, cuttingGeoIds[1], newIds.back(), PointPos::start, cutPoints[1])
                .release()
        );
    }
}

int SketchObject::trim(int GeoId, const Base::Vector3d& point)
{
    if (!isGeoIdAllowedForTrim(this, GeoId)) {
        return -1;
    }
    // Remove internal geometry beforehand for now
    // FIXME: we should be able to transfer these to new curves smoothly
    // auto geo = getGeometry(GeoId);
    const auto* geoAsCurve = getGeometry<Part::GeomCurve>(GeoId);

    if (geoAsCurve == nullptr) {
        return -1;
    }

    bool isOriginalCurveConstruction = GeometryFacade::getConstruction(geoAsCurve);
    bool isOriginalCurvePeriodic = isClosedCurve(geoAsCurve);

    //******************* Step A => Detection of intersection - Common to all Geometries
    //****************************************//
    // GeoIds intersecting the curve around `point`
    std::array<int, 2> cuttingGeoIds {GeoEnum::GeoUndef, GeoEnum::GeoUndef};
    // Points at the intersection
    std::array<Base::Vector3d, 2> cutPoints;

    // Using SketchObject wrapper, as Part2DObject version returns GeoId = -1 when intersection not
    // found, which is wrong for a GeoId (axis). seekTrimPoints returns:
    // - For a parameter associated with "point" between an intersection and the end point
    // (non-periodic case) cuttingGeoIds[0] != GeoUndef and cuttingGeoIds[1] == GeoUndef
    // - For a parameter associated with "point" between the start point and an intersection
    // (non-periodic case) cuttingGeoIds[1] != GeoUndef and cuttingGeoIds[0] == GeoUndef
    // - For a parameter associated with "point" between two intersection points, cuttingGeoIds[0]
    // != GeoUndef and cuttingGeoIds[1] != GeoUndef
    //
    // FirstParam < point1param < point2param < LastParam
    if (!SketchObject::seekTrimPoints(
            GeoId,
            point,
            cuttingGeoIds[0],
            cutPoints[0],
            cuttingGeoIds[1],
            cutPoints[1]
        )) {
        // If no suitable trim points are found, then trim defaults to deleting the geometry
        delGeometry(GeoId);
        return 0;
    }

    // TODO: find trim parameters
    std::vector<std::pair<double, double>> paramsOfNewGeos;
    paramsOfNewGeos.reserve(2);
    if (!getParamLimitsOfNewGeosForTrim(this, GeoId, cuttingGeoIds, cutPoints, paramsOfNewGeos)) {
        return -1;
    }

    //******************* Step B => Creation of new geometries
    //****************************************//
    std::vector<int> newIds;
    std::vector<Part::Geometry*> newGeos;
    std::vector<const Part::Geometry*> newGeosAsConsts;

    switch (paramsOfNewGeos.size()) {
        case 0: {
            delGeometry(GeoId);
            return 0;
        }
        case 1: {
            newIds.push_back(GeoId);
            break;
        }
        case 2: {
            newIds.push_back(GeoId);
            newIds.push_back(getHighestCurveIndex() + 1);
            break;
        }
        default: {
            return -1;
        }
    }

    createArcsFromGeoWithLimits(geoAsCurve, paramsOfNewGeos, newGeos);
    for (const auto* geo : newGeos) {
        newGeosAsConsts.push_back(geo);
    }

    //******************* Step C => Creation of new constraints
    //****************************************//
    // Now that we have the new curves, change constraints as needed
    // Some are covered with `deriveConstraintsForPieces`, others are specific to trim
    // FIXME: We are using non-smart pointers since that's what's needed in `addConstraints`.
    const auto& allConstraints = this->Constraints.getValues();
    std::vector<Constraint*> newConstraints;
    std::vector<int> idsOfOldConstraints;
    std::set<int, std::greater<>> geoIdsToBeDeleted;
    getConstraintIndices(GeoId, idsOfOldConstraints);
    // remove the constraints that we want to manually transfer
    // We could transfer beforehand but in case of exception that transfer is permanent
    if (!isOriginalCurvePeriodic) {
        std::erase_if(idsOfOldConstraints, [&GeoId, &allConstraints, &cuttingGeoIds](const auto& i) {
            auto* constr = allConstraints[i];
            bool involvesStart = constr->involvesGeoIdAndPosId(GeoId, PointPos::start);
            bool involvesEnd = constr->involvesGeoIdAndPosId(GeoId, PointPos::end);
            bool keepStart = cuttingGeoIds[0] != GeoEnum::GeoUndef;
            bool keepEnd = cuttingGeoIds[1] != GeoEnum::GeoUndef;
            bool involvesBothButNotBothKept = involvesStart && involvesEnd && !(keepStart && keepEnd);
            return !involvesBothButNotBothKept
                && ((involvesStart && keepStart) || (involvesEnd && keepEnd));
        });
    }
    std::erase_if(idsOfOldConstraints, [&GeoId, &allConstraints](const auto& i) {
        return (allConstraints[i]->involvesGeoIdAndPosId(GeoId, PointPos::mid));
    });

    createNewConstraintsForTrim(
        this,
        GeoId,
        cuttingGeoIds,
        cutPoints,
        newIds,
        newGeosAsConsts,
        idsOfOldConstraints,
        newConstraints,
        geoIdsToBeDeleted
    );

    //******************* Step D => Replacing geometries and constraints
    //****************************************//

    // Constraints related to start/mid/end points of original
    [[maybe_unused]] auto constrainAsEqual = [this](int GeoId1, int GeoId2) {
        auto newConstr = std::make_unique<Sketcher::Constraint>();

        // Build Constraints associated with new pair of arcs
        newConstr->Type = Sketcher::Equal;
        newConstr->First = GeoId1;
        newConstr->FirstPos = Sketcher::PointPos::none;
        newConstr->Second = GeoId2;
        newConstr->SecondPos = Sketcher::PointPos::none;
        addConstraint(std::move(newConstr));
    };

    delConstraints(std::move(idsOfOldConstraints), DeleteOption::NoFlag);

    if (!isOriginalCurvePeriodic) {
        transferConstraints(GeoId, PointPos::start, newIds.front(), PointPos::start, true);
        transferConstraints(GeoId, PointPos::end, newIds.back(), PointPos::end, true);
    }
    bool geomHasMid = geoAsCurve->isDerivedFrom<Part::GeomConic>()
        || geoAsCurve->isDerivedFrom<Part::GeomArcOfConic>();
    if (geomHasMid) {
        transferConstraints(GeoId, PointPos::mid, newIds.front(), PointPos::mid, true);
        // Make centers coincident
        if (newIds.size() > 1) {
            auto* joint = new Constraint();
            joint->Type = Coincident;
            joint->First = newIds.front();
            joint->FirstPos = PointPos::mid;
            joint->Second = newIds.back();
            joint->SecondPos = PointPos::mid;
            newConstraints.push_back(joint);

            // Any radius etc. equality constraints here
            // TODO: There could be some form of equality between the constraints here. However, it
            // may happen that this is imposed by an elaborate set of additional constraints. When
            // that happens, this causes redundant constraints, and in worse cases (incorrect)
            // complaints of over-constraint and solver failures.

            // if (std::ranges::none_of(newConstraints, [](const auto& constr) {
            //         return constr->Type == ConstraintType::Equal;
            //     })) {
            //     constrainAsEqual(newIds.front(), newIds.back());
            // }
            // TODO: ensure alignment as well?
        }
    }

    replaceGeometries({GeoId}, newGeos);
    for (auto newId : newIds) {
        setConstruction(newId, isOriginalCurveConstruction);
    }

    if (noRecomputes) {
        solve();
    }

    for (auto& deletedGeoId : geoIdsToBeDeleted) {
        for (auto& cons : newConstraints) {
            changeConstraintAfterDeletingGeo(cons, deletedGeoId);
        }
    }
    std::erase_if(newConstraints, [](const auto& constr) {
        return constr->Type == ConstraintType::None;
    });
    delGeometries(geoIdsToBeDeleted.begin(), geoIdsToBeDeleted.end());
    addConstraints(newConstraints);

    if (noRecomputes) {
        solve();
    }

    //******************* Cleanup
    //****************************************//

    // Since we used regular "non-smart" pointers, we have to handle cleanup
    for (auto& cons : newConstraints) {
        delete cons;
    }

    return 0;
}

int SketchObject::split(int GeoId, const Base::Vector3d& point)
{
    // No need to check input data validity as this is an sketchobject managed operation

    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return -1;
    }

    // FIXME: we should be able to transfer these to new curves smoothly
    deleteUnusedInternalGeometryAndUpdateGeoId(GeoId);
    const auto* geoAsCurve = getGeometry<Part::GeomCurve>(GeoId);

    bool isOriginalCurvePeriodic = isClosedCurve(geoAsCurve);
    std::vector<int> newIds;
    std::vector<Part::Geometry*> newGeos;
    std::vector<Constraint*> newConstraints;

    double splitParam;
    geoAsCurve->closestParameter(point, splitParam);

    // TODO: find trim parameters
    std::vector<std::pair<double, double>> paramsOfNewGeos(
        isOriginalCurvePeriodic ? 1 : 2,
        {geoAsCurve->getFirstParameter(), geoAsCurve->getLastParameter()}
    );
    paramsOfNewGeos.front().second = isOriginalCurvePeriodic
        ? (splitParam + geoAsCurve->getLastParameter() - geoAsCurve->getFirstParameter())
        : splitParam;
    paramsOfNewGeos.back().first = splitParam;

    switch (paramsOfNewGeos.size()) {
        case 0: {
            delGeometry(GeoId);
            return 0;
        }
        case 1: {
            newIds.push_back(GeoId);
            break;
        }
        case 2: {
            newIds.push_back(GeoId);
            newIds.push_back(getHighestCurveIndex() + 1);
            break;
        }
        default: {
            return -1;
        }
    }

    createArcsFromGeoWithLimits(geoAsCurve, paramsOfNewGeos, newGeos);

    std::vector<int> idsOfOldConstraints;
    getConstraintIndices(GeoId, idsOfOldConstraints);

    const auto& allConstraints = this->Constraints.getValues();

    std::erase_if(idsOfOldConstraints, [&GeoId, &allConstraints](const auto& i) {
        return !allConstraints[i]->involvesGeoIdAndPosId(GeoId, PointPos::none);
    });

    for (const auto& oldConstrId : idsOfOldConstraints) {
        Constraint* con = allConstraints[oldConstrId];
        deriveConstraintsForPieces(GeoId, newIds, con, newConstraints);
    }

    // This also seems to reset SketchObject::Geometry.
    // TODO: figure out why, and if that check must be used
    geoAsCurve = getGeometry<Part::GeomCurve>(GeoId);

    if (!isOriginalCurvePeriodic) {
        Constraint* joint = new Constraint();
        joint->Type = Coincident;
        joint->First = newIds.front();
        joint->FirstPos = PointPos::end;
        joint->Second = newIds.back();
        joint->SecondPos = PointPos::start;
        newConstraints.push_back(joint);

        transferConstraints(GeoId, PointPos::start, newIds.front(), PointPos::start);
        transferConstraints(GeoId, PointPos::end, newIds.back(), PointPos::end);
    }

    // This additional constraint is there to maintain existing behavior.
    // TODO: Decide whether to remove it altogether or also apply to other curves with centers.
    if (geoAsCurve->is<Part::GeomArcOfCircle>()) {
        Constraint* joint = new Constraint();
        joint->Type = Coincident;
        joint->First = newIds.front();
        joint->FirstPos = PointPos::mid;
        joint->Second = newIds.back();
        joint->SecondPos = PointPos::mid;
        newConstraints.push_back(joint);
    }

    if (geoAsCurve->isDerivedFrom<Part::GeomConic>()
        || geoAsCurve->isDerivedFrom<Part::GeomArcOfConic>()) {
        transferConstraints(GeoId, PointPos::mid, newIds.front(), PointPos::mid);
    }

    delConstraints(std::move(idsOfOldConstraints), DeleteOption::NoSolve);
    replaceGeometries({GeoId}, newGeos);
    addConstraints(newConstraints);

    // `if (noRecomputes)` results in a failed test (`testPD_TNPSketchPadSketchSplit(self)`)
    // TODO: figure out why, and if that check must be used
    solve();

    for (auto& cons : newConstraints) {
        delete cons;
    }

    return 0;
}

// clang-format off

int SketchObject::join(int geoId1, Sketcher::PointPos posId1, int geoId2, Sketcher::PointPos posId2, int continuity)
{
    // No need to check input data validity as this is an sketchobject managed operation

    Base::StateLocker lock(managedoperation, true);

    if (Sketcher::PointPos::start != posId1 && Sketcher::PointPos::end != posId1
        && Sketcher::PointPos::start != posId2 && Sketcher::PointPos::end != posId2) {
        THROWM(ValueError, "Invalid positions: points must be start or end points of a curve.");
        return -1;
    }

    if (geoId1 == geoId2) {
        THROWM(ValueError, "Connecting the end points of the same curve is not yet supported.");
        return -1;
    }

    if (geoId1 < 0 || geoId1 > getHighestCurveIndex() || geoId2 < 0
        || geoId2 > getHighestCurveIndex()) {
        return -1;
    }

    // get the old splines
    auto* geo1 = dynamic_cast<const Part::GeomCurve*>(getGeometry(geoId1));
    auto* geo2 = dynamic_cast<const Part::GeomCurve*>(getGeometry(geoId2));

    if (GeometryFacade::getConstruction(geo1) != GeometryFacade::getConstruction(geo2)) {
        THROWM(ValueError, "Cannot join construction and non-construction geometries.");
        return -1;
    }

    // TODO: make both curves b-splines here itself
    if (!geo1 || !geo2) {
        return -1;
    }

    // TODO: is there a cleaner way to get our mutable bsp's?
    // we need the splines to be mutable because we may reverse them
    // and/or change their degree
    std::unique_ptr<Part::GeomBSplineCurve> bsp1(
        geo1->toNurbs(geo1->getFirstParameter(), geo1->getLastParameter()));
    std::unique_ptr<Part::GeomBSplineCurve> bsp2(
        geo2->toNurbs(geo2->getFirstParameter(), geo2->getLastParameter()));

    if (bsp1->isPeriodic() || bsp2->isPeriodic()) {
        THROWM(ValueError, "It is only possible to join non-periodic curves.");
        return -1;
    }

    // reverse the splines if needed: join end of 1st to start of 2nd
    if (Sketcher::PointPos::start == posId1)
        bsp1->reverse();
    if (Sketcher::PointPos::end == posId2)
        bsp2->reverse();

    // ensure the degrees of both curves are the same
    if (bsp1->getDegree() < bsp2->getDegree())
        bsp1->increaseDegree(bsp2->getDegree());
    else if (bsp2->getDegree() < bsp1->getDegree())
        bsp2->increaseDegree(bsp1->getDegree());

    // TODO: Check for tangent constraint here
    bool makeC1Continuous = (continuity >= 1);

    // TODO: Rescale one or both sections to fulfill some purpose.
    // This could include making param between [0,1], and/or making
    // C1 continuity possible.
    if (makeC1Continuous) {
        // We assume here that there is already G1 continuity.
        // Just scale parameters to get C1.
        Base::Vector3d slope1 = bsp1->firstDerivativeAtParameter(bsp1->getLastParameter());
        Base::Vector3d slope2 = bsp2->firstDerivativeAtParameter(bsp2->getFirstParameter());
        // TODO: slope2 can technically be a zero vector
        // But that seems not possible unless the spline is trivial.
        // Prove or account for the possibility.
        double scale = slope2.Length() / slope1.Length();
        bsp2->scaleKnotsToBounds(0, scale * (bsp2->getLastParameter() - bsp2->getFirstParameter()));
    }

    // set up vectors for new poles, knots, mults
    std::vector<Base::Vector3d> poles1 = bsp1->getPoles();
    std::vector<double> weights1 = bsp1->getWeights();
    std::vector<double> knots1 = bsp1->getKnots();
    std::vector<int> mults1 = bsp1->getMultiplicities();
    std::vector<Base::Vector3d> poles2 = bsp2->getPoles();
    std::vector<double> weights2 = bsp2->getWeights();
    std::vector<double> knots2 = bsp2->getKnots();
    std::vector<int> mults2 = bsp2->getMultiplicities();

    std::vector<Base::Vector3d> newPoles(std::move(poles1));
    std::vector<double> newWeights(std::move(weights1));
    std::vector<double> newKnots(std::move(knots1));
    std::vector<int> newMults(std::move(mults1));

    poles2.erase(poles2.begin());
    if (makeC1Continuous)
        newPoles.erase(newPoles.end()-1);
    newPoles.insert(newPoles.end(),
                    std::make_move_iterator(poles2.begin()),
                    std::make_move_iterator(poles2.end()));

    // TODO: Weights might need to be scaled
    weights2.erase(weights2.begin());
    if (makeC1Continuous)
        newWeights.erase(newWeights.end()-1);
    newWeights.insert(newWeights.end(),
                      std::make_move_iterator(weights2.begin()),
                      std::make_move_iterator(weights2.end()));

    // knots of the second spline come after all of the first
    double offset = newKnots.back() - knots2.front();
    knots2.erase(knots2.begin());
    for (auto& knot : knots2)
        knot += offset;
    newKnots.insert(newKnots.end(),
                    std::make_move_iterator(knots2.begin()),
                    std::make_move_iterator(knots2.end()));

    // end knots can have a multiplicity of (degree + 1)
    if (bsp1->getDegree() < newMults.back()) {
        newMults.back() = bsp1->getDegree();
        if (makeC1Continuous) {
            newMults.back() -= 1;
        }
    }

    mults2.erase(mults2.begin());
    newMults.insert(newMults.end(),
                    std::make_move_iterator(mults2.begin()),
                    std::make_move_iterator(mults2.end()));

    Part::GeomBSplineCurve* newSpline = new Part::GeomBSplineCurve(
        newPoles, newWeights, newKnots, newMults, bsp1->getDegree(), false, true);

    int newGeoId = addGeometry(newSpline);

    if (newGeoId < 0) {
        THROWM(ValueError, "Failed to create joined curve.");
        return -1;
    }

    exposeInternalGeometry(newGeoId);
    setConstruction(newGeoId, GeometryFacade::getConstruction(geo1));

    // TODO: transfer constraints on the non-connected ends
    auto otherPosId1 = (Sketcher::PointPos::start == posId1) ? Sketcher::PointPos::end
        : Sketcher::PointPos::start;
    auto otherPosId2 = (Sketcher::PointPos::start == posId2) ? Sketcher::PointPos::end
        : Sketcher::PointPos::start;

    transferConstraints(geoId1, otherPosId1, newGeoId, PointPos::start, true);
    transferConstraints(geoId2, otherPosId2, newGeoId, PointPos::end, true);

    delGeometries({geoId1, geoId2});

    return 0;
}

int SketchObject::addSymmetric(const std::vector<int>& geoIdList, int refGeoId,
                               Sketcher::PointPos refPosId , bool addSymmetryConstraints )
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Constraint*>& constrvals = this->Constraints.getValues();
    std::vector<Constraint*> newconstrVals(constrvals);

    std::map<int, int> geoIdMap;
    std::map<int, bool> isStartEndInverted;

    // Find out if reference is aligned with V or H axis,
    // if so we can keep Vertical and Horizontal constraints in the mirrored geometry.
    bool refIsLine = refPosId == Sketcher::PointPos::none;
    bool refIsAxisAligned = false;
    if (refGeoId == Sketcher::GeoEnum::VAxis || refGeoId == Sketcher::GeoEnum::HAxis || !refIsLine) {
        refIsAxisAligned = true;
    }
    else {
        for (auto* constr : constrvals) {
            if (constr->First == refGeoId
                && (constr->Type == Sketcher::Vertical || constr->Type == Sketcher::Horizontal)){
                refIsAxisAligned = true;
            }
        }
    }

    std::vector<Part::Geometry*> symgeos = getSymmetric(geoIdList, geoIdMap, isStartEndInverted, refGeoId, refPosId);

    {
        addGeometry(symgeos);

        for (auto* constr :  constrvals) {
            // we look in the map, because we might have skipped internal alignment geometry
            auto fit = geoIdMap.find(constr->First);

            if (fit != geoIdMap.end()) {// if First of constraint is in geoIdList
                if (addSymmetryConstraints && constr->Type != Sketcher::InternalAlignment) {
                    // if we are making symmetric constraints, then we don't want to copy all constraints
                    continue;
                }

                if (constr->Second == GeoEnum::GeoUndef ){
                    if (refIsAxisAligned) {
                        // in this case we want to keep the Vertical, Horizontal constraints
                        // DistanceX ,and DistanceY constraints should also be possible to keep in
                        // this case, but keeping them causes segfault, not sure why.

                        if (constr->Type != Sketcher::DistanceX
                            && constr->Type != Sketcher::DistanceY) {
                            Constraint* constNew = constr->copy();
                            constNew->Name = ""; // Make sure we don't have 2 constraint with same name.
                            constNew->First = fit->second;
                            newconstrVals.push_back(constNew);
                        }
                    }
                    else if (constr->Type != Sketcher::DistanceX
                                && constr->Type != Sketcher::DistanceY
                                && constr->Type != Sketcher::Vertical
                                && constr->Type != Sketcher::Horizontal) {
                        // this includes all non-directional single GeoId constraints, as radius,
                        // diameter, weight,...

                        Constraint* constNew = constr->copy();
                        constNew->Name = "";
                        constNew->First = fit->second;
                        newconstrVals.push_back(constNew);
                    }
                }
                else {// other geoids intervene in this constraint

                    auto sit = geoIdMap.find(constr->Second);

                    if (sit != geoIdMap.end()) {// Second is also in the list

                        if (constr->Third == GeoEnum::GeoUndef) {
                            if (constr->Type == Sketcher::Coincident
                                || constr->Type == Sketcher::Perpendicular
                                || constr->Type == Sketcher::Parallel
                                || constr->Type == Sketcher::Tangent
                                || constr->Type == Sketcher::Distance
                                || constr->Type == Sketcher::Equal || constr->Type == Sketcher::Angle
                                || constr->Type == Sketcher::PointOnObject
                                || constr->Type == Sketcher::InternalAlignment) {
                                Constraint* constNew = constr->copy();
                                constNew->Name = "";
                                constNew->First = fit->second;
                                constNew->Second = sit->second;
                                if (isStartEndInverted[constr->First]) {
                                    if (constr->FirstPos == Sketcher::PointPos::start)
                                        constNew->FirstPos = Sketcher::PointPos::end;
                                    else if (constr->FirstPos == Sketcher::PointPos::end)
                                        constNew->FirstPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[constr->Second]) {
                                    if (constr->SecondPos == Sketcher::PointPos::start)
                                        constNew->SecondPos = Sketcher::PointPos::end;
                                    else if (constr->SecondPos == Sketcher::PointPos::end)
                                        constNew->SecondPos = Sketcher::PointPos::start;
                                }

                                if (constNew->Type == Tangent || constNew->Type == Perpendicular)
                                    AutoLockTangencyAndPerpty(constNew, true);

                                if ((constr->Type == Sketcher::Angle)
                                    && (refPosId == Sketcher::PointPos::none)) {
                                    constNew->setValue(-constr->getValue());
                                }

                                newconstrVals.push_back(constNew);
                            }
                        }
                        else {// three GeoIds intervene in constraint
                            auto tit = geoIdMap.find(constr->Third);

                            if (tit != geoIdMap.end()) {// Third is also in the list
                                Constraint* constNew = constr->copy();
                                constNew->Name = "";
                                constNew->First = fit->second;
                                constNew->Second = sit->second;
                                constNew->Third = tit->second;
                                if (isStartEndInverted[constr->First]) {
                                    if (constr->FirstPos == Sketcher::PointPos::start)
                                        constNew->FirstPos = Sketcher::PointPos::end;
                                    else if (constr->FirstPos == Sketcher::PointPos::end)
                                        constNew->FirstPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[constr->Second]) {
                                    if (constr->SecondPos == Sketcher::PointPos::start)
                                        constNew->SecondPos = Sketcher::PointPos::end;
                                    else if (constr->SecondPos == Sketcher::PointPos::end)
                                        constNew->SecondPos = Sketcher::PointPos::start;
                                }
                                if (isStartEndInverted[constr->Third]) {
                                    if (constr->ThirdPos == Sketcher::PointPos::start)
                                        constNew->ThirdPos = Sketcher::PointPos::end;
                                    else if (constr->ThirdPos == Sketcher::PointPos::end)
                                        constNew->ThirdPos = Sketcher::PointPos::start;
                                }
                                newconstrVals.push_back(constNew);
                            }
                        }
                    }
                }
            }
        }

        if (addSymmetryConstraints) {
            auto createSymConstr = [&]
            (int first, int second, Sketcher::PointPos firstPos, Sketcher::PointPos secondPos) {
                auto symConstr = new Constraint();
                symConstr->Type = Symmetric;
                symConstr->First = first;
                symConstr->Second = second;
                symConstr->Third = refGeoId;
                symConstr->FirstPos = firstPos;
                symConstr->SecondPos = secondPos;
                symConstr->ThirdPos = refPosId;
                newconstrVals.push_back(symConstr);
            };
            auto createEqualityConstr = [&]
            (int first, int second) {
                auto symConstr = new Constraint();
                symConstr->Type = Equal;
                symConstr->First = first;
                symConstr->Second = second;
                newconstrVals.push_back(symConstr);
            };

            for (auto geoIdPair : geoIdMap) {
                int geoId1 = geoIdPair.first;
                int geoId2 = geoIdPair.second;
                const Part::Geometry* geo = getGeometry(geoId1);

                if (geo->is<Part::GeomLineSegment>()) {
                    auto gf = GeometryFacade::getFacade(geo);
                    if (!gf->isInternalAligned()) {
                        // Note internal aligned lines (ellipse, parabola, hyperbola) are causing redundant constraint.
                        createSymConstr(geoId1, geoId2, PointPos::start, isStartEndInverted[geoId1] ? PointPos::end : PointPos::start);
                        createSymConstr(geoId1, geoId2, PointPos::end, isStartEndInverted[geoId1] ? PointPos::start : PointPos::end);
                    }
                }
                else if (geo->is<Part::GeomCircle>() || geo->is<Part::GeomEllipse>()) {
                    createEqualityConstr(geoId1, geoId2);
                    createSymConstr(geoId1, geoId2, PointPos::mid, PointPos::mid);
                }
                else if (geo->is<Part::GeomArcOfCircle>()
                    || geo->is<Part::GeomArcOfEllipse>()
                    || geo->is<Part::GeomArcOfHyperbola>()
                    || geo->is<Part::GeomArcOfParabola>()) {
                    createEqualityConstr(geoId1, geoId2);
                    createSymConstr(geoId1, geoId2, PointPos::start, isStartEndInverted[geoId1] ? PointPos::end : PointPos::start);
                    createSymConstr(geoId1, geoId2, PointPos::end, isStartEndInverted[geoId1] ? PointPos::start : PointPos::end);
                }
                else if (geo->is<Part::GeomPoint>()) {
                    auto gf = GeometryFacade::getFacade(geo);
                    if (!gf->isInternalAligned()) {
                        createSymConstr(geoId1, geoId2, PointPos::start, PointPos::start);
                    }
                }
                // Note bspline has symmetric by the internal aligned circles.
            }
        }

        if (newconstrVals.size() > constrvals.size()){
            Constraints.setValues(std::move(newconstrVals));
        }
    }

    // we delayed update, so trigger it now.
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return Geometry.getSize() - 1;
}

std::vector<Part::Geometry*> SketchObject::getSymmetric(const std::vector<int>& geoIdList,
    std::map<int, int>& geoIdMap,
    std::map<int, bool>& isStartEndInverted,
    int refGeoId,
    Sketcher::PointPos refPosId)
{
    using std::numbers::pi;

    std::vector<Part::Geometry*> symmetricVals;
    bool refIsLine = refPosId == Sketcher::PointPos::none;
    int cgeoid = getHighestCurveIndex() + 1;

    auto shouldCopyGeometry = [&](auto* geo, int geoId) -> bool {
        auto gf = GeometryFacade::getFacade(geo);
        if (gf->isInternalAligned()) {
            // only add if the corresponding geometry it defines is also in the list.
            int definedGeo = GeoEnum::GeoUndef;
            for (auto c : Constraints.getValues()) {
                if (c->Type == Sketcher::InternalAlignment && c->First == geoId) {
                    definedGeo = c->Second;
                    break;
                }
            }
            // Return true if definedGeo is in geoIdList, false otherwise
            return std::ranges::find(geoIdList, definedGeo) != geoIdList.end();
        }
        // Return true if not internal aligned, indicating it should always be copied
        return true;
    };

    if (refIsLine) {
        const Part::Geometry* georef = getGeometry(refGeoId);
        if (!georef->is<Part::GeomLineSegment>()) {
            return {};
        }

        auto* refGeoLine = static_cast<const Part::GeomLineSegment*>(georef);
        // line
        Base::Vector3d refstart = refGeoLine->getStartPoint();
        Base::Vector3d vectline = refGeoLine->getEndPoint() - refstart;

        for (auto geoId : geoIdList) {
            const Part::Geometry* geo = getGeometry(geoId);
            Part::Geometry* geosym;

            if (!shouldCopyGeometry(geo, geoId)) {
                continue;
            }

            geosym = geo->copy();

            // Handle Geometry
            if (geosym->is<Part::GeomLineSegment>()) {
                auto* geosymline = static_cast<Part::GeomLineSegment*>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();

                geosymline->setPoints(
                    sp + 2.0 * (sp.Perpendicular(refGeoLine->getStartPoint(), vectline) - sp),
                    ep + 2.0 * (ep.Perpendicular(refGeoLine->getStartPoint(), vectline) - ep));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomCircle>()) {
                auto* geosymcircle = static_cast<Part::GeomCircle*>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfCircle>()) {
                auto* geoaoc = static_cast<Part::GeomArcOfCircle*>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp =
                    sp + 2.0 * (sp.Perpendicular(refGeoLine->getStartPoint(), vectline) - sp);
                Base::Vector3d sep =
                    ep + 2.0 * (ep.Perpendicular(refGeoLine->getStartPoint(), vectline) - ep);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                double theta1 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f * std::numbers::pi);
                double theta2 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f * std::numbers::pi);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomEllipse>()) {
                auto* geosymellipse = static_cast<Part::GeomEllipse*>(geosym);
                Base::Vector3d cp = geosymellipse->getCenter();

                Base::Vector3d majdir = geosymellipse->getMajorAxisDir();
                double majord = geosymellipse->getMajorRadius();
                double minord = geosymellipse->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymellipse->setMajorAxisDir(sf1 - scp);

                geosymellipse->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfEllipse>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfEllipse*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);

                double theta1, theta2;
                geosymaoe->getRange(theta1, theta2, true);
                theta1 = 2.0 * pi - theta1;
                theta2 = 2.0 * pi - theta2;
                std::swap(theta1, theta2);
                if (theta1 < 0) {
                    theta1 += 2.0 * pi;
                    theta2 += 2.0 * pi;
                }

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomArcOfHyperbola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfHyperbola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord + minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);

                double theta1, theta2;
                geosymaoe->getRange(theta1, theta2, true);
                theta1 = -theta1;
                theta2 = -theta2;
                std::swap(theta1, theta2);

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomArcOfParabola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfParabola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 =
                    f1 + 2.0 * (f1.Perpendicular(refGeoLine->getStartPoint(), vectline) - f1);
                Base::Vector3d scp =
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp);

                geosymaoe->setXAxisDir(sf1 - scp);
                geosymaoe->setCenter(scp);

                double theta1, theta2;
                geosymaoe->getRange(theta1, theta2, true);
                theta1 = -theta1;
                theta2 = -theta2;
                std::swap(theta1, theta2);

                geosymaoe->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, true));
            }
            else if (geosym->is<Part::GeomBSplineCurve>()) {
                auto* geosymbsp = static_cast<Part::GeomBSplineCurve*>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for (auto& pole : poles) {
                    pole = pole
                        + 2.0 * (pole.Perpendicular(refGeoLine->getStartPoint(), vectline) - pole);
                }

                geosymbsp->setPoles(poles);

                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomPoint>()) {
                auto* geosympoint = static_cast<Part::GeomPoint*>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(
                    cp + 2.0 * (cp.Perpendicular(refGeoLine->getStartPoint(), vectline) - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else {
                Base::Console().error("Unsupported Geometry!! Just copying it.\n");
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }

            symmetricVals.push_back(geosym);
            geoIdMap.insert(std::make_pair(geoId, cgeoid));
            cgeoid++;
        }
    }
    else {// reference is a point
        Vector3d refpoint;
        const Part::Geometry* georef = getGeometry(refGeoId);

        if (georef->is<Part::GeomPoint>()) {
            refpoint = static_cast<const Part::GeomPoint*>(georef)->getPoint();
        }
        else if (refGeoId == -1 && refPosId == Sketcher::PointPos::start) {
            refpoint = Vector3d(0, 0, 0);
        }
        else {
            if (refPosId == Sketcher::PointPos::none) {
                Base::Console().error("Wrong PointPosId.\n");
                return {};
            }
            refpoint = getPoint(georef, refPosId);
        }

        for (auto geoId : geoIdList) {
            const Part::Geometry* geo = getGeometry(geoId);
            Part::Geometry* geosym;

            if (!shouldCopyGeometry(geo, geoId)) {
                continue;
            }

            geosym = geo->copy();

            // Handle Geometry
            if (geosym->is<Part::GeomLineSegment>()) {
                auto* geosymline = static_cast<Part::GeomLineSegment*>(geosym);
                Base::Vector3d sp = geosymline->getStartPoint();
                Base::Vector3d ep = geosymline->getEndPoint();
                Base::Vector3d ssp = sp + 2.0 * (refpoint - sp);
                Base::Vector3d sep = ep + 2.0 * (refpoint - ep);

                geosymline->setPoints(ssp, sep);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomCircle>()) {
                auto* geosymcircle = static_cast<Part::GeomCircle*>(geosym);
                Base::Vector3d cp = geosymcircle->getCenter();

                geosymcircle->setCenter(cp + 2.0 * (refpoint - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfCircle>()) {
                auto* geoaoc = static_cast<Part::GeomArcOfCircle*>(geosym);
                Base::Vector3d sp = geoaoc->getStartPoint(true);
                Base::Vector3d ep = geoaoc->getEndPoint(true);
                Base::Vector3d cp = geoaoc->getCenter();

                Base::Vector3d ssp = sp + 2.0 * (refpoint - sp);
                Base::Vector3d sep = ep + 2.0 * (refpoint - ep);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                double theta1 = Base::fmod(atan2(ssp.y - scp.y, ssp.x - scp.x), 2.f * pi);
                double theta2 = Base::fmod(atan2(sep.y - scp.y, sep.x - scp.x), 2.f * pi);

                geoaoc->setCenter(scp);
                geoaoc->setRange(theta1, theta2, true);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomEllipse>()) {
                auto* geosymellipse = static_cast<Part::GeomEllipse*>(geosym);
                Base::Vector3d cp = geosymellipse->getCenter();

                Base::Vector3d majdir = geosymellipse->getMajorAxisDir();
                double majord = geosymellipse->getMajorRadius();
                double minord = geosymellipse->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymellipse->setMajorAxisDir(sf1 - scp);

                geosymellipse->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfEllipse>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfEllipse*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord - minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfHyperbola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfHyperbola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();

                Base::Vector3d majdir = geosymaoe->getMajorAxisDir();
                double majord = geosymaoe->getMajorRadius();
                double minord = geosymaoe->getMinorRadius();
                double df = sqrt(majord * majord + minord * minord);
                Base::Vector3d f1 = cp + df * majdir;

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setMajorAxisDir(sf1 - scp);

                geosymaoe->setCenter(scp);
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomArcOfParabola>()) {
                auto* geosymaoe = static_cast<Part::GeomArcOfParabola*>(geosym);
                Base::Vector3d cp = geosymaoe->getCenter();
                Base::Vector3d f1 = geosymaoe->getFocus();

                Base::Vector3d sf1 = f1 + 2.0 * (refpoint - f1);
                Base::Vector3d scp = cp + 2.0 * (refpoint - cp);

                geosymaoe->setXAxisDir(sf1 - scp);
                geosymaoe->setCenter(scp);

                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else if (geosym->is<Part::GeomBSplineCurve>()) {
                auto* geosymbsp = static_cast<Part::GeomBSplineCurve*>(geosym);

                std::vector<Base::Vector3d> poles = geosymbsp->getPoles();

                for (auto& pole : poles) {
                    pole = pole + 2.0 * (refpoint - pole);
                }

                geosymbsp->setPoles(poles);

            }
            else if (geosym->is<Part::GeomPoint>()) {
                auto* geosympoint = static_cast<Part::GeomPoint*>(geosym);
                Base::Vector3d cp = geosympoint->getPoint();

                geosympoint->setPoint(cp + 2.0 * (refpoint - cp));
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }
            else {
                Base::Console().error("Unsupported Geometry!! Just copying it.\n");
                isStartEndInverted.insert(std::make_pair(geoId, false));
            }

            symmetricVals.push_back(geosym);
            geoIdMap.insert(std::make_pair(geoId, cgeoid));
            cgeoid++;
        }
    }
    return symmetricVals;
}

int SketchObject::addCopy(const std::vector<int>& geoIdList, const Base::Vector3d& displacement,
                          bool moveonly , bool clone , int csize , int rsize , bool constraindisplacement ,
                          double perpscale )
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& geovals = getInternalGeometry();
    std::vector<Part::Geometry*> newgeoVals(geovals);

    const std::vector<Constraint*>& constrvals = this->Constraints.getValues();
    std::vector<Constraint*> newconstrVals(constrvals);

    if (!moveonly) {
        newgeoVals.reserve(geovals.size() + geoIdList.size());
    }

    std::vector<int> newgeoIdList(geoIdList);

    if (newgeoIdList.empty()) {// default option to operate on all the geometry
        for (int i = 0; i < int(geovals.size()); i++)
            newgeoIdList.push_back(i);
    }

    int cgeoid = getHighestCurveIndex() + 1;

    int iterfirstgeoid = -1;

    Base::Vector3d iterfirstpoint;

    int refgeoid = -1;

    int colrefgeoid = 0, rowrefgeoid = 0;

    int currentrowfirstgeoid = -1, prevrowstartfirstgeoid = -1, prevfirstgeoid = -1;

    Sketcher::PointPos refposId = Sketcher::PointPos::none;

    std::map<int, int> geoIdMap;

    Base::Vector3d perpendicularDisplacement =
        Base::Vector3d(perpscale * displacement.y, perpscale * -displacement.x, 0);

    int x, y;

    for (y = 0; y < rsize; y++) {
        for (x = 0; x < csize; x++) {
            // the reference for constraining array elements is the first valid point of the first
            // element
            if (x == 0 && y == 0) {
                const Part::Geometry* geo = getGeometry(*(newgeoIdList.begin()));

                auto gf = GeometryFacade::getFacade(geo);

                if (gf->isInternalAligned() && !moveonly) {
                    // only add this geometry if the corresponding geometry it defines is also in
                    // the list.
                    int definedGeo = GeoEnum::GeoUndef;

                    for (auto c : Constraints.getValues()) {
                        if (c->Type == Sketcher::InternalAlignment
                            && c->First == *(newgeoIdList.begin())) {
                            definedGeo = c->Second;
                            break;
                        }
                    }

                    if (std::ranges::find(newgeoIdList, definedGeo) == newgeoIdList.end()) {
                        // the first element setting the reference is an internal alignment
                        // geometry, wherein the geometry it defines is not part of the copy
                        // operation.
                        THROWM(Base::ValueError,
                               "A move/copy/array operation on an internal alignment geometry is "
                               "only possible together with the geometry it defines.")
                    }
                }

                refgeoid = *(newgeoIdList.begin());
                currentrowfirstgeoid = refgeoid;
                iterfirstgeoid = refgeoid;
                if (geo->is<Part::GeomCircle>()
                    || geo->is<Part::GeomEllipse>()) {
                    refposId = Sketcher::PointPos::mid;
                }
                else
                    refposId = Sketcher::PointPos::start;

                continue;// the first element is already in place
            }
            else {
                prevfirstgeoid = iterfirstgeoid;

                iterfirstgeoid = cgeoid;

                if (x == 0) {// if first element of second row
                    prevrowstartfirstgeoid = currentrowfirstgeoid;
                    currentrowfirstgeoid = cgeoid;
                }
            }

            int index = 0;
            for (std::vector<int>::const_iterator it = newgeoIdList.begin();
                 it != newgeoIdList.end();
                 ++it, index++) {
                const Part::Geometry* geo = getGeometry(*it);

                Part::Geometry* geocopy;

                auto gf = GeometryFacade::getFacade(geo);

                if (gf->isInternalAligned() && !moveonly) {
                    // only add this geometry if the corresponding geometry it defines is also in
                    // the list.
                    int definedGeo = GeoEnum::GeoUndef;

                    for (auto c : Constraints.getValues()) {
                        if (c->Type == Sketcher::InternalAlignment && c->First == *it) {
                            definedGeo = c->Second;
                            break;
                        }
                    }

                    if (std::ranges::find(newgeoIdList, definedGeo)
                        == newgeoIdList.end()) {
                        // we should not copy internal alignment geometry, unless the element they
                        // define is also mirrored
                        continue;
                    }
                }

                // We have already cloned all geometry and constraints, we only need a copy if not
                // moving
                if (!moveonly) {
                    geocopy = geo->copy();
                    generateId(geocopy);
                } else
                    geocopy = newgeoVals[*it];

                // Handle Geometry
                if (geocopy->is<Part::GeomLineSegment>()) {
                    Part::GeomLineSegment* geosymline =
                        static_cast<Part::GeomLineSegment*>(geocopy);
                    Base::Vector3d ep = geosymline->getEndPoint();
                    Base::Vector3d ssp = geosymline->getStartPoint() + double(x) * displacement
                        + double(y) * perpendicularDisplacement;

                    geosymline->setPoints(
                        ssp, ep + double(x) * displacement + double(y) * perpendicularDisplacement);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = ssp;
                }
                else if (geocopy->is<Part::GeomCircle>()) {
                    auto* geosymcircle = static_cast<Part::GeomCircle*>(geocopy);
                    Base::Vector3d cp = geosymcircle->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geosymcircle->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if (geocopy->is<Part::GeomArcOfCircle>()) {
                    auto* geoaoc = static_cast<Part::GeomArcOfCircle*>(geocopy);
                    Base::Vector3d cp = geoaoc->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoc->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoc->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomEllipse>()) {
                    auto* geosymellipse = static_cast<Part::GeomEllipse*>(geocopy);
                    Base::Vector3d cp = geosymellipse->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geosymellipse->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else if (geocopy->is<Part::GeomArcOfEllipse>()) {
                    auto* geoaoe = static_cast<Part::GeomArcOfEllipse*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomArcOfHyperbola>()) {
                    Part::GeomArcOfHyperbola* geoaoe =
                        static_cast<Part::GeomArcOfHyperbola*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomArcOfParabola>()) {
                    Part::GeomArcOfParabola* geoaoe =
                        static_cast<Part::GeomArcOfParabola*>(geocopy);
                    Base::Vector3d cp = geoaoe->getCenter();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;

                    geoaoe->setCenter(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geoaoe->getStartPoint(true);
                }
                else if (geocopy->is<Part::GeomBSplineCurve>()) {
                    auto* geobsp = static_cast<Part::GeomBSplineCurve*>(geocopy);

                    std::vector<Base::Vector3d> poles = geobsp->getPoles();

                    for (std::vector<Base::Vector3d>::iterator jt = poles.begin();
                         jt != poles.end();
                         ++jt) {

                        (*jt) = (*jt) + double(x) * displacement
                            + double(y) * perpendicularDisplacement;
                    }

                    geobsp->setPoles(poles);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = geobsp->getStartPoint();
                }
                else if (geocopy->is<Part::GeomPoint>()) {
                    auto* geopoint = static_cast<Part::GeomPoint*>(geocopy);
                    Base::Vector3d cp = geopoint->getPoint();
                    Base::Vector3d scp =
                        cp + double(x) * displacement + double(y) * perpendicularDisplacement;
                    geopoint->setPoint(scp);

                    if (it == newgeoIdList.begin())
                        iterfirstpoint = scp;
                }
                else {
                    Base::Console().error("Unsupported Geometry!! Just skipping it.\n");
                    continue;
                }

                if (!moveonly) {// we are copying
                    newgeoVals.push_back(geocopy);
                    geoIdMap.insert(std::make_pair(*it, cgeoid));
                    cgeoid++;
                }
            }

            if (!moveonly) {
                // handle geometry constraints
                for (std::vector<Constraint*>::const_iterator it = constrvals.begin();
                     it != constrvals.end();
                     ++it) {

                    auto fit = geoIdMap.find((*it)->First);

                    if (fit != geoIdMap.end()) {// if First of constraint is in geoIdList

                        if ((*it)->Second
                            == GeoEnum::GeoUndef /*&& (*it)->Third == GeoEnum::GeoUndef*/) {
                            if (((*it)->Type != Sketcher::DistanceX
                                 && (*it)->Type != Sketcher::DistanceY)
                                || (*it)->FirstPos == Sketcher::PointPos::none) {
                                // if it is not a point locking DistanceX/Y
                                if (((*it)->Type == Sketcher::DistanceX
                                     || (*it)->Type == Sketcher::DistanceY
                                     || (*it)->Type == Sketcher::Distance
                                     || (*it)->Type == Sketcher::Diameter
                                     || (*it)->Type == Sketcher::Weight
                                     || (*it)->Type == Sketcher::Radius)
                                    && clone) {
                                    // Distances on a single Element are mapped to equality
                                    // constraints in clone mode
                                    Constraint* constNew = (*it)->copy();
                                    constNew->Type = Sketcher::Equal;
                                    constNew->isDriving = true;
                                    // first is already (*it->First)
                                    constNew->Second = fit->second;
                                    newconstrVals.push_back(constNew);
                                }
                                else if ((*it)->Type == Sketcher::Angle && clone) {
                                    if (getGeometry((*it)->First)->is<Part::GeomLineSegment>()) {
                                        // Angles on a single Element are mapped to parallel
                                        // constraints in clone mode
                                        Constraint* constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Parallel;
                                        constNew->isDriving = true;
                                        // first is already (*it->First)
                                        constNew->Second = fit->second;
                                        newconstrVals.push_back(constNew);
                                    }
                                }
                                else {
                                    Constraint* constNew = (*it)->copy();
                                    constNew->First = fit->second;
                                    newconstrVals.push_back(constNew);
                                }
                            }
                        }
                        else {// other geoids intervene in this constraint

                            auto sit = geoIdMap.find((*it)->Second);

                            if (sit != geoIdMap.end()) {// Second is also in the list
                                if ((*it)->Third == GeoEnum::GeoUndef) {
                                    if (((*it)->Type == Sketcher::DistanceX
                                         || (*it)->Type == Sketcher::DistanceY
                                         || (*it)->Type == Sketcher::Distance)
                                        && ((*it)->First == (*it)->Second) && clone) {
                                        // Distances on a two Elements, which must be points of the
                                        // same line are mapped to equality constraints in clone
                                        // mode
                                        Constraint* constNew = (*it)->copy();
                                        constNew->Type = Sketcher::Equal;
                                        constNew->isDriving = true;
                                        constNew->FirstPos = Sketcher::PointPos::none;
                                        // first is already (*it->First)
                                        constNew->Second = fit->second;
                                        constNew->SecondPos = Sketcher::PointPos::none;
                                        newconstrVals.push_back(constNew);
                                    }
                                    else {// this includes InternalAlignment constraints
                                        Constraint* constNew = (*it)->copy();
                                        constNew->First = fit->second;
                                        constNew->Second = sit->second;
                                        newconstrVals.push_back(constNew);
                                    }
                                }
                                else {
                                    auto tit = geoIdMap.find((*it)->Third);

                                    if (tit != geoIdMap.end()) {// Third is also in the list
                                        Constraint* constNew = (*it)->copy();
                                        constNew->First = fit->second;
                                        constNew->Second = sit->second;
                                        constNew->Third = tit->second;

                                        newconstrVals.push_back(constNew);
                                    }
                                }
                            }
                        }
                    }
                }

                // handle inter-geometry constraints
                if (constraindisplacement) {

                    // add a construction line
                    Part::GeomLineSegment* constrline = new Part::GeomLineSegment();

                    // position of the reference point
                    Base::Vector3d sp = getPoint(refgeoid, refposId)
                        + ((x == 0) ? (double(x) * displacement
                                       + double(y - 1) * perpendicularDisplacement)
                                    : (double(x - 1) * displacement
                                       + double(y) * perpendicularDisplacement));

                    // position of the current instance corresponding point
                    Base::Vector3d ep = iterfirstpoint;
                    constrline->setPoints(sp, ep);
                    GeometryFacade::setConstruction(constrline, true);

                    generateId(constrline);
                    newgeoVals.push_back(constrline);

                    Constraint* constNew;

                    if (x == 0) {// first element of a row

                        // add coincidents for construction line
                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = prevrowstartfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::start;
                        newconstrVals.push_back(constNew);

                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = iterfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::end;
                        newconstrVals.push_back(constNew);

                        // it is the first added element of this row in the perpendicular to
                        // displacementvector direction
                        if (y == 1) {
                            rowrefgeoid = cgeoid;
                            cgeoid++;

                            // add length (or equal if perpscale==1) and perpendicular
                            if (perpscale == 1.0) {
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Equal;
                                constNew->First = rowrefgeoid;
                                constNew->FirstPos = Sketcher::PointPos::none;
                                constNew->Second = colrefgeoid;
                                constNew->SecondPos = Sketcher::PointPos::none;
                                newconstrVals.push_back(constNew);
                            }
                            else {
                                constNew = new Constraint();
                                constNew->Type = Sketcher::Distance;
                                constNew->First = rowrefgeoid;
                                constNew->FirstPos = Sketcher::PointPos::none;
                                constNew->setValue(perpendicularDisplacement.Length());
                                newconstrVals.push_back(constNew);
                            }

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Perpendicular;
                            constNew->First = rowrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);
                        }
                        else {// it is just one more element in the col direction
                            cgeoid++;

                            // all other first rowers get an equality and perpendicular constraint
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Equal;
                            constNew->First = rowrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = cgeoid - 1;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Perpendicular;
                            constNew->First = cgeoid - 1;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);
                        }
                    }
                    else {// any element not being the first element of a row

                        // add coincidents for construction line
                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = prevfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::start;
                        newconstrVals.push_back(constNew);

                        constNew = new Constraint();
                        constNew->Type = Sketcher::Coincident;
                        constNew->First = iterfirstgeoid;
                        constNew->FirstPos = refposId;
                        constNew->Second = cgeoid;
                        constNew->SecondPos = Sketcher::PointPos::end;
                        newconstrVals.push_back(constNew);

                        if (y == 0 && x == 1) {// first element of the first row
                            colrefgeoid = cgeoid;
                            cgeoid++;

                            // add length and Angle
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Distance;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->setValue(displacement.Length());
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Angle;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->setValue(atan2(displacement.y, displacement.x));
                            newconstrVals.push_back(constNew);
                        }
                        else {// any other element
                            cgeoid++;

                            // all other elements get an equality and parallel constraint
                            constNew = new Constraint();
                            constNew->Type = Sketcher::Equal;
                            constNew->First = colrefgeoid;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = cgeoid - 1;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);

                            constNew = new Constraint();
                            constNew->Type = Sketcher::Parallel;
                            constNew->First = cgeoid - 1;
                            constNew->FirstPos = Sketcher::PointPos::none;
                            constNew->Second = colrefgeoid;
                            constNew->SecondPos = Sketcher::PointPos::none;
                            newconstrVals.push_back(constNew);
                        }
                    }
                }
                // after each creation reset map so that the key-value is univoque (only for
                // operations other than move)
                geoIdMap.clear();
            }
        }
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newgeoVals));

        if (newconstrVals.size() > constrvals.size())
            Constraints.setValues(std::move(newconstrVals));
    }

    // we inhibited update, so we trigger it now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return Geometry.getSize() - 1;
}

bool SketchObject::convertToNURBS(int GeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId > getHighestCurveIndex()
        || (GeoId < 0 && -GeoId > static_cast<int>(ExternalGeo.getSize())) || GeoId == -1
        || GeoId == -2)
        return false;

    const Part::Geometry* geo = getGeometry(GeoId);

    if (geo->is<Part::GeomPoint>())
        return false;

    const auto* geo1 = static_cast<const Part::GeomCurve*>(geo);

    Part::GeomBSplineCurve* bspline;

    try {
        bspline = geo1->toNurbs(geo1->getFirstParameter(), geo1->getLastParameter());

        if (geo1->isDerivedFrom<Part::GeomArcOfConic>()) {
            const auto* geoaoc = static_cast<const Part::GeomArcOfConic*>(geo1);

            if (geoaoc->isReversed())
                bspline->reverse();
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        // revert to original values
        return false;
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    // Block checks and updates in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);

        if (GeoId < 0) {// external geometry
            newVals.push_back(bspline);
            generateId(bspline);
        }
        else {// normal geometry

            newVals[GeoId] = bspline;
            GeometryFacade::copyId(geo, bspline);

            const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

            std::vector<Constraint*> newcVals(cvals);

            int index = cvals.size() - 1;
            // delete constraints on this elements other than coincident constraints (bspline does
            // not support them currently), except for coincidents on mid point of the
            // to-be-converted curve.
            for (; index >= 0; index--) {
                auto otherthancoincident = cvals[index]->Type != Sketcher::Coincident
                    && (cvals[index]->First == GeoId || cvals[index]->Second == GeoId
                        || cvals[index]->Third == GeoId);

                auto coincidentonmidpoint = cvals[index]->Type == Sketcher::Coincident
                    && ((cvals[index]->First == GeoId
                         && cvals[index]->FirstPos == Sketcher::PointPos::mid)
                        || (cvals[index]->Second == GeoId
                            && cvals[index]->SecondPos == Sketcher::PointPos::mid));

                if (otherthancoincident || coincidentonmidpoint)
                    newcVals.erase(newcVals.begin() + index);
            }

            this->Constraints.setValues(std::move(newcVals));
        }

        Geometry.setValues(std::move(newVals));
    }

    // trigger update now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    return true;
}

bool SketchObject::increaseBSplineDegree(int GeoId, int degreeincrement /*= 1*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        return false;
    }

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>()) {
        return false;
    }

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));

    try {
        int cdegree = bspline->getDegree();

        bspline->increaseDegree(cdegree + degreeincrement);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    GeometryFacade::copyId(geo, bspline.get());
    newVals[GeoId] = bspline.release();

    // AcceptGeometry called from onChanged
    Geometry.setValues(std::move(newVals));

    return true;
}

bool SketchObject::decreaseBSplineDegree(int GeoId, int degreedecrement /*= 1*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return false;

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>())
        return false;

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    const Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(bsp->handle());

    std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));

    try {
        int cdegree = bspline->getDegree();

        // degree must be >= 1
        int maxdegree = cdegree - degreedecrement;
        if (maxdegree == 0)
            return false;
        bspline->approximate(Precision::Confusion(), 20, maxdegree, GeomAbs_C0);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    // FIXME: Avoid to delete the whole geometry but only delete invalid constraints
    // and unused construction geometries
#if 0
    const std::vector< Part::Geometry * > &vals = getInternalGeometry();

    std::vector< Part::Geometry * > newVals(vals);

    newVals[GeoId] = bspline.release();

    // AcceptGeometry called from onChanged
    Geometry.setValues(newVals);
#else
    delGeometry(GeoId);
    int newId = addGeometry(bspline.release());
    exposeInternalGeometry(newId);
#endif

    return true;
}

// clang-format on
bool SketchObject::modifyBSplineKnotMultiplicity(int GeoId, int knotIndex, int multiplicityincr)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "B-spline Geometry Index (GeoID) is out of bounds.")
        );
    }

    if (multiplicityincr == 0) {
        // no change in multiplicity
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "You are requesting no change in knot multiplicity.")
        );
    }

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>()) {
        THROWMT(
            Base::TypeError,
            QT_TRANSLATE_NOOP("Exceptions", "The Geometry Index (GeoId) provided is not a B-spline.")
        );
    }

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    int degree = bsp->getDegree();

    if (knotIndex > bsp->countKnots() || knotIndex < 1) {
        // knotindex in OCC 1 -> countKnots
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP(
                "Exceptions",
                "The knot index is out of bounds. Note that in accordance with "
                "OCC notation, the first knot has index 1 and not zero."
            )
        );
    }

    std::unique_ptr<Part::GeomBSplineCurve> bspline;

    int curmult = bsp->getMultiplicity(knotIndex);

    // zero is removing the knot, degree is just positional continuity
    if ((curmult + multiplicityincr) > degree) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP(
                "Exceptions",
                "The multiplicity cannot be increased beyond the degree of the B-spline."
            )
        );
    }

    // zero is removing the knot, degree is just positional continuity
    if ((curmult + multiplicityincr) < 0) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "The multiplicity cannot be decreased beyond zero.")
        );
    }

    try {
        bspline.reset(static_cast<Part::GeomBSplineCurve*>(bsp->clone()));

        if (multiplicityincr > 0) {  // increase multiplicity
            bspline->increaseMultiplicity(knotIndex, curmult + multiplicityincr);
        }
        else {  // decrease multiplicity
            bool result = bspline->removeKnot(knotIndex, curmult + multiplicityincr, 1E6);

            if (!result) {
                THROWMT(
                    Base::CADKernelError,
                    QT_TRANSLATE_NOOP(
                        "Exceptions",
                        "OCC is unable to decrease the multiplicity within the "
                        "maximum tolerance."
                    )
                );
            }
        }
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    // we succeeded with the multiplicity modification, so alignment geometry may be
    // invalid/inconsistent for the new bspline
    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newPoles = bspline->getPoles();

    // on fully removing a knot the knot geometry changes
    std::vector<double> knots = bsp->getKnots();
    std::vector<double> newKnots = bspline->getKnots();

    std::map<Sketcher::InternalAlignmentType, std::vector<int>> indexInNew {
        {Sketcher::BSplineControlPoint, {}},
        {Sketcher::BSplineKnotPoint, {}}
    };
    indexInNew[Sketcher::BSplineControlPoint].reserve(poles.size());
    indexInNew[Sketcher::BSplineKnotPoint].reserve(knots.size());

    for (const auto& pole : poles) {
        const auto it = std::ranges::find(newPoles, pole);
        indexInNew[Sketcher::BSplineControlPoint].emplace_back(it - newPoles.begin());
    }
    std::ranges::replace(indexInNew[Sketcher::BSplineControlPoint], int(newPoles.size()), -1);

    for (const auto& knot : knots) {
        const auto it = std::ranges::find(newKnots, knot);
        indexInNew[Sketcher::BSplineKnotPoint].emplace_back(it - newKnots.begin());
    }
    std::ranges::replace(indexInNew[Sketcher::BSplineKnotPoint], int(newKnots.size()), -1);

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Constraint*> newcVals(0);

    // modify pole and knot constraints
    for (const auto& constr : cvals) {
        if (!(constr->Type == Sketcher::InternalAlignment && constr->Second == GeoId)) {
            newcVals.push_back(constr);
            continue;
        }

        int index = indexInNew.at(constr->AlignmentType).at(constr->InternalAlignmentIndex);

        if (index == -1) {
            // it is an internal alignment geometry that is no longer valid
            // => delete it and the geometry
            delGeoId.push_back(constr->First);
            continue;
        }

        Constraint* newConstr = constr->clone();
        newConstr->InternalAlignmentIndex = index;
        newcVals.push_back(newConstr);
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    GeometryFacade::copyId(geo, bspline.get());
    newVals[GeoId] = bspline.release();

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newVals));

        this->Constraints.setValues(std::move(newcVals));
    }

    // Trigger update now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    if (!delGeoId.empty()) {
        delGeometriesExclusiveList(delGeoId);
    }
    else {
        Geometry.touch();
    }
    return true;
}

bool SketchObject::insertBSplineKnot(int GeoId, double param, int multiplicity)
{
    // TODO: Check if this is still valid: no need to check input data validity as this is an
    // sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    // handling unacceptable cases
    if (GeoId < 0 || GeoId > getHighestCurveIndex()) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "B-spline Geometry Index (GeoID) is out of bounds.")
        );
    }

    if (multiplicity == 0) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "Knot cannot have zero multiplicity.")
        );
    }

    const Part::Geometry* geo = getGeometry(GeoId);

    if (!geo->is<Part::GeomBSplineCurve>()) {
        THROWMT(
            Base::TypeError,
            QT_TRANSLATE_NOOP("Exceptions", "The Geometry Index (GeoId) provided is not a B-spline.")
        );
    }

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);

    int degree = bsp->getDegree();
    double firstParam = bsp->getFirstParameter();
    double lastParam = bsp->getLastParameter();

    if (multiplicity > degree) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP(
                "Exceptions",
                "Knot multiplicity cannot be higher than the degree of the B-spline."
            )
        );
    }

    if (param > lastParam || param < firstParam) {
        THROWMT(
            Base::ValueError,
            QT_TRANSLATE_NOOP("Exceptions", "Knot cannot be inserted outside the B-spline parameter range.")
        );
    }

    std::unique_ptr<Part::GeomBSplineCurve> bspline;

    // run the command
    try {
        bspline.reset(static_cast<Part::GeomBSplineCurve*>(bsp->clone()));

        bspline->insertKnot(param, multiplicity);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("%s\n", e.what());
        return false;
    }

    // once command is run update the internal geometries
    std::vector<int> delGeoId;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<Base::Vector3d> newPoles = bspline->getPoles();
    std::vector<int> poleIndexInNew(poles.size(), -1);

    for (size_t j = 0; j < poles.size(); j++) {
        const auto it = std::ranges::find(newPoles, poles[j]);
        poleIndexInNew[j] = it - newPoles.begin();
    }
    std::ranges::replace(poleIndexInNew, int(newPoles.size()), -1);

    std::vector<double> knots = bsp->getKnots();
    std::vector<double> newKnots = bspline->getKnots();
    std::vector<int> knotIndexInNew(knots.size(), -1);

    for (size_t j = 0; j < knots.size(); j++) {
        const auto it = std::ranges::find(newKnots, knots[j]);
        knotIndexInNew[j] = it - newKnots.begin();
    }
    std::ranges::replace(knotIndexInNew, int(newKnots.size()), -1);

    const std::vector<Sketcher::Constraint*>& cvals = Constraints.getValues();

    std::vector<Constraint*> newcVals(0);

    // modify pole and knot constraints
    for (const auto& constr : cvals) {
        if (!(constr->Type == Sketcher::InternalAlignment && constr->Second == GeoId)) {
            newcVals.push_back(constr);
            continue;
        }

        std::vector<int>* indexInNew = nullptr;

        if (constr->AlignmentType == Sketcher::BSplineControlPoint) {
            indexInNew = &poleIndexInNew;
        }
        else if (constr->AlignmentType == Sketcher::BSplineKnotPoint) {
            indexInNew = &knotIndexInNew;
        }
        else {
            // it is a bspline geometry, but not a controlpoint or knot
            newcVals.push_back(constr);
            continue;
        }

        if (indexInNew && indexInNew->at(constr->InternalAlignmentIndex) == -1) {
            // it is an internal alignment geometry that is no longer valid
            // => delete it and the pole circle
            delGeoId.push_back(constr->First);
            continue;
        }

        Constraint* newConstr = constr->clone();
        newConstr->InternalAlignmentIndex = indexInNew->at(constr->InternalAlignmentIndex);
        newcVals.push_back(newConstr);
    }

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    GeometryFacade::copyId(geo, bspline.get());
    newVals[GeoId] = bspline.release();

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        Geometry.setValues(std::move(newVals));

        this->Constraints.setValues(std::move(newcVals));
    }

    // Trigger update now
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    if (!delGeoId.empty()) {
        // NOTE: There have been a couple of instances when knot insertion has
        // led to a segmentation fault: see
        // https://forum.freecad.org/viewtopic.php?f=19&t=64962&sid=10272db50a635c633260517b14ecad37.
        // If a segfault happens again and a `Geometry.touch()` here fixes it,
        // it is possible that `delGeometriesExclusiveList` is causing an update
        // in constraint GUI features during an intermediate step.
        // See 247a9f0876a00e08c25b07d1f8802479d8623e87 for suggestions.
        // Geometry.touch();
        delGeometriesExclusiveList(delGeoId);
        return true;
    }

    Geometry.touch();

    return true;
}
