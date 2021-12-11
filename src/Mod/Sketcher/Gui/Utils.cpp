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
    bool autoUtils = hGrp->GetBool("AutoUtils",false);
    bool autoRemoveRedundants = hGrp->GetBool("AutoRemoveRedundants",false);

    // We need to make sure the solver has right redundancy information before trying to remove the redundants.
    // for example if a non-driving constraint has been added.
    if(autoRemoveRedundants && autoUtils)
        obj->solve();

    if(autoRemoveRedundants)
        obj->autoRemoveRedundants();

    if (autoUtils)
        Gui::Command::updateActive();

    autoremoveredundants = autoRemoveRedundants;

    return autoUtils;
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
