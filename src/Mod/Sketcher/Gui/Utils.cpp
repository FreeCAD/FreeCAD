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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <cfloat>
# include <QMessageBox>
# include <Precision.hxx>
# include <QPainter>
#endif

#include <Base/Tools.h>
#include <Base/Tools2D.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludePropertyExternal.h>
#include <Gui/Action.h>
#include <Gui/BitmapFactory.h>
#include <Gui/DlgCheckableMessageBox.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Sketch.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"
#include "ui_InsertDatum.h"
#include "EditDatumDialog.h"
#include "Utils.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject* obj, bool &autoremoveredundants)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
    bool autoRemoveRedundants = hGrp->GetBool("AutoRemoveRedundants",false);

    // We need to make sure the solver has right redundancy information before trying to remove the redundants.
    // for example if a non-driving constraint has been added.
    if(autoRemoveRedundants && autoRecompute)
        obj->solve();

    if(autoRemoveRedundants)
        obj->autoRemoveRedundants();

    if (autoRecompute)
        Gui::Command::updateActive();

    autoremoveredundants = autoRemoveRedundants;

    return autoRecompute;
}

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject* obj)
{
    bool autoremoveredundants;

    return tryAutoRecompute(obj,autoremoveredundants);
}

void SketcherGui::tryAutoRecomputeIfNotSolve(Sketcher::SketchObject* obj)
{
    bool autoremoveredundants;

    if(!tryAutoRecompute(obj,autoremoveredundants)) {
        obj->solve();

        if(autoremoveredundants) {
            obj->autoRemoveRedundants();
        }
    }
}

std::string SketcherGui::getStrippedPythonExceptionString(const Base::Exception& e)
{
    std::string msg = e.what();

    if( msg.length() > 26 && msg.substr(0,26) == "FreeCAD exception thrown (") {
        return msg.substr(26, msg.length()-27);
    }
    else
        return msg;
}

bool SketcherGui::ReleaseHandler(Gui::Document* doc) {
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*> (doc->getInEdit());

            if (static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
                ->getSketchMode() == ViewProviderSketch::STATUS_SKETCH_UseHandler) {

                vp->purgeHandler();
                return true;
            }
        }
    }
    return false;
}

void SketcherGui::getIdsFromName(const std::string &name, const Sketcher::SketchObject* Obj,
                                 int &GeoId, PointPos &PosId)
{
    GeoId = GeoEnum::GeoUndef;
    PosId = Sketcher::PointPos::none;

    if (name.size() > 4 && name.substr(0,4) == "Edge") {
        GeoId = std::atoi(name.substr(4,4000).c_str()) - 1;
    }
    else if (name.size() == 9 && name.substr(0,9) == "RootPoint") {
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = Sketcher::PointPos::start;
    }
    else if (name.size() == 6 && name.substr(0,6) == "H_Axis")
        GeoId = Sketcher::GeoEnum::HAxis;
    else if (name.size() == 6 && name.substr(0,6) == "V_Axis")
        GeoId = Sketcher::GeoEnum::VAxis;
    else if (name.size() > 12 && name.substr(0,12) == "ExternalEdge")
        GeoId = Sketcher::GeoEnum::RefExt + 1 - std::atoi(name.substr(12,4000).c_str());
    else if (name.size() > 6 && name.substr(0,6) == "Vertex") {
        int VtId = std::atoi(name.substr(6,4000).c_str()) - 1;
        Obj->getGeoVertexIndex(VtId,GeoId,PosId);
    }
}

bool SketcherGui::checkBothExternal(int GeoId1, int GeoId2)
{
    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef)
        return false;
    else
        return (GeoId1 < 0 && GeoId2 < 0);
}

bool SketcherGui::checkBothExternalOrBSplinePoints(const Sketcher::SketchObject* Obj,int GeoId1, int GeoId2)
{
    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef)
        return false;
    else
        return (GeoId1 < 0 && GeoId2 < 0) || (isBsplineKnot(Obj,GeoId1) && isBsplineKnot(Obj,GeoId2)) ||
        (GeoId1 < 0 && isBsplineKnot(Obj,GeoId2)) || (GeoId2 < 0 && isBsplineKnot(Obj,GeoId1));
}

bool SketcherGui::isPointOrSegmentFixed(const Sketcher::SketchObject* Obj, int GeoId)
{
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (GeoId == GeoEnum::GeoUndef)
        return false;
    else
        return checkConstraint(vals, Sketcher::Block, GeoId, Sketcher::PointPos::none) || GeoId <= Sketcher::GeoEnum::RtPnt || isBsplineKnot(Obj,GeoId);
}

bool SketcherGui::areBothPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2)
{
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef)
        return false;
    else
        return ((checkConstraint(vals, Sketcher::Block, GeoId1, Sketcher::PointPos::none) || GeoId1 <= Sketcher::GeoEnum::RtPnt || isBsplineKnot(Obj,GeoId1)) &&
                (checkConstraint(vals, Sketcher::Block, GeoId2, Sketcher::PointPos::none) || GeoId2 <= Sketcher::GeoEnum::RtPnt || isBsplineKnot(Obj,GeoId2)));
}

bool SketcherGui::areAllPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2, int GeoId3)
{
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (GeoId1 == GeoEnum::GeoUndef || GeoId2 == GeoEnum::GeoUndef || GeoId3 == GeoEnum::GeoUndef)
        return false;
    else
        return ((checkConstraint(vals, Sketcher::Block, GeoId1, Sketcher::PointPos::none) || GeoId1 <= Sketcher::GeoEnum::RtPnt || isBsplineKnot(Obj,GeoId1)) &&
                (checkConstraint(vals, Sketcher::Block, GeoId2, Sketcher::PointPos::none) || GeoId2 <= Sketcher::GeoEnum::RtPnt || isBsplineKnot(Obj,GeoId2)) &&
                (checkConstraint(vals, Sketcher::Block, GeoId3, Sketcher::PointPos::none) || GeoId3 <= Sketcher::GeoEnum::RtPnt || isBsplineKnot(Obj,GeoId3)));
}

bool SketcherGui::isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, PointPos PosId)
{
    if (PosId == Sketcher::PointPos::start && (GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis))
        return true;
    const Part::Geometry *geo = Obj->getGeometry(GeoId);
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId())
        return true;
    else if (PosId == Sketcher::PointPos::mid)
        return true;
    else
        return false;
}

bool SketcherGui::isBsplineKnot(const Sketcher::SketchObject* Obj, int GeoId)
{
    auto gf = Obj->getGeometryFacade(GeoId);
    return (gf && gf->getInternalType() == Sketcher::InternalType::BSplineKnotPoint);
}

bool SketcherGui::isBsplineKnotOrEndPoint(const Sketcher::SketchObject* Obj, int GeoId, Sketcher::PointPos PosId)
{
    // check first using geometry facade
    if (isBsplineKnot(Obj, GeoId))
        return true;

    const Part::Geometry *geo = Obj->getGeometry(GeoId);
    // end points of B-Splines are also knots
    if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() &&
        (PosId == Sketcher::PointPos::start || PosId == Sketcher::PointPos::end))
        return true;

    return false;
}

bool SketcherGui::IsPointAlreadyOnCurve(int GeoIdCurve, int GeoIdPoint, Sketcher::PointPos PosIdPoint, Sketcher::SketchObject* Obj)
{
    //This func is a "smartness" behind three-element tangent-, perp.- and angle-via-point.
    //We want to find out, if the point supplied by user is already on
    // both of the curves. If not, necessary point-on-object constraints
    // are to be added automatically.
    //Simple geometric test seems to be the best, because a point can be
    // constrained to a curve in a number of ways (e.g. it is an endpoint of an
    // arc, or is coincident to endpoint of an arc, or it is an endpoint of an
    // ellipse's majopr diameter line). Testing all those possibilities is way
    // too much trouble, IMO(DeepSOIC).
    Base::Vector3d p = Obj->getPoint(GeoIdPoint, PosIdPoint);
    return Obj->isPointOnCurve(GeoIdCurve, p.x, p.y);
}

bool SketcherGui::isBsplinePole(const Part::Geometry * geo)
{
    auto gf = GeometryFacade::getFacade(geo);

    if(gf)
        return gf->getInternalType() == InternalType::BSplineControlPoint;

    THROWM(Base::ValueError, "Null geometry in isBsplinePole - please report")
}

bool SketcherGui::isBsplinePole(const Sketcher::SketchObject* Obj, int GeoId)
{

    auto geom = Obj->getGeometry(GeoId);

    return isBsplinePole(geom);
}

bool SketcherGui::checkConstraint(const std::vector< Sketcher::Constraint * > &vals, ConstraintType type, int geoid, PointPos pos)
{
    for (std::vector< Sketcher::Constraint * >::const_iterator itc= vals.begin(); itc != vals.end(); ++itc) {
        if ((*itc)->Type == type && (*itc)->First == geoid && (*itc)->FirstPos == pos){
            return true;
        }
    }

    return false;
}

/* helper functions ======================================================*/

// Return counter-clockwise angle from horizontal out of p1 to p2 in radians.
double SketcherGui::GetPointAngle (const Base::Vector2d &p1, const Base::Vector2d &p2)
{
  double dX = p2.x - p1.x;
  double dY = p2.y - p1.y;
  return dY >= 0 ? atan2(dY, dX) : atan2(dY, dX) + 2*M_PI;
}

void SketcherGui::ActivateHandler(Gui::Document *doc, DrawSketchHandler *handler)
{
    std::unique_ptr<DrawSketchHandler> ptr(handler);
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*> (doc->getInEdit());
            vp->purgeHandler();
            vp->activateHandler(ptr.release());
        }
    }
}

bool SketcherGui::isCommandActive(Gui::Document *doc, bool actsOnSelection)
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            auto mode = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
                ->getSketchMode();
            if (mode == ViewProviderSketch::STATUS_NONE ||
                mode == ViewProviderSketch::STATUS_SKETCH_UseHandler) {
                if (!actsOnSelection)
                    return true;
                else if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0)
                    return true;
            }
        }
    }

    return false;
}

SketcherGui::ViewProviderSketch* SketcherGui::getSketchViewprovider(Gui::Document *doc)
{
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom
            (SketcherGui::ViewProviderSketch::getClassTypeId()) )
            return dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    }
    return nullptr;
}

void SketcherGui::removeRedundantHorizontalVertical(Sketcher::SketchObject* psketch,
                                       std::vector<AutoConstraint> &sug1,
                                       std::vector<AutoConstraint> &sug2)
{
    if(!sug1.empty() && !sug2.empty()) {

        bool rmvhorvert = false;

        // we look for:
        // 1. Coincident to external on both endpoints
        // 2. Coincident in one endpoint to origin and pointonobject/tangent to an axis on the other
        auto detectredundant = [psketch](std::vector<AutoConstraint> &sug, bool &ext, bool &orig, bool &axis) {

            ext = false;
            orig = false;
            axis = false;

            for(std::vector<AutoConstraint>::const_iterator it = sug.begin(); it!=sug.end(); ++it) {
                if( (*it).Type == Sketcher::Coincident && ext == false) {
                    const std::map<int, Sketcher::PointPos> coincidents = psketch->getAllCoincidentPoints((*it).GeoId, (*it).PosId);

                    if(!coincidents.empty()) {
                        // the keys are ordered, so if the first is negative, it is coincident with external
                        ext = coincidents.begin()->first < 0;

                        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

                        geoId1iterator = coincidents.find(-1);

                        if( geoId1iterator != coincidents.end()) {
                            if( (*geoId1iterator).second == Sketcher::PointPos::start )
                                orig = true;
                        }
                    }
                    else { // it may be that there is no constraint at all, but there is external geometry
                        ext = (*it).GeoId < 0;
                        orig = ((*it).GeoId == -1 && (*it).PosId == Sketcher::PointPos::start);
                    }
                }
                else if( (*it).Type == Sketcher::PointOnObject && axis == false) {
                    axis = (((*it).GeoId == -1 && (*it).PosId == Sketcher::PointPos::none) || ((*it).GeoId == -2 && (*it).PosId == Sketcher::PointPos::none));
                }

            }
        };

        bool firstext = false, secondext = false, firstorig = false, secondorig = false, firstaxis = false, secondaxis = false;

        detectredundant(sug1, firstext, firstorig, firstaxis);
        detectredundant(sug2, secondext, secondorig, secondaxis);


        rmvhorvert = ((firstext && secondext)   ||  // coincident with external on both endpoints
                      (firstorig && secondaxis) ||  // coincident origin and point on object on other
                      (secondorig && firstaxis));

        if(rmvhorvert) {
            for(std::vector<AutoConstraint>::reverse_iterator it = sug2.rbegin(); it!=sug2.rend(); ++it) {
                if( (*it).Type == Sketcher::Horizontal || (*it).Type == Sketcher::Vertical) {
                    sug2.erase(std::next(it).base());
                    it = sug2.rbegin(); // erase invalidates the iterator
                }
            }
        }
    }
}

void SketcherGui::ConstraintToAttachment(Sketcher::GeoElementId element, Sketcher::GeoElementId attachment, double distance, App::DocumentObject* obj) {
    if (distance == 0.) {

        if(attachment.isCurve()) {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                element.GeoId, element.posIdAsInt(), attachment.GeoId);

        }
        else {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                element.GeoId, element.posIdAsInt(), attachment.GeoId, attachment.posIdAsInt());
        }
    }
    else {
        if(attachment == Sketcher::GeoElementId::VAxis) {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
                element.GeoId, element.posIdAsInt(), distance);
        }
        else if(attachment == Sketcher::GeoElementId::HAxis) {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
                element.GeoId, element.posIdAsInt(), distance);
        }
    }
}
