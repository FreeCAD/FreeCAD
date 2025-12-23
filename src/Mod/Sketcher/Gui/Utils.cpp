/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <QCursor>
#include <QLocale>
#include <QRegularExpression>

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool Sketcher::isCircle(const Part::Geometry& geom)
{
    return geom.is<Part::GeomCircle>();
}

bool Sketcher::isArcOfCircle(const Part::Geometry& geom)
{
    return geom.is<Part::GeomArcOfCircle>();
}

bool Sketcher::isEllipse(const Part::Geometry& geom)
{
    return geom.is<Part::GeomEllipse>();
}

bool Sketcher::isArcOfEllipse(const Part::Geometry& geom)
{
    return geom.is<Part::GeomArcOfEllipse>();
}

bool Sketcher::isLineSegment(const Part::Geometry& geom)
{
    return geom.is<Part::GeomLineSegment>();
}

bool Sketcher::isArcOfHyperbola(const Part::Geometry& geom)
{
    return geom.is<Part::GeomArcOfHyperbola>();
}

bool Sketcher::isArcOfParabola(const Part::Geometry& geom)
{
    return geom.is<Part::GeomArcOfParabola>();
}

bool Sketcher::isBSplineCurve(const Part::Geometry& geom)
{
    return geom.is<Part::GeomBSplineCurve>();
}

bool Sketcher::isPeriodicBSplineCurve(const Part::Geometry& geom)
{
    if (geom.is<Part::GeomBSplineCurve>()) {
        auto* spline = static_cast<const Part::GeomBSplineCurve*>(&geom);
        return spline->isPeriodic();
    }
    return false;
}

bool Sketcher::isPoint(const Part::Geometry& geom)
{
    return geom.is<Part::GeomPoint>();
}

bool Sketcher::isCircleOrArc(const Part::Geometry& geo)
{
    return isCircle(geo) || isArcOfCircle(geo);
};

std::tuple<double, Base::Vector3d> Sketcher::getRadiusCenterCircleArc(const Part::Geometry* geo)
{
    if (isArcOfCircle(*geo)) {
        auto arc = static_cast<const Part::GeomArcOfCircle*>(geo);  // NOLINT
        return std::tuple<double, Base::Vector3d>(arc->getRadius(), arc->getCenter());
    }
    else if (isCircle(*geo)) {
        auto circ = static_cast<const Part::GeomCircle*>(geo);  // NOLINT
        return std::tuple<double, Base::Vector3d>(circ->getRadius(), circ->getCenter());
    }

    THROWM(Base::TypeError, "getRadiusCenterCircleArc - Neither an arc nor a circle")
};

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject* obj, bool& autoremoveredundants)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher"
    );
    bool autoRecompute = hGrp->GetBool("AutoRecompute", false);
    bool autoRemoveRedundants = hGrp->GetBool("AutoRemoveRedundants", false);

    // We need to make sure the solver has right redundancy information before trying to remove the
    // redundants. for example if a non-driving constraint has been added.
    if (autoRemoveRedundants && autoRecompute) {
        obj->solve();
    }

    if (autoRemoveRedundants) {
        obj->autoRemoveRedundants();
    }

    if (autoRecompute) {
        Gui::Command::updateActive();
    }

    autoremoveredundants = autoRemoveRedundants;

    return autoRecompute;
}

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject* obj)
{
    bool autoremoveredundants;

    return tryAutoRecompute(obj, autoremoveredundants);
}

void SketcherGui::tryAutoRecomputeIfNotSolve(Sketcher::SketchObject* obj)
{
    bool autoremoveredundants;

    if (!tryAutoRecompute(obj, autoremoveredundants)) {
        obj->solve();

        if (autoremoveredundants) {
            obj->autoRemoveRedundants();
        }
    }
}

std::string SketcherGui::getStrippedPythonExceptionString(const Base::Exception& e)
{
    std::string msg = e.what();

    if (msg.length() > 26 && msg.substr(0, 26) == "FreeCAD exception thrown (") {
        return msg.substr(26, msg.length() - 27);
    }
    else {
        return msg;
    }
}

bool SketcherGui::ReleaseHandler(Gui::Document* doc)
{
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom<SketcherGui::ViewProviderSketch>()) {
            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(
                doc->getInEdit()
            );

            if (static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getSketchMode()
                == ViewProviderSketch::STATUS_SKETCH_UseHandler) {

                vp->purgeHandler();
                return true;
            }
        }
    }
    return false;
}

void SketcherGui::getIdsFromName(
    const std::string& name,
    const Sketcher::SketchObject* Obj,
    int& GeoId,
    PointPos& PosId
)
{
    GeoId = GeoEnum::GeoUndef;
    PosId = Sketcher::PointPos::none;

    if (name.size() > 4 && name.substr(0, 4) == "Edge") {
        GeoId = std::atoi(name.substr(4, 4000).c_str()) - 1;
    }
    else if (name.size() == 9 && name.substr(0, 9) == "RootPoint") {
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = Sketcher::PointPos::start;
    }
    else if (name.size() == 6 && name.substr(0, 6) == "H_Axis") {
        GeoId = Sketcher::GeoEnum::HAxis;
    }
    else if (name.size() == 6 && name.substr(0, 6) == "V_Axis") {
        GeoId = Sketcher::GeoEnum::VAxis;
    }
    else if (name.size() > 12 && name.substr(0, 12) == "ExternalEdge") {
        GeoId = Sketcher::GeoEnum::RefExt + 1 - std::atoi(name.substr(12, 4000).c_str());
    }
    else if (name.size() > 6 && name.substr(0, 6) == "Vertex") {
        int VtId = std::atoi(name.substr(6, 4000).c_str()) - 1;
        Obj->getGeoVertexIndex(VtId, GeoId, PosId);
    }
}

std::vector<int> SketcherGui::getGeoIdsOfEdgesFromNames(
    const Sketcher::SketchObject* Obj,
    const std::vector<std::string>& names
)
{
    std::vector<int> geoids;

    for (const auto& name : names) {
        if (name.size() > 4 && name.substr(0, 4) == "Edge") {
            geoids.push_back(std::atoi(name.substr(4, 4000).c_str()) - 1);
        }
        else if (name.size() > 12 && name.substr(0, 12) == "ExternalEdge") {
            geoids.push_back(Sketcher::GeoEnum::RefExt + 1 - std::atoi(name.substr(12, 4000).c_str()));
        }
        else if (name.size() > 6 && name.substr(0, 6) == "Vertex") {
            int VtId = std::atoi(name.substr(6, 4000).c_str()) - 1;
            int GeoId;
            Sketcher::PointPos PosId;
            Obj->getGeoVertexIndex(VtId, GeoId, PosId);
            const Part::Geometry* geo = Obj->getGeometry(GeoId);
            if (geo->is<Part::GeomPoint>()) {
                geoids.push_back(GeoId);
            }
        }
    }

    return geoids;
}

bool SketcherGui::checkBothExternal(int GeoId1, int GeoId2)
{
    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef) {
        return false;
    }
    else {
        return (GeoId1 < 0 && GeoId2 < 0);
    }
}

bool SketcherGui::isPointOrSegmentFixed(const Sketcher::SketchObject* Obj, int GeoId)
{
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    if (GeoId == GeoEnum::GeoUndef) {
        return false;
    }
    else {
        return checkConstraint(vals, Sketcher::Block, GeoId, Sketcher::PointPos::none)
            || GeoId <= Sketcher::GeoEnum::RtPnt;
    }
}

bool SketcherGui::areBothPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2)
{
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef) {
        return false;
    }
    else {
        return (
            (checkConstraint(vals, Sketcher::Block, GeoId1, Sketcher::PointPos::none)
             || GeoId1 <= Sketcher::GeoEnum::RtPnt)
            && (checkConstraint(vals, Sketcher::Block, GeoId2, Sketcher::PointPos::none)
                || GeoId2 <= Sketcher::GeoEnum::RtPnt)
        );
    }
}

bool SketcherGui::areAllPointsOrSegmentsFixed(
    const Sketcher::SketchObject* Obj,
    int GeoId1,
    int GeoId2,
    int GeoId3
)
{
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef || GeoId3 == GeoEnum::GeoUndef) {
        return false;
    }
    else {
        return (
            (checkConstraint(vals, Sketcher::Block, GeoId1, Sketcher::PointPos::none)
             || GeoId1 <= Sketcher::GeoEnum::RtPnt)
            && (checkConstraint(vals, Sketcher::Block, GeoId2, Sketcher::PointPos::none)
                || GeoId2 <= Sketcher::GeoEnum::RtPnt)
            && (checkConstraint(vals, Sketcher::Block, GeoId3, Sketcher::PointPos::none)
                || GeoId3 <= Sketcher::GeoEnum::RtPnt)
        );
    }
}

bool SketcherGui::isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, PointPos PosId)
{
    if (PosId == Sketcher::PointPos::start
        && (GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis)) {
        return true;
    }
    const Part::Geometry* geo = Obj->getGeometry(GeoId);
    if (geo->is<Part::GeomPoint>()) {
        return true;
    }
    else if (PosId == Sketcher::PointPos::mid) {
        return true;
    }
    else {
        return false;
    }
}

bool SketcherGui::isBsplineKnot(const Sketcher::SketchObject* Obj, int GeoId)
{
    auto gf = Obj->getGeometryFacade(GeoId);
    return (gf && gf->getInternalType() == Sketcher::InternalType::BSplineKnotPoint);
}

bool SketcherGui::isBsplineKnotOrEndPoint(
    const Sketcher::SketchObject* Obj,
    int GeoId,
    Sketcher::PointPos PosId
)
{
    // check first using geometry facade
    if (isBsplineKnot(Obj, GeoId)) {
        return true;
    }

    const Part::Geometry* geo = Obj->getGeometry(GeoId);
    // end points of B-Splines are also knots
    if (geo && geo->is<Part::GeomBSplineCurve>()
        && (PosId == Sketcher::PointPos::start || PosId == Sketcher::PointPos::end)) {
        return true;
    }

    return false;
}

bool SketcherGui::IsPointAlreadyOnCurve(
    int GeoIdCurve,
    int GeoIdPoint,
    Sketcher::PointPos PosIdPoint,
    Sketcher::SketchObject* Obj
)
{
    // This func is a "smartness" behind three-element tangent-, perp.- and angle-via-point.
    // We want to find out, if the point supplied by user is already on
    //  both of the curves. If not, necessary point-on-object constraints
    //  are to be added automatically.
    // Simple geometric test seems to be the best, because a point can be
    //  constrained to a curve in a number of ways (e.g. it is an endpoint of an
    //  arc, or is coincident to endpoint of an arc, or it is an endpoint of an
    //  ellipse's major diameter line). Testing all those possibilities is way
    //  too much trouble, IMO(DeepSOIC).
    //  One exception: check for knots on their B-splines, at least until point on B-spline is
    //  implemented. (Ajinkya)
    if (isBsplineKnot(Obj, GeoIdPoint)) {
        const Part::Geometry* geoCurve = Obj->getGeometry(GeoIdCurve);
        if (geoCurve->is<Part::GeomBSplineCurve>()) {
            const std::vector<Constraint*>& constraints = Obj->Constraints.getValues();
            for (const auto& constraint : constraints) {
                if (constraint->Type == Sketcher::ConstraintType::InternalAlignment
                    && constraint->First == GeoIdPoint && constraint->Second == GeoIdCurve) {
                    return true;
                }
            }
        }
    }

    Base::Vector3d p = Obj->getPoint(GeoIdPoint, PosIdPoint);
    return Obj->isPointOnCurve(GeoIdCurve, p.x, p.y);
}

bool SketcherGui::isBsplinePole(const Part::Geometry* geo)
{
    auto gf = GeometryFacade::getFacade(geo);

    if (gf) {
        return gf->getInternalType() == InternalType::BSplineControlPoint;
    }

    THROWM(Base::ValueError, "Null geometry in isBsplinePole - please report")
}

bool SketcherGui::isBsplinePole(const Sketcher::SketchObject* Obj, int GeoId)
{

    auto geom = Obj->getGeometry(GeoId);

    return isBsplinePole(geom);
}

bool SketcherGui::checkConstraint(
    const std::vector<Sketcher::Constraint*>& vals,
    ConstraintType type,
    int geoid,
    PointPos pos
)
{
    for (std::vector<Sketcher::Constraint*>::const_iterator itc = vals.begin(); itc != vals.end();
         ++itc) {
        if ((*itc)->Type == type && (*itc)->First == geoid && (*itc)->FirstPos == pos) {
            return true;
        }
    }

    return false;
}

/* helper functions ======================================================*/

// Return counter-clockwise angle from horizontal out of p1 to p2 in radians.
double SketcherGui::GetPointAngle(const Base::Vector2d& p1, const Base::Vector2d& p2)
{
    double dX = p2.x - p1.x;
    double dY = p2.y - p1.y;
    return dY >= 0 ? atan2(dY, dX) : atan2(dY, dX) + 2 * std::numbers::pi;
}

// Set the two points on circles at minimal distance
// in concentric case set points on relative X axis
void SketcherGui::GetCirclesMinimalDistance(
    const Part::Geometry* geom1,
    const Part::Geometry* geom2,
    Base::Vector3d& point1,
    Base::Vector3d& point2
)
{
    // This will throw if geom1 or geom2 are not circles or arcs
    auto [radius1, center1] = getRadiusCenterCircleArc(geom1);
    auto [radius2, center2] = getRadiusCenterCircleArc(geom2);

    point1 = center1;
    point2 = center2;

    Base::Vector3d v = point2 - point1;
    double length = v.Length();

    if (length == 0) {  // concentric case
        point1.x += radius1;
        point2.x += radius2;
    }
    else {
        v = v.Normalize();
        if (length <= std::max(radius1, radius2)) {  // inner case
            if (radius1 > radius2) {
                point1 += v * radius1;
                point2 += v * radius2;
            }
            else {
                point1 += -v * radius1;
                point2 += -v * radius2;
            }
        }
        else {  // outer case
            point1 += v * radius1;
            point2 += -v * radius2;
        }
    }
}

void SketcherGui::ActivateHandler(Gui::Document* doc, std::unique_ptr<DrawSketchHandler> handler)
{
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom<SketcherGui::ViewProviderSketch>()) {
            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*>(
                doc->getInEdit()
            );
            vp->purgeHandler();
            vp->activateHandler(std::move(handler));
        }
    }
}

bool SketcherGui::isSketchInEdit(Gui::Document* doc)
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        auto* vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        return (vp != nullptr);
    }
    return false;
}

bool SketcherGui::isCommandActive(Gui::Document* doc)
{
    if (isSketchInEdit(doc)) {
        auto mode = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->getSketchMode();

        if (mode == ViewProviderSketch::STATUS_NONE
            || mode == ViewProviderSketch::STATUS_SKETCH_UseHandler) {
            return true;
        }
    }

    return false;
}

bool SketcherGui::isCommandNeedingConstraintActive(Gui::Document* doc)
{
    if (!isCommandActive(doc)) {
        return false;
    }

    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(
        doc->getDocument()->getName(),
        Sketcher::SketchObject::getClassTypeId()
    );
    if (sel.size() == 1) {
        for (const std::string& name : sel[0].getSubNames()) {
            if (name.starts_with("Constraint")) {
                return true;
            }
        }
    }
    return false;
}

bool SketcherGui::isCommandNeedingGeometryActive(Gui::Document* doc)
{
    if (!isCommandActive(doc)) {
        return false;
    }

    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(
        doc->getDocument()->getName(),
        Sketcher::SketchObject::getClassTypeId()
    );
    if (sel.size() == 1) {
        auto* Obj = static_cast<Sketcher::SketchObject*>(sel[0].getObject());
        for (const std::string& name : sel[0].getSubNames()) {
            int geoId {GeoEnum::GeoUndef};
            PointPos posId {PointPos::none};
            getIdsFromName(name, Obj, geoId, posId);

            if (geoId != GeoEnum::GeoUndef) {
                return true;
            }
        }
    }
    return false;
}

bool SketcherGui::isCommandNeedingBSplineActive(Gui::Document* doc)
{
    if (!isCommandActive(doc)) {
        return false;
    }

    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(
        doc->getDocument()->getName(),
        Sketcher::SketchObject::getClassTypeId()
    );
    if (sel.size() == 1) {
        auto* Obj = static_cast<Sketcher::SketchObject*>(sel[0].getObject());
        for (const std::string& name : sel[0].getSubNames()) {

            int geoId {GeoEnum::GeoUndef};
            PointPos posId {PointPos::none};
            getIdsFromName(name, Obj, geoId, posId);

            if (geoId != GeoEnum::GeoUndef) {
                const Part::Geometry* geo = Obj->getGeometry(geoId);

                if (geo && geo->is<Part::GeomBSplineCurve>()) {
                    return true;
                }
            }
        }
    }
    return false;
}

SketcherGui::ViewProviderSketch* SketcherGui::getInactiveHandlerEditModeSketchViewProvider(
    Gui::Document* doc
)
{
    if (doc) {
        return dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    }

    return nullptr;
}

SketcherGui::ViewProviderSketch* SketcherGui::getInactiveHandlerEditModeSketchViewProvider()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();

    return getInactiveHandlerEditModeSketchViewProvider(doc);
}

void SketcherGui::removeRedundantHorizontalVertical(
    Sketcher::SketchObject* psketch,
    std::vector<AutoConstraint>& sug1,
    std::vector<AutoConstraint>& sug2
)
{
    if (sug1.empty() || sug2.empty()) {
        return;
    }

    bool rmvhorvert = false;

    // we look for:
    // 1. Coincident to external on both endpoints
    // 2. Coincident in one endpoint to origin and pointonobject/tangent to an axis on the other
    auto detectredundant =
        [psketch](std::vector<AutoConstraint>& sug, bool& ext, bool& orig, bool& axis) {
            ext = false;
            orig = false;
            axis = false;

            for (auto& it : sug) {
                if (it.Type == Sketcher::Coincident && !ext) {
                    const std::map<int, Sketcher::PointPos> coincidents
                        = psketch->getAllCoincidentPoints(it.GeoId, it.PosId);

                    if (!coincidents.empty()) {
                        // the keys are ordered, so if the first is negative, it is coincident
                        // with external
                        ext = coincidents.begin()->first < 0;

                        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

                        geoId1iterator = coincidents.find(-1);

                        if (geoId1iterator != coincidents.end()) {
                            if ((*geoId1iterator).second == Sketcher::PointPos::start) {
                                orig = true;
                            }
                        }
                    }
                    else {  // it may be that there is no constraint at all, but there is
                            // external geometry
                        ext = it.GeoId < 0;
                        orig = (it.GeoId == -1 && it.PosId == Sketcher::PointPos::start);
                    }
                }
                else if (it.Type == Sketcher::PointOnObject && !axis) {
                    axis
                        = ((it.GeoId == -1 && it.PosId == Sketcher::PointPos::none)
                           || (it.GeoId == -2 && it.PosId == Sketcher::PointPos::none));
                }
            }
        };

    bool firstext = false, secondext = false, firstorig = false, secondorig = false,
         firstaxis = false, secondaxis = false;

    detectredundant(sug1, firstext, firstorig, firstaxis);
    detectredundant(sug2, secondext, secondorig, secondaxis);


    rmvhorvert
        = ((firstext && secondext) ||    // coincident with external on both endpoints
           (firstorig && secondaxis) ||  // coincident origin and point on object on other
           (secondorig && firstaxis));

    if (rmvhorvert) {
        for (std::vector<AutoConstraint>::reverse_iterator it = sug2.rbegin(); it != sug2.rend();
             ++it) {
            if ((*it).Type == Sketcher::Horizontal || (*it).Type == Sketcher::Vertical) {
                sug2.erase(std::next(it).base());
                it = sug2.rbegin();  // erase invalidates the iterator
            }
        }
    }
}

void SketcherGui::ConstraintToAttachment(
    Sketcher::GeoElementId element,
    Sketcher::GeoElementId attachment,
    double distance,
    App::DocumentObject* obj
)
{
    if (distance == 0.) {

        if (attachment.isCurve()) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                element.GeoId,
                element.posIdAsInt(),
                attachment.GeoId
            );
        }
        else {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                element.GeoId,
                element.posIdAsInt(),
                attachment.GeoId,
                attachment.posIdAsInt()
            );
        }
    }
    else {
        if (attachment == Sketcher::GeoElementId::VAxis) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
                element.GeoId,
                element.posIdAsInt(),
                distance
            );
        }
        else if (attachment == Sketcher::GeoElementId::HAxis) {
            Gui::cmdAppObjectArgs(
                obj,
                "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
                element.GeoId,
                element.posIdAsInt(),
                distance
            );
        }
    }
}

void SketcherGui::ConstraintLineByAngle(int geoId, double angle, App::DocumentObject* obj)
{
    using std::numbers::pi;
    double angleModPi = std::fmod(angle, pi);
    double angleModHalfPi = std::fmod(angle, pi / 2);

    if (fabs(angleModPi - pi) < Precision::Confusion()
        || fabs(angleModPi + pi) < Precision::Confusion()
        || fabs(angleModPi) < Precision::Confusion()) {
        Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('Horizontal',%d)) ", geoId);
    }
    else if (fabs(angleModHalfPi - pi / 2) < Precision::Confusion()
             || fabs(angleModHalfPi + pi / 2) < Precision::Confusion()) {
        Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('Vertical',%d)) ", geoId);
    }
    else {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
            Sketcher::GeoEnum::HAxis,
            geoId,
            angle
        );
    }
}

void SketcherGui::Constraint2LinesByAngle(int geoId1, int geoId2, double angle, App::DocumentObject* obj)
{
    using std::numbers::pi;
    double angleModPi = std::fmod(angle, pi);
    double angleModHalfPi = std::fmod(angle, pi / 2);

    if (fabs(angleModPi) < Precision::Confusion()) {
        Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('Parallel',%d,%d)) ", geoId1, geoId2);
    }
    else if (fabs(angleModHalfPi) < Precision::Confusion()) {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Perpendicular',%d,%d)) ",
            geoId1,
            geoId2
        );
    }
    else {
        Gui::cmdAppObjectArgs(
            obj,
            "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
            geoId1,
            geoId2,
            angle
        );
    }
}

// convenience functions for cursor display
bool SketcherGui::hideUnits()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Sketcher");
    return hGrp->GetBool("HideUnits", false);
}

bool SketcherGui::showCursorCoords()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Sketcher");
    return hGrp->GetBool("ShowCursorCoords", true);  // true for testing. set to false for prod.
}

bool SketcherGui::useSystemDecimals()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Sketcher");
    return hGrp->GetBool("UseSystemDecimals", true);
}

// convert value to display format %0.[digits]f. Units are displayed if
// preference "ShowUnits" is true, or if the unit schema in effect uses
// multiple units (ex. Ft/In). Digits parameter is ignored for multi-unit
// schemata
// TODO:: if the user string is delivered in 1.23e45 format, this might not work
//        correctly.
std::string SketcherGui::lengthToDisplayFormat(double value, int digits)
{
    Base::Quantity asQuantity;
    asQuantity.setValue(value);
    asQuantity.setUnit(Base::Unit::Length);
    std::string userString = asQuantity.getUserString();
    if (Base::UnitsApi::isMultiUnitLength() || (!hideUnits() && useSystemDecimals())) {
        // just return the user string
        return userString;
    }

    // find the unit of measure
    double factor = 1.0;
    std::string unitString;
    std::string translate = Base::UnitsApi::schemaTranslate(asQuantity, factor, unitString);
    std::string unitPart = " " + unitString;

    // get the numeric part of the user string
    QRegularExpression rxNoUnits(QStringLiteral("(.*) \\D*$"));  // text before space + any non
                                                                 // digits at end of string
    QRegularExpressionMatch match = rxNoUnits.match(QString::fromStdString(userString));
    if (!match.hasMatch()) {
        // no units in userString?
        return userString;
    }
    QString matched = match.captured(1);  // matched is the numeric part of user string
    auto smatched = matched.toStdString();
    int dpPos = matched.indexOf(QLocale().decimalPoint());
    if (dpPos < 0) {
        // no decimal separator (ie an integer), return all the digits
        if (!hideUnits()) {
            smatched.append(unitPart);
        }
        return smatched;
    }

    // real number
    if (useSystemDecimals() && hideUnits()) {
        // return just the numeric part of the user string
        return smatched;
    }

    // real number and not using system decimals
    int requiredLength = dpPos + digits + 1;
    if (requiredLength > matched.size()) {
        // just take the whole thing
        requiredLength = matched.size();
    }
    auto numericPart = matched.left(requiredLength).toStdString();
    if (!hideUnits()) {
        numericPart.append(unitPart);
    }
    return numericPart;
}

// convert value to display format %0.[digits]f. Units are always displayed for
// angles - 123.456° or 12°34'56". Digits parameter is ignored for multi-unit
// schemata. Note small differences between this method and lengthToDisplyFormat
// TODO:: if the user string is delivered in 1.23e45 format, this might not work
//        correctly.
std::string SketcherGui::angleToDisplayFormat(double value, int digits)
{
    Base::Quantity asQuantity;
    asQuantity.setValue(value);
    asQuantity.setUnit(Base::Unit::Angle);
    QString qUserString = QString::fromStdString(asQuantity.getUserString());
    if (Base::UnitsApi::isMultiUnitAngle()) {
        // just return the user string
        // Coin SbString doesn't handle utf8 well, so we convert to ascii
        QString schemeMinute = QStringLiteral("\xE2\x80\xB2");  // prime symbol
        QString schemeSecond = QStringLiteral("\xE2\x80\xB3");  // double prime symbol
        QString escapeMinute = QStringLiteral("\'");            // substitute ascii single quote
        QString escapeSecond = QStringLiteral("\"");            // substitute ascii double quote
        QString displayString = qUserString.replace(schemeMinute, escapeMinute);
        displayString = displayString.replace(schemeSecond, escapeSecond);
        return displayString.toStdString();
    }

    // we always use use U+00B0 (°) as the unit of measure for angles in
    // single unit schema.  Will need a change to support rads or grads.
    std::string unitString = "°";
    auto decimalSep = QLocale().decimalPoint();

    // get the numeric part of the user string
    QRegularExpression rxNoUnits(
        QStringLiteral("(\\d*\\%1?\\d*)(\\D*)$").arg(decimalSep)
    );  // number + non digits at end of string
    QRegularExpressionMatch match = rxNoUnits.match(qUserString);
    if (!match.hasMatch()) {
        // no units in userString?
        return qUserString.toStdString();
    }
    QString matched = match.captured(1);  // matched is the numeric part of user string
    int dpPos = matched.indexOf(decimalSep);
    if (dpPos < 0 || useSystemDecimals()) {
        // just the numeric part of the user string + degree symbol
        auto angle = matched.toStdString();
        angle.append(unitString);
        return angle;
    }

    // real number and not using system decimals
    int requiredLength = dpPos + digits + 1;
    if (requiredLength > matched.size()) {
        // just take the whole thing
        requiredLength = matched.size();
    }
    auto numericPart = matched.left(requiredLength).toStdString();
    numericPart.append(unitString);
    return numericPart;
}


bool SketcherGui::areCollinear(
    const Base::Vector2d& p1,
    const Base::Vector2d& p2,
    const Base::Vector2d& p3
)
{
    Base::Vector2d u = p2 - p1;
    Base::Vector2d v = p3 - p2;
    Base::Vector2d w = p1 - p3;

    double uu = u * u;
    double vv = v * v;
    double ww = w * w;

    double eps2 = Precision::SquareConfusion();
    if (uu < eps2 || vv < eps2 || ww < eps2) {
        return true;
    }

    double uv = -(u * v);
    double vw = -(v * w);
    double uw = -(u * w);

    double w0 = (2 * sqrt(abs(uu * ww - uw * uw)) * uw / (uu * ww));
    double w1 = (2 * sqrt(abs(uu * vv - uv * uv)) * uv / (uu * vv));
    double w2 = (2 * sqrt(abs(vv * ww - vw * vw)) * vw / (vv * ww));

    double wx = w0 + w1 + w2;

    if (abs(wx) < Precision::Confusion()) {
        return true;
    }

    return false;
}

int SketcherGui::indexOfGeoId(const std::vector<int>& vec, int elem)
{
    if (elem == GeoEnum::GeoUndef) {
        return GeoEnum::GeoUndef;
    }
    for (size_t i = 0; i < vec.size(); i++) {
        if (vec[i] == elem) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
