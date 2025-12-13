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

#include <BRep_Tool.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>

#include <boost/algorithm/string/predicate.hpp>

#include <App/IndexedName.h>
#include <App/MappedName.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>

#include <memory>

#include "GeoEnum.h"
#include "SketchObject.h"
#include "SolverGeometryExtension.h"
#include "ExternalGeometryFacade.h"


#undef DEBUG
// #define DEBUG

// clang-format off
using namespace Sketcher;
using namespace Base;

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomPoint *geomPoint, PointPos PosId)
{
    if (PosId == PointPos::start || PosId == PointPos::mid || PosId == PointPos::end)
        return geomPoint->getPoint();
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomLineSegment *lineSeg, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return lineSeg->getStartPoint();
    }
    case PointPos::end: {
        return lineSeg->getEndPoint();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomCircle *circle, PointPos PosId)
{
    auto pt = circle->getCenter();
    if(PosId != PointPos::mid)
        pt.x += circle->getRadius();
    return pt;
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomEllipse *ellipse, PointPos PosId)
{
    auto pt = ellipse->getCenter();
    if(PosId != PointPos::mid)
        pt += ellipse->getMajorAxisDir()*ellipse->getMajorRadius();
    return pt;
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfCircle *aoc, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aoc->getStartPoint(/*emulateCCW=*/true);
    }
    case PointPos::end: {
        return aoc->getEndPoint(/*emulateCCW=*/true);
    }
    case PointPos::mid: {
        return aoc->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfEllipse *aoe, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aoe->getStartPoint(/*emulateCCW=*/true);
    }
    case PointPos::end: {
        return aoe->getEndPoint(/*emulateCCW=*/true);
    }
    case PointPos::mid: {
        return aoe->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfHyperbola *aoh, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aoh->getStartPoint();
    }
    case PointPos::end: {
        return aoh->getEndPoint();
    }
    case PointPos::mid: {
        return aoh->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomArcOfParabola *aop, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return aop->getStartPoint();
    }
    case PointPos::end: {
        return aop->getEndPoint();
    }
    case PointPos::mid: {
        return aop->getCenter();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

template <>
Base::Vector3d SketchObject::getPointForGeometry<>(const Part::GeomBSplineCurve *bsp, PointPos PosId)
{
    switch (PosId) {
    case PointPos::start: {
        return bsp->getStartPoint();
    }
    case PointPos::end: {
        return bsp->getEndPoint();
    }
    default:
        break;
    }
    return Base::Vector3d();
}

Base::Vector3d SketchObject::getPoint(const Part::Geometry *geo, PointPos PosId)
{
    if (auto point = freecad_cast<Part::GeomPoint*>(geo)) {
        return getPointForGeometry<Part::GeomPoint>(point, PosId);
    }
    else if (auto lineSegment = freecad_cast<Part::GeomLineSegment*>(geo)) {
        return getPointForGeometry<Part::GeomLineSegment>(lineSegment, PosId);
    }
    else if (auto circle = freecad_cast<Part::GeomCircle*>(geo)) {
        return getPointForGeometry<Part::GeomCircle>(circle, PosId);
    }
    else if (auto ellipse = freecad_cast<Part::GeomEllipse*>(geo)) {
        return getPointForGeometry<Part::GeomEllipse>(ellipse, PosId);
    }
    else if (auto arcOfCircle = freecad_cast<Part::GeomArcOfCircle*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfCircle>(arcOfCircle, PosId);
    }
    else if (auto arcOfEllipse = freecad_cast<Part::GeomArcOfEllipse*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfEllipse>(arcOfEllipse, PosId);
    }
    else if (auto arcOfHyperbola = freecad_cast<Part::GeomArcOfHyperbola*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfHyperbola>(arcOfHyperbola, PosId);
    }
    else if (auto arcOfParabola = freecad_cast<Part::GeomArcOfParabola*>(geo)) {
        return getPointForGeometry<Part::GeomArcOfParabola>(arcOfParabola, PosId);
    }
    else if (auto bSplineCurve = freecad_cast<Part::GeomBSplineCurve*>(geo)) {
        return getPointForGeometry<Part::GeomBSplineCurve>(bSplineCurve, PosId);
    }
    return Base::Vector3d();
}

Base::Vector3d SketchObject::getPoint(int GeoId, PointPos PosId) const
{
    if (!(GeoId == H_Axis || GeoId == V_Axis
          || (GeoId <= getHighestCurveIndex() && GeoId >= -getExternalGeometryCount())))
        throw Base::ValueError("SketchObject::getPoint. Invalid GeoId was supplied.");
    const Part::Geometry* geo = getGeometry(GeoId);
    return getPoint(geo,PosId);
}

int SketchObject::getAxisCount() const
{
    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    int count = 0;
    for (const auto& geo : vals) {
        if (geo && GeometryFacade::getConstruction(geo)
            && geo->is<Part::GeomLineSegment>()) {
            count++;
        }
    }

    return count;
}

Base::Axis SketchObject::getAxis(int axId) const
{
    if (axId == H_Axis || axId == V_Axis || axId == N_Axis)
        return Part::Part2DObject::getAxis(axId);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    int count = 0;
    for (const auto& geo : vals) {
        if (geo && GeometryFacade::getConstruction(geo)
            && geo->is<Part::GeomLineSegment>()) {
            if (count == axId) {
                auto* lineSeg = static_cast<Part::GeomLineSegment*>(geo);
                Base::Vector3d start = lineSeg->getStartPoint();
                Base::Vector3d end = lineSeg->getEndPoint();
                return Base::Axis(start, end - start);
            }
            count++;
        }
    }

    return Base::Axis();
}

bool SketchObject::isSupportedGeometry(const Part::Geometry* geo) const
{
    if (geo->is<Part::GeomPoint>()
        || geo->is<Part::GeomCircle>()
        || geo->is<Part::GeomEllipse>()
        || geo->is<Part::GeomArcOfCircle>()
        || geo->is<Part::GeomArcOfEllipse>()
        || geo->is<Part::GeomArcOfHyperbola>()
        || geo->is<Part::GeomArcOfParabola>()
        || geo->is<Part::GeomBSplineCurve>()
        || geo->is<Part::GeomLineSegment>()) {
        return true;
    }
    if (geo->is<Part::GeomTrimmedCurve>()) {
        Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast(geo->handle());
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());
        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());
        if (!circle.IsNull() || !ellipse.IsNull()) {
            return true;
        }
    }
    return false;
}

std::vector<Part::Geometry*>
SketchObject::supportedGeometry(const std::vector<Part::Geometry*>& geoList) const
{
    std::vector<Part::Geometry*> supportedGeoList;
    supportedGeoList.reserve(geoList.size());
    // read-in geometry that the sketcher cannot handle
    for (const auto& geo : geoList) {
        if (isSupportedGeometry(geo)) {
            supportedGeoList.push_back(geo);
        }
    }

    return supportedGeoList;
}

int SketchObject::addGeometry(const std::vector<Part::Geometry*>& geoList,
                              bool construction /*=false*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);
    newVals.reserve(newVals.size() + geoList.size());
    for (auto& v : geoList) {
        Part::Geometry* copy = v->copy();
        generateId(copy);

        if (construction) {
            GeometryFacade::setConstruction(copy, construction);
        }

        newVals.push_back(copy);
    }

    // On setting geometry the onChanged method will call acceptGeometry(), thereby updating
    // constraint geometry indices and rebuilding the vertex index
    Geometry.setValues(std::move(newVals));

    return Geometry.getSize() - 1;
}

int SketchObject::addGeometry(const Part::Geometry* geo, bool construction /*=false*/)
{
    // this copy has a new random tag (see copy() vs clone())
    auto geoNew = std::unique_ptr<Part::Geometry>(geo->copy());

    return addGeometry(std::move(geoNew), construction);
}

int SketchObject::addGeometry(std::unique_ptr<Part::Geometry> newgeo, bool construction /*=false*/)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    auto* geoNew = newgeo.release();
    generateId(geoNew);

    if (construction) {
        GeometryFacade::setConstruction(geoNew, construction);
    }

    newVals.push_back(geoNew);

    // On setting geometry the onChanged method will call acceptGeometry(), thereby updating
    // constraint geometry indices and rebuilding the vertex index
    Geometry.setValues(std::move(newVals));

    return Geometry.getSize() - 1;
}

bool SketchObject::isClosedCurve(const Part::Geometry* geo)
{
    return (geo->is<Part::GeomCircle>()
            || geo->is<Part::GeomEllipse>()
            || (geo->is<Part::GeomBSplineCurve>()
                && static_cast<const Part::GeomBSplineCurve*>(geo)->isPeriodic()));
}

bool SketchObject::hasInternalGeometry(const Part::Geometry* geo)
{
    return (geo->is<Part::GeomEllipse>()
            || geo->is<Part::GeomArcOfEllipse>()
            || geo->is<Part::GeomArcOfHyperbola>()
            || geo->is<Part::GeomArcOfParabola>()
            || geo->is<Part::GeomBSplineCurve>());
}

int SketchObject::delGeometry(int GeoId, DeleteOptions options)
{
    if (GeoId < 0) {
        if(GeoId > GeoEnum::RefExt)
            return -1;
        return delExternal(-GeoId-1);
    }

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (GeoId >= int(vals.size())) {
        return -1;
    }

    if (options.testFlag(DeleteOption::IncludeInternalGeometry) && hasInternalGeometry(getGeometry(GeoId))) {
        // Only for supported types
        this->deleteUnusedInternalGeometry(GeoId, true);
        return 0;
    }

    std::vector<Part::Geometry*> newVals(vals);
    newVals.erase(newVals.begin() + GeoId);

    // Find coincident points to replace the points of the deleted geometry
    std::vector<int> GeoIdList;
    std::vector<PointPos> PosIdList;
    for (PointPos PosId : {PointPos::start, PointPos::end, PointPos::mid}) {
        getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
        if (GeoIdList.size() > 1) {
            delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
            transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
        }
    }

    const std::vector<Constraint*>& constraints = this->Constraints.getValues();
    std::vector<Constraint*> newConstraints;
    newConstraints.reserve(constraints.size());
    for (const auto& constr : constraints) {
        if (auto newConstr = getConstraintAfterDeletingGeo(constr, GeoId)) {
            newConstraints.push_back(newConstr.release());
        }
    }

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
        this->Constraints.setValues(std::move(newConstraints));
    }

    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::delGeometriesExclusiveList(const std::vector<int>& GeoIds, DeleteOptions options)
{
    std::vector<int> sGeoIds(GeoIds);

    std::ranges::sort(sGeoIds);
    if (sGeoIds.empty()) {
        return 0;
    }

    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();
    if (sGeoIds.front() < 0 || sGeoIds.back() >= int(vals.size())) {
        return -1;
    }

    std::vector<Part::Geometry*> newVals(vals);
    for (auto it = sGeoIds.rbegin(); it != sGeoIds.rend(); ++it) {
        int GeoId = *it;
        newVals.erase(newVals.begin() + GeoId);

        // Find coincident points to replace the points of the deleted geometry
        std::vector<int> GeoIdList;
        std::vector<PointPos> PosIdList;
        for (PointPos PosId : {PointPos::start, PointPos::end, PointPos::mid}) {
            getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
            if (GeoIdList.size() > 1) {
                delConstraintOnPoint(GeoId, PosId, true /* only coincidence */);
                transferConstraints(GeoIdList[0], PosIdList[0], GeoIdList[1], PosIdList[1]);
            }
        }
    }

    // Copy the original constraints
    std::vector<Constraint*> constraints;
    for (const auto& ptr : this->Constraints.getValues()) {
        constraints.push_back(ptr->clone());
    }
    for (auto itGeo = sGeoIds.rbegin(); itGeo != sGeoIds.rend(); ++itGeo) {
        const int GeoId = *itGeo;
        for (auto& constr : constraints) {
            changeConstraintAfterDeletingGeo(constr, GeoId);
        }
    }

    constraints.erase(std::remove_if(constraints.begin(),
                                     constraints.end(),
                                     [](const auto& constr) {
                                         return constr->Type == ConstraintType::None;
                                     }),
                      constraints.end());

    // Block acceptGeometry in OnChanged to avoid unnecessary checks and updates
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(newVals);
        this->Constraints.setValues(std::move(constraints));
    }
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::deleteAllGeometry(DeleteOptions options)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    std::vector<Part::Geometry*> newVals(0);
    std::vector<Constraint*> newConstraints(0);

    // Avoid unnecessary updates and checks as this is a transaction
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(newVals);
        this->Constraints.setValues(newConstraints);
    }
    // Update geometry indices and rebuild vertexindex now via onChanged, so that
    // ViewProvider::UpdateData is triggered.
    Geometry.touch();

    // if we do not have a recompute, the sketch must be solved to update the DoF of the solver
    if (noRecomputes && !options.testFlag(DeleteOption::NoSolve)) {
        solve(options.testFlag(DeleteOption::UpdateGeometry));
    }

    return 0;
}

int SketchObject::toggleConstruction(int GeoId)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId >= 0) {
        const std::vector<Part::Geometry*>& vals = getInternalGeometry();
        if (GeoId >= int(vals.size())) {
            return -1;
        }

        if (getGeometryFacade(GeoId)->isInternalAligned()) {
            return -1;
        }

        // While it may seem that there is not a need to trigger an update at this time, because the
        // solver has its own copy of the geometry, and updateColors of the viewprovider may be
        // triggered by the clearselection of the UI command, this won't update the elements widget, in
        // the accumulative of actions it is judged that it is worth to trigger an update here.

        std::unique_ptr<Part::Geometry> geo(vals[GeoId]->clone());
        auto gft = GeometryFacade::getFacade(geo.get());
        gft->setConstruction(!gft->getConstruction());
        this->Geometry.set1Value(GeoId, std::move(geo));
    }
    else {
        if (GeoId > GeoEnum::RefExt) {
            return -1;
        }

        const std::vector<Part::Geometry*>& extGeos = getExternalGeometry();
        std::unique_ptr<Part::Geometry> geo(extGeos[-GeoId - 1]->clone());
        auto egf = ExternalGeometryFacade::getFacade(geo.get());
        egf->setFlag(ExternalGeometryExtension::Defining, !egf->testFlag(ExternalGeometryExtension::Defining));
        this->ExternalGeo.set1Value(-GeoId - 1, std::move(geo));
    }

    solverNeedsUpdate = true;
    signalSolverUpdate();  // FIXME:  In theory this is totally redundant, but now seems required
                           // for UI to update.
    return 0;
}

int SketchObject::setConstruction(int GeoId, bool on)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

   Part::PropertyGeometryList *prop;
    int idx;
    if (GeoId >= 0) {
        prop = &Geometry;
        if (GeoId < Geometry.getSize())
            idx = GeoId;
        else
            return -1;
    }else if (GeoId <= GeoEnum::RefExt && -GeoId-1 < ExternalGeo.getSize()) {
        prop = &ExternalGeo;
        idx = -GeoId-1;
    }else
        return -1;

    // While it may seem that there is not a need to trigger an update at this time, because the
    // solver has its own copy of the geometry, and updateColors of the viewprovider may be
    // triggered by the clearselection of the UI command, this won't update the elements widget, in
    // the accumulative of actions it is judged that it is worth to trigger an update here.

    std::unique_ptr<Part::Geometry> geo(prop->getValues()[idx]->clone());
    if(prop == &Geometry)
        GeometryFacade::setConstruction(geo.get(), on);
    else {
        auto egf = ExternalGeometryFacade::getFacade(geo.get());
        egf->setFlag(ExternalGeometryExtension::Defining, on);
    }

    prop->set1Value(idx,std::move(geo));
    solverNeedsUpdate = true;
    return 0;
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomEllipse>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool major = false;
    bool minor = false;
    bool focus1 = false;
    bool focus2 = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::EllipseMajorDiameter:
            major = true;
            break;
        case Sketcher::EllipseMinorDiameter:
            minor = true;
            break;
        case Sketcher::EllipseFocus1:
            focus1 = true;
            break;
        case Sketcher::EllipseFocus2:
            focus2 = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    const auto* ellipse = static_cast<const Part::GeomEllipse*>(geo);

    Base::Vector3d center {ellipse->getCenter()};
    double majord {ellipse->getMajorRadius()};
    double minord {ellipse->getMinorRadius()};
    Base::Vector3d majdir {ellipse->getMajorAxisDir()};

    Base::Vector3d mindir = Vector3d(-majdir.y, majdir.x);

    Base::Vector3d majorpositiveend = center + majord * majdir;
    Base::Vector3d majornegativeend = center - majord * majdir;
    Base::Vector3d minorpositiveend = center + minord * mindir;
    Base::Vector3d minornegativeend = center - minord * mindir;

    double df = sqrt(majord * majord - minord * minord);

    Base::Vector3d focus1P = center + df * majdir;
    Base::Vector3d focus2P = center - df * majdir;

    if (!major) {
        Part::GeomLineSegment* lmajor = new Part::GeomLineSegment();
        lmajor->setPoints(majorpositiveend, majornegativeend);

        igeo.push_back(lmajor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMajorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!minor) {
        Part::GeomLineSegment* lminor = new Part::GeomLineSegment();
        lminor->setPoints(minorpositiveend, minornegativeend);

        igeo.push_back(lminor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMinorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus1) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focus1P);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus1;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus2) {
        Part::GeomPoint* pf2 = new Part::GeomPoint();
        pf2->setPoint(focus2P);
        igeo.push_back(pf2);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus2;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
    }

    addAndCleanup(igeo, icon);
    return incrgeo;
}

void SketchObject::addAndCleanup(std::vector<Part::Geometry*> igeo, std::vector<Constraint*> icon)
{
    this->addGeometry(igeo, true);
    this->addConstraints(icon);

    for (auto& geoToDelete : igeo) {
        delete geoToDelete;
    }

    for (auto& constraintToDelete : icon) {
        delete constraintToDelete;
    }
}

// TODO: This is a repeat of ellipse. Can we do some code reuse?
template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomArcOfEllipse>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool major = false;
    bool minor = false;
    bool focus1 = false;
    bool focus2 = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (const auto& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::EllipseMajorDiameter:
            major = true;
            break;
        case Sketcher::EllipseMinorDiameter:
            minor = true;
            break;
        case Sketcher::EllipseFocus1:
            focus1 = true;
            break;
        case Sketcher::EllipseFocus2:
            focus2 = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    const auto* aoe = static_cast<const Part::GeomArcOfEllipse*>(geo);

    Base::Vector3d center {aoe->getCenter()};
    double majord {aoe->getMajorRadius()};
    double minord {aoe->getMinorRadius()};
    Base::Vector3d majdir {aoe->getMajorAxisDir()};

    Base::Vector3d mindir {-majdir.y, majdir.x};

    Base::Vector3d majorpositiveend {center + majord * majdir};
    Base::Vector3d majornegativeend {center - majord * majdir};
    Base::Vector3d minorpositiveend {center + minord * mindir};
    Base::Vector3d minornegativeend {center - minord * mindir};

    double df = sqrt(majord * majord - minord * minord);

    Base::Vector3d focus1P {center + df * majdir};
    Base::Vector3d focus2P {center - df * majdir};

    if (!major) {
        Part::GeomLineSegment* lmajor = new Part::GeomLineSegment();
        lmajor->setPoints(majorpositiveend, majornegativeend);

        igeo.push_back(lmajor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMajorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!minor) {
        Part::GeomLineSegment* lminor = new Part::GeomLineSegment();
        lminor->setPoints(minorpositiveend, minornegativeend);

        igeo.push_back(lminor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseMinorDiameter;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus1) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focus1P);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus1;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!focus2) {
        Part::GeomPoint* pf2 = new Part::GeomPoint();
        pf2->setPoint(focus2P);
        igeo.push_back(pf2);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = EllipseFocus2;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
    }

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomArcOfHyperbola>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool major = false;
    bool minor = false;
    bool focus = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::HyperbolaMajor:
            major = true;
            break;
        case Sketcher::HyperbolaMinor:
            minor = true;
            break;
        case Sketcher::HyperbolaFocus:
            focus = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    const auto* aoh = static_cast<const Part::GeomArcOfHyperbola*>(geo);

    Base::Vector3d center {aoh->getCenter()};
    double majord {aoh->getMajorRadius()};
    double minord {aoh->getMinorRadius()};
    Base::Vector3d majdir {aoh->getMajorAxisDir()};

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    Base::Vector3d mindir = Vector3d(-majdir.y, majdir.x);

    Base::Vector3d majorpositiveend = center + majord * majdir;
    Base::Vector3d majornegativeend = center - majord * majdir;
    Base::Vector3d minorpositiveend = majorpositiveend + minord * mindir;
    Base::Vector3d minornegativeend = majorpositiveend - minord * mindir;

    double df = sqrt(majord * majord + minord * minord);

    Base::Vector3d focus1P = center + df * majdir;

    if (!major) {
        Part::GeomLineSegment* lmajor = new Part::GeomLineSegment();
        lmajor->setPoints(majorpositiveend, majornegativeend);

        igeo.push_back(lmajor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::HyperbolaMajor;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }
    if (!minor) {
        Part::GeomLineSegment* lminor = new Part::GeomLineSegment();
        lminor->setPoints(minorpositiveend, minornegativeend);

        igeo.push_back(lminor);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::HyperbolaMinor;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);

        incrgeo++;
    }
    if (!focus) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focus1P);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::HyperbolaFocus;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomArcOfParabola>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);
    // First we search what has to be restored
    bool focus = false;
    bool focus_to_vertex = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::ParabolaFocus:
            focus = true;
            break;
        case Sketcher::ParabolaFocalAxis:
            focus_to_vertex = true;
            break;
        default:
            return -1;
        }
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    const auto* aop = static_cast<const Part::GeomArcOfParabola*>(geo);

    Base::Vector3d center {aop->getCenter()};
    Base::Vector3d focusp {aop->getFocus()};

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    if (!focus) {
        Part::GeomPoint* pf1 = new Part::GeomPoint();
        pf1->setPoint(focusp);

        igeo.push_back(pf1);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::ParabolaFocus;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);
        incrgeo++;
    }

    if (!focus_to_vertex) {
        Part::GeomLineSegment* paxis = new Part::GeomLineSegment();
        paxis->setPoints(center, focusp);

        igeo.push_back(paxis);

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::ParabolaFocalAxis;
        newConstr->First = currentgeoid + incrgeo + 1;
        newConstr->FirstPos = Sketcher::PointPos::none;
        newConstr->Second = GeoId;

        icon.push_back(newConstr);

        incrgeo++;
    }

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

template <>
int SketchObject::exposeInternalGeometryForType<Part::GeomBSplineCurve>(const int GeoId)
{
    const Part::Geometry* geo = getGeometry(GeoId);

    const auto* bsp = static_cast<const Part::GeomBSplineCurve*>(geo);
    // First we search what has to be restored
    std::vector<int> controlpointgeoids(bsp->countPoles(), GeoEnum::GeoUndef);

    std::vector<int> knotgeoids(bsp->countKnots(), GeoEnum::GeoUndef);

    bool isfirstweightconstrained = false;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    // search for existing poles
    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::BSplineControlPoint:
            controlpointgeoids[constr->InternalAlignmentIndex] = constr->First;
            break;
        case Sketcher::BSplineKnotPoint:
            knotgeoids[constr->InternalAlignmentIndex] = constr->First;
            break;
        default:
            return -1;
        }
    }

    if (controlpointgeoids[0] != GeoEnum::GeoUndef) {
        isfirstweightconstrained =
            std::ranges::any_of(vals, [&controlpointgeoids](const auto& constr) {
                return (constr->Type == Sketcher::Weight && constr->First == controlpointgeoids[0]);
            });
    }

    int currentgeoid = getHighestCurveIndex();
    int incrgeo = 0;

    std::vector<Part::Geometry*> igeo;
    std::vector<Constraint*> icon;

    std::vector<Base::Vector3d> poles = bsp->getPoles();
    std::vector<double> weights = bsp->getWeights();
    std::vector<double> knots = bsp->getKnots();

    double distance_p0_p1 = (poles[1] - poles[0]).Length();// for visual purposes only

    for (size_t index = 0; index < controlpointgeoids.size(); ++index) {
        auto& cpGeoId = controlpointgeoids.at(index);
        if (cpGeoId != GeoEnum::GeoUndef) {
            continue;
        }

        // if controlpoint not existing
        Part::GeomCircle* pc = new Part::GeomCircle();
        pc->setCenter(poles[index]);
        pc->setRadius(distance_p0_p1 / 6);

        igeo.push_back(pc);
        incrgeo++;

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::BSplineControlPoint;
        newConstr->First = currentgeoid + incrgeo;
        newConstr->FirstPos = Sketcher::PointPos::mid;
        newConstr->Second = GeoId;
        newConstr->InternalAlignmentIndex = index;

        icon.push_back(newConstr);

        if (index == 0) {
            controlpointgeoids[0] = currentgeoid + incrgeo;
            if (weights[0] == 1.0) {
                // if the first weight is 1.0 it's probably going to be non-rational
                Sketcher::Constraint* newConstr3 = new Sketcher::Constraint();
                newConstr3->Type = Sketcher::Weight;
                newConstr3->First = controlpointgeoids[0];
                newConstr3->setValue(weights[0]);

                icon.push_back(newConstr3);

                isfirstweightconstrained = true;
            }

            continue;
        }

        if (isfirstweightconstrained && weights[0] == weights[index]) {
            // if pole-weight newly created AND first weight is radius-constrained,
            // AND these weights are equal, constrain them to be equal
            Sketcher::Constraint* newConstr2 = new Sketcher::Constraint();
            newConstr2->Type = Sketcher::Equal;
            newConstr2->First = currentgeoid + incrgeo;
            newConstr2->FirstPos = Sketcher::PointPos::none;
            newConstr2->Second = controlpointgeoids[0];
            newConstr2->SecondPos = Sketcher::PointPos::none;

            icon.push_back(newConstr2);
        }
    }

    for (size_t index = 0; index < knotgeoids.size(); ++index) {
        auto& kGeoId = knotgeoids.at(index);
        if (kGeoId != GeoEnum::GeoUndef) {
            continue;
        }

        // if knot point not existing
        Part::GeomPoint* kp = new Part::GeomPoint();

        kp->setPoint(bsp->pointAtParameter(knots[index]));

        igeo.push_back(kp);
        incrgeo++;

        Sketcher::Constraint* newConstr = new Sketcher::Constraint();
        newConstr->Type = Sketcher::InternalAlignment;
        newConstr->AlignmentType = Sketcher::BSplineKnotPoint;
        newConstr->First = currentgeoid + incrgeo;
        newConstr->FirstPos = Sketcher::PointPos::start;
        newConstr->Second = GeoId;
        newConstr->InternalAlignmentIndex = index;

        icon.push_back(newConstr);
    }

    Q_UNUSED(isfirstweightconstrained);

    addAndCleanup(igeo, icon);
    return incrgeo;  // number of added elements
}

int SketchObject::exposeInternalGeometry(int GeoId)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry* geo = getGeometry(GeoId);
    // Only for supported types
    if (geo->is<Part::GeomEllipse>()) {
        return exposeInternalGeometryForType<Part::GeomEllipse>(GeoId);
    }
    else if (geo->is<Part::GeomArcOfEllipse>()) {
        return exposeInternalGeometryForType<Part::GeomArcOfEllipse>(GeoId);
    }
    else if (geo->is<Part::GeomArcOfHyperbola>()) {
        return exposeInternalGeometryForType<Part::GeomArcOfHyperbola>(GeoId);
    }
    else if (geo->is<Part::GeomArcOfParabola>()) {
        return exposeInternalGeometryForType<Part::GeomArcOfParabola>(GeoId);
    }
    else if (geo->is<Part::GeomBSplineCurve>()) {
        return exposeInternalGeometryForType<Part::GeomBSplineCurve>(GeoId);
    }
    else
        return -1;// not supported type
}

int SketchObject::deleteUnusedInternalGeometry(int GeoId, bool delgeoid)
{
    if (GeoId < 0 || GeoId > getHighestCurveIndex())
        return -1;

    const Part::Geometry* geo = getGeometry(GeoId);
    // Only for supported types
    if (geo->is<Part::GeomEllipse>()
        || geo->is<Part::GeomArcOfEllipse>()
        || geo->is<Part::GeomArcOfHyperbola>()) {
        return deleteUnusedInternalGeometryWhenTwoFoci(GeoId, delgeoid);
    }

    if (geo->is<Part::GeomArcOfParabola>()) {
        return deleteUnusedInternalGeometryWhenOneFocus(GeoId, delgeoid);
    }

    if (geo->is<Part::GeomBSplineCurve>()) {
        return deleteUnusedInternalGeometryWhenBSpline(GeoId, delgeoid);
    }

    // Default case: type not supported
        return -1;
}

int SketchObject::deleteUnusedInternalGeometryWhenTwoFoci(int GeoId, bool delgeoid)
{
    int majorelementindex = -1;
    int minorelementindex = -1;
    int focus1elementindex = -1;
    int focus2elementindex = -1;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::EllipseMajorDiameter:
        case Sketcher::HyperbolaMajor:
            majorelementindex = constr->First;
            break;
        case Sketcher::EllipseMinorDiameter:
        case Sketcher::HyperbolaMinor:
            minorelementindex = constr->First;
            break;
        case Sketcher::EllipseFocus1:
        case Sketcher::HyperbolaFocus:
            focus1elementindex = constr->First;
            break;
        case Sketcher::EllipseFocus2:
            focus2elementindex = constr->First;
            break;
        default:
            return -1;
        }
    }

    // Hide unused geometry here
    int majorconstraints = 0;// number of constraints associated to the geoid of the major axis
    int minorconstraints = 0;
    int focus1constraints = 0;
    int focus2constraints = 0;

    for (const auto& constr : vals) {
        if (constr->involvesGeoId(majorelementindex))
            majorconstraints++;
        else if (constr->involvesGeoId(minorelementindex))
            minorconstraints++;
        else if (constr->involvesGeoId(focus1elementindex))
            focus1constraints++;
        else if (constr->involvesGeoId(focus2elementindex))
            focus2constraints++;
    }

    std::vector<int> delgeometries;

    // those with less than 2 constraints must be removed
    if (focus2constraints < 2)
        delgeometries.push_back(focus2elementindex);

    if (focus1constraints < 2)
        delgeometries.push_back(focus1elementindex);

    if (minorconstraints < 2)
        delgeometries.push_back(minorelementindex);

    if (majorconstraints < 2)
        delgeometries.push_back(majorelementindex);

    if (delgeoid)
        delgeometries.push_back(GeoId);

    // indices over an erased element get automatically updated!!
    std::sort(delgeometries.begin(), delgeometries.end(), std::greater<>());

    for (auto& dGeoId : delgeometries) {
        delGeometry(dGeoId, DeleteOption::UpdateGeometry);
    }

    int ndeleted = delgeometries.size();

    return ndeleted;// number of deleted elements
}

int SketchObject::deleteUnusedInternalGeometryWhenOneFocus(int GeoId, bool delgeoid)
{
    // if the focus-to-vertex line is constrained, then never delete the focus
    // if the line is unconstrained, then the line may be deleted,
    // in this case the focus may be deleted if unconstrained.
    int majorelementindex = -1;
    int focus1elementindex = -1;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::ParabolaFocus:
            focus1elementindex = constr->First;
            break;
        case Sketcher::ParabolaFocalAxis:
            majorelementindex = constr->First;
            break;
        default:
            return -1;
        }
    }

    // Hide unused geometry here
    // number of constraints associated to the geoid of the major axis other than the coincident
    // ones
    int majorconstraints = 0;
    int focus1constraints = 0;

    for (const auto& constr : vals) {
        if (constr->involvesGeoId(majorelementindex)) {
            majorconstraints++;
        }
        else if (constr->involvesGeoId(focus1elementindex)) {
            focus1constraints++;
        }
    }

    std::vector<int> delgeometries;

    // major has minimum one constraint, the specific internal alignment constraint
    if (majorelementindex != -1 && majorconstraints < 2)
        delgeometries.push_back(majorelementindex);

    // focus has minimum one constraint now, the specific internal alignment constraint
    if (focus1elementindex != -1 && focus1constraints < 2)
        delgeometries.push_back(focus1elementindex);

    if (delgeoid)
        delgeometries.push_back(GeoId);

    // indices over an erased element get automatically updated!!
    std::sort(delgeometries.begin(), delgeometries.end(), std::greater<>());

    for (auto& dGeoId : delgeometries) {
        delGeometry(dGeoId, DeleteOption::UpdateGeometry);
    }

    int ndeleted = delgeometries.size();

    delgeometries.clear();

    return ndeleted;// number of deleted elements
}

int SketchObject::deleteUnusedInternalGeometryWhenBSpline(int GeoId, bool delgeoid)
{
    // First we search existing IA
    std::map<int, int> poleGeoIdsAndConstraints;
    std::map<int, int> knotGeoIdsAndConstraints;

    const std::vector<Sketcher::Constraint*>& vals = Constraints.getValues();

    // search for existing poles
    for (auto const& constr : vals) {
        if (constr->Type != Sketcher::InternalAlignment || constr->Second != GeoId) {
            continue;
        }

        switch (constr->AlignmentType) {
        case Sketcher::BSplineControlPoint:
            poleGeoIdsAndConstraints[constr->First] = 0;
            break;
        case Sketcher::BSplineKnotPoint:
            knotGeoIdsAndConstraints[constr->First] = 0;
            break;
        default:
            return -1;
        }
    }

    std::vector<int> delgeometries;

    // Update all control point constraint counts.
    // EXCLUDES internal alignment and related constraints.
    for (auto const& constr : vals) {
        // We do not ignore weight constraints as we did with radius constraints,
        // because the radius magnitude no longer makes sense without the B-Spline.
        if (constr->Type == Sketcher::InternalAlignment
            || constr->Type == Sketcher::Weight) {
            continue;
        }
        bool firstIsInCPGeoIds = poleGeoIdsAndConstraints.count(constr->First) == 1;
        bool secondIsInCPGeoIds = poleGeoIdsAndConstraints.count(constr->Second) == 1;
        if (constr->Type == Sketcher::Equal && firstIsInCPGeoIds == secondIsInCPGeoIds) {
            continue;
        }
        // any equality constraint constraining a pole is not interpole
        if (firstIsInCPGeoIds) {
            ++poleGeoIdsAndConstraints[constr->First];
        }
        if (secondIsInCPGeoIds) {
            ++poleGeoIdsAndConstraints[constr->Second];
        }
    }

    for (auto& [cpGeoId, numConstr] : poleGeoIdsAndConstraints) {
        if (numConstr < 1) { // IA
            delgeometries.push_back(cpGeoId);
        }
    }

    for (auto& [kGeoId, numConstr] : knotGeoIdsAndConstraints) {
        // Update all control point constraint counts.
        // INCLUDES internal alignment and related constraints.
        auto tempGeoID = kGeoId;  // C++17 and earlier do not support captured structured bindings
        numConstr = std::count_if(vals.begin(), vals.end(), [&tempGeoID](const auto& constr) {
            return constr->involvesGeoId(tempGeoID);
        });

        if (numConstr < 2) { // IA
            delgeometries.push_back(kGeoId);
        }
    }

    if (delgeoid) {
        delgeometries.push_back(GeoId);
    }

    int ndeleted = delGeometriesExclusiveList(delgeometries);

    return ndeleted;// number of deleted elements
}

int SketchObject::deleteUnusedInternalGeometryAndUpdateGeoId(int& GeoId, bool delgeoid)
{
    const Part::Geometry* geo = getGeometry(GeoId);

    if (!hasInternalGeometry(geo)) {
        return -1;
    }
    // We need to remove the internal geometry of the BSpline, as BSplines change in number
    // of poles and knots We save the tags of the relevant geometry to retrieve the new
    // GeoIds later on.
    boost::uuids::uuid GeoIdTag;

    GeoIdTag = geo->getTag();

    int returnValue = deleteUnusedInternalGeometry(GeoId, delgeoid);

    if (delgeoid) {
        GeoId = GeoEnum::GeoUndef;
        return returnValue;
    }

    auto vals = getCompleteGeometry();

    for (size_t i = 0; i < vals.size(); i++) {
        if (vals[i]->getTag() == GeoIdTag) {
            GeoId = getGeoIdFromCompleteGeometryIndex(i);
            break;
        }
    }

    return returnValue;
}

const Part::Geometry* SketchObject::_getGeometry(int GeoId) const
{
    if (GeoId >= 0) {
        const std::vector<Part::Geometry *> &geomlist = getInternalGeometry();
        if (GeoId < int(geomlist.size()))
            return geomlist[GeoId];
    }
    else if (-GeoId-1 < ExternalGeo.getSize()) {
        return ExternalGeo[-GeoId-1];
    }

    return nullptr;
}

int SketchObject::getCompleteGeometryIndex(int GeoId) const
{
    if (GeoId >= 0) {
        if (GeoId < int(Geometry.getSize()))
            return GeoId;
    }
    else if (-GeoId <= int(ExternalGeo.getSize()))
        return -GeoId - 1;

    return GeoEnum::GeoUndef;
}

int SketchObject::getGeoIdFromCompleteGeometryIndex(int completeGeometryIndex) const
{
    int completeGeometryCount = int(Geometry.getSize() + ExternalGeo.getSize());

    if (completeGeometryIndex < 0 || completeGeometryIndex >= completeGeometryCount)
        return GeoEnum::GeoUndef;

    if (completeGeometryIndex < Geometry.getSize())
        return completeGeometryIndex;
    else
        return (completeGeometryIndex - completeGeometryCount);
}

std::unique_ptr<const GeometryFacade> SketchObject::getGeometryFacade(int GeoId) const
{
    return GeometryFacade::getFacade(getGeometry(GeoId));
}

std::vector<Part::Geometry*> SketchObject::getCompleteGeometry() const
{
    std::vector<Part::Geometry*> vals = getInternalGeometry();
    const auto &geos = getExternalGeometry();
    vals.insert(vals.end(), geos.rbegin(), geos.rend()); // in reverse order
    return vals;
}

GeoListFacade SketchObject::getGeoListFacade() const
{
    std::vector<GeometryFacadeUniquePtr> facade;
    facade.reserve(Geometry.getSize() + ExternalGeo.getSize());

    for (auto geo : Geometry.getValues())
        facade.push_back(GeometryFacade::getFacade(geo));

    const auto &externalGeos = ExternalGeo.getValues();
    for(auto rit = externalGeos.rbegin(); rit != externalGeos.rend(); rit++)
        facade.push_back(GeometryFacade::getFacade(*rit));

    return GeoListFacade::getGeoListModel(std::move(facade), Geometry.getSize());
}

void SketchObject::rebuildVertexIndex()
{
    VertexId2GeoId.resize(0);
    VertexId2PosId.resize(0);
    int imax = getHighestCurveIndex();
    int i = 0;
    const std::vector<Part::Geometry*> geometry = getCompleteGeometry();
    if (geometry.size() <= 2)
        return;
    for (std::vector<Part::Geometry*>::const_iterator it = geometry.begin();
         it != geometry.end() - 2;
         ++it, i++) {
        if (i > imax)
            i = -getExternalGeometryCount();
        if ((*it)->is<Part::GeomPoint>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
        }
        else if ((*it)->is<Part::GeomLineSegment>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
        }
        else if ((*it)->is<Part::GeomCircle>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomEllipse>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfCircle>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfEllipse>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfHyperbola>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomArcOfParabola>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::mid);
        }
        else if ((*it)->is<Part::GeomBSplineCurve>()) {
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::start);
            VertexId2GeoId.push_back(i);
            VertexId2PosId.push_back(PointPos::end);
        }
    }
}


void SketchObject::getGeometryWithDependentParameters(
    std::vector<std::pair<int, PointPos>>& geometrymap)
{
    auto geos = getInternalGeometry();

    int geoid = -1;

    for (auto geo : geos) {
        ++geoid;

        if (!geo) {
            continue;
        }

        if (!geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {
            continue;
        }

        auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
            geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

        if (solvext->getGeometry()
            != Sketcher::SolverGeometryExtension::NotFullyConstraint) {
            continue;
        }

        // The solver differentiates whether the parameters that are dependent are not
        // those of start, end, mid, and assigns them to the edge (edge params = curve
        // params - parms of start, end, mid). The user looking at the UI expects that
        // the edge of a NotFullyConstraint geometry will always move, even if the edge
        // parameters are independent, for example if mid is the only dependent
        // parameter. In other words, the user could reasonably restrict the edge to
        // reach a fully constrained element. Under this understanding, the edge
        // parameter would always be dependent, unless the element is fully constrained.
        //
        // While this is ok from a user visual expectation point of view, it leads to a
        // loss of information of whether restricting the point start, end, mid that is
        // dependent may suffice, or even if such points are restricted, the edge would
        // still need to be restricted.
        //
        // Because Python gets the information in this function, it would lead to Python
        // users having access to a lower amount of detail.
        //
        // For this reason, this function returns edge as dependent parameter if and
        // only if constraining the parameters of the points would not suffice to
        // constraint the element.
        if (solvext->getEdge() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::none);
        if (solvext->getStart() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::start);
        if (solvext->getEnd() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::start);
        if (solvext->getMid() == SolverGeometryExtension::Dependent)
            geometrymap.emplace_back(geoid, Sketcher::PointPos::start);
    }
}

void SketchObject::getGeoVertexIndex(int VertexId, int& GeoId, PointPos& PosId) const
{
    if (VertexId < 0 || VertexId >= int(VertexId2GeoId.size())) {
        GeoId = GeoEnum::GeoUndef;
        PosId = PointPos::none;
        return;
    }
    GeoId = VertexId2GeoId[VertexId];
    PosId = VertexId2PosId[VertexId];
}

int SketchObject::getVertexIndexGeoPos(int GeoId, PointPos PosId) const
{
    for (std::size_t i = 0; i < VertexId2GeoId.size(); i++) {
        if (VertexId2GeoId[i] == GeoId && VertexId2PosId[i] == PosId)
            return i;
    }

    return -1;
}

Part::TopoShape SketchObject::getEdge(const Part::Geometry *geo, const char *name) const
{
    Part::TopoShape shape(geo->toShape());
    // Originally in ComplexGeoData::setElementName
    // LinkStable/src/App/ComplexGeoData.cpp#L1631
    // No longer possible after map separated in ElementMap.cpp
    if ( !shape.hasElementMap() ) {
        shape.resetElementMap(std::make_shared<Data::ElementMap>());
    }
    shape.setElementName(Data::IndexedName::fromConst("Edge", 1),
                          Data::MappedName::fromRawData(name),0L);
    TopTools_IndexedMapOfShape vmap;
    TopExp::MapShapes(shape.getShape(), TopAbs_VERTEX, vmap);
    std::ostringstream ss;
    for(int i=1;i<=vmap.Extent();++i) {
        auto gpt = BRep_Tool::Pnt(TopoDS::Vertex(vmap(i)));
        Base::Vector3d pt(gpt.X(),gpt.Y(),gpt.Z());
        PointPos pos[] = {PointPos::start,PointPos::end};
        for(size_t j=0;j<sizeof(pos)/sizeof(pos[0]);++j) {
            if(getPoint(geo,pos[j]) == pt) {
                ss.str("");
                ss << name << 'v' << static_cast<int>(pos[j]);
                shape.setElementName(Data::IndexedName::fromConst("Vertex", i),
                                      Data::MappedName::fromRawData(ss.str().c_str()),0L);
                break;
            }
        }
    }
    return shape;
}

Data::IndexedName SketchObject::shapeTypeFromGeoId(int geoId, PointPos posId) const
{
    if (geoId == GeoEnum::HAxis) {
        if (posId == PointPos::start) {
            return Data::IndexedName::fromConst("RootPoint", 0);
        }
        return Data::IndexedName::fromConst("H_Axis", 0);
    }
    if (geoId == GeoEnum::VAxis) {
        return Data::IndexedName::fromConst("V_Axis", 0);
    }

    if (posId == PointPos::none) {
        auto geo = getGeometry(geoId);
        if (geo && geo->isDerivedFrom<Part::GeomPoint>()) {
            posId = PointPos::start;
        }
    }
    if(posId != PointPos::none) {
        int idx = getVertexIndexGeoPos(geoId, posId);
        if (idx < 0) {
            return Data::IndexedName();
        }
        return Data::IndexedName::fromConst("Vertex", idx + 1);
    }
    if (geoId >= 0) {
        return Data::IndexedName::fromConst("Edge", geoId + 1);
    }
    return Data::IndexedName::fromConst("ExternalEdge", -geoId - 2);
}

bool SketchObject::geoIdFromShapeType(const Data::IndexedName & indexedName,
                                      int &geoId,
                                      PointPos &posId) const
{
    posId = PointPos::none;
    geoId = Sketcher::GeoEnum::GeoUndef;
    if (!indexedName)
        return false;
    const char *shapetype = indexedName.getType();
    if (boost::equals(shapetype,"Edge") ||
        boost::equals(shapetype,"edge")) {
        geoId = indexedName.getIndex() - 1;
    } else if (boost::equals(shapetype,"ExternalEdge")) {
        geoId = indexedName.getIndex() - 1;
        geoId = Sketcher::GeoEnum::RefExt - geoId;
    } else if (boost::equals(shapetype,"Vertex") ||
               boost::equals(shapetype,"vertex")) {
        int VtId = indexedName.getIndex() - 1;
        getGeoVertexIndex(VtId,geoId,posId);
        if (posId==PointPos::none) return false;
    } else if (boost::equals(shapetype,"H_Axis")) {
        geoId = Sketcher::GeoEnum::HAxis;
    } else if (boost::equals(shapetype,"V_Axis")) {
        geoId = Sketcher::GeoEnum::VAxis;
    } else if (boost::equals(shapetype,"RootPoint")) {
        geoId = Sketcher::GeoEnum::RtPnt;
        posId = PointPos::start;
    } else
        return false;
    return true;
}

// SketchGeometryExtension interface

size_t setGeometryIdHelper(int GeoId, long id, std::vector<Part::Geometry*>& newVals, size_t searchOffset = 0, bool returnOnMatch = false)
{
    // deep copy
    for (size_t i = searchOffset; i < newVals.size(); i++) {
        newVals[i] = newVals[i]->clone();

        if ((int)i == GeoId) {
            auto gf = GeometryFacade::getFacade(newVals[i]);

            gf->setId(id);

            if (returnOnMatch) {
                return i;
            }
        }
    }
    return 0;
}
int SketchObject::setGeometryId(int GeoId, long id)
{
    // no need to check input data validity as this is an sketchobject managed operation.
    Base::StateLocker lock(managedoperation, true);

    if (GeoId < 0 || GeoId >= int(Geometry.getValues().size()))
        return -1;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);
    setGeometryIdHelper(GeoId, id, newVals);

    // There is not actual internal transaction going on here, however neither the geometry indices
    // nor the vertices need to be updated so this is a convenient way of preventing it.
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
    }

    return 0;
}
int SketchObject::setGeometryIds(std::vector<std::pair<int, long>> GeoIdsToIds)
{
    Base::StateLocker lock(managedoperation, true);

    std::sort(GeoIdsToIds.begin(), GeoIdsToIds.end());

    size_t searchOffset = 0;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    std::vector<Part::Geometry*> newVals(vals);

    for (size_t i = 0; i < GeoIdsToIds.size(); ++i) {
        int GeoId = GeoIdsToIds[i].first;
        long id = GeoIdsToIds[i].second;
        searchOffset = setGeometryIdHelper(GeoId, id, newVals, searchOffset, i != GeoIdsToIds.size()-1);
    }

    // There is not actual internal transaction going on here, however neither the geometry indices
    // nor the vertices need to be updated so this is a convenient way of preventing it.
    {
        Base::StateLocker preventUpdate(internaltransaction, true);
        this->Geometry.setValues(std::move(newVals));
    }

    return 0;
}

int SketchObject::getGeometryId(int GeoId, long& id) const
{
    if (GeoId < 0 || GeoId >= int(Geometry.getValues().size()))
        return -1;

    const std::vector<Part::Geometry*>& vals = getInternalGeometry();

    auto gf = GeometryFacade::getFacade(vals[GeoId]);

    id = gf->getId();

    return 0;
}
