/***************************************************************************
 *   Copyright (c) 2023 Pierre-Louis Boyer <pierrelouis.boyer@gmail.com>   *
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
#include <QApplication>
#endif  // #ifndef _PreComp_

#include <Mod/Sketcher/App/SketchObject.h>

#include "SnapManager.h"
#include "ViewProviderSketch.h"


using namespace SketcherGui;
using namespace Sketcher;

/************************************ Attorney *******************************************/

inline int ViewProviderSketchSnapAttorney::getPreselectPoint(const ViewProviderSketch& vp)
{
    return vp.getPreselectPoint();
}

inline int ViewProviderSketchSnapAttorney::getPreselectCross(const ViewProviderSketch& vp)
{
    return vp.getPreselectCross();
}

inline int ViewProviderSketchSnapAttorney::getPreselectCurve(const ViewProviderSketch& vp)
{
    return vp.getPreselectCurve();
}

/**************************** ParameterObserver nested class *****************************/
SnapManager::ParameterObserver::ParameterObserver(SnapManager& client)
    : client(client)
{
    initParameters();
    subscribeToParameters();
}

SnapManager::ParameterObserver::~ParameterObserver()
{
    unsubscribeToParameters();
}

void SnapManager::ParameterObserver::initParameters()
{
    // static map to avoid substantial if/else branching
    //
    // key->first               => String of parameter,
    // key->second              => Update function to be called for the parameter,
    str2updatefunction = {
        {"Snap",
         [this](const std::string& param) {
             updateSnapParameter(param);
         }},
        {"SnapToObjects",
         [this](const std::string& param) {
             updateSnapToObjectParameter(param);
         }},
        {"SnapToGrid",
         [this](const std::string& param) {
             updateSnapToGridParameter(param);
         }},
        {"SnapAngle",
         [this](const std::string& param) {
             updateSnapAngleParameter(param);
         }},
    };

    for (auto& val : str2updatefunction) {
        auto string = val.first;
        auto function = val.second;

        function(string);
    }
}

void SnapManager::ParameterObserver::updateSnapParameter(const std::string& parametername)
{
    ParameterGrp::handle hGrp = getParameterGrpHandle();

    client.snapRequested = hGrp->GetBool(parametername.c_str(), true);
}

void SnapManager::ParameterObserver::updateSnapToObjectParameter(const std::string& parametername)
{
    ParameterGrp::handle hGrp = getParameterGrpHandle();

    client.snapToObjectsRequested = hGrp->GetBool(parametername.c_str(), true);
}

void SnapManager::ParameterObserver::updateSnapToGridParameter(const std::string& parametername)
{
    ParameterGrp::handle hGrp = getParameterGrpHandle();

    client.snapToGridRequested = hGrp->GetBool(parametername.c_str(), false);
}

void SnapManager::ParameterObserver::updateSnapAngleParameter(const std::string& parametername)
{
    ParameterGrp::handle hGrp = getParameterGrpHandle();

    client.snapAngle = fmod(hGrp->GetFloat(parametername.c_str(), 5.) * M_PI / 180, 2 * M_PI);
}

void SnapManager::ParameterObserver::subscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = getParameterGrpHandle();
        hGrp->Attach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the exception is not propagated
        Base::Console().DeveloperError("SnapManager", "Malformed parameter string: %s\n", e.what());
    }
}

void SnapManager::ParameterObserver::unsubscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = getParameterGrpHandle();
        hGrp->Detach(this);
    }
    catch (const Base::ValueError&
               e) {  // ensure that if parameter strings are not well-formed, the program is not
                     // terminated when calling the noexcept destructor.
        Base::Console().DeveloperError("SnapManager", "Malformed parameter string: %s\n", e.what());
    }
}

void SnapManager::ParameterObserver::OnChange(Base::Subject<const char*>& rCaller,
                                              const char* sReason)
{
    (void)rCaller;

    auto key = str2updatefunction.find(sReason);
    if (key != str2updatefunction.end()) {
        auto string = key->first;
        auto function = key->second;

        function(string);
    }
}

ParameterGrp::handle SnapManager::ParameterObserver::getParameterGrpHandle()
{
    return App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Snap");
}

//**************************** SnapManager class ******************************

SnapManager::SnapManager(ViewProviderSketch& vp)
    : viewProvider(vp)
    , angleSnapRequested(false)
    , referencePoint(Base::Vector2d(0., 0.))
    , lastMouseAngle(0.0)
{
    // Create parameter observer and initialise watched parameters
    pObserver = std::make_unique<SnapManager::ParameterObserver>(*this);
}

SnapManager::~SnapManager()
{}

bool SnapManager::snap(double& x, double& y)
{
    if (!snapRequested) {
        return false;
    }

    // In order of priority :

    // 1 - Snap at an angle
    if (angleSnapRequested && QApplication::keyboardModifiers() == Qt::ControlModifier) {
        return snapAtAngle(x, y);
    }
    else {
        lastMouseAngle = 0.0;
    }

    // 2 - Snap to objects
    if (snapToObjectsRequested && snapToObject(x, y)) {
        return true;
    }

    // 3 - Snap to grid
    if (snapToGridRequested /*&& viewProvider.ShowGrid.getValue() */) {  // Snap to grid is enabled
                                                                         // even if the grid is not
                                                                         // visible.
        return snapToGrid(x, y);
    }

    return false;
}

bool SnapManager::snapAtAngle(double& x, double& y)
{
    Base::Vector2d pointToOverride(x, y);
    double length = (pointToOverride - referencePoint).Length();

    double angle1 = (pointToOverride - referencePoint).Angle();
    double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
    lastMouseAngle = abs(angle1 - lastMouseAngle) < abs(angle2 - lastMouseAngle) ? angle1 : angle2;

    double angle = round(lastMouseAngle / snapAngle) * snapAngle;
    pointToOverride = referencePoint + length * Base::Vector2d(cos(angle), sin(angle));
    x = pointToOverride.x;
    y = pointToOverride.y;

    return true;
}

bool SnapManager::snapToObject(double& x, double& y)
{
    Sketcher::SketchObject* Obj = viewProvider.getSketchObject();
    int geoId = GeoEnum::GeoUndef;
    Sketcher::PointPos posId = Sketcher::PointPos::none;

    int VtId = ViewProviderSketchSnapAttorney::getPreselectPoint(viewProvider);
    int CrsId = ViewProviderSketchSnapAttorney::getPreselectCross(viewProvider);
    int CrvId = ViewProviderSketchSnapAttorney::getPreselectCurve(viewProvider);

    if (CrsId == 0 || VtId >= 0) {
        if (CrsId == 0) {
            geoId = Sketcher::GeoEnum::RtPnt;
            posId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            Obj->getGeoVertexIndex(VtId, geoId, posId);
        }

        x = Obj->getPoint(geoId, posId).x;
        y = Obj->getPoint(geoId, posId).y;
        return true;
    }
    else if (CrsId == 1) {  // H_Axis
        y = 0;
        return true;
    }
    else if (CrsId == 2) {  // V_Axis
        x = 0;
        return true;
    }
    else if (CrvId >= 0 || CrvId <= Sketcher::GeoEnum::RefExt) {  // Curves

        const Part::Geometry* geo = Obj->getGeometry(CrvId);

        Base::Vector3d pointToOverride(x, y, 0.);

        double pointParam = 0.0;
        auto curve = dynamic_cast<const Part::GeomCurve*>(geo);
        if (curve) {
            try {
                curve->closestParameter(pointToOverride, pointParam);
                pointToOverride = curve->pointAtParameter(pointParam);
            }
            catch (Base::CADKernelError& e) {
                e.ReportException();
                return false;
            }

            // If it is a line, then we check if we need to snap to the middle.
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo);
                snapToLineMiddle(pointToOverride, line);
            }

            // If it is an arc, then we check if we need to snap to the middle (not the center).
            if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle* arc = static_cast<const Part::GeomArcOfCircle*>(geo);
                snapToArcMiddle(pointToOverride, arc);
            }

            x = pointToOverride.x;
            y = pointToOverride.y;

            return true;
        }
    }

    return false;
}

bool SnapManager::snapToGrid(double& x, double& y)
{
    // Snap Tolerance in pixels
    const double snapTol = viewProvider.getGridSize() / 5;

    double tmpX = x, tmpY = y;

    viewProvider.getClosestGridPoint(tmpX, tmpY);

    bool snapped = false;

    // Check if x within snap tolerance
    if (x < tmpX + snapTol && x > tmpX - snapTol) {
        x = tmpX;  // Snap X Mouse Position
        snapped = true;
    }

    // Check if y within snap tolerance
    if (y < tmpY + snapTol && y > tmpY - snapTol) {
        y = tmpY;  // Snap Y Mouse Position
        snapped = true;
    }

    return snapped;
}

bool SnapManager::snapToLineMiddle(Base::Vector3d& pointToOverride,
                                   const Part::GeomLineSegment* line)
{
    Base::Vector3d startPoint = line->getStartPoint();
    Base::Vector3d endPoint = line->getEndPoint();
    Base::Vector3d midPoint = (startPoint + endPoint) / 2;

    // Check if we are at middle of the line and if so snap to it.
    if ((pointToOverride - midPoint).Length() < (endPoint - startPoint).Length() * 0.05) {
        pointToOverride = midPoint;
        return true;
    }

    return false;
}

bool SnapManager::snapToArcMiddle(Base::Vector3d& pointToOverride, const Part::GeomArcOfCircle* arc)
{
    Base::Vector3d centerPoint = arc->getCenter();
    Base::Vector3d startVec = (arc->getStartPoint() - centerPoint);
    Base::Vector3d middleVec = startVec + (arc->getEndPoint() - centerPoint);

    /* Handle the case of arc angle = 180 */
    if (middleVec.Length() < Precision::Confusion()) {
        middleVec.x = startVec.y;
        middleVec.y = -startVec.x;
    }
    else {
        middleVec = middleVec / middleVec.Length() * arc->getRadius();
    }

    Base::Vector2d mVec = Base::Vector2d(middleVec.x, middleVec.y);
    Base::Vector3d pointVec = pointToOverride - centerPoint;
    Base::Vector2d pVec = Base::Vector2d(pointVec.x, pointVec.y);

    double u, v;
    arc->getRange(u, v, true);
    if (v < u) {
        v += 2 * M_PI;
    }
    double angle = v - u;
    int revert = angle < M_PI ? 1 : -1;

    /*To know if we are close to the middle of the arc, we are going to compare the angle of the
     * (mouse cursor - center) to the angle of the middle of the arc. If it's less than 10% of the
     * arc angle, then we snap.
     */
    if (fabs(pVec.Angle() - (revert * mVec).Angle()) < 0.10 * angle) {
        pointToOverride = centerPoint + middleVec * revert;
        return true;
    }

    return false;
}

void SnapManager::setAngleSnapping(bool enable, Base::Vector2d referencepoint)
{
    angleSnapRequested = enable;
    referencePoint = referencepoint;
}
