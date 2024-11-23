/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

//! DrawLeaderLine - a class for storing leader line attributes and providing methods to apply transformations on leader geometry.
//! Waypoints are to be stored as displacements from the first Waypoint in printed page coordinates (mm, conventional
//! X and Y axes, (0, 0) at lower left).  The first Waypoint is set to (0,0) after the displacements are calculated.
//! The leader's X,Y position is relative to the parent's origin.  The X,Y position is unrotated and unscaled.

#include "PreCompiled.h"

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

#include "DrawViewPart.h"
#include "DrawPage.h"
#include "DrawLeaderLine.h"
#include "DrawLeaderLinePy.h"  // generated from DrawLeaderLinePy.xml
#include "ArrowPropEnum.h"
#include "DrawView.h"
#include "Preferences.h"
#include "DrawUtil.h"


using namespace TechDraw;
using DU = DrawUtil;

//===========================================================================
// DrawLeaderLine - Base class for drawing leader based features
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawLeaderLine, TechDraw::DrawView)

//TODO: share this between DrawViewBalloon, DrawLeaderLine, QGIArrow, Prefs, etc
//const char* DrawLeaderLine::ArrowTypeEnums[]= {
//                               "FILLED_ARROW",
//                               "OPEN_ARROW",
//                               "TICK",
//                               "DOT",
//                               "OPEN_CIRCLE",
//                               "FORK",
//                               "FILLED_TRIANGLE",
//                               "NONE"
//                               NULL};
//const char* DrawLeaderLine::ArrowTypeEnums2[]= {
//                               "FILLED_ARROW",
//                               "OPEN_ARROW",
//                               "TICK",
//                               "DOT",
//                               "OPEN_CIRCLE",
//                               "FORK",
//                               "FILLED_TRIANGLE",
//                               "NONE"
//                               NULL};

DrawLeaderLine::DrawLeaderLine()
{
    static const char *group = "Leader";

    constexpr long int FilledArrow{0l};
    constexpr long int NoEnd{7l};

    ADD_PROPERTY_TYPE(LeaderParent, (nullptr), group, (App::PropertyType)(App::Prop_None),
                      "View to which this leader is attached");
    LeaderParent.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(WayPoints, (Base::Vector3d()) ,group, App::Prop_None,
                      "Intermediate points for Leader line");

    StartSymbol.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY(StartSymbol, (FilledArrow));              //filled arrow

    EndSymbol.setEnums(ArrowPropEnum::ArrowTypeEnums);
    ADD_PROPERTY(EndSymbol, (NoEnd));                //no symbol

    ADD_PROPERTY_TYPE(Scalable ,(false), group, App::Prop_None, "Scale line with LeaderParent");
    ADD_PROPERTY_TYPE(AutoHorizontal ,(getDefAuto()), group, App::Prop_None, "Forces last line segment to be horizontal");
    ADD_PROPERTY_TYPE(RotatesWithParent ,(true), group, App::Prop_None,
                      "If true, leader rotates around parent.  If false, only first segment of leader changes with parent rotation.");

    //hide the DrawView properties that don't apply to Leader
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::ReadOnly, true);
    Scale.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::ReadOnly, true);
    Rotation.setStatus(App::Property::Hidden, true);
    Caption.setStatus(App::Property::Hidden, true);

    LockPosition.setValue(true);
    LockPosition.setStatus(App::Property::Hidden, true);
}

short DrawLeaderLine::mustExecute() const
{
    if (!isRestoring() && LeaderParent.isTouched()) {
        return 1;  // Property changed
    }

    const App::DocumentObject* docObj = getBaseObject();
    if (docObj && docObj->isTouched()) {
        return 1;  // Object property points to is touched
    }

    if (WayPoints.isTouched()) {
        return 1;
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawLeaderLine::execute()
{
     // Base::Console().Message("DLL::execute()\n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }

    overrideKeepUpdated(false);
    return DrawView::execute();
}

DrawView* DrawLeaderLine::getBaseView() const
{
    App::DocumentObject* baseObj = LeaderParent.getValue();
    if (!baseObj) {
        return nullptr;
    }

    auto cast = dynamic_cast<DrawView*>(baseObj);
    return cast;
}

App::DocumentObject* DrawLeaderLine::getBaseObject() const
{
    return getBaseView();
}

bool DrawLeaderLine::keepUpdated()
{
    DrawView* view = getBaseView();
    if (!view) {
        return false;
    }
    return view->keepUpdated();
}

double DrawLeaderLine::getBaseScale() const
{
//    Base::Console().Message("DLL::getBaseScale()\n");
    DrawView* parent = getBaseView();
    if (!parent) {
        return 1.0;
    }
    return parent->getScale();
}

double DrawLeaderLine::getScale() const
{
//    Base::Console().Message("DLL::getScale()\n");
    if (!Scalable.getValue()) {
        return 1.0;
    }

    return getBaseScale();
}

Base::Vector3d DrawLeaderLine::getAttachPoint()
{
    return Base::Vector3d(
        X.getValue(),
        Y.getValue(),
        0.0
    );
}

//! unit agnostic conversion of last segment to horizontal.  need to do this at drawing time otherwise
//! we just realign the canonical form.
std::vector<Base::Vector3d> DrawLeaderLine::horizLastSegment(const std::vector<Base::Vector3d>& inDeltas, double rotationDeg)
{

    Base::Vector3d stdX{1, 0, 0};

    std::vector<Base::Vector3d> wp = inDeltas;
    if (wp.size() > 1) {
        size_t iLast = wp.size() - 1;
        size_t iPen  = wp.size() - 2;
        Base::Vector3d last = wp.at(iLast);
        Base::Vector3d penUlt = wp.at(iPen);

        auto lastSeg = DU::invertY(last - penUlt);
        auto lastSegLong = lastSeg.Length();
        auto lastSegRotated = lastSeg;
        lastSegRotated.RotateZ(Base::toRadians(rotationDeg));
        auto lastSegRotatedUnit = lastSegRotated;
        lastSegRotatedUnit.Normalize();
        auto dot = lastSegRotatedUnit.Dot(stdX);

        auto newLast = penUlt + stdX * lastSegLong;
        if (dot < 0) {
            newLast = penUlt - stdX * lastSegLong;
        }

        wp.at(iLast) = newLast;
    }
    return wp;
}


//! returns the waypoints with scale, rotation and horizontal last applied.
std::vector<Base::Vector3d> DrawLeaderLine::getTransformedWayPoints() const
{
    auto doScale = Scalable.getValue();
    auto doRotate = RotatesWithParent.getValue();
    auto vPoints =  getScaledAndRotatedPoints(doScale, doRotate);
    if (AutoHorizontal.getValue()) {
        vPoints = DrawLeaderLine::horizLastSegment(vPoints, getBaseView()->Rotation.getValue());
    }

    return vPoints;
}

//! returns the mid point of last segment.  used by leader decorators like weld symbol.
//! the returned point is unscaled and unrotated.
Base::Vector3d DrawLeaderLine::getTileOrigin() const
{
    std::vector<Base::Vector3d> wp = getTransformedWayPoints();
    if (wp.size() > 1) {
        Base::Vector3d last = wp.rbegin()[0];
        Base::Vector3d second = wp.rbegin()[1];
        return (last + second) / 2;
    }

    return Base::Vector3d();
}

//! returns start of last line segment
Base::Vector3d DrawLeaderLine::getKinkPoint() const
{
    std::vector<Base::Vector3d> wp = getTransformedWayPoints();
    if (wp.size() > 1) {
        return wp.rbegin()[1];  // second point from end
    }

    return Base::Vector3d();
}

//end of last line segment
Base::Vector3d DrawLeaderLine::getTailPoint() const
{
    std::vector<Base::Vector3d> wp = getTransformedWayPoints();
    if (!wp.empty()) {
        return wp.rbegin()[0];  // Last
    }

    return Base::Vector3d();
}

//! returns the (transformed) direction of the last segment of the leader line
Base::Vector3d DrawLeaderLine::lastSegmentDirection() const
{
    std::vector<Base::Vector3d> wp = getTransformedWayPoints();
    if (wp.empty()) {
        return Base::Vector3d(1,0,0);
    }
    // this direction is in conventional coords? Y+ up?
    // vertical segment will be small negative Y - large negative Y  -> a positive Y but we want a negative Y
    auto tailPoint = DU::invertY(wp.rbegin()[0]);
    auto kinkPoint = DU::invertY(wp.rbegin()[1]);
    auto direction = kinkPoint - tailPoint;  // from kink to tail
    direction = DU::invertY(direction);
    direction.Normalize();
    return direction;
}

//! create a new leader feature from parameters.  Used by python method makeLeader.
//! pagePoints are in mm from bottom left of page.
DrawLeaderLine* DrawLeaderLine::makeLeader(DrawViewPart* parent, std::vector<Base::Vector3d> pagePoints, int iStartSymbol, int iEndSymbol)
{
    Base::Console().Message("DLL::makeLeader(%s, %d, %d, %d)\n", parent->getNameInDocument(), pagePoints.size(), iStartSymbol, iEndSymbol);
    if (pagePoints.size() < 2) {
        Base::Console().Message("DLL::makeLeader - not enough pagePoints\n");
        return {};
    }

    // this is +/- the same code as in TaskLeaderLine::createLeaderFeature()
    const std::string objectName{"LeaderLine"};
    const std::string leaderType = "TechDraw::DrawLeaderLine";
    std::string leaderName = parent->getDocument()->getUniqueObjectName(objectName.c_str());
    std::string pageName = parent->findParentPage()->getNameInDocument();
    std::string parentName = parent->getNameInDocument();

    Base::Interpreter().runStringArg("App.activeDocument().addObject('%s', '%s')",
                                     leaderType.c_str(), leaderName.c_str());
    Base::Interpreter().runStringArg("App.activeDocument().%s.translateLabel('DrawLeaderLine', 'LeaderLine', '%s')",
                                     leaderName.c_str(), leaderName.c_str());
    Base::Interpreter().runStringArg("App.activeDocument().%s.addView(App.activeDocument().%s)",
                                     pageName.c_str(), leaderName.c_str());
    Base::Interpreter().runStringArg("App.activeDocument().%s.LeaderParent = App.activeDocument().%s",
                                     leaderName.c_str(), parentName.c_str());
    // we assume here that the caller will handle AutoHorizontal, Scalable and Rotatable as required


    App::DocumentObject* obj = parent->getDocument()->getObject(leaderName.c_str());
    if (!obj || !obj->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
        throw Base::RuntimeError("DrawLeaderLine::makeLeader - new object not found");
    }

    // set leader x,y position
    auto leaderFeature = static_cast<TechDraw::DrawLeaderLine*>(obj);
    Base::Vector3d parentPagePos{ parent->X.getValue(), parent->Y.getValue(), 0.0};
    Base::Vector3d leaderPagePos = pagePoints.front() - parentPagePos;
    bool force = true;   // update position even though leaders default to locked.
    leaderFeature->setPosition(leaderPagePos.x, leaderPagePos.y, force);

    // page positions to deltas
    std::vector<Base::Vector3d> pageDeltas;
    for (auto& point : pagePoints) {
        auto temp = point - pagePoints.front();
        pageDeltas.emplace_back(temp);
    }

    // deltas to unscaled, unrotated form
    auto leaderPoints = leaderFeature->makeCanonicalPoints(pageDeltas);
    // invert the canonical points
    std::vector<Base::Vector3d> inverted;
    inverted.reserve(leaderPoints.size());
    for (auto& point : leaderPoints) {
        inverted.push_back(DU::invertY(point));
    }
    leaderFeature->WayPoints.setValues(inverted);

    leaderFeature->StartSymbol.setValue(iStartSymbol);
    leaderFeature->EndSymbol.setValue(iEndSymbol);

    parent->touch();

    return leaderFeature;
}

//! return scaled and rotated copies of the WayPoints (page position deltas from 1st point)
//! used by QGILL.
std::vector<Base::Vector3d>  DrawLeaderLine::getScaledAndRotatedPoints(bool doScale, bool doRotate) const
{
    auto dvp = getBaseView();
    if (!dvp) {
        // document is restoring?
        // Base::Console().Message("DLL::getScaledAndRotatedPoints - no DV\n");
        return {};
    }

    double scale{1.0};
    if (Scalable.getValue() && doScale) {
        scale = dvp->getScale();
    }

    double rotationRad{0.0};
    if (doRotate) {
        rotationRad = dvp->Rotation.getValue() * M_PI / DegreesHalfCircle;
    }

    std::vector<Base::Vector3d> pointsAll = WayPoints.getValues();
    std::vector<Base::Vector3d> result;
    for (auto& point : pointsAll) {
        Base::Vector3d newPoint = DU::invertY(point * scale);
        if (rotationRad != 0.0) {
            // the waypoints use conventional coords
            newPoint.RotateZ(rotationRad);
        }
        result.push_back(DU::invertY(newPoint));
    }

    return result;
}

//! return unscaled and unrotated versions of the input points. input is expected to in mm (Rez::appX()),
//! and conventional Y axis (+ up)
//! used by QGILL.
std::vector<Base::Vector3d>
DrawLeaderLine::makeCanonicalPoints(const std::vector<Base::Vector3d>& inPoints,
                                    bool doScale,
                                    bool doRotate) const
{
    auto dvp = getBaseView();

    double scale{1.0};
    if (Scalable.getValue() && doScale) {
        scale = dvp->getScale();
    }

    double rotationRad{0.0};
    if (doRotate) {
        rotationRad = - dvp->Rotation.getValue() * M_PI / DegreesHalfCircle;
    }

    std::vector<Base::Vector3d> result;
    for (auto& point : inPoints) {
        Base::Vector3d newPoint = point / scale;
        if (rotationRad != 0.0) {
            newPoint.RotateZ(rotationRad);
        }
        result.push_back(newPoint);
    }

    return result;
}

//! as makeCanonicalPoints, but accepts and returns inverted points
std::vector<Base::Vector3d>
DrawLeaderLine::makeCanonicalPointsInverted(const std::vector<Base::Vector3d>& inPoints,
                                    bool doScale,
                                    bool doRotate) const
{
    std::vector<Base::Vector3d> conventionalPoints;
    conventionalPoints.reserve(inPoints.size());
    for (auto& point : inPoints) {
        conventionalPoints.push_back(DU::invertY(point));
    }

    auto conventionalCanon = makeCanonicalPoints(conventionalPoints, doScale, doRotate);

    std::vector<Base::Vector3d> invertedPoints;
    invertedPoints.reserve(inPoints.size());
    for (auto& point : conventionalCanon) {
        invertedPoints.push_back(DU::invertY(point));
    }

    return invertedPoints;
}

//! returns true if parent exists. if parent is a DVP it must have geometry.
bool DrawLeaderLine::isParentReady() const
{
    TechDraw::DrawView* parent = getBaseView();
    auto dvp = dynamic_cast<TechDraw::DrawViewPart*>(parent);
    if (!parent || (dvp && !dvp->hasGeometry()))  {
        // still restoring or
        // we are attached to a dvp that has no geometry, so don't bother trying to draw yet
        Base::Console().Message("DLL:: - no parent or geometry\n");
        return false;
    }
    return true;
}

bool DrawLeaderLine::getDefAuto() const
{
    return Preferences::getPreferenceGroup("LeaderLine")->GetBool("AutoHorizontal", true);
}

void DrawLeaderLine::dumpWaypoints(const std::vector<Base::Vector3d> &points, const std::string &label)
{
    Base::Console().Message("DLL::dumpWaypoints - %s\n", label.c_str());
    for (auto& p : points) {
        Base::Console().Message(">>>> a point: %s\n", DU::formatVector(p).c_str());
    }
}


PyObject *DrawLeaderLine::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawLeaderLinePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawLeaderLinePython, TechDraw::DrawLeaderLine)
template<> const char* TechDraw::DrawLeaderLinePython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderLeader";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawLeaderLine>;
}

