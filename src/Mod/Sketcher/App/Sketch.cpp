/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <cmath>
#include <iostream>

#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRep_Builder.hxx>
#include <Precision.hxx>
#include <ShapeFix_Wire.hxx>
#include <Standard_Version.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/TimeInfo.h>
#include <Base/VectorPy.h>
#include <Base/Writer.h>
#include <Mod/Part/App/ArcOfCirclePy.h>
#include <Mod/Part/App/ArcOfEllipsePy.h>
#include <Mod/Part/App/ArcOfHyperbolaPy.h>
#include <Mod/Part/App/ArcOfParabolaPy.h>
#include <Mod/Part/App/BSplineCurvePy.h>
#include <Mod/Part/App/CirclePy.h>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/HyperbolaPy.h>
#include <Mod/Part/App/LineSegmentPy.h>
#include <Mod/Part/App/ParabolaPy.h>

#include "Constraint.h"
#include "GeometryFacade.h"
#include "Sketch.h"
#include "SolverGeometryExtension.h"


// #define DEBUG_BLOCK_CONSTRAINT
#undef DEBUG_BLOCK_CONSTRAINT

using namespace Sketcher;
using namespace Base;
using namespace Part;

TYPESYSTEM_SOURCE(Sketcher::Sketch, Base::Persistence)

Sketch::Sketch()
    : SolveTime(0)
    , RecalculateInitialSolutionWhileMovingPoint(false)
    , resolveAfterGeometryUpdated(false)
    , GCSsys()
    , ConstraintsCounter(0)
    , isInitMove(false)
    , isFine(true)
    , moveStep(0)
    , defaultSolver(GCS::DogLeg)
    , defaultSolverRedundant(GCS::DogLeg)
    , debugMode(GCS::Minimal)
{}

Sketch::~Sketch()
{
    clear();
}

void Sketch::clear()
{
    // clear all internal data sets
    Points.clear();
    Lines.clear();
    Arcs.clear();
    Circles.clear();
    Ellipses.clear();
    ArcsOfEllipse.clear();
    ArcsOfHyperbola.clear();
    ArcsOfParabola.clear();
    BSplines.clear();
    resolveAfterGeometryUpdated = false;

    // deleting the doubles allocated with new
    for (std::vector<double*>::iterator it = Parameters.begin(); it != Parameters.end(); ++it) {
        if (*it) {
            delete *it;
        }
    }
    Parameters.clear();
    DrivenParameters.clear();
    for (std::vector<double*>::iterator it = FixParameters.begin(); it != FixParameters.end();
         ++it) {
        if (*it) {
            delete *it;
        }
    }
    FixParameters.clear();

    param2geoelement.clear();
    pDependencyGroups.clear();
    solverExtensions.clear();

    internalAlignmentGeometryMap.clear();

    // deleting the geometry copied into this sketch
    for (std::vector<GeoDef>::iterator it = Geoms.begin(); it != Geoms.end(); ++it) {
        if (it->geo) {
            delete it->geo;
        }
    }
    Geoms.clear();

    // deleting the non-Driving constraints copied into this sketch
    // for (std::vector<Constraint *>::iterator it = NonDrivingConstraints.begin(); it !=
    // NonDrivingConstraints.end(); ++it)
    //    if (*it) delete *it;
    Constrs.clear();

    GCSsys.clear();
    isInitMove = false;
    ConstraintsCounter = 0;
    Conflicting.clear();
    Redundant.clear();
    PartiallyRedundant.clear();
    MalformedConstraints.clear();
}

bool Sketch::analyseBlockedGeometry(const std::vector<Part::Geometry*>& internalGeoList,
                                    const std::vector<Constraint*>& constraintList,
                                    std::vector<bool>& onlyblockedGeometry,
                                    std::vector<int>& blockedGeoIds) const
{
    // To understand this function read the documentation in Sketch.h
    // It is important that "onlyblockedGeometry" ONLY identifies blocked geometry
    // that is not affected by any other driving constraint
    bool doesBlockAffectOtherConstraints = false;

    int geoindex = 0;
    for (auto g : internalGeoList) {
        if (GeometryFacade::getBlocked(g)) {
            // is it only affected by one constraint, the block constraint (and this is driving), or
            // by any other driving constraint ?
            bool blockOnly = true;
            bool blockisDriving = false;

            for (auto c : constraintList) {
                // is block driving
                if (c->Type == Sketcher::Block && c->isDriving && c->First == geoindex) {
                    blockisDriving = true;
                }
                // We have another driving constraint (which may be InternalAlignment)
                if (c->Type != Sketcher::Block && c->isDriving
                    && (c->First == geoindex || c->Second == geoindex || c->Third == geoindex)) {
                    blockOnly = false;
                }
            }

            if (blockisDriving) {
                if (blockOnly) {
                    onlyblockedGeometry[geoindex] = true;  // we pre-fix this geometry
                }
                else {
                    // we will have to pos-analyse the first diagnose result for these geometries
                    // in order to avoid redundant constraints
                    doesBlockAffectOtherConstraints = true;
                    blockedGeoIds.push_back(geoindex);
                }
            }
        }
        geoindex++;
    }

    return doesBlockAffectOtherConstraints;
}

int Sketch::setUpSketch(const std::vector<Part::Geometry*>& GeoList,
                        const std::vector<Constraint*>& ConstraintList,
                        int extGeoCount)
{
    Base::TimeInfo start_time;

    clear();

    std::vector<Part::Geometry*> intGeoList, extGeoList;
    for (int i = 0; i < int(GeoList.size()) - extGeoCount; i++) {
        intGeoList.push_back(GeoList[i]);
    }
    for (int i = int(GeoList.size()) - extGeoCount; i < int(GeoList.size()); i++) {
        extGeoList.push_back(GeoList[i]);
    }

    // these geometries are blocked, frozen and sent as fixed parameters to the solver
    std::vector<bool> onlyBlockedGeometry(intGeoList.size(), false);
    // these constraints are unenforceable due to a Blocked constraint
    std::vector<bool> unenforceableConstraints(ConstraintList.size(), false);

    /* This implements the old block constraint. I have decided not to remove it at this time while
     * the new is tested, just in case the change needs to be reverted */
    /*if(!intGeoList.empty())
        getBlockedGeometry(blockedGeometry, unenforceableConstraints, ConstraintList);*/

    // Pre-analysis of blocked geometry (new block constraint) to fix geometry only affected by a
    // block constraint (see comment in Sketch.h)
    std::vector<int> blockedGeoIds;
    bool doesBlockAffectOtherConstraints =
        analyseBlockedGeometry(intGeoList, ConstraintList, onlyBlockedGeometry, blockedGeoIds);

#ifdef DEBUG_BLOCK_CONSTRAINT
    if (doesBlockAffectOtherConstraints) {
        Base::Console().Log("\n  Block interferes with other constraints: Post-analysis required");
    }

    Base::Console().Log("\nOnlyBlocked GeoIds:");
    size_t i = 0;
    bool found = false;
    for (; i < onlyBlockedGeometry.size(); i++) {
        if (onlyBlockedGeometry[i]) {
            Base::Console().Log("\n  GeoId=%d", i);
            found = true;
        }
    }
    if (found) {
        Base::Console().Log("\n  None");
    }

    Base::Console().Log("\nNotOnlyBlocked GeoIds:");
    i = 0;
    for (; i < blockedGeoIds.size(); i++) {
        Base::Console().Log("\n  GeoId=%d", blockedGeoIds[i]);
    }
    if (i == 0) {
        Base::Console().Log("\n  None");
    }
    Base::Console().Log("\n");
#endif  // DEBUG_BLOCK_CONSTRAINT

    buildInternalAlignmentGeometryMap(ConstraintList);

    addGeometry(intGeoList, onlyBlockedGeometry);
    int extStart = Geoms.size();
    addGeometry(extGeoList, true);
    int extEnd = Geoms.size() - 1;
    for (int i = extStart; i <= extEnd; i++) {
        Geoms[i].external = true;
    }

    // The Geoms list might be empty after an undo/redo
    if (!Geoms.empty()) {
        addConstraints(ConstraintList, unenforceableConstraints);
    }
    clearTemporaryConstraints();
    GCSsys.declareUnknowns(Parameters);
    GCSsys.declareDrivenParams(DrivenParameters);
    GCSsys.initSolution(defaultSolverRedundant);

    // Post-analysis
    // Now that we have all the parameters information, we deal properly with the block constraints
    // if necessary
    if (doesBlockAffectOtherConstraints) {

        std::vector<double*> params_to_block;

        bool unsatisfied_groups =
            analyseBlockedConstraintDependentParameters(blockedGeoIds, params_to_block);

        // I am unsure if more than one QR iterations are needed with the current implementation.
        //
        // With previous implementations mostly one QR iteration was enough, but if block constraint
        // is abused, more iterations were needed.
        int index = 0;
        while (unsatisfied_groups) {
            // We tried hard not to arrive to an unsatisfied group, so we try harder
            // This loop has the advantage that the user will notice increased effort to solve,
            // so they may understand that they are abusing the block constraint, while guaranteeing
            // that wrong behaviour of the block constraint is not undetected.

            // Another QR iteration
            fixParametersAndDiagnose(params_to_block);

            unsatisfied_groups =
                analyseBlockedConstraintDependentParameters(blockedGeoIds, params_to_block);

            if (debugMode == GCS::IterationLevel) {
                Base::Console().Log("Sketcher::setUpSketch()-BlockConstraint-PostAnalysis:%d\n",
                                    index);
            }
            index++;
        }

        // 2. If something needs blocking, block-it
        fixParametersAndDiagnose(params_to_block);

#ifdef DEBUG_BLOCK_CONSTRAINT
        if (params_to_block.size() > 0) {
            std::vector<std::vector<double*>> groups;
            GCSsys.getDependentParamsGroups(groups);

            // Debug code block
            for (size_t i = 0; i < groups.size(); i++) {
                Base::Console().Log("\nDepParams: Group %d:", i);
                for (size_t j = 0; j < groups[i].size(); j++) {
                    Base::Console().Log(
                        "\n  Param=%x ,GeoId=%d, GeoPos=%d",
                        param2geoelement.find(*std::next(groups[i].begin(), j))->first,
                        param2geoelement.find(*std::next(groups[i].begin(), j))->second.first,
                        param2geoelement.find(*std::next(groups[i].begin(), j))->second.second);
                }
            }
        }
#endif  // DEBUG_BLOCK_CONSTRAINT
    }

    // Now we set the Sketch status with the latest solver information
    GCSsys.getConflicting(Conflicting);
    GCSsys.getRedundant(Redundant);
    GCSsys.getPartiallyRedundant(PartiallyRedundant);
    GCSsys.getDependentParams(pDependentParametersList);

    calculateDependentParametersElements();

    if (debugMode == GCS::Minimal || debugMode == GCS::IterationLevel) {
        Base::TimeInfo end_time;

        Base::Console().Log("Sketcher::setUpSketch()-T:%s\n",
                            Base::TimeInfo::diffTime(start_time, end_time).c_str());
    }

    return GCSsys.dofsNumber();
}

void Sketch::buildInternalAlignmentGeometryMap(const std::vector<Constraint*>& constraintList)
{
    for (auto* c : constraintList) {
        if (c->Type == InternalAlignment) {
            internalAlignmentGeometryMap[c->First] = c->Second;
        }
    }
}

void Sketch::fixParametersAndDiagnose(std::vector<double*>& params_to_block)
{
    if (!params_to_block.empty()) {  // only there are parameters to fix
        for (auto p : params_to_block) {
            auto findparam = std::find(Parameters.begin(), Parameters.end(), p);

            if (findparam != Parameters.end()) {
                FixParameters.push_back(*findparam);
                Parameters.erase(findparam);
            }
        }

        pDependencyGroups.clear();
        clearTemporaryConstraints();
        GCSsys.invalidatedDiagnosis();
        GCSsys.declareUnknowns(Parameters);
        GCSsys.declareDrivenParams(DrivenParameters);
        GCSsys.initSolution(defaultSolverRedundant);
        /*GCSsys.getConflicting(Conflicting);
        GCSsys.getRedundant(Redundant);
        GCSsys.getPartlyRedundant(PartiallyRedundant);
        GCSsys.getDependentParams(pDependentParametersList);

        calculateDependentParametersElements();*/
    }
}

bool Sketch::analyseBlockedConstraintDependentParameters(
    std::vector<int>& blockedGeoIds,
    std::vector<double*>& params_to_block) const
{
    // 1. Retrieve solver information
    std::vector<std::vector<double*>> groups;
    GCSsys.getDependentParamsGroups(groups);

    // 2. Determine blockable parameters for each group (see documentation in header file).
    struct group
    {
        std::vector<double*> blockable_params_in_group;
        double* blocking_param_in_group = nullptr;
    };

    std::vector<group> prop_groups(groups.size());

#ifdef DEBUG_BLOCK_CONSTRAINT
    for (size_t i = 0; i < groups.size(); i++) {
        Base::Console().Log("\nDepParams: Group %d:", i);
        for (size_t j = 0; j < groups[i].size(); j++) {
            Base::Console().Log(
                "\n  Param=%x ,GeoId=%d, GeoPos=%d",
                param2geoelement.find(*std::next(groups[i].begin(), j))->first,
                param2geoelement.find(*std::next(groups[i].begin(), j))->second.first,
                param2geoelement.find(*std::next(groups[i].begin(), j))->second.second);
        }
    }
#endif  // DEBUG_BLOCK_CONSTRAINT

    for (size_t i = 0; i < groups.size(); i++) {
        for (size_t j = 0; j < groups[i].size(); j++) {

            double* thisparam = *std::next(groups[i].begin(), j);

            auto element = param2geoelement.find(thisparam);

            if (element != param2geoelement.end()) {

                auto blockable = std::find(blockedGeoIds.begin(),
                                           blockedGeoIds.end(),
                                           std::get<0>(element->second));

                if (blockable != blockedGeoIds.end()) {
                    // This dependent parameter group contains at least one parameter that should be
                    // blocked, so added to the blockable list.
                    prop_groups[i].blockable_params_in_group.push_back(thisparam);
                }
            }
        }
    }

    // 3. Apply heuristic - pick the last blockable param available to block the group, starting
    // from the last group
    for (size_t i = prop_groups.size(); i-- > 0;) {
        for (size_t j = prop_groups[i].blockable_params_in_group.size(); j-- > 0;) {
            // check if parameter is already satisfying one group
            double* thisparam = prop_groups[i].blockable_params_in_group[j];
            auto pos = std::find(params_to_block.begin(), params_to_block.end(), thisparam);

            if (pos == params_to_block.end()) {  // not found, so add
                params_to_block.push_back(thisparam);
                prop_groups[i].blocking_param_in_group = thisparam;
#ifdef DEBUG_BLOCK_CONSTRAINT
                Base::Console().Log("\nTentatively blocking group %d, with param=%x", i, thisparam);
#endif  // DEBUG_BLOCK_CONSTRAINT
                break;
            }
        }
    }

    // 4. Check if groups are satisfied or are licitly unsatisfiable and thus deemed as satisfied
    bool unsatisfied_groups = false;
    for (size_t i = 0; i < prop_groups.size(); i++) {
        // 4.1. unsatisfiable group
        if (prop_groups[i].blockable_params_in_group.empty()) {
            // this group does not contain any blockable parameter, so it is by definition satisfied
            // (or impossible to satisfy by block constraints)
            continue;
        }
        // 4.2. satisfiable and not satisfied
        if (!prop_groups[i].blocking_param_in_group) {
            unsatisfied_groups = true;
        }
    }

    return unsatisfied_groups;
}


void Sketch::clearTemporaryConstraints()
{
    GCSsys.clearByTag(GCS::DefaultTemporaryConstraint);
}

void Sketch::calculateDependentParametersElements()
{
    // initialize solve extensions to a know state
    solverExtensions.resize(Geoms.size());

    int i = 0;
    for (auto geo : Geoms) {

        if (!geo.geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {
            geo.geo->setExtension(std::make_unique<Sketcher::SolverGeometryExtension>());
        }

        auto solvext = std::static_pointer_cast<Sketcher::SolverGeometryExtension>(
            geo.geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

        if (GCSsys.isEmptyDiagnoseMatrix()) {
            solvext->init(SolverGeometryExtension::Dependent);
        }
        else {
            solvext->init(SolverGeometryExtension::Independent);
        }

        solverExtensions[i] = solvext;
        i++;
    }

    for (auto param : pDependentParametersList) {

        // auto element = param2geoelement.at(param);
        auto element = param2geoelement.find(param);

        if (element != param2geoelement.end()) {
            auto geoid = std::get<0>(element->second);
            auto geopos = std::get<1>(element->second);
            auto solvext = std::static_pointer_cast<Sketcher::SolverGeometryExtension>(
                Geoms[geoid]
                    .geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId())
                    .lock());

            auto index = std::get<2>(element->second);

            switch (geopos) {
                case PointPos::none:
                    solvext->setEdge(index, SolverGeometryExtension::Dependent);
                    break;
                case PointPos::start:
                    if (index == 0) {
                        solvext->setStartx(SolverGeometryExtension::Dependent);
                    }
                    else {
                        solvext->setStarty(SolverGeometryExtension::Dependent);
                    }
                    break;
                case PointPos::end:
                    if (index == 0) {
                        solvext->setEndx(SolverGeometryExtension::Dependent);
                    }
                    else {
                        solvext->setEndy(SolverGeometryExtension::Dependent);
                    }
                    break;
                case PointPos::mid:
                    if (index == 0) {
                        solvext->setMidx(SolverGeometryExtension::Dependent);
                    }
                    else {
                        solvext->setMidy(SolverGeometryExtension::Dependent);
                    }
                    break;
            }
        }
    }

    std::vector<std::vector<double*>> groups;
    GCSsys.getDependentParamsGroups(groups);

    pDependencyGroups.resize(groups.size());

    // translate parameters into elements (Geoid, PointPos)
    for (size_t i = 0; i < groups.size(); i++) {
        for (size_t j = 0; j < groups[i].size(); j++) {

            auto element = param2geoelement.find(groups[i][j]);

            if (element != param2geoelement.end()) {
                pDependencyGroups[i].insert(
                    std::pair(std::get<0>(element->second), std::get<1>(element->second)));
            }
        }
    }

    // check if groups have a common element, if yes merge the groups
    auto havecommonelement = [](std::set<std::pair<int, Sketcher::PointPos>>::iterator begin1,
                                std::set<std::pair<int, Sketcher::PointPos>>::iterator end1,
                                std::set<std::pair<int, Sketcher::PointPos>>::iterator begin2,
                                std::set<std::pair<int, Sketcher::PointPos>>::iterator end2) {
        while (begin1 != end1 && begin2 != end2) {
            if (*begin1 < *begin2) {
                ++begin1;
            }
            else if (*begin2 < *begin1) {
                ++begin2;
            }
            else {
                return true;
            }
        }

        return false;
    };

    if (pDependencyGroups.size() > 1) {  // only if there is more than 1 group
        size_t endcount = pDependencyGroups.size() - 1;

        for (size_t i = 0; i < endcount; i++) {
            if (havecommonelement(pDependencyGroups[i].begin(),
                                  pDependencyGroups[i].end(),
                                  pDependencyGroups[i + 1].begin(),
                                  pDependencyGroups[i + 1].end())) {
                pDependencyGroups[i].insert(pDependencyGroups[i + 1].begin(),
                                            pDependencyGroups[i + 1].end());
                pDependencyGroups.erase(pDependencyGroups.begin() + i + 1);
                endcount--;
            }
        }
    }
}

std::set<std::pair<int, Sketcher::PointPos>> Sketch::getDependencyGroup(int geoId,
                                                                        PointPos pos) const
{
    geoId = checkGeoId(geoId);

    std::set<std::pair<int, Sketcher::PointPos>> group;

    auto key = std::make_pair(geoId, pos);

    for (auto& set : pDependencyGroups) {
        if (set.find(key) != set.end()) {
            group = set;
            break;
        }
    }

    return group;
}

std::shared_ptr<SolverGeometryExtension> Sketch::getSolverExtension(int geoId) const
{
    if (geoId >= 0 && geoId < int(solverExtensions.size())) {
        return solverExtensions[geoId];
    }

    return nullptr;
}

int Sketch::resetSolver()
{
    clearTemporaryConstraints();
    GCSsys.declareUnknowns(Parameters);
    GCSsys.declareDrivenParams(DrivenParameters);
    GCSsys.initSolution(defaultSolverRedundant);
    GCSsys.getConflicting(Conflicting);
    GCSsys.getRedundant(Redundant);
    GCSsys.getPartiallyRedundant(PartiallyRedundant);
    GCSsys.getDependentParams(pDependentParametersList);

    calculateDependentParametersElements();

    return GCSsys.dofsNumber();
}

const char* nameByType(Sketch::GeoType type)
{
    switch (type) {
        case Sketch::Point:
            return "point";
        case Sketch::Line:
            return "line";
        case Sketch::Arc:
            return "arc";
        case Sketch::Circle:
            return "circle";
        case Sketch::Ellipse:
            return "ellipse";
        case Sketch::ArcOfEllipse:
            return "arcofellipse";
        case Sketch::ArcOfHyperbola:
            return "arcofhyperbola";
        case Sketch::ArcOfParabola:
            return "arcofparabola";
        case Sketch::BSpline:
            return "bspline";
        case Sketch::None:
        default:
            return "unknown";
    }
}

// Geometry adding ==========================================================

int Sketch::addGeometry(const Part::Geometry* geo, bool fixed)
{
    if (geo->getTypeId() == GeomPoint::getClassTypeId()) {  // add a point
        const GeomPoint* point = static_cast<const GeomPoint*>(geo);
        auto pointf = GeometryFacade::getFacade(point);
        // create the definition struct for that geom
        return addPoint(*point, fixed);
    }
    else if (geo->getTypeId() == GeomLineSegment::getClassTypeId()) {  // add a line
        const GeomLineSegment* lineSeg = static_cast<const GeomLineSegment*>(geo);
        // create the definition struct for that geom
        return addLineSegment(*lineSeg, fixed);
    }
    else if (geo->getTypeId() == GeomCircle::getClassTypeId()) {  // add a circle
        const GeomCircle* circle = static_cast<const GeomCircle*>(geo);
        // create the definition struct for that geom
        return addCircle(*circle, fixed);
    }
    else if (geo->getTypeId() == GeomEllipse::getClassTypeId()) {  // add a ellipse
        const GeomEllipse* ellipse = static_cast<const GeomEllipse*>(geo);
        // create the definition struct for that geom
        return addEllipse(*ellipse, fixed);
    }
    else if (geo->getTypeId() == GeomArcOfCircle::getClassTypeId()) {  // add an arc
        const GeomArcOfCircle* aoc = static_cast<const GeomArcOfCircle*>(geo);
        // create the definition struct for that geom
        return addArc(*aoc, fixed);
    }
    else if (geo->getTypeId() == GeomArcOfEllipse::getClassTypeId()) {  // add an arc
        const GeomArcOfEllipse* aoe = static_cast<const GeomArcOfEllipse*>(geo);
        // create the definition struct for that geom
        return addArcOfEllipse(*aoe, fixed);
    }
    else if (geo->getTypeId() == GeomArcOfHyperbola::getClassTypeId()) {  // add an arc of hyperbola
        const GeomArcOfHyperbola* aoh = static_cast<const GeomArcOfHyperbola*>(geo);
        // create the definition struct for that geom
        return addArcOfHyperbola(*aoh, fixed);
    }
    else if (geo->getTypeId() == GeomArcOfParabola::getClassTypeId()) {  // add an arc of parabola
        const GeomArcOfParabola* aop = static_cast<const GeomArcOfParabola*>(geo);
        // create the definition struct for that geom
        return addArcOfParabola(*aop, fixed);
    }
    else if (geo->getTypeId() == GeomBSplineCurve::getClassTypeId()) {  // add a bspline
        const GeomBSplineCurve* bsp = static_cast<const GeomBSplineCurve*>(geo);

        // Current B-Spline implementation relies on OCCT calculations, so a second solve
        // is necessary to update actual solver implementation to account for changes in B-Spline
        // geometry
        resolveAfterGeometryUpdated = true;
        return addBSpline(*bsp, fixed);
    }
    else {
        throw Base::TypeError(
            "Sketch::addGeometry(): Unknown or unsupported type added to a sketch");
    }
}

int Sketch::addGeometry(const std::vector<Part::Geometry*>& geo, bool fixed)
{
    int ret = -1;
    for (std::vector<Part::Geometry*>::const_iterator it = geo.begin(); it != geo.end(); ++it) {
        ret = addGeometry(*it, fixed);
    }
    return ret;
}

int Sketch::addGeometry(const std::vector<Part::Geometry*>& geo,
                        const std::vector<bool>& blockedGeometry)
{
    assert(geo.size() == blockedGeometry.size());

    int ret = -1;
    std::vector<Part::Geometry*>::const_iterator it;
    std::vector<bool>::const_iterator bit;

    for (it = geo.begin(), bit = blockedGeometry.begin();
         it != geo.end() && bit != blockedGeometry.end();
         ++it, ++bit) {
        ret = addGeometry(*it, *bit);
    }
    return ret;
}

int Sketch::addPoint(const Part::GeomPoint& point, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomPoint* p = static_cast<GeomPoint*>(point.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = p;
    def.type = Point;

    // set the parameter for the solver
    params.push_back(new double(p->getPoint().x));
    params.push_back(new double(p->getPoint().y));

    // set the points for later constraints
    GCS::Point p1;
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];
    def.startPointId = Points.size();
    def.endPointId = Points.size();
    def.midPointId = Points.size();
    Points.push_back(p1);

    // store complete set
    Geoms.push_back(def);

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addLine(const Part::GeomLineSegment& /*line*/, bool /*fixed*/)
{
    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addLineSegment(const Part::GeomLineSegment& lineSegment, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomLineSegment* lineSeg = static_cast<GeomLineSegment*>(lineSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = lineSeg;
    def.type = Line;

    // get the points from the line
    Base::Vector3d start = lineSeg->getStartPoint();
    Base::Vector3d end = lineSeg->getEndPoint();

    // the points for later constraints
    GCS::Point p1, p2;

    params.push_back(new double(start.x));
    params.push_back(new double(start.y));
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];

    params.push_back(new double(end.x));
    params.push_back(new double(end.y));
    p2.x = params[params.size() - 2];
    p2.y = params[params.size() - 1];

    // add the points
    def.startPointId = Points.size();
    def.endPointId = Points.size() + 1;
    Points.push_back(p1);
    Points.push_back(p2);

    // set the line for later constraints
    GCS::Line l;
    l.p1 = p1;
    l.p2 = p2;
    def.index = Lines.size();
    Lines.push_back(l);

    // store complete set
    Geoms.push_back(def);

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 1));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addArc(const Part::GeomArcOfCircle& circleSegment, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfCircle* aoc = static_cast<GeomArcOfCircle*>(circleSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = aoc;
    def.type = Arc;

    Base::Vector3d center = aoc->getCenter();
    Base::Vector3d startPnt = aoc->getStartPoint(/*emulateCCW=*/true);
    Base::Vector3d endPnt = aoc->getEndPoint(/*emulateCCW=*/true);
    double radius = aoc->getRadius();
    double startAngle, endAngle;
    aoc->getRange(startAngle, endAngle, /*emulateCCW=*/true);

    GCS::Point p1, p2, p3;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size() - 2];
    p2.y = params[params.size() - 1];

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p3.x = params[params.size() - 2];
    p3.y = params[params.size() - 1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    params.push_back(new double(radius));
    double* r = params[params.size() - 1];
    params.push_back(new double(startAngle));
    double* a1 = params[params.size() - 1];
    params.push_back(new double(endAngle));
    double* a2 = params[params.size() - 1];

    // set the arc for later constraints
    GCS::Arc a;
    a.start = p1;
    a.end = p2;
    a.center = p3;
    a.rad = r;
    a.startAngle = a1;
    a.endAngle = a2;
    def.index = Arcs.size();
    Arcs.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed) {
        GCSsys.addConstraintArcRules(a);
    }

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(r),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a1),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a2),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 2));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addArcOfEllipse(const Part::GeomArcOfEllipse& ellipseSegment, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfEllipse* aoe = static_cast<GeomArcOfEllipse*>(ellipseSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = aoe;
    def.type = ArcOfEllipse;

    Base::Vector3d center = aoe->getCenter();
    Base::Vector3d startPnt = aoe->getStartPoint(/*emulateCCW=*/true);
    Base::Vector3d endPnt = aoe->getEndPoint(/*emulateCCW=*/true);
    double radmaj = aoe->getMajorRadius();
    double radmin = aoe->getMinorRadius();
    Base::Vector3d radmajdir = aoe->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj * radmaj - radmin * radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F * radmajdir;

    double startAngle, endAngle;
    aoe->getRange(startAngle, endAngle, /*emulateCCW=*/true);

    GCS::Point p1, p2, p3;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size() - 2];
    p2.y = params[params.size() - 1];

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p3.x = params[params.size() - 2];
    p3.y = params[params.size() - 1];

    params.push_back(new double(focus1.x));
    params.push_back(new double(focus1.y));
    double* f1X = params[params.size() - 2];
    double* f1Y = params[params.size() - 1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    // Points.push_back(f1);

    // add the radius parameters
    params.push_back(new double(radmin));
    double* rmin = params[params.size() - 1];
    params.push_back(new double(startAngle));
    double* a1 = params[params.size() - 1];
    params.push_back(new double(endAngle));
    double* a2 = params[params.size() - 1];

    // set the arc for later constraints
    GCS::ArcOfEllipse a;
    a.start = p1;
    a.end = p2;
    a.center = p3;
    a.focus1.x = f1X;
    a.focus1.y = f1Y;
    a.radmin = rmin;
    a.startAngle = a1;
    a.endAngle = a2;
    def.index = ArcsOfEllipse.size();
    ArcsOfEllipse.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed) {
        GCSsys.addConstraintArcOfEllipseRules(a);
    }

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(f1X),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(f1Y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(rmin),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 2));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a1),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 3));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a2),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 4));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addArcOfHyperbola(const Part::GeomArcOfHyperbola& hyperbolaSegment, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfHyperbola* aoh = static_cast<GeomArcOfHyperbola*>(hyperbolaSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = aoh;
    def.type = ArcOfHyperbola;

    Base::Vector3d center = aoh->getCenter();
    Base::Vector3d startPnt = aoh->getStartPoint();
    Base::Vector3d endPnt = aoh->getEndPoint();
    double radmaj = aoh->getMajorRadius();
    double radmin = aoh->getMinorRadius();
    Base::Vector3d radmajdir = aoh->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj * radmaj + radmin * radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F * radmajdir;  //+x

    double startAngle, endAngle;
    aoh->getRange(startAngle, endAngle, /*emulateCCW=*/true);

    GCS::Point p1, p2, p3;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size() - 2];
    p2.y = params[params.size() - 1];

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p3.x = params[params.size() - 2];
    p3.y = params[params.size() - 1];

    params.push_back(new double(focus1.x));
    params.push_back(new double(focus1.y));
    double* f1X = params[params.size() - 2];
    double* f1Y = params[params.size() - 1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    // add the radius parameters
    params.push_back(new double(radmin));
    double* rmin = params[params.size() - 1];
    params.push_back(new double(startAngle));
    double* a1 = params[params.size() - 1];
    params.push_back(new double(endAngle));
    double* a2 = params[params.size() - 1];

    // set the arc for later constraints
    GCS::ArcOfHyperbola a;
    a.start = p1;
    a.end = p2;
    a.center = p3;
    a.focus1.x = f1X;
    a.focus1.y = f1Y;
    a.radmin = rmin;
    a.startAngle = a1;
    a.endAngle = a2;
    def.index = ArcsOfHyperbola.size();
    ArcsOfHyperbola.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed) {
        GCSsys.addConstraintArcOfHyperbolaRules(a);
    }

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(f1X),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(f1Y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(rmin),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 2));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a1),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 3));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a2),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 4));
    }


    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addArcOfParabola(const Part::GeomArcOfParabola& parabolaSegment, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomArcOfParabola* aop = static_cast<GeomArcOfParabola*>(parabolaSegment.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = aop;
    def.type = ArcOfParabola;

    Base::Vector3d vertex = aop->getCenter();
    Base::Vector3d startPnt = aop->getStartPoint();
    Base::Vector3d endPnt = aop->getEndPoint();
    Base::Vector3d focus = aop->getFocus();

    double startAngle, endAngle;
    aop->getRange(startAngle, endAngle, /*emulateCCW=*/true);

    GCS::Point p1, p2, p3, p4;

    params.push_back(new double(startPnt.x));
    params.push_back(new double(startPnt.y));
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];

    params.push_back(new double(endPnt.x));
    params.push_back(new double(endPnt.y));
    p2.x = params[params.size() - 2];
    p2.y = params[params.size() - 1];

    params.push_back(new double(vertex.x));
    params.push_back(new double(vertex.y));
    p3.x = params[params.size() - 2];
    p3.y = params[params.size() - 1];

    params.push_back(new double(focus.x));
    params.push_back(new double(focus.y));
    p4.x = params[params.size() - 2];
    p4.y = params[params.size() - 1];

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);
    def.midPointId = Points.size();
    Points.push_back(p3);

    // add the radius parameters
    params.push_back(new double(startAngle));
    double* a1 = params[params.size() - 1];
    params.push_back(new double(endAngle));
    double* a2 = params[params.size() - 1];

    // set the arc for later constraints
    GCS::ArcOfParabola a;
    a.start = p1;
    a.end = p2;
    a.vertex = p3;
    a.focus1 = p4;
    a.startAngle = a1;
    a.endAngle = a2;
    def.index = ArcsOfParabola.size();
    ArcsOfParabola.push_back(a);

    // store complete set
    Geoms.push_back(def);

    // arcs require an ArcRules constraint for the end points
    if (!fixed) {
        GCSsys.addConstraintArcOfParabolaRules(a);
    }

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p3.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p4.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p4.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a1),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 2));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(a2),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 3));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addBSpline(const Part::GeomBSplineCurve& bspline, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomBSplineCurve* bsp = static_cast<GeomBSplineCurve*>(bspline.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = bsp;
    def.type = BSpline;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<double> weights = bsp->getWeights();
    std::vector<double> knots = bsp->getKnots();
    std::vector<int> mult = bsp->getMultiplicities();
    int degree = bsp->getDegree();
    bool periodic = bsp->isPeriodic();

    // OCC hack
    // c means there is a constraint on that weight, nc no constraint
    // OCC provides normalized weights when polynomic [1 1 1] [c c c] and unnormalized weights when
    // rational [5 1 5] [c nc c] then when changing from polynomic to rational, after the first
    // solve any not-constrained pole circle gets normalized to 1. This only happens when changing
    // from polynomic to rational, any subsequent change remains unnormalized [5 1 5] [c nc nc] This
    // creates a visual problem that one of the poles shrinks to 1 mm when deleting an equality
    // constraint.

    int lastoneindex = -1;
    int countones = 0;
    double lastnotone = 1.0;

    for (size_t i = 0; i < weights.size(); i++) {
        if (weights[i] != 1.0) {
            lastnotone = weights[i];
        }
        else {  // is 1.0
            lastoneindex = i;
            countones++;
        }
    }

    if (countones == 1) {
        weights[lastoneindex] = (lastnotone * 0.99);
    }

    // end hack

    Base::Vector3d startPnt = bsp->getStartPoint();
    Base::Vector3d endPnt = bsp->getEndPoint();

    std::vector<GCS::Point> spoles;

    int i = 0;
    for (std::vector<Base::Vector3d>::const_iterator it = poles.begin(); it != poles.end(); ++it) {
        params.push_back(new double((*it).x));
        params.push_back(new double((*it).y));

        GCS::Point p;
        p.x = params[params.size() - 2];
        p.y = params[params.size() - 1];

        spoles.push_back(p);

        if (!fixed) {
            param2geoelement.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(p.x),
                std::forward_as_tuple(Geoms.size(), Sketcher::PointPos::none, i++));
            param2geoelement.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(p.y),
                std::forward_as_tuple(Geoms.size(), Sketcher::PointPos::none, i++));
        }
    }

    std::vector<double*> sweights;

    for (std::vector<double>::const_iterator it = weights.begin(); it != weights.end(); ++it) {
        auto r = new double((*it));
        params.push_back(r);
        sweights.push_back(params[params.size() - 1]);

        if (!fixed) {
            param2geoelement.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(r),
                std::forward_as_tuple(Geoms.size(), Sketcher::PointPos::none, i++));
        }
    }

    std::vector<double*> sknots;

    for (std::vector<double>::const_iterator it = knots.begin(); it != knots.end(); ++it) {
        double* knot = new double((*it));
        // params.push_back(knot);
        sknots.push_back(knot);
    }

    GCS::Point p1, p2;

    double* p1x = new double(startPnt.x);
    double* p1y = new double(startPnt.y);

    // If periodic, startpoint and endpoint do not play a role in the solver, this can remove
    // unnecessary DoF of determining where in the curve the start and the stop should be. However,
    // since start and end points are placed above knots, removing them leads to that knot being
    // unusable.
    params.push_back(p1x);
    params.push_back(p1y);

    p1.x = p1x;
    p1.y = p1y;

    double* p2x = new double(endPnt.x);
    double* p2y = new double(endPnt.y);

    // If periodic, startpoint and endpoint do not play a role in the solver, this can remove
    // unnecessary DoF of determining where in the curve the start and the stop should be. However,
    // since start and end points are placed above knots, removing them leads to that knot being
    // unusable.
    params.push_back(p2x);
    params.push_back(p2y);

    p2.x = p2x;
    p2.y = p2y;

    def.startPointId = Points.size();
    Points.push_back(p1);
    def.endPointId = Points.size();
    Points.push_back(p2);

    GCS::BSpline bs;
    bs.start = p1;
    bs.end = p2;
    bs.poles = spoles;
    bs.weights = sweights;
    bs.knots = sknots;
    bs.mult = mult;
    bs.degree = degree;
    bs.periodic = periodic;
    def.index = BSplines.size();

    // non-solver related, just to enable initialization of knotspoints which is not a parameter of
    // the solver
    bs.knotpointGeoids.resize(knots.size());

    for (std::vector<int>::iterator it = bs.knotpointGeoids.begin(); it != bs.knotpointGeoids.end();
         ++it) {
        (*it) = GeoEnum::GeoUndef;
    }

    BSplines.push_back(bs);

    // store complete set
    Geoms.push_back(def);

    // WARNING: This is only valid where the multiplicity of the endpoints conforms with a BSpline
    // only then the startpoint is the first control point and the endpoint is the last control
    // point accordingly, it is never the case for a periodic BSpline. NOTE: For an external
    // B-spline (i.e. fixed=true) we must not set the coincident constraints as the points are not
    // movable anyway. See #issue 0003176: Sketcher: always over-constrained when referencing
    // external B-Spline
    if (!fixed && !bs.periodic) {
        if (bs.mult[0] > bs.degree) {
            GCSsys.addConstraintP2PCoincident(*(bs.poles.begin()), bs.start);
        }
        if (bs.mult[mult.size() - 1] > bs.degree) {
            GCSsys.addConstraintP2PCoincident(*(bs.poles.end() - 1), bs.end);
        }
    }

    if (!fixed) {
        // Note: Poles and weight parameters are emplaced above
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::start, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p2.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::end, 1));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addCircle(const Part::GeomCircle& cir, bool fixed)
{
    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomCircle* circ = static_cast<GeomCircle*>(cir.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = circ;
    def.type = Circle;

    Base::Vector3d center = circ->getCenter();
    double radius = circ->getRadius();

    GCS::Point p1;

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    p1.x = params[params.size() - 2];
    p1.y = params[params.size() - 1];

    params.push_back(new double(radius));

    def.midPointId = Points.size();
    Points.push_back(p1);

    // add the radius parameter
    double* r = params[params.size() - 1];

    // set the circle for later constraints
    GCS::Circle c;
    c.center = p1;
    c.rad = r;
    def.index = Circles.size();
    Circles.push_back(c);

    // store complete set
    Geoms.push_back(def);

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(p1.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(r),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 0));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int Sketch::addEllipse(const Part::GeomEllipse& elip, bool fixed)
{

    std::vector<double*>& params = fixed ? FixParameters : Parameters;

    // create our own copy
    GeomEllipse* elips = static_cast<GeomEllipse*>(elip.clone());
    // create the definition struct for that geom
    GeoDef def;
    def.geo = elips;
    def.type = Ellipse;

    Base::Vector3d center = elips->getCenter();
    double radmaj = elips->getMajorRadius();
    double radmin = elips->getMinorRadius();
    Base::Vector3d radmajdir = elips->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj * radmaj - radmin * radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F * radmajdir;  //+x
    // double *radmin;

    GCS::Point c;

    params.push_back(new double(center.x));
    params.push_back(new double(center.y));
    c.x = params[params.size() - 2];
    c.y = params[params.size() - 1];

    def.midPointId = Points.size();  // this takes midPointId+1
    Points.push_back(c);

    params.push_back(new double(focus1.x));
    params.push_back(new double(focus1.y));
    double* f1X = params[params.size() - 2];
    double* f1Y = params[params.size() - 1];

    // add the radius parameters
    params.push_back(new double(radmin));
    double* rmin = params[params.size() - 1];

    // set the ellipse for later constraints
    GCS::Ellipse e;
    e.focus1.x = f1X;
    e.focus1.y = f1Y;
    e.center = c;
    e.radmin = rmin;

    def.index = Ellipses.size();
    Ellipses.push_back(e);

    // store complete set
    Geoms.push_back(def);

    if (!fixed) {
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(c.x),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(c.y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::mid, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(f1X),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 0));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(f1Y),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 1));
        param2geoelement.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(rmin),
            std::forward_as_tuple(Geoms.size() - 1, Sketcher::PointPos::none, 2));
    }

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

std::vector<Part::Geometry*> Sketch::extractGeometry(bool withConstructionElements,
                                                     bool withExternalElements) const
{
    std::vector<Part::Geometry*> temp;
    temp.reserve(Geoms.size());
    for (std::vector<GeoDef>::const_iterator it = Geoms.begin(); it != Geoms.end(); ++it) {
        auto gf = GeometryFacade::getFacade(it->geo);
        if ((!it->external || withExternalElements)
            && (!gf->getConstruction() || withConstructionElements)) {
            temp.push_back(it->geo->clone());
        }
    }

    return temp;
}

GeoListFacade Sketch::extractGeoListFacade() const
{
    std::vector<GeometryFacadeUniquePtr> temp;
    temp.reserve(Geoms.size());
    int internalGeometryCount = 0;
    for (std::vector<GeoDef>::const_iterator it = Geoms.begin(); it != Geoms.end(); ++it) {
        // GeometryFacade is the owner of this allocation
        auto gf = GeometryFacade::getFacade(it->geo->clone(), true);
        if (!it->external) {
            internalGeometryCount++;
        }

        temp.push_back(std::move(gf));
    }

    return GeoListFacade::getGeoListModel(std::move(temp), internalGeometryCount);
}

void Sketch::updateExtension(int geoId, std::unique_ptr<Part::GeometryExtension>&& ext)
{
    geoId = checkGeoId(geoId);

    Geoms[geoId].geo->setExtension(std::move(ext));
}

Py::Tuple Sketch::getPyGeometry() const
{
    Py::Tuple tuple(Geoms.size());
    int i = 0;
    for (std::vector<GeoDef>::const_iterator it = Geoms.begin(); it != Geoms.end(); ++it, i++) {
        if (it->type == Point) {
            Base::Vector3d temp(*(Points[it->startPointId].x), *(Points[it->startPointId].y), 0);
            tuple[i] = Py::asObject(new VectorPy(temp));
        }
        else if (it->type == Line) {
            GeomLineSegment* lineSeg = static_cast<GeomLineSegment*>(it->geo->clone());
            tuple[i] = Py::asObject(new LineSegmentPy(lineSeg));
        }
        else if (it->type == Arc) {
            GeomArcOfCircle* aoc = static_cast<GeomArcOfCircle*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfCirclePy(aoc));
        }
        else if (it->type == Circle) {
            GeomCircle* circle = static_cast<GeomCircle*>(it->geo->clone());
            tuple[i] = Py::asObject(new CirclePy(circle));
        }
        else if (it->type == Ellipse) {
            GeomEllipse* ellipse = static_cast<GeomEllipse*>(it->geo->clone());
            tuple[i] = Py::asObject(new EllipsePy(ellipse));
        }
        else if (it->type == ArcOfEllipse) {
            GeomArcOfEllipse* ellipse = static_cast<GeomArcOfEllipse*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfEllipsePy(ellipse));
        }
        else if (it->type == ArcOfHyperbola) {
            GeomArcOfHyperbola* aoh = static_cast<GeomArcOfHyperbola*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfHyperbolaPy(aoh));
        }
        else if (it->type == ArcOfParabola) {
            GeomArcOfParabola* aop = static_cast<GeomArcOfParabola*>(it->geo->clone());
            tuple[i] = Py::asObject(new ArcOfParabolaPy(aop));
        }
        else if (it->type == BSpline) {
            GeomBSplineCurve* bsp = static_cast<GeomBSplineCurve*>(it->geo->clone());
            tuple[i] = Py::asObject(new BSplineCurvePy(bsp));
        }
        else {
            // not implemented type in the sketch!
        }
    }
    return tuple;
}

int Sketch::checkGeoId(int geoId) const
{
    if (geoId < 0) {
        geoId += Geoms.size();  // convert negative external-geometry index to index into Geoms
    }
    if (!(geoId >= 0 && geoId < int(Geoms.size()))) {
        throw Base::IndexError("Sketch::checkGeoId. GeoId index out range.");
    }
    return geoId;
}

GCS::Curve* Sketch::getGCSCurveByGeoId(int geoId)
{
    geoId = checkGeoId(geoId);
    switch (Geoms[geoId].type) {
        case Line:
            return &Lines[Geoms[geoId].index];
            break;
        case Circle:
            return &Circles[Geoms[geoId].index];
            break;
        case Arc:
            return &Arcs[Geoms[geoId].index];
            break;
        case Ellipse:
            return &Ellipses[Geoms[geoId].index];
            break;
        case ArcOfEllipse:
            return &ArcsOfEllipse[Geoms[geoId].index];
            break;
        case ArcOfHyperbola:
            return &ArcsOfHyperbola[Geoms[geoId].index];
            break;
        case ArcOfParabola:
            return &ArcsOfParabola[Geoms[geoId].index];
            break;
        case BSpline:
            return &BSplines[Geoms[geoId].index];
            break;
        default:
            return nullptr;
    };
}

const GCS::Curve* Sketch::getGCSCurveByGeoId(int geoId) const
{
    // I hereby guarantee that if I modify the non-const version, I will still
    // never modify (this). I return const copy to enforce on my users.
    return const_cast<Sketch*>(this)->getGCSCurveByGeoId(geoId);
}

// constraint adding ==========================================================

int Sketch::addConstraint(const Constraint* constraint)
{
    if (Geoms.empty()) {
        throw Base::ValueError(
            "Sketch::addConstraint. Can't add constraint to a sketch with no geometry!");
    }
    int rtn = -1;

    ConstrDef c;
    c.constr = const_cast<Constraint*>(constraint);
    c.driving = constraint->isDriving;

    switch (constraint->Type) {
        case DistanceX:
            if (constraint->FirstPos == PointPos::none) {  // horizontal length of a line
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addDistanceXConstraint(constraint->First, c.value, c.driving);
            }
            else if (constraint->Second == GeoEnum::GeoUndef) {  // point on fixed x-coordinate
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addCoordinateXConstraint(constraint->First,
                                               constraint->FirstPos,
                                               c.value,
                                               c.driving);
            }
            else if (constraint->SecondPos
                     != PointPos::none) {  // point to point horizontal distance
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addDistanceXConstraint(constraint->First,
                                             constraint->FirstPos,
                                             constraint->Second,
                                             constraint->SecondPos,
                                             c.value,
                                             c.driving);
            }
            break;
        case DistanceY:
            if (constraint->FirstPos == PointPos::none) {  // vertical length of a line
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addDistanceYConstraint(constraint->First, c.value, c.driving);
            }
            else if (constraint->Second == GeoEnum::GeoUndef) {  // point on fixed y-coordinate
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addCoordinateYConstraint(constraint->First,
                                               constraint->FirstPos,
                                               c.value,
                                               c.driving);
            }
            else if (constraint->SecondPos != PointPos::none) {  // point to point vertical distance
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addDistanceYConstraint(constraint->First,
                                             constraint->FirstPos,
                                             constraint->Second,
                                             constraint->SecondPos,
                                             c.value,
                                             c.driving);
            }
            break;
        case Horizontal:
            if (constraint->Second == GeoEnum::GeoUndef) {  // horizontal line
                rtn = addHorizontalConstraint(constraint->First);
            }
            else {  // two points on the same horizontal line
                rtn = addHorizontalConstraint(constraint->First,
                                              constraint->FirstPos,
                                              constraint->Second,
                                              constraint->SecondPos);
            }
            break;
        case Vertical:
            if (constraint->Second == GeoEnum::GeoUndef) {  // vertical line
                rtn = addVerticalConstraint(constraint->First);
            }
            else {  // two points on the same vertical line
                rtn = addVerticalConstraint(constraint->First,
                                            constraint->FirstPos,
                                            constraint->Second,
                                            constraint->SecondPos);
            }
            break;
        case Coincident:
            rtn = addPointCoincidentConstraint(constraint->First,
                                               constraint->FirstPos,
                                               constraint->Second,
                                               constraint->SecondPos);
            break;
        case PointOnObject:
            if (Geoms[checkGeoId(constraint->Second)].type == BSpline) {
                c.value = new double(constraint->getValue());
                // Driving doesn't make sense here
                Parameters.push_back(c.value);

                rtn = addPointOnObjectConstraint(constraint->First,
                                                 constraint->FirstPos,
                                                 constraint->Second,
                                                 c.value);
            }
            else {
                rtn = addPointOnObjectConstraint(constraint->First,
                                                 constraint->FirstPos,
                                                 constraint->Second);
            }
            break;
        case Parallel:
            rtn = addParallelConstraint(constraint->First, constraint->Second);
            break;
        case Perpendicular:
            if (constraint->FirstPos == PointPos::none && constraint->SecondPos == PointPos::none
                && constraint->Third == GeoEnum::GeoUndef) {
                // simple perpendicularity
                rtn = addPerpendicularConstraint(constraint->First, constraint->Second);
            }
            else {
                // any other point-wise perpendicularity
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addAngleAtPointConstraint(constraint->First,
                                                constraint->FirstPos,
                                                constraint->Second,
                                                constraint->SecondPos,
                                                constraint->Third,
                                                constraint->ThirdPos,
                                                c.value,
                                                constraint->Type,
                                                c.driving);
            }
            break;
        case Tangent: {
            bool isSpecialCase = false;

            if (constraint->FirstPos == PointPos::none && constraint->SecondPos == PointPos::none
                && constraint->Third == GeoEnum::GeoUndef) {
                // simple tangency
                rtn = addTangentConstraint(constraint->First, constraint->Second);

                isSpecialCase = true;
            }
            else if (constraint->FirstPos == PointPos::start
                     && constraint->Third == GeoEnum::GeoUndef) {
                // check for B-Spline Knot to curve tangency
                auto knotgeoId = checkGeoId(constraint->First);
                if (Geoms[knotgeoId].type == Point) {
                    auto* point = static_cast<const GeomPoint*>(Geoms[knotgeoId].geo);

                    if (GeometryFacade::isInternalType(point, InternalType::BSplineKnotPoint)) {
                        auto bsplinegeoid = internalAlignmentGeometryMap.at(constraint->First);

                        bsplinegeoid = checkGeoId(bsplinegeoid);

                        auto linegeoid = checkGeoId(constraint->Second);

                        if (Geoms[linegeoid].type == Line) {
                            if (constraint->SecondPos == PointPos::none) {
                                rtn = addTangentLineAtBSplineKnotConstraint(linegeoid,
                                                                            bsplinegeoid,
                                                                            knotgeoId);

                                isSpecialCase = true;
                            }
                            else if (constraint->SecondPos == PointPos::start
                                     || constraint->SecondPos == PointPos::end) {
                                rtn = addTangentLineEndpointAtBSplineKnotConstraint(
                                    linegeoid,
                                    constraint->SecondPos,
                                    bsplinegeoid,
                                    knotgeoId);

                                isSpecialCase = true;
                            }
                        }
                    }
                }
            }
            if (!isSpecialCase) {
                // any other point-wise tangency (endpoint-to-curve, endpoint-to-endpoint,
                // tangent-via-point)
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addAngleAtPointConstraint(constraint->First,
                                                constraint->FirstPos,
                                                constraint->Second,
                                                constraint->SecondPos,
                                                constraint->Third,
                                                constraint->ThirdPos,
                                                c.value,
                                                constraint->Type,
                                                c.driving);
            }
            break;
        }
        case Distance:
            if (constraint->SecondPos != PointPos::none) {  // point to point distance
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }
                rtn = addDistanceConstraint(constraint->First,
                                            constraint->FirstPos,
                                            constraint->Second,
                                            constraint->SecondPos,
                                            c.value,
                                            c.driving);
            }
            else if (constraint->FirstPos == PointPos::none
                     && constraint->SecondPos == PointPos::none
                     && constraint->Second != GeoEnum::GeoUndef
                     && constraint->Third
                         == GeoEnum::GeoUndef) {  // circle to circle, circle to arc, etc.

                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }
                rtn = addDistanceConstraint(constraint->First,
                                            constraint->Second,
                                            c.value,
                                            c.driving);
            }
            else if (constraint->Second != GeoEnum::GeoUndef) {
                if (constraint->FirstPos != PointPos::none) {  // point to line distance
                    c.value = new double(constraint->getValue());
                    if (c.driving) {
                        FixParameters.push_back(c.value);
                    }
                    else {
                        Parameters.push_back(c.value);
                        DrivenParameters.push_back(c.value);
                    }
                    rtn = addDistanceConstraint(constraint->First,
                                                constraint->FirstPos,
                                                constraint->Second,
                                                c.value,
                                                c.driving);
                }
            }
            else {  // line length
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addDistanceConstraint(constraint->First, c.value, c.driving);
            }
            break;
        case Angle:
            if (constraint->Third != GeoEnum::GeoUndef) {
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addAngleAtPointConstraint(constraint->First,
                                                constraint->FirstPos,
                                                constraint->Second,
                                                constraint->SecondPos,
                                                constraint->Third,
                                                constraint->ThirdPos,
                                                c.value,
                                                constraint->Type,
                                                c.driving);
            }
            // angle between two lines (with explicit start points)
            else if (constraint->SecondPos != PointPos::none) {
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addAngleConstraint(constraint->First,
                                         constraint->FirstPos,
                                         constraint->Second,
                                         constraint->SecondPos,
                                         c.value,
                                         c.driving);
            }
            else if (constraint->Second != GeoEnum::GeoUndef) {  // angle between two lines
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addAngleConstraint(constraint->First, constraint->Second, c.value, c.driving);
            }
            else if (constraint->First != GeoEnum::GeoUndef) {  // orientation angle of a line
                c.value = new double(constraint->getValue());
                if (c.driving) {
                    FixParameters.push_back(c.value);
                }
                else {
                    Parameters.push_back(c.value);
                    DrivenParameters.push_back(c.value);
                }

                rtn = addAngleConstraint(constraint->First, c.value, c.driving);
            }
            break;
        case Radius: {
            c.value = new double(constraint->getValue());
            if (c.driving) {
                FixParameters.push_back(c.value);
            }
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addRadiusConstraint(constraint->First, c.value, c.driving);
            break;
        }
        case Diameter: {
            c.value = new double(constraint->getValue());
            if (c.driving) {
                FixParameters.push_back(c.value);
            }
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addDiameterConstraint(constraint->First, c.value, c.driving);
            break;
        }
        case Weight: {
            c.value = new double(constraint->getValue());
            if (c.driving) {
                FixParameters.push_back(c.value);
            }
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addRadiusConstraint(constraint->First, c.value, c.driving);
            break;
        }
        case Equal:
            rtn = addEqualConstraint(constraint->First, constraint->Second);
            break;
        case Symmetric:
            if (constraint->ThirdPos != PointPos::none) {
                rtn = addSymmetricConstraint(constraint->First,
                                             constraint->FirstPos,
                                             constraint->Second,
                                             constraint->SecondPos,
                                             constraint->Third,
                                             constraint->ThirdPos);
            }
            else {
                rtn = addSymmetricConstraint(constraint->First,
                                             constraint->FirstPos,
                                             constraint->Second,
                                             constraint->SecondPos,
                                             constraint->Third);
            }
            break;
        case InternalAlignment:
            switch (constraint->AlignmentType) {
                case EllipseMajorDiameter:
                    rtn = addInternalAlignmentEllipseMajorDiameter(constraint->First,
                                                                   constraint->Second);
                    break;
                case EllipseMinorDiameter:
                    rtn = addInternalAlignmentEllipseMinorDiameter(constraint->First,
                                                                   constraint->Second);
                    break;
                case EllipseFocus1:
                    rtn = addInternalAlignmentEllipseFocus1(constraint->First, constraint->Second);
                    break;
                case EllipseFocus2:
                    rtn = addInternalAlignmentEllipseFocus2(constraint->First, constraint->Second);
                    break;
                case HyperbolaMajor:
                    rtn = addInternalAlignmentHyperbolaMajorDiameter(constraint->First,
                                                                     constraint->Second);
                    break;
                case HyperbolaMinor:
                    rtn = addInternalAlignmentHyperbolaMinorDiameter(constraint->First,
                                                                     constraint->Second);
                    break;
                case HyperbolaFocus:
                    rtn = addInternalAlignmentHyperbolaFocus(constraint->First, constraint->Second);
                    break;
                case ParabolaFocus:
                    rtn = addInternalAlignmentParabolaFocus(constraint->First, constraint->Second);
                    break;
                case BSplineControlPoint:
                    rtn =
                        addInternalAlignmentBSplineControlPoint(constraint->First,
                                                                constraint->Second,
                                                                constraint->InternalAlignmentIndex);
                    break;
                case BSplineKnotPoint:
                    rtn = addInternalAlignmentKnotPoint(constraint->First,
                                                        constraint->Second,
                                                        constraint->InternalAlignmentIndex);
                    break;
                case ParabolaFocalAxis:
                    rtn = addInternalAlignmentParabolaFocalDistance(constraint->First,
                                                                    constraint->Second);
                    break;
                default:
                    break;
            }
            break;
        case SnellsLaw: {
            c.value = new double(constraint->getValue());
            c.secondvalue = new double(constraint->getValue());

            if (c.driving) {
                FixParameters.push_back(c.value);
                FixParameters.push_back(c.secondvalue);
            }
            else {
                Parameters.push_back(c.value);
                Parameters.push_back(c.secondvalue);
                DrivenParameters.push_back(c.value);
                DrivenParameters.push_back(c.secondvalue);
            }

            // assert(constraint->ThirdPos==none); //will work anyway...
            rtn = addSnellsLawConstraint(constraint->First,
                                         constraint->FirstPos,
                                         constraint->Second,
                                         constraint->SecondPos,
                                         constraint->Third,
                                         c.value,
                                         c.secondvalue,
                                         c.driving);
        } break;
        case Sketcher::None:   // ambiguous enum value
        case Sketcher::Block:  // handled separately while adding geometry
        case NumConstraintTypes:
            break;
    }

    Constrs.push_back(c);
    return rtn;
}

int Sketch::addConstraints(const std::vector<Constraint*>& ConstraintList)
{
    int rtn = -1;
    int cid = 0;

    for (std::vector<Constraint*>::const_iterator it = ConstraintList.begin();
         it != ConstraintList.end();
         ++it, ++cid) {
        rtn = addConstraint(*it);

        if (rtn == -1) {
            int humanconstraintid = cid + 1;
            Base::Console().Error("Sketcher constraint number %d is malformed!\n",
                                  humanconstraintid);
            MalformedConstraints.push_back(humanconstraintid);
        }
    }

    return rtn;
}

int Sketch::addConstraints(const std::vector<Constraint*>& ConstraintList,
                           const std::vector<bool>& unenforceableConstraints)
{
    int rtn = -1;

    int cid = 0;
    for (std::vector<Constraint*>::const_iterator it = ConstraintList.begin();
         it != ConstraintList.end();
         ++it, ++cid) {
        if (!unenforceableConstraints[cid] && (*it)->Type != Block && (*it)->isActive) {
            rtn = addConstraint(*it);

            if (rtn == -1) {
                int humanconstraintid = cid + 1;
                Base::Console().Error("Sketcher constraint number %d is malformed!\n",
                                      humanconstraintid);
                MalformedConstraints.push_back(humanconstraintid);
            }
        }
        else {
            ++ConstraintsCounter;  // For correct solver redundant reporting
        }
    }

    return rtn;
}

void Sketch::getBlockedGeometry(std::vector<bool>& blockedGeometry,
                                std::vector<bool>& unenforceableConstraints,
                                const std::vector<Constraint*>& ConstraintList) const
{
    std::vector<int> internalAlignmentConstraintIndex;
    std::vector<int> internalAlignmentgeo;

    std::vector<int> geo2blockingconstraintindex(blockedGeometry.size(), -1);

    // Detect Blocked and internal constraints
    int i = 0;
    for (std::vector<Constraint*>::const_iterator it = ConstraintList.begin();
         it != ConstraintList.end();
         ++it, ++i) {
        switch ((*it)->Type) {
            case Block: {
                int geoid = (*it)->First;

                if (geoid >= 0 && geoid < int(blockedGeometry.size())) {
                    blockedGeometry[geoid] = true;
                    geo2blockingconstraintindex[geoid] = i;
                }
            } break;
            case InternalAlignment:
                internalAlignmentConstraintIndex.push_back(i);
                break;
            default:
                break;
        }
    }

    // if a GeoId is blocked and it is linked to Internal Alignment, then GeoIds linked via Internal
    // Alignment are also to be blocked
    for (std::vector<int>::iterator it = internalAlignmentConstraintIndex.begin();
         it != internalAlignmentConstraintIndex.end();
         it++) {
        if (blockedGeometry[ConstraintList[(*it)]->Second]) {
            blockedGeometry[ConstraintList[(*it)]->First] = true;
            // associated geometry gets the same blocking constraint index as the blocked element
            geo2blockingconstraintindex[ConstraintList[(*it)]->First] =
                geo2blockingconstraintindex[ConstraintList[(*it)]->Second];
            internalAlignmentgeo.push_back(ConstraintList[(*it)]->First);
            unenforceableConstraints[(*it)] = true;
        }
    }

    i = 0;
    for (std::vector<Constraint*>::const_iterator it = ConstraintList.begin();
         it != ConstraintList.end();
         ++it, ++i) {
        if ((*it)->isDriving) {
            // additionally any further constraint on auxiliary elements linked via Internal
            // Alignment are also unenforceable.
            for (std::vector<int>::iterator itg = internalAlignmentgeo.begin();
                 itg != internalAlignmentgeo.end();
                 itg++) {
                if ((*it)->First == *itg || (*it)->Second == *itg || (*it)->Third == *itg) {
                    unenforceableConstraints[i] = true;
                }
            }
            // IMPORTANT NOTE:
            // The rest of the ignoring of redundant/conflicting applies to constraints introduced
            // before the blocking constraint only Constraints introduced after the block will not
            // be ignored and will lead to redundancy/conflicting status as per normal solver
            // behaviour

            // further, any constraint taking only one element, which is blocked is also
            // unenforceable
            if ((*it)->Second == GeoEnum::GeoUndef && (*it)->Third == GeoEnum::GeoUndef
                && (*it)->First >= 0) {
                if (blockedGeometry[(*it)->First]
                    && i < geo2blockingconstraintindex[(*it)->First]) {
                    unenforceableConstraints[i] = true;
                }
            }
            // further any constraint on only two elements where both elements are blocked or one is
            // blocked and the other is an axis or external provided that the constraints precede
            // the last block constraint.
            else if ((*it)->Third == GeoEnum::GeoUndef) {
                if (((*it)->First >= 0 && (*it)->Second >= 0 && blockedGeometry[(*it)->First]
                     && blockedGeometry[(*it)->Second]
                     && (i < geo2blockingconstraintindex[(*it)->First]
                         || i < geo2blockingconstraintindex[(*it)->Second]))
                    || ((*it)->First < 0 && (*it)->Second >= 0 && blockedGeometry[(*it)->Second]
                        && i < geo2blockingconstraintindex[(*it)->Second])
                    || ((*it)->First >= 0 && (*it)->Second < 0 && blockedGeometry[(*it)->First]
                        && i < geo2blockingconstraintindex[(*it)->First])) {
                    unenforceableConstraints[i] = true;
                }
            }
            // further any constraint on three elements where the three of them are blocked, or two
            // are blocked and the other is an axis or external geo or any constraint on three
            // elements where one is blocked and the other two are axis or external geo, provided
            // that the constraints precede the last block constraint.
            else {
                if (((*it)->First >= 0 && (*it)->Second >= 0 && (*it)->Third >= 0
                     && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second]
                     && blockedGeometry[(*it)->Third]
                     && (i < geo2blockingconstraintindex[(*it)->First]
                         || i < geo2blockingconstraintindex[(*it)->Second]
                         || i < geo2blockingconstraintindex[(*it)->Third]))
                    || ((*it)->First < 0 && (*it)->Second >= 0 && (*it)->Third >= 0
                        && blockedGeometry[(*it)->Second] && blockedGeometry[(*it)->Third]
                        && (i < geo2blockingconstraintindex[(*it)->Second]
                            || i < geo2blockingconstraintindex[(*it)->Third]))
                    || ((*it)->First >= 0 && (*it)->Second < 0 && (*it)->Third >= 0
                        && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Third]
                        && (i < geo2blockingconstraintindex[(*it)->First]
                            || i < geo2blockingconstraintindex[(*it)->Third]))
                    || ((*it)->First >= 0 && (*it)->Second >= 0 && (*it)->Third < 0
                        && blockedGeometry[(*it)->First] && blockedGeometry[(*it)->Second]
                        && (i < geo2blockingconstraintindex[(*it)->First]
                            || i < geo2blockingconstraintindex[(*it)->Second]))
                    || ((*it)->First >= 0 && (*it)->Second < 0 && (*it)->Third < 0
                        && blockedGeometry[(*it)->First]
                        && i < geo2blockingconstraintindex[(*it)->First])
                    || ((*it)->First < 0 && (*it)->Second >= 0 && (*it)->Third < 0
                        && blockedGeometry[(*it)->Second]
                        && i < geo2blockingconstraintindex[(*it)->Second])
                    || ((*it)->First < 0 && (*it)->Second < 0 && (*it)->Third >= 0
                        && blockedGeometry[(*it)->Third]
                        && i < geo2blockingconstraintindex[(*it)->Third])) {

                    unenforceableConstraints[i] = true;
                }
            }
        }
    }
}

int Sketch::addCoordinateXConstraint(int geoId, PointPos pos, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {

        GCS::Point& p = Points[pointId];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCoordinateX(p, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addCoordinateYConstraint(int geoId, PointPos pos, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {
        GCS::Point& p = Points[pointId];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCoordinateY(p, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDistanceXConstraint(int geoId, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line) {
        return -1;
    }

    GCS::Line& l = Lines[Geoms[geoId].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintDifference(l.p1.x, l.p2.x, value, tag, driving);
    return ConstraintsCounter;
}

int Sketch::addDistanceYConstraint(int geoId, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line) {
        return -1;
    }

    GCS::Line& l = Lines[Geoms[geoId].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintDifference(l.p1.y, l.p2.y, value, tag, driving);
    return ConstraintsCounter;
}

int Sketch::addDistanceXConstraint(int geoId1,
                                   PointPos pos1,
                                   int geoId2,
                                   PointPos pos2,
                                   double* value,
                                   bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintDifference(p1.x, p2.x, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDistanceYConstraint(int geoId1,
                                   PointPos pos1,
                                   int geoId2,
                                   PointPos pos2,
                                   double* value,
                                   bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintDifference(p1.y, p2.y, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// horizontal line constraint
int Sketch::addHorizontalConstraint(int geoId)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line) {
        return -1;
    }

    GCS::Line& l = Lines[Geoms[geoId].index];
    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintHorizontal(l, tag);
    return ConstraintsCounter;
}

// two points on a horizontal line constraint
int Sketch::addHorizontalConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintHorizontal(p1, p2, tag);
        return ConstraintsCounter;
    }
    return -1;
}

// vertical line constraint
int Sketch::addVerticalConstraint(int geoId)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line) {
        return -1;
    }

    GCS::Line& l = Lines[Geoms[geoId].index];
    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintVertical(l, tag);
    return ConstraintsCounter;
}

// two points on a vertical line constraint
int Sketch::addVerticalConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintVertical(p1, p2, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addPointCoincidentConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p1, p2, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addParallelConstraint(int geoId1, int geoId2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line || Geoms[geoId2].type != Line) {
        return -1;
    }

    GCS::Line& l1 = Lines[Geoms[geoId1].index];
    GCS::Line& l2 = Lines[Geoms[geoId2].index];
    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintParallel(l1, l2, tag);
    return ConstraintsCounter;
}

// simple perpendicularity constraint
int Sketch::addPerpendicularConstraint(int geoId1, int geoId2)
{
    // accepts the following combinations:
    // 1) Line1, Line2/Circle2/Arc2
    // 2) Circle1, Line2 (converted to case #1)
    // 3) Arc1, Line2 (converted to case #1)
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId2].type == Line) {
        if (Geoms[geoId1].type == Line) {
            GCS::Line& l1 = Lines[Geoms[geoId1].index];
            GCS::Line& l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPerpendicular(l1, l2, tag);
            return ConstraintsCounter;
        }
        else {
            std::swap(geoId1, geoId2);
        }
    }

    if (Geoms[geoId1].type == Line) {
        GCS::Line& l1 = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc || Geoms[geoId2].type == Circle) {
            GCS::Point& p2 = Points[Geoms[geoId2].midPointId];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p2, l1, tag);
            return ConstraintsCounter;
        }
    }

    Base::Console().Warning("Perpendicular constraints between %s and %s are not supported.\n",
                            nameByType(Geoms[geoId1].type),
                            nameByType(Geoms[geoId2].type));
    return -1;
}

// simple tangency constraint
int Sketch::addTangentConstraint(int geoId1, int geoId2)
{
    // accepts the following combinations:
    // 1) Line1, Line2/Circle2/Arc2
    // 2) Circle1, Line2 (converted to case #1)
    //    Circle1, Circle2/Arc2
    // 3) Arc1, Line2 (converted to case #1)
    //    Arc1, Circle2/Arc2
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId2].type == Line) {
        if (Geoms[geoId1].type == Line) {
            GCS::Line& l1 = Lines[Geoms[geoId1].index];
            GCS::Point& l2p1 = Points[Geoms[geoId2].startPointId];
            GCS::Point& l2p2 = Points[Geoms[geoId2].endPointId];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(l2p1, l1, tag);
            GCSsys.addConstraintPointOnLine(l2p2, l1, tag);
            return ConstraintsCounter;
        }
        else {
            std::swap(geoId1, geoId2);
        }
    }

    if (Geoms[geoId1].type == Line) {
        GCS::Line& l = Lines[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc& a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, a, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle& c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, c, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Ellipse) {
            GCS::Ellipse& e = Ellipses[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, e, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfEllipse) {
            GCS::ArcOfEllipse& a = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(l, a, tag);
            return ConstraintsCounter;
        }
    }
    else if (Geoms[geoId1].type == Circle) {
        GCS::Circle& c = Circles[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle& c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, c2, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Ellipse) {
            Base::Console().Error("Direct tangency constraint between circle and ellipse is not "
                                  "supported. Use tangent-via-point instead.");
            return -1;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc& a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, a, tag);
            return ConstraintsCounter;
        }
    }
    else if (Geoms[geoId1].type == Ellipse) {
        if (Geoms[geoId2].type == Circle) {
            Base::Console().Error("Direct tangency constraint between circle and ellipse is not "
                                  "supported. Use tangent-via-point instead.");
            return -1;
        }
        else if (Geoms[geoId2].type == Arc) {
            Base::Console().Error("Direct tangency constraint between arc and ellipse is not "
                                  "supported. Use tangent-via-point instead.");
            return -1;
        }
    }
    else if (Geoms[geoId1].type == Arc) {
        GCS::Arc& a = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle& c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(c, a, tag);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Ellipse) {
            Base::Console().Error("Direct tangency constraint between arc and ellipse is not "
                                  "supported. Use tangent-via-point instead.");
            return -1;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc& a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintTangent(a, a2, tag);
            return ConstraintsCounter;
        }
    }

    return -1;
}

int Sketch::addTangentLineAtBSplineKnotConstraint(int checkedlinegeoId,
                                                  int checkedbsplinegeoId,
                                                  int checkedknotgeoid)
{
    GCS::BSpline& b = BSplines[Geoms[checkedbsplinegeoId].index];
    GCS::Line& l = Lines[Geoms[checkedlinegeoId].index];

    size_t knotindex = b.knots.size();

    auto knotIt = std::find(b.knotpointGeoids.begin(), b.knotpointGeoids.end(), checkedknotgeoid);

    knotindex = std::distance(b.knotpointGeoids.begin(), knotIt);

    if (knotindex >= b.knots.size()) {
        Base::Console().Error("addConstraint: Knot index out-of-range!\n");
        return -1;
    }

    if (b.mult[knotindex] >= b.degree) {
        if (b.periodic || (knotindex > 0 && knotindex < (b.knots.size() - 1))) {
            Base::Console().Error("addTangentLineAtBSplineKnotConstraint: cannot set constraint "
                                  "when B-spline slope is discontinuous at knot!\n");
            return -1;
        }
        else {
            // TODO: Let angle-at-point do the work. Requires a `double * value`
            // return addAngleAtPointConstraint(
            //     linegeoid, PointPos::none,
            //     bsplinegeoid, PointPos::none,
            //     knotgeoId, PointPos::start,
            //     nullptr, Tangent, true);

            // For now we just throw an error.
            Base::Console().Error(
                "addTangentLineAtBSplineKnotConstraint: This method cannot set tangent constraint "
                "at end knots of a B-spline. Please constrain the start/end points instead.\n");
            return -1;
        }
    }
    else {
        // increases ConstraintsCounter
        int tag =
            Sketch::addPointOnObjectConstraint(checkedknotgeoid, PointPos::start, checkedlinegeoId);
        GCSsys.addConstraintTangentAtBSplineKnot(b, l, knotindex, tag);
        return ConstraintsCounter;
    }
}

int Sketch::addTangentLineEndpointAtBSplineKnotConstraint(int checkedlinegeoId,
                                                          PointPos endpointPos,
                                                          int checkedbsplinegeoId,
                                                          int checkedknotgeoid)
{
    GCS::BSpline& b = BSplines[Geoms[checkedbsplinegeoId].index];
    GCS::Line& l = Lines[Geoms[checkedlinegeoId].index];
    auto pointId = getPointId(checkedlinegeoId, endpointPos);
    auto pointIdKnot = getPointId(checkedknotgeoid, PointPos::start);
    GCS::Point& p = Points[pointId];
    GCS::Point& pk = Points[pointIdKnot];

    size_t knotindex = b.knots.size();

    auto knotIt = std::find(b.knotpointGeoids.begin(), b.knotpointGeoids.end(), checkedknotgeoid);

    knotindex = std::distance(b.knotpointGeoids.begin(), knotIt);

    if (knotindex >= b.knots.size()) {
        Base::Console().Error("addConstraint: Knot index out-of-range!\n");
        return -1;
    }

    if (b.mult[knotindex] >= b.degree) {
        if (b.periodic || (knotindex > 0 && knotindex < (b.knots.size() - 1))) {
            Base::Console().Error("addTangentLineEndpointAtBSplineKnotConstraint: cannot set "
                                  "constraint when B-spline slope is discontinuous at knot!\n");
            return -1;
        }
        else {
            // TODO: Let angle-at-point do the work. Requires a `double * value`
            // return addAngleAtPointConstraint(
            //     linegeoid, endpointPos,
            //     bsplinegeoid, PointPos::none,
            //     knotgeoId, PointPos::start,
            //     nullptr, Tangent, true);

            // For now we just throw an error.
            Base::Console().Error("addTangentLineEndpointAtBSplineKnotConstraint: This method "
                                  "cannot set tangent constraint at end knots of a B-spline. "
                                  "Please constrain the start/end points instead.\n");
            return -1;
        }
    }
    else {
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p, pk, tag);
        GCSsys.addConstraintTangentAtBSplineKnot(b, l, knotindex, tag);
        return ConstraintsCounter;
    }
}

// This function handles any type of tangent, perpendicular and angle
//  constraint that involves a point.
//  i.e. endpoint-to-curve, endpoint-to-endpoint and tangent-via-point
// geoid1, geoid2 and geoid3 as in the constraint object.
// For perp-ty and tangency, angle is used to lock the direction.
// angle==0 - autodetect direction. +pi/2, -pi/2 - specific direction.
int Sketch::addAngleAtPointConstraint(int geoId1,
                                      PointPos pos1,
                                      int geoId2,
                                      PointPos pos2,
                                      int geoId3,
                                      PointPos pos3,
                                      double* value,
                                      ConstraintType cTyp,
                                      bool driving)
{

    if (!(cTyp == Angle || cTyp == Tangent || cTyp == Perpendicular)) {
        // assert(0);//none of the three types. Why are we here??
        return -1;
    }

    bool avp = geoId3 != GeoEnum::GeoUndef;                       // is angle-via-point?
    bool e2c = pos2 == PointPos::none && pos1 != PointPos::none;  // is endpoint-to-curve?
    bool e2e = pos2 != PointPos::none && pos1 != PointPos::none;  // is endpoint-to-endpoint?

    if (!(avp || e2c || e2e)) {
        // assert(0);//none of the three types. Why are we here??
        return -1;
    }

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);
    if (avp) {
        geoId3 = checkGeoId(geoId3);
    }

    if (Geoms[geoId1].type == Point || Geoms[geoId2].type == Point) {
        Base::Console().Error("addAngleAtPointConstraint: one of the curves is a point!\n");
        return -1;
    }

    GCS::Curve* crv1 = getGCSCurveByGeoId(geoId1);
    GCS::Curve* crv2 = getGCSCurveByGeoId(geoId2);
    if (!crv1 || !crv2) {
        Base::Console().Error("addAngleAtPointConstraint: getGCSCurveByGeoId returned NULL!\n");
        return -1;
    }

    int pointId = -1;
    if (avp) {
        pointId = getPointId(geoId3, pos3);
    }
    else if (e2e || e2c) {
        pointId = getPointId(geoId1, pos1);
    }

    if (pointId < 0 || pointId >= int(Points.size())) {
        Base::Console().Error("addAngleAtPointConstraint: point index out of range.\n");
        return -1;
    }
    GCS::Point& p = Points[pointId];
    GCS::Point* p2 = nullptr;
    if (e2e) {  // we need second point
        int pointId = getPointId(geoId2, pos2);
        if (pointId < 0 || pointId >= int(Points.size())) {
            Base::Console().Error("addAngleAtPointConstraint: point index out of range.\n");
            return -1;
        }
        p2 = &(Points[pointId]);
    }

    double* angle = value;

    // For tangency/perpendicularity, we don't just copy the angle.
    // The angle stored for tangency/perpendicularity is offset, so that the options
    //  are -Pi/2 and Pi/2. If value is 0 - this is an indicator of an old sketch.
    //  Use autodetect then.
    // The same functionality is implemented in SketchObject.cpp, where
    //  it is used to permanently lock down the autodecision.
    if (cTyp != Angle) {
        // The same functionality is implemented in SketchObject.cpp, where
        //  it is used to permanently lock down the autodecision.
        // the difference between the datum value and the actual angle to apply.
        // (datum=angle+offset)
        double angleOffset = 0.0;
        // the desired angle value (and we are to decide if 180* should be added to it)
        double angleDesire = 0.0;
        if (cTyp == Tangent) {
            angleOffset = -M_PI / 2;
            angleDesire = 0.0;
        }
        if (cTyp == Perpendicular) {
            angleOffset = 0;
            angleDesire = M_PI / 2;
        }

        if (*value
            == 0.0) {  // autodetect tangency internal/external (and same for perpendicularity)
            double angleErr = GCSsys.calculateAngleViaPoint(*crv1, *crv2, p) - angleDesire;
            // bring angleErr to -pi..pi
            if (angleErr > M_PI) {
                angleErr -= M_PI * 2;
            }
            if (angleErr < -M_PI) {
                angleErr += M_PI * 2;
            }

            // the autodetector
            if (fabs(angleErr) > M_PI / 2) {
                angleDesire += M_PI;
            }

            *angle = angleDesire;
        }
        else {
            *angle = *value - angleOffset;
        }
    }

    int tag = -1;
    if (e2c) {
        // increases ConstraintsCounter
        tag = Sketch::addPointOnObjectConstraint(geoId1, pos1, geoId2, driving);
    }
    if (e2e) {
        tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p, *p2, tag, driving);
    }
    if (avp) {
        tag = ++ConstraintsCounter;
    }

    GCSsys.addConstraintAngleViaPoint(*crv1, *crv2, p, angle, tag, driving);
    return ConstraintsCounter;
}

// line length constraint
int Sketch::addDistanceConstraint(int geoId, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type != Line) {
        return -1;
    }

    GCS::Line& l = Lines[Geoms[geoId].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintP2PDistance(l.p1, l.p2, value, tag, driving);
    return ConstraintsCounter;
}

// point to line distance constraint
int Sketch::addDistanceConstraint(int geoId1,
                                  PointPos pos1,
                                  int geoId2,
                                  double* value,
                                  bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    if (pointId1 < 0 && pointId1 >= int(Points.size())) {
        return -1;
    }
    GCS::Point& p1 = Points[pointId1];

    if (Geoms[geoId2].type == Line) {
        GCS::Line& l2 = Lines[Geoms[geoId2].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2LDistance(p1, l2, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId2].type == Circle) {
        GCS::Circle& c2 = Circles[Geoms[geoId2].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2CDistance(p1, c2, value, tag, driving);
        return ConstraintsCounter;
    }
    else {
        return -1;
    }
}

// point to point distance constraint
int Sketch::addDistanceConstraint(int geoId1,
                                  PointPos pos1,
                                  int geoId2,
                                  PointPos pos2,
                                  double* value,
                                  bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PDistance(p1, p2, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// circle-(circle or line) distance constraint
int Sketch::addDistanceConstraint(int geoId1, int geoId2, double* value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type == Circle) {
        if (Geoms[geoId2].type == Circle) {
            GCS::Circle& c1 = Circles[Geoms[geoId1].index];
            GCS::Circle& c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintC2CDistance(c1, c2, value, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Line) {
            GCS::Circle& c = Circles[Geoms[geoId1].index];
            GCS::Line& l = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintC2LDistance(c, l, value, tag, driving);
            return ConstraintsCounter;
        }
    }
    return -1;
}


int Sketch::addRadiusConstraint(int geoId, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Circle) {
        GCS::Circle& c = Circles[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCircleRadius(c, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc& a = Arcs[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintArcRadius(a, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addDiameterConstraint(int geoId, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Circle) {
        GCS::Circle& c = Circles[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintCircleDiameter(c, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc& a = Arcs[Geoms[geoId].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintArcDiameter(a, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// line orientation angle constraint
int Sketch::addAngleConstraint(int geoId, double* value, bool driving)
{
    geoId = checkGeoId(geoId);

    if (Geoms[geoId].type == Line) {
        GCS::Line& l = Lines[Geoms[geoId].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PAngle(l.p1, l.p2, value, tag, driving);
        return ConstraintsCounter;
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Arc& a = Arcs[Geoms[geoId].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintL2LAngle(a.center, a.start, a.center, a.end, value, tag, driving);
        return ConstraintsCounter;
    }
    return -1;
}

// line to line angle constraint
int Sketch::addAngleConstraint(int geoId1, int geoId2, double* value, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line || Geoms[geoId2].type != Line) {
        return -1;
    }

    GCS::Line& l1 = Lines[Geoms[geoId1].index];
    GCS::Line& l2 = Lines[Geoms[geoId2].index];

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintL2LAngle(l1, l2, value, tag, driving);
    return ConstraintsCounter;
}

// line to line angle constraint (with explicitly given start points)
int Sketch::addAngleConstraint(int geoId1,
                               PointPos pos1,
                               int geoId2,
                               PointPos pos2,
                               double* value,
                               bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Line || Geoms[geoId2].type != Line) {
        return -1;
    }

    GCS::Point *l1p1 = nullptr, *l1p2 = nullptr;
    if (pos1 == PointPos::start) {
        l1p1 = &Points[Geoms[geoId1].startPointId];
        l1p2 = &Points[Geoms[geoId1].endPointId];
    }
    else if (pos1 == PointPos::end) {
        l1p1 = &Points[Geoms[geoId1].endPointId];
        l1p2 = &Points[Geoms[geoId1].startPointId];
    }

    GCS::Point *l2p1 = nullptr, *l2p2 = nullptr;
    if (pos2 == PointPos::start) {
        l2p1 = &Points[Geoms[geoId2].startPointId];
        l2p2 = &Points[Geoms[geoId2].endPointId];
    }
    else if (pos2 == PointPos::end) {
        l2p1 = &Points[Geoms[geoId2].endPointId];
        l2p2 = &Points[Geoms[geoId2].startPointId];
    }

    if (!l1p1 || !l2p1) {
        return -1;
    }

    int tag = ++ConstraintsCounter;
    GCSsys.addConstraintL2LAngle(*l1p1, *l1p2, *l2p1, *l2p2, value, tag, driving);
    return ConstraintsCounter;
}


int Sketch::addEqualConstraint(int geoId1, int geoId2)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type == Line && Geoms[geoId2].type == Line) {
        GCS::Line& l1 = Lines[Geoms[geoId1].index];
        GCS::Line& l2 = Lines[Geoms[geoId2].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintEqualLength(l1, l2, tag);
        return ConstraintsCounter;
    }

    if (Geoms[geoId2].type == Circle) {
        if (Geoms[geoId1].type == Circle) {
            GCS::Circle& c1 = Circles[Geoms[geoId1].index];
            GCS::Circle& c2 = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadius(c1, c2, tag);
            return ConstraintsCounter;
        }
        else {
            std::swap(geoId1, geoId2);
        }
    }

    if (Geoms[geoId2].type == Ellipse) {
        if (Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse& e1 = Ellipses[Geoms[geoId1].index];
            GCS::Ellipse& e2 = Ellipses[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(e1, e2, tag);
            return ConstraintsCounter;
        }
        else {
            std::swap(geoId1, geoId2);
        }
    }

    if (Geoms[geoId1].type == Circle) {
        GCS::Circle& c1 = Circles[Geoms[geoId1].index];
        if (Geoms[geoId2].type == Arc) {
            GCS::Arc& a2 = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadius(c1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId1].type == Arc && Geoms[geoId2].type == Arc) {
        GCS::Arc& a1 = Arcs[Geoms[geoId1].index];
        GCS::Arc& a2 = Arcs[Geoms[geoId2].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintEqualRadius(a1, a2, tag);
        return ConstraintsCounter;
    }

    if (Geoms[geoId2].type == ArcOfEllipse) {
        if (Geoms[geoId1].type == ArcOfEllipse) {
            GCS::ArcOfEllipse& a1 = ArcsOfEllipse[Geoms[geoId1].index];
            GCS::ArcOfEllipse& a2 = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(a1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId2].type == ArcOfHyperbola) {
        if (Geoms[geoId1].type == ArcOfHyperbola) {
            GCS::ArcOfHyperbola& a1 = ArcsOfHyperbola[Geoms[geoId1].index];
            GCS::ArcOfHyperbola& a2 = ArcsOfHyperbola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(a1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId2].type == ArcOfParabola) {
        if (Geoms[geoId1].type == ArcOfParabola) {
            GCS::ArcOfParabola& a1 = ArcsOfParabola[Geoms[geoId1].index];
            GCS::ArcOfParabola& a2 = ArcsOfParabola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualFocus(a1, a2, tag);
            return ConstraintsCounter;
        }
    }

    if (Geoms[geoId1].type == Ellipse) {
        GCS::Ellipse& e1 = Ellipses[Geoms[geoId1].index];
        if (Geoms[geoId2].type == ArcOfEllipse) {
            GCS::ArcOfEllipse& a2 = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintEqualRadii(a2, e1, tag);
            return ConstraintsCounter;
        }
    }

    Base::Console().Warning("Equality constraints between %s and %s are not supported.\n",
                            nameByType(Geoms[geoId1].type),
                            nameByType(Geoms[geoId2].type));
    return -1;
}

// point on object constraint
int Sketch::addPointOnObjectConstraint(int geoId1, PointPos pos1, int geoId2, bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];

        if (Geoms[geoId2].type == Line) {
            GCS::Line& l2 = Lines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnLine(p1, l2, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Arc) {
            GCS::Arc& a = Arcs[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnArc(p1, a, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Circle) {
            GCS::Circle& c = Circles[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnCircle(p1, c, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == Ellipse) {
            GCS::Ellipse& e = Ellipses[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnEllipse(p1, e, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfEllipse) {
            GCS::ArcOfEllipse& a = ArcsOfEllipse[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnEllipse(p1, a, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfHyperbola) {
            GCS::ArcOfHyperbola& a = ArcsOfHyperbola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnHyperbolicArc(p1, a, tag, driving);
            return ConstraintsCounter;
        }
        else if (Geoms[geoId2].type == ArcOfParabola) {
            GCS::ArcOfParabola& a = ArcsOfParabola[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintPointOnParabolicArc(p1, a, tag, driving);
            return ConstraintsCounter;
        }
    }
    return -1;
}

int Sketch::addPointOnObjectConstraint(int geoId1,
                                       PointPos pos1,
                                       int geoId2,
                                       double* pointparam,
                                       bool driving)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    int pointId1 = getPointId(geoId1, pos1);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];

        if (Geoms[geoId2].type == BSpline) {
            GCS::BSpline& b = BSplines[Geoms[geoId2].index];
            int tag = ++ConstraintsCounter;
            auto partBsp = static_cast<GeomBSplineCurve*>(Geoms[geoId2].geo);
            double uNear;
            partBsp->closestParameter(Base::Vector3d(*p1.x, *p1.y, 0.0), uNear);
            *pointparam = uNear;
            GCSsys.addConstraintPointOnBSpline(p1, b, pointparam, tag, driving);

            return ConstraintsCounter;
        }
    }
    return -1;
}

// symmetric points constraint
int Sketch::addSymmetricConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, int geoId3)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);
    geoId3 = checkGeoId(geoId3);

    if (Geoms[geoId3].type != Line) {
        return -1;
    }

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];
        GCS::Line& l = Lines[Geoms[geoId3].index];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PSymmetric(p1, p2, l, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addSymmetricConstraint(int geoId1,
                                   PointPos pos1,
                                   int geoId2,
                                   PointPos pos2,
                                   int geoId3,
                                   PointPos pos3)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);
    geoId3 = checkGeoId(geoId3);

    int pointId1 = getPointId(geoId1, pos1);
    int pointId2 = getPointId(geoId2, pos2);
    int pointId3 = getPointId(geoId3, pos3);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size()) && pointId3 >= 0 && pointId3 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];
        GCS::Point& p = Points[pointId3];
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PSymmetric(p1, p2, p, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addSnellsLawConstraint(int geoIdRay1,
                                   PointPos posRay1,
                                   int geoIdRay2,
                                   PointPos posRay2,
                                   int geoIdBnd,
                                   double* value,
                                   double* secondvalue,
                                   bool driving)
{

    geoIdRay1 = checkGeoId(geoIdRay1);
    geoIdRay2 = checkGeoId(geoIdRay2);
    geoIdBnd = checkGeoId(geoIdBnd);

    if (Geoms[geoIdRay1].type == Point || Geoms[geoIdRay2].type == Point) {
        Base::Console().Error("addSnellsLawConstraint: point is not a curve. Not applicable!\n");
        return -1;
    }

    GCS::Curve* ray1 = getGCSCurveByGeoId(geoIdRay1);
    GCS::Curve* ray2 = getGCSCurveByGeoId(geoIdRay2);
    GCS::Curve* boundary = getGCSCurveByGeoId(geoIdBnd);
    if (!ray1 || !ray2 || !boundary) {
        Base::Console().Error("addSnellsLawConstraint: getGCSCurveByGeoId returned NULL!\n");
        return -1;
    }

    int pointId1 = getPointId(geoIdRay1, posRay1);
    int pointId2 = getPointId(geoIdRay2, posRay2);
    if (pointId1 < 0 || pointId1 >= int(Points.size()) || pointId2 < 0
        || pointId2 >= int(Points.size())) {
        Base::Console().Error("addSnellsLawConstraint: point index out of range.\n");
        return -1;
    }
    GCS::Point& p1 = Points[pointId1];

    // add the parameters (refractive indexes)
    // n1 uses the place hold by n2divn1, so that is retrievable in updateNonDrivingConstraints
    double* n1 = value;
    double* n2 = secondvalue;

    double n2divn1 = *value;

    if (fabs(n2divn1) >= 1.0) {
        *n2 = n2divn1;
        *n1 = 1.0;
    }
    else {
        *n2 = 1.0;
        *n1 = 1 / n2divn1;
    }

    int tag = -1;
    // increases ConstraintsCounter
    // tag = Sketch::addPointOnObjectConstraint(geoIdRay1, posRay1, geoIdBnd);
    tag = ++ConstraintsCounter;
    // GCSsys.addConstraintP2PCoincident(p1, p2, tag);
    GCSsys.addConstraintSnellsLaw(*ray1,
                                  *ray2,
                                  *boundary,
                                  p1,
                                  n1,
                                  n2,
                                  posRay1 == PointPos::start,
                                  posRay2 == PointPos::end,
                                  tag,
                                  driving);
    return ConstraintsCounter;
}


int Sketch::addInternalAlignmentEllipseMajorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse) {
        return -1;
    }
    if (Geoms[geoId2].type != Line) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);
    int pointId2 = getPointId(geoId2, PointPos::end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        if (Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse& e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMajorDiameter(e1, p1, p2, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse& a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMajorDiameter(a1, p1, p2, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}

int Sketch::addInternalAlignmentEllipseMinorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse) {
        return -1;
    }
    if (Geoms[geoId2].type != Line) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);
    int pointId2 = getPointId(geoId2, PointPos::end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        if (Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse& e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMinorDiameter(e1, p1, p2, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse& a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseMinorDiameter(a1, p1, p2, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}

int Sketch::addInternalAlignmentEllipseFocus1(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse) {
        return -1;
    }
    if (Geoms[geoId2].type != Point) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];

        if (Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse& e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus1(e1, p1, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse& a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus1(a1, p1, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}


int Sketch::addInternalAlignmentEllipseFocus2(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != Ellipse && Geoms[geoId1].type != ArcOfEllipse) {
        return -1;
    }
    if (Geoms[geoId2].type != Point) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];

        if (Geoms[geoId1].type == Ellipse) {
            GCS::Ellipse& e1 = Ellipses[Geoms[geoId1].index];

            // constraints
            // 1. start point with ellipse -a
            // 2. end point with ellipse +a
            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus2(e1, p1, tag);
            return ConstraintsCounter;
        }
        else {
            GCS::ArcOfEllipse& a1 = ArcsOfEllipse[Geoms[geoId1].index];

            int tag = ++ConstraintsCounter;
            GCSsys.addConstraintInternalAlignmentEllipseFocus2(a1, p1, tag);
            return ConstraintsCounter;
        }
    }
    return -1;
}


int Sketch::addInternalAlignmentHyperbolaMajorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfHyperbola) {
        return -1;
    }
    if (Geoms[geoId2].type != Line) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);
    int pointId2 = getPointId(geoId2, PointPos::end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {

        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        GCS::ArcOfHyperbola& a1 = ArcsOfHyperbola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentHyperbolaMajorDiameter(a1, p1, p2, tag);
        return ConstraintsCounter;
    }

    return -1;
}

int Sketch::addInternalAlignmentHyperbolaMinorDiameter(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfHyperbola) {
        return -1;
    }
    if (Geoms[geoId2].type != Line) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);
    int pointId2 = getPointId(geoId2, PointPos::end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {

        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        GCS::ArcOfHyperbola& a1 = ArcsOfHyperbola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentHyperbolaMinorDiameter(a1, p1, p2, tag);
        return ConstraintsCounter;
    }

    return -1;
}

int Sketch::addInternalAlignmentHyperbolaFocus(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfHyperbola) {
        return -1;
    }
    if (Geoms[geoId2].type != Point) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];

        GCS::ArcOfHyperbola& a1 = ArcsOfHyperbola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentHyperbolaFocus(a1, p1, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addInternalAlignmentParabolaFocus(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfParabola) {
        return -1;
    }
    if (Geoms[geoId2].type != Point) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p1 = Points[pointId1];

        GCS::ArcOfParabola& a1 = ArcsOfParabola[Geoms[geoId1].index];

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentParabolaFocus(a1, p1, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addInternalAlignmentParabolaFocalDistance(int geoId1, int geoId2)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != ArcOfParabola) {
        return -1;
    }
    if (Geoms[geoId2].type != Line) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);
    int pointId2 = getPointId(geoId2, PointPos::end);

    if (pointId1 >= 0 && pointId1 < int(Points.size()) && pointId2 >= 0
        && pointId2 < int(Points.size())) {

        GCS::Point& p1 = Points[pointId1];
        GCS::Point& p2 = Points[pointId2];

        GCS::ArcOfParabola& a1 = ArcsOfParabola[Geoms[geoId1].index];

        auto& vertexpoint = a1.vertex;
        auto& focuspoint = a1.focus1;

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p1, vertexpoint, tag);

        tag = ++ConstraintsCounter;
        GCSsys.addConstraintP2PCoincident(p2, focuspoint, tag);

        return ConstraintsCounter;
    }

    return -1;
}

int Sketch::addInternalAlignmentBSplineControlPoint(int geoId1, int geoId2, int poleindex)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != BSpline) {
        return -1;
    }
    if (Geoms[geoId2].type != Circle) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::mid);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Circle& c = Circles[Geoms[geoId2].index];

        GCS::BSpline& b = BSplines[Geoms[geoId1].index];

        assert(poleindex < static_cast<int>(b.poles.size()) && poleindex >= 0);

        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentBSplineControlPoint(b, c, poleindex, tag);
        return ConstraintsCounter;
    }
    return -1;
}

int Sketch::addInternalAlignmentKnotPoint(int geoId1, int geoId2, int knotindex)
{
    std::swap(geoId1, geoId2);

    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    if (Geoms[geoId1].type != BSpline) {
        return -1;
    }
    if (Geoms[geoId2].type != Point) {
        return -1;
    }

    int pointId1 = getPointId(geoId2, PointPos::start);

    if (pointId1 >= 0 && pointId1 < int(Points.size())) {
        GCS::Point& p = Points[pointId1];
        GCS::BSpline& b = BSplines[Geoms[geoId1].index];

        assert(knotindex < static_cast<int>(b.knots.size()) && knotindex >= 0);

        b.knotpointGeoids[knotindex] = geoId2;
        int tag = ++ConstraintsCounter;
        GCSsys.addConstraintInternalAlignmentKnotPoint(b, p, knotindex, tag);

        return ConstraintsCounter;
    }
    return -1;
}

double Sketch::calculateAngleViaPoint(int geoId1, int geoId2, double px, double py)
{
    geoId1 = checkGeoId(geoId1);
    geoId2 = checkGeoId(geoId2);

    GCS::Point p;
    p.x = &px;
    p.y = &py;

    // check pointers
    GCS::Curve* crv1 = getGCSCurveByGeoId(geoId1);
    GCS::Curve* crv2 = getGCSCurveByGeoId(geoId2);
    if (!crv1 || !crv2) {
        throw Base::ValueError("calculateAngleViaPoint: getGCSCurveByGeoId returned NULL!");
    }

    return GCSsys.calculateAngleViaPoint(*crv1, *crv2, p);
}

Base::Vector3d Sketch::calculateNormalAtPoint(int geoIdCurve, double px, double py) const
{
    geoIdCurve = checkGeoId(geoIdCurve);

    GCS::Point p;
    p.x = &px;
    p.y = &py;

    // check pointers
    const GCS::Curve* crv = getGCSCurveByGeoId(geoIdCurve);
    if (!crv) {
        throw Base::ValueError("calculateNormalAtPoint: getGCSCurveByGeoId returned NULL!\n");
    }

    double tx = 0.0, ty = 0.0;
    GCSsys.calculateNormalAtPoint(*crv, p, tx, ty);
    return Base::Vector3d(tx, ty, 0.0);
}

bool Sketch::updateGeometry()
{
    int i = 0;
    for (std::vector<GeoDef>::const_iterator it = Geoms.begin(); it != Geoms.end(); ++it, i++) {
        try {
            if (it->type == Point) {
                GeomPoint* point = static_cast<GeomPoint*>(it->geo);
                auto pointf = GeometryFacade::getFacade(point);

                point->setPoint(
                    Vector3d(*Points[it->startPointId].x, *Points[it->startPointId].y, 0.0));
            }
            else if (it->type == Line) {
                GeomLineSegment* lineSeg = static_cast<GeomLineSegment*>(it->geo);
                lineSeg->setPoints(Vector3d(*Lines[it->index].p1.x, *Lines[it->index].p1.y, 0.0),
                                   Vector3d(*Lines[it->index].p2.x, *Lines[it->index].p2.y, 0.0));
            }
            else if (it->type == Arc) {
                GCS::Arc& myArc = Arcs[it->index];
                // the following 4 lines are redundant since these equations are already included in
                // the arc constraints *myArc.start.x = *myArc.center.x + *myArc.rad *
                // cos(*myArc.startAngle); *myArc.start.y = *myArc.center.y + *myArc.rad *
                // sin(*myArc.startAngle); *myArc.end.x = *myArc.center.x + *myArc.rad *
                // cos(*myArc.endAngle); *myArc.end.y = *myArc.center.y + *myArc.rad *
                // sin(*myArc.endAngle);
                GeomArcOfCircle* aoc = static_cast<GeomArcOfCircle*>(it->geo);
                aoc->setCenter(Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0));
                aoc->setRadius(*myArc.rad);
                aoc->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            }
            else if (it->type == ArcOfEllipse) {
                GCS::ArcOfEllipse& myArc = ArcsOfEllipse[it->index];

                GeomArcOfEllipse* aoe = static_cast<GeomArcOfEllipse*>(it->geo);

                Base::Vector3d center =
                    Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);
                double radmin = *myArc.radmin;

                Base::Vector3d fd = f1 - center;
                double radmaj = sqrt(fd * fd + radmin * radmin);

                aoe->setCenter(center);
                // ensure that ellipse's major radius is always larger than minor radius... may
                // still cause problems with degenerates.
                if (radmaj >= aoe->getMinorRadius()) {
                    aoe->setMajorRadius(radmaj);
                    aoe->setMinorRadius(radmin);
                }
                else {
                    aoe->setMinorRadius(radmin);
                    aoe->setMajorRadius(radmaj);
                }
                aoe->setMajorAxisDir(fd);
                aoe->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            }
            else if (it->type == Circle) {
                GeomCircle* circ = static_cast<GeomCircle*>(it->geo);
                circ->setCenter(
                    Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0));
                circ->setRadius(*Circles[it->index].rad);
            }
            else if (it->type == Ellipse) {

                GeomEllipse* ellipse = static_cast<GeomEllipse*>(it->geo);

                Base::Vector3d center =
                    Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 =
                    Vector3d(*Ellipses[it->index].focus1.x, *Ellipses[it->index].focus1.y, 0.0);
                double radmin = *Ellipses[it->index].radmin;

                Base::Vector3d fd = f1 - center;
                double radmaj = sqrt(fd * fd + radmin * radmin);

                ellipse->setCenter(center);
                // ensure that ellipse's major radius is always larger than minor radius... may
                // still cause problems with degenerates.
                if (radmaj >= ellipse->getMinorRadius()) {
                    ellipse->setMajorRadius(radmaj);
                    ellipse->setMinorRadius(radmin);
                }
                else {
                    ellipse->setMinorRadius(radmin);
                    ellipse->setMajorRadius(radmaj);
                }
                ellipse->setMajorAxisDir(fd);
            }
            else if (it->type == ArcOfHyperbola) {
                GCS::ArcOfHyperbola& myArc = ArcsOfHyperbola[it->index];

                GeomArcOfHyperbola* aoh = static_cast<GeomArcOfHyperbola*>(it->geo);

                Base::Vector3d center =
                    Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);
                double radmin = *myArc.radmin;

                Base::Vector3d fd = f1 - center;
                double radmaj = sqrt(fd * fd - radmin * radmin);

                aoh->setCenter(center);
                if (radmaj >= aoh->getMinorRadius()) {
                    aoh->setMajorRadius(radmaj);
                    aoh->setMinorRadius(radmin);
                }
                else {
                    aoh->setMinorRadius(radmin);
                    aoh->setMajorRadius(radmaj);
                }
                aoh->setMajorAxisDir(fd);
                aoh->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            }
            else if (it->type == ArcOfParabola) {
                GCS::ArcOfParabola& myArc = ArcsOfParabola[it->index];

                GeomArcOfParabola* aop = static_cast<GeomArcOfParabola*>(it->geo);

                Base::Vector3d vertex =
                    Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);

                Base::Vector3d fd = f1 - vertex;

                aop->setXAxisDir(fd);
                aop->setCenter(vertex);
                aop->setFocal(fd.Length());
                aop->setRange(*myArc.startAngle, *myArc.endAngle, /*emulateCCW=*/true);
            }
            else if (it->type == BSpline) {
                GCS::BSpline& mybsp = BSplines[it->index];

                GeomBSplineCurve* bsp = static_cast<GeomBSplineCurve*>(it->geo);

                std::vector<Base::Vector3d> poles;
                std::vector<double> weights;

                std::vector<GCS::Point>::const_iterator it1;
                std::vector<double*>::const_iterator it2;

                for (it1 = mybsp.poles.begin(), it2 = mybsp.weights.begin();
                     it1 != mybsp.poles.end() && it2 != mybsp.weights.end();
                     ++it1, ++it2) {
                    poles.emplace_back(*(*it1).x, *(*it1).y, 0.0);
                    weights.push_back(*(*it2));
                }

                bsp->setPoles(poles, weights);

                std::vector<double> knots;
                std::vector<int> mult;

                // This is the code that should be here when/if b-spline gets its full
                // implementation in the solver.
                /*std::vector<double *>::const_iterator it3;
                std::vector<int>::const_iterator it4;

                for( it3 = mybsp.knots.begin(), it4 = mybsp.mult.begin(); it3 != mybsp.knots.end()
                && it4 != mybsp.mult.end(); ++it3, ++it4) { knots.push_back(*(*it3));
                    mult.push_back((*it4));
                }

                bsp->setKnots(knots,mult);*/
            }
        }
        catch (Base::Exception& e) {
            Base::Console().Error("Updating geometry: Error build geometry(%d): %s\n", i, e.what());
            return false;
        }
    }
    return true;
}

bool Sketch::updateNonDrivingConstraints()
{
    for (std::vector<ConstrDef>::iterator it = Constrs.begin(); it != Constrs.end(); ++it) {
        if (!(*it).driving) {
            if ((*it).constr->Type == SnellsLaw) {
                double n1 = *((*it).value);
                double n2 = *((*it).secondvalue);

                (*it).constr->setValue(n2 / n1);
            }
            else if ((*it).constr->Type == Angle) {

                (*it).constr->setValue(std::fmod(*((*it).value), 2.0 * M_PI));
            }
            else if ((*it).constr->Type == Diameter && (*it).constr->First >= 0) {

                // two cases, the geometry parameter is fixed or it is not
                // NOTE: This is different from being blocked, as new block constraint may fix
                // the parameter or not depending on whether other driving constraints are present
                int geoId = (*it).constr->First;

                geoId = checkGeoId(geoId);

                double* rad = nullptr;

                if (Geoms[geoId].type == Circle) {
                    GCS::Circle& c = Circles[Geoms[geoId].index];
                    rad = c.rad;
                }
                else if (Geoms[geoId].type == Arc) {
                    GCS::Arc& a = Arcs[Geoms[geoId].index];
                    rad = a.rad;
                }

                auto pos = std::find(FixParameters.begin(), FixParameters.end(), rad);

                if (pos != FixParameters.end()) {
                    (*it).constr->setValue(*((*it).value));
                }
                else {
                    (*it).constr->setValue(2.0 * *((*it).value));
                }
            }
            else {
                (*it).constr->setValue(*((*it).value));
            }
        }
    }

    return true;
}

// solving ==========================================================

int Sketch::solve()
{
    Base::TimeInfo start_time;
    std::string solvername;

    auto result = internalSolve(solvername);

    Base::TimeInfo end_time;

    if (debugMode == GCS::Minimal || debugMode == GCS::IterationLevel) {

        Base::Console().Log("Sketcher::Solve()-%s-T:%s\n",
                            solvername.c_str(),
                            Base::TimeInfo::diffTime(start_time, end_time).c_str());
    }

    SolveTime = Base::TimeInfo::diffTimeF(start_time, end_time);

    return result;
}

int Sketch::internalSolve(std::string& solvername, int level)
{
    if (!isInitMove) {  // make sure we are in single subsystem mode
        clearTemporaryConstraints();
        isFine = true;
    }

    int ret = -1;
    bool valid_solution;
    int defaultsoltype = -1;

    if (isInitMove) {
        solvername = "DogLeg";  // DogLeg is used for dragging (same as before)
        ret = GCSsys.solve(isFine, GCS::DogLeg);
    }
    else {
        switch (defaultSolver) {
            case 0:
                solvername = "BFGS";
                ret = GCSsys.solve(isFine, GCS::BFGS);
                defaultsoltype = 2;
                break;
            case 1:  // solving with the LevenbergMarquardt solver
                solvername = "LevenbergMarquardt";
                ret = GCSsys.solve(isFine, GCS::LevenbergMarquardt);
                defaultsoltype = 1;
                break;
            case 2:  // solving with the BFGS solver
                solvername = "DogLeg";
                ret = GCSsys.solve(isFine, GCS::DogLeg);
                defaultsoltype = 0;
                break;
        }
    }

    // if successfully solved try to write the parameters back
    if (ret == GCS::Success) {
        GCSsys.applySolution();
        valid_solution = updateGeometry();
        if (!valid_solution) {
            GCSsys.undoSolution();
            updateGeometry();
            Base::Console().Warning("Invalid solution from %s solver.\n", solvername.c_str());
        }
        else {
            updateNonDrivingConstraints();
        }
    }
    else {
        valid_solution = false;
        if (debugMode == GCS::Minimal || debugMode == GCS::IterationLevel) {

            Base::Console().Log("Sketcher::Solve()-%s- Failed!! Falling back...\n",
                                solvername.c_str());
        }
    }

    if (!valid_solution && !isInitMove) {  // Fall back to other solvers
        for (int soltype = 0; soltype < 4; soltype++) {

            if (soltype == defaultsoltype) {
                continue;  // skip default solver
            }

            switch (soltype) {
                case 0:
                    solvername = "DogLeg";
                    ret = GCSsys.solve(isFine, GCS::DogLeg);
                    break;
                case 1:  // solving with the LevenbergMarquardt solver
                    solvername = "LevenbergMarquardt";
                    ret = GCSsys.solve(isFine, GCS::LevenbergMarquardt);
                    break;
                case 2:  // solving with the BFGS solver
                    solvername = "BFGS";
                    ret = GCSsys.solve(isFine, GCS::BFGS);
                    break;
                // last resort: augment the system with a second subsystem and use the SQP solver
                case 3:
                    solvername = "SQP(augmented system)";
                    InitParameters.resize(Parameters.size());
                    int i = 0;
                    for (std::vector<double*>::iterator it = Parameters.begin();
                         it != Parameters.end();
                         ++it, i++) {
                        InitParameters[i] = **it;
                        GCSsys.addConstraintEqual(*it,
                                                  &InitParameters[i],
                                                  GCS::DefaultTemporaryConstraint);
                    }
                    GCSsys.initSolution();
                    ret = GCSsys.solve(isFine);
                    break;
            }

            // if successfully solved try to write the parameters back
            if (ret == GCS::Success) {
                GCSsys.applySolution();
                valid_solution = updateGeometry();
                if (!valid_solution) {
                    GCSsys.undoSolution();
                    updateGeometry();
                    Base::Console().Warning("Invalid solution from %s solver.\n",
                                            solvername.c_str());
                    ret = GCS::SuccessfulSolutionInvalid;
                }
                else {
                    updateNonDrivingConstraints();
                }
            }
            else {
                valid_solution = false;
                if (debugMode == GCS::Minimal || debugMode == GCS::IterationLevel) {

                    Base::Console().Log("Sketcher::Solve()-%s- Failed!! Falling back...\n",
                                        solvername.c_str());
                }
            }

            if (soltype == 3) {  // cleanup temporary constraints of the augmented system
                clearTemporaryConstraints();
            }

            if (valid_solution) {
                if (soltype == 1) {
                    Base::Console().Log("Important: the LevenbergMarquardt solver succeeded where "
                                        "the DogLeg solver had failed.\n");
                }
                else if (soltype == 2) {
                    Base::Console().Log("Important: the BFGS solver succeeded where the DogLeg and "
                                        "LevenbergMarquardt solvers have failed.\n");
                }
                else if (soltype == 3) {
                    Base::Console().Log("Important: the SQP solver succeeded where all single "
                                        "subsystem solvers have failed.\n");
                }

                if (soltype > 0) {
                    Base::Console().Log("If you see this message please report a way of "
                                        "reproducing this result at\n");
                    Base::Console().Log("http://www.freecad.org/tracker/main_page.php\n");
                }

                break;
            }
        }  // soltype
    }

    // For OCCT reliant geometry that needs an extra solve() for example to update non-driving
    // constraints.
    if (resolveAfterGeometryUpdated && ret == GCS::Success && level == 0) {
        return internalSolve(solvername, 1);
    }

    return ret;
}

int Sketch::initMove(int geoId, PointPos pos, bool fine)
{
    isFine = fine;

    geoId = checkGeoId(geoId);

    clearTemporaryConstraints();

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts()) {
        isInitMove = false;
        return -1;
    }

    if (Geoms[geoId].type == Point) {
        if (pos == PointPos::start) {
            GCS::Point& point = Points[Geoms[geoId].startPointId];
            GCS::Point p0;
            MoveParameters.resize(2);  // px,py
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *point.x;
            *p0.y = *point.y;
            GCSsys.addConstraintP2PCoincident(p0, point, GCS::DefaultTemporaryConstraint);
        }
    }
    else if (Geoms[geoId].type == Line) {
        if (pos == PointPos::start || pos == PointPos::end) {
            MoveParameters.resize(2);  // x,y
            GCS::Point p0;
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            if (pos == PointPos::start) {
                GCS::Point& p = Points[Geoms[geoId].startPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
            else if (pos == PointPos::end) {
                GCS::Point& p = Points[Geoms[geoId].endPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
        }
        else if (pos == PointPos::none || pos == PointPos::mid) {
            MoveParameters.resize(4);  // x1,y1,x2,y2
            GCS::Point p1, p2;
            p1.x = &MoveParameters[0];
            p1.y = &MoveParameters[1];
            p2.x = &MoveParameters[2];
            p2.y = &MoveParameters[3];
            GCS::Line& l = Lines[Geoms[geoId].index];
            *p1.x = *l.p1.x;
            *p1.y = *l.p1.y;
            *p2.x = *l.p2.x;
            *p2.y = *l.p2.y;
            GCSsys.addConstraintP2PCoincident(p1, l.p1, GCS::DefaultTemporaryConstraint);
            GCSsys.addConstraintP2PCoincident(p2, l.p2, GCS::DefaultTemporaryConstraint);
        }
    }
    else if (Geoms[geoId].type == Circle) {
        GCS::Point& center = Points[Geoms[geoId].midPointId];
        GCS::Point p0, p1;
        if (pos == PointPos::mid) {
            MoveParameters.resize(2);  // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0, center, GCS::DefaultTemporaryConstraint);
        }
        else if (pos == PointPos::none) {
            // bool pole = GeometryFacade::isInternalType(Geoms[geoId].geo,
            // InternalType::BSplineControlPoint);
            MoveParameters.resize(4);  // x,y,cx,cy - For poles blocking the center
            GCS::Circle& c = Circles[Geoms[geoId].index];
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y + *c.rad;
            GCSsys.addConstraintPointOnCircle(p0, c, GCS::DefaultTemporaryConstraint);
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;
            int i = GCSsys.addConstraintP2PCoincident(p1, center, GCS::DefaultTemporaryConstraint);
            GCSsys.rescaleConstraint(i - 1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    else if (Geoms[geoId].type == Ellipse) {

        GCS::Point& center = Points[Geoms[geoId].midPointId];
        GCS::Point p0, p1;
        if (pos == PointPos::mid || pos == PointPos::none) {
            MoveParameters.resize(2);  // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0, center, GCS::DefaultTemporaryConstraint);
        }
    }
    else if (Geoms[geoId].type == ArcOfEllipse) {

        GCS::Point& center = Points[Geoms[geoId].midPointId];
        GCS::Point p0, p1;
        if (pos == PointPos::mid || pos == PointPos::none) {
            MoveParameters.resize(2);  // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0, center, GCS::DefaultTemporaryConstraint);
        }
        else if (pos == PointPos::start || pos == PointPos::end) {

            MoveParameters.resize(4);  // x,y,cx,cy
            if (pos == PointPos::start || pos == PointPos::end) {
                GCS::Point& p = (pos == PointPos::start) ? Points[Geoms[geoId].startPointId]
                                                         : Points[Geoms[geoId].endPointId];
                ;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }

            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i = GCSsys.addConstraintP2PCoincident(p1, center, GCS::DefaultTemporaryConstraint);
            GCSsys.rescaleConstraint(i - 1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    else if (Geoms[geoId].type == ArcOfHyperbola) {

        GCS::Point& center = Points[Geoms[geoId].midPointId];
        GCS::Point p0, p1;
        if (pos == PointPos::mid || pos == PointPos::none) {
            MoveParameters.resize(2);  // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0, center, GCS::DefaultTemporaryConstraint);
        }
        else if (pos == PointPos::start || pos == PointPos::end) {

            MoveParameters.resize(4);  // x,y,cx,cy
            if (pos == PointPos::start || pos == PointPos::end) {
                GCS::Point& p = (pos == PointPos::start) ? Points[Geoms[geoId].startPointId]
                                                         : Points[Geoms[geoId].endPointId];
                ;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i = GCSsys.addConstraintP2PCoincident(p1, center, GCS::DefaultTemporaryConstraint);
            GCSsys.rescaleConstraint(i - 1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    else if (Geoms[geoId].type == ArcOfParabola) {

        GCS::Point& center = Points[Geoms[geoId].midPointId];
        GCS::Point p0, p1;
        if (pos == PointPos::mid || pos == PointPos::none) {
            MoveParameters.resize(2);  // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0, center, GCS::DefaultTemporaryConstraint);
        }
        else if (pos == PointPos::start || pos == PointPos::end) {

            MoveParameters.resize(4);  // x,y,cx,cy
            if (pos == PointPos::start || pos == PointPos::end) {
                GCS::Point& p = (pos == PointPos::start) ? Points[Geoms[geoId].startPointId]
                                                         : Points[Geoms[geoId].endPointId];
                ;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i = GCSsys.addConstraintP2PCoincident(p1, center, GCS::DefaultTemporaryConstraint);
            GCSsys.rescaleConstraint(i - 1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    else if (Geoms[geoId].type == BSpline) {
        if (pos == PointPos::start || pos == PointPos::end) {
            MoveParameters.resize(2);  // x,y
            GCS::Point p0;
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            if (pos == PointPos::start) {
                GCS::Point& p = Points[Geoms[geoId].startPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
            else if (pos == PointPos::end) {
                GCS::Point& p = Points[Geoms[geoId].endPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
        }
        else if (pos == PointPos::none || pos == PointPos::mid) {
            GCS::BSpline& bsp = BSplines[Geoms[geoId].index];
            MoveParameters.resize(bsp.poles.size() * 2);  // x0,y0,x1,y1,....xp,yp

            int mvindex = 0;
            for (std::vector<GCS::Point>::iterator it = bsp.poles.begin(); it != bsp.poles.end();
                 it++, mvindex++) {
                GCS::Point p1;
                p1.x = &MoveParameters[mvindex];
                mvindex++;
                p1.y = &MoveParameters[mvindex];

                *p1.x = *(*it).x;
                *p1.y = *(*it).y;

                GCSsys.addConstraintP2PCoincident(p1, (*it), GCS::DefaultTemporaryConstraint);
            }
        }
    }
    else if (Geoms[geoId].type == Arc) {
        GCS::Point& center = Points[Geoms[geoId].midPointId];
        GCS::Point p0, p1;
        if (pos == PointPos::mid) {
            MoveParameters.resize(2);  // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0, center, GCS::DefaultTemporaryConstraint);
        }
        else if (pos == PointPos::start || pos == PointPos::end || pos == PointPos::none) {
            MoveParameters.resize(4);  // x,y,cx,cy
            if (pos == PointPos::start || pos == PointPos::end) {
                GCS::Point& p = (pos == PointPos::start) ? Points[Geoms[geoId].startPointId]
                                                         : Points[Geoms[geoId].endPointId];
                ;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0, p, GCS::DefaultTemporaryConstraint);
            }
            else if (pos == PointPos::none) {
                GCS::Arc& a = Arcs[Geoms[geoId].index];
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *center.x;
                *p0.y = *center.y + *a.rad;
                GCSsys.addConstraintPointOnArc(p0, a, GCS::DefaultTemporaryConstraint);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;
            int i = GCSsys.addConstraintP2PCoincident(p1, center, GCS::DefaultTemporaryConstraint);
            GCSsys.rescaleConstraint(i - 1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    InitParameters = MoveParameters;

    GCSsys.initSolution();
    isInitMove = true;
    return 0;
}

void Sketch::resetInitMove()
{
    isInitMove = false;
}

int Sketch::initBSplinePieceMove(int geoId,
                                 PointPos pos,
                                 const Base::Vector3d& firstPoint,
                                 bool fine)
{
    isFine = fine;

    geoId = checkGeoId(geoId);

    clearTemporaryConstraints();

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts()) {
        isInitMove = false;
        return -1;
    }

    // this is only meant for B-Splines
    if (Geoms[geoId].type != BSpline || pos == PointPos::start || pos == PointPos::end) {
        return -1;
    }

    GCS::BSpline& bsp = BSplines[Geoms[geoId].index];

    // If spline has too few poles, just move all
    if (bsp.poles.size() <= std::size_t(bsp.degree + 1)) {
        return initMove(geoId, pos, fine);
    }

    // Find the closest knot
    auto partBsp = static_cast<GeomBSplineCurve*>(Geoms[geoId].geo);
    double uNear;
    partBsp->closestParameter(firstPoint, uNear);
    auto& knots = bsp.knots;
    auto upperknot =
        std::upper_bound(knots.begin(), knots.end(), uNear, [](double u, double* element) {
            return u < *element;
        });

    size_t idx = 0;
    // skipping the first knot for adjustment
    // TODO: ensure this works for periodic as well
    for (size_t i = 1; i < bsp.mult.size() && knots[i] != *upperknot; ++i) {
        idx += bsp.mult[i];
    }

    MoveParameters.resize(2 * (bsp.degree + 1));  // x[idx],y[idx],x[idx+1],y[idx+1],...

    size_t mvindex = 0;
    auto lastIt = (idx + bsp.degree + 1) % bsp.poles.size();
    for (size_t i = idx; i != lastIt; i = (i + 1) % bsp.poles.size(), ++mvindex) {
        GCS::Point p1;
        p1.x = &MoveParameters[mvindex];
        ++mvindex;
        p1.y = &MoveParameters[mvindex];

        *p1.x = *bsp.poles[i].x;
        *p1.y = *bsp.poles[i].y;

        GCSsys.addConstraintP2PCoincident(p1, bsp.poles[i], GCS::DefaultTemporaryConstraint);
    }

    InitParameters = MoveParameters;

    GCSsys.initSolution();
    isInitMove = true;
    return 0;
}

int Sketch::movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative)
{
    geoId = checkGeoId(geoId);

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts()) {
        return -1;
    }

    if (!isInitMove) {
        initMove(geoId, pos);
        initToPoint = toPoint;
        moveStep = 0;
    }
    else {
        if (!relative && RecalculateInitialSolutionWhileMovingPoint) {
            if (moveStep == 0) {
                moveStep = (toPoint - initToPoint).Length();
            }
            else {
                // I am getting too far away from the original solution so reinit the solution
                if ((toPoint - initToPoint).Length() > 20 * moveStep) {
                    initMove(geoId, pos);
                    initToPoint = toPoint;
                }
            }
        }
    }

    if (relative) {
        for (int i = 0; i < int(MoveParameters.size() - 1); i += 2) {
            MoveParameters[i] = InitParameters[i] + toPoint.x;
            MoveParameters[i + 1] = InitParameters[i + 1] + toPoint.y;
        }
    }
    else if (Geoms[geoId].type == Point) {
        if (pos == PointPos::start) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == Line) {
        if (pos == PointPos::start || pos == PointPos::end) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
        else if (pos == PointPos::none || pos == PointPos::mid) {
            double dx = (InitParameters[2] - InitParameters[0]) / 2;
            double dy = (InitParameters[3] - InitParameters[1]) / 2;
            MoveParameters[0] = toPoint.x - dx;
            MoveParameters[1] = toPoint.y - dy;
            MoveParameters[2] = toPoint.x + dx;
            MoveParameters[3] = toPoint.y + dy;
        }
    }
    else if (Geoms[geoId].type == Circle) {
        if (pos == PointPos::mid || pos == PointPos::none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == Arc) {
        if (pos == PointPos::start || pos == PointPos::end || pos == PointPos::mid
            || pos == PointPos::none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == Ellipse) {
        if (pos == PointPos::mid || pos == PointPos::none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == ArcOfEllipse) {
        if (pos == PointPos::start || pos == PointPos::end || pos == PointPos::mid
            || pos == PointPos::none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == ArcOfHyperbola) {
        if (pos == PointPos::start || pos == PointPos::end || pos == PointPos::mid
            || pos == PointPos::none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == ArcOfParabola) {
        if (pos == PointPos::start || pos == PointPos::end || pos == PointPos::mid
            || pos == PointPos::none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    }
    else if (Geoms[geoId].type == BSpline) {
        if (pos == PointPos::start || pos == PointPos::end) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
        else if (pos == PointPos::none || pos == PointPos::mid) {
            GCS::BSpline& bsp = BSplines[Geoms[geoId].index];

            double cx = 0, cy = 0;  // geometric center
            for (int i = 0; i < int(InitParameters.size() - 1); i += 2) {
                cx += InitParameters[i];
                cy += InitParameters[i + 1];
            }

            cx /= bsp.poles.size();
            cy /= bsp.poles.size();

            for (int i = 0; i < int(MoveParameters.size() - 1); i += 2) {

                MoveParameters[i] = toPoint.x + InitParameters[i] - cx;
                MoveParameters[i + 1] = toPoint.y + InitParameters[i + 1] - cy;
            }
        }
    }

    return solve();
}

int Sketch::setDatum(int /*constrId*/, double /*value*/)
{
    return -1;
}

int Sketch::getPointId(int geoId, PointPos pos) const
{
    // do a range check first
    if (geoId < 0 || geoId >= (int)Geoms.size()) {
        return -1;
    }
    switch (pos) {
        case PointPos::start:
            return Geoms[geoId].startPointId;
        case PointPos::end:
            return Geoms[geoId].endPointId;
        case PointPos::mid:
            return Geoms[geoId].midPointId;
        case PointPos::none:
            break;
    }
    return -1;
}

Base::Vector3d Sketch::getPoint(int geoId, PointPos pos) const
{
    geoId = checkGeoId(geoId);
    int pointId = getPointId(geoId, pos);
    if (pointId != -1) {
        return Base::Vector3d(*Points[pointId].x, *Points[pointId].y, 0);
    }

    return Base::Vector3d();
}

TopoShape Sketch::toShape() const
{
    TopoShape result;
    std::vector<GeoDef>::const_iterator it = Geoms.begin();

#if 0

    bool first = true;
    for (;it!=Geoms.end();++it) {
        if (!it->geo->Construction) {
            TopoDS_Shape sh = it->geo->toShape();
            if (first) {
                first = false;
                result.setShape(sh);
            }
            else {
                result.setShape(result.fuse(sh));
            }
        }
    }
    return result;
#else
    std::list<TopoDS_Edge> edge_list;
    std::list<TopoDS_Vertex> vertex_list;
    std::list<TopoDS_Wire> wires;

    // collecting all (non constructive and non external) edges out of the sketch
    for (; it != Geoms.end(); ++it) {
        auto gf = GeometryFacade::getFacade(it->geo);
        if (!it->external && !gf->getConstruction()) {

            if (it->type != Point) {
                auto shape = it->geo->toShape();
                if (!shape.IsNull()) {
                    edge_list.push_back(TopoDS::Edge(shape));
                }
            }
            else {
                vertex_list.push_back(TopoDS::Vertex(it->geo->toShape()));
            }
        }
    }

    // Hint: Use ShapeAnalysis_FreeBounds::ConnectEdgesToWires() as an alternative
    //
    // sort them together to wires
    while (!edge_list.empty()) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        mkWire.Add(edge_list.front());
        edge_list.pop_front();

        TopoDS_Wire new_wire = mkWire.Wire();  // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are
        // connectible
        bool found = false;
        do {
            found = false;
            for (std::list<TopoDS_Edge>::iterator pE = edge_list.begin(); pE != edge_list.end();
                 ++pE) {
                mkWire.Add(*pE);
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edge_list.erase(pE);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        } while (found);

        // Fix any topological issues of the wire
        ShapeFix_Wire aFix;
        aFix.SetPrecision(Precision::Confusion());
        aFix.Load(new_wire);
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        wires.push_back(aFix.Wire());
    }

    if (wires.size() == 1 && vertex_list.empty()) {
        result = *wires.begin();
    }
    else if (wires.size() > 1 || !vertex_list.empty()) {
        // FIXME: The right way here would be to determine the outer and inner wires and
        // generate a face with holes (inner wires have to be tagged REVERSE or INNER).
        // that's the only way to transport a somewhat more complex sketch...
        // result = *wires.begin();

        // I think a compound can be used as container because it is just a collection of
        // shapes and doesn't need too much information about the topology.
        // The actual knowledge how to create a prism from several wires should go to the Pad
        // feature (Werner).
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for (std::list<TopoDS_Wire>::iterator wt = wires.begin(); wt != wires.end(); ++wt) {
            builder.Add(comp, *wt);
        }
        for (std::list<TopoDS_Vertex>::iterator wt = vertex_list.begin(); wt != vertex_list.end();
             ++wt) {
            builder.Add(comp, *wt);
        }
        result.setShape(comp);
    }
#endif

    return result;
}

// Persistence implementer -------------------------------------------------

unsigned int Sketch::getMemSize() const
{
    return 0;
}

void Sketch::Save(Writer&) const
{}

void Sketch::Restore(XMLReader&)
{}
