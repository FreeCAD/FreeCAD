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

#include <QApplication>

#include <Base/Tools.h>
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
        {"Snap", [this](const std::string& param) { updateSnapParameter(param); }},
        {"SnapToObjects", [this](const std::string& param) { updateSnapToObjectParameter(param); }},
        {"SnapToGrid", [this](const std::string& param) { updateSnapToGridParameter(param); }},
        {"SnapAngle", [this](const std::string& param) { updateSnapAngleParameter(param); }},
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

    client.snapAngle
        = fmod(Base::toRadians(hGrp->GetFloat(parametername.c_str(), 5.)), 2 * std::numbers::pi);
}

void SnapManager::ParameterObserver::subscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = getParameterGrpHandle();
        hGrp->Attach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the exception is not propagated
        Base::Console().developerError("SnapManager", "Malformed parameter string: %s\n", e.what());
    }
}

void SnapManager::ParameterObserver::unsubscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = getParameterGrpHandle();
        hGrp->Detach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the program is not terminated when calling the noexcept
                                         // destructor.
        Base::Console().developerError("SnapManager", "Malformed parameter string: %s\n", e.what());
    }
}

void SnapManager::ParameterObserver::OnChange(Base::Subject<const char*>& rCaller, const char* sReason)
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
        "User parameter:BaseApp/Preferences/Mod/Sketcher/Snap"
    );
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

Base::Vector2d SnapManager::snap(Base::Vector2d inputPos, SnapType mask)
{
    if (!snapRequested) {
        return inputPos;
    }

    Base::Vector2d snapPos = inputPos;

    // In order of priority:

    // 1 - Snap at an angle
    if ((static_cast<int>(mask) & static_cast<int>(SnapType::Angle)) && angleSnapRequested
        && QApplication::keyboardModifiers() == Qt::ControlModifier
        && snapAtAngle(inputPos, snapPos)) {
        return snapPos;
    }
    else {
        lastMouseAngle = 0.0;
    }

    // 2 - Snap to objects (may partially snap to axis, leaving other coordinate for grid)
    bool snappedToObject = false;
    if ((static_cast<int>(mask)
         & (static_cast<int>(SnapType::Point) | static_cast<int>(SnapType::Edge)))
        && snapToObjectsRequested) {
        snappedToObject = snapToObject(inputPos, snapPos, mask);
        if (snappedToObject) {
            return snapPos;  // Full snap (point or curve) - done
        }
        // if false was returned but snapPos was modified (axis case), continue to grid snap
    }

    // 3 - Snap to grid (will work on coordinates not already locked by axis snap)
    if ((static_cast<int>(mask) & static_cast<int>(SnapType::Grid)) && snapToGridRequested
        /*&& viewProvider.ShowGrid.getValue() */) {  // Snap to grid is enabled
                                                     // even if the grid is not visible.

        // use snapPos as input (which may have one coordinate locked by axis)
        Base::Vector2d gridSnapResult = snapPos;
        if (snapToGrid(snapPos, gridSnapResult)) {
            return gridSnapResult;
        }
        // if grid snap happened, return the result which combines axis + grid
        // if axis locked a coordinate, snapPos already had it, and grid snap respected it
        return snapPos;
    }

    return inputPos;
}

bool SnapManager::snapAtAngle(Base::Vector2d inputPos, Base::Vector2d& snapPos)
{
    double length = (inputPos - referencePoint).Length();

    double angle1 = (inputPos - referencePoint).Angle();
    double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * std::numbers::pi;
    lastMouseAngle = abs(angle1 - lastMouseAngle) < abs(angle2 - lastMouseAngle) ? angle1 : angle2;

    double angle = round(lastMouseAngle / snapAngle) * snapAngle;
    snapPos = referencePoint + length * Base::Vector2d(cos(angle), sin(angle));

    return true;
}

bool SnapManager::snapToObject(Base::Vector2d inputPos, Base::Vector2d& snapPos, SnapType mask)
{
    Sketcher::SketchObject* Obj = viewProvider.getSketchObject();
    int geoId = GeoEnum::GeoUndef;
    Sketcher::PointPos posId = Sketcher::PointPos::none;

    int VtId = ViewProviderSketchSnapAttorney::getPreselectPoint(viewProvider);
    int CrsId = ViewProviderSketchSnapAttorney::getPreselectCross(viewProvider);
    int CrvId = ViewProviderSketchSnapAttorney::getPreselectCurve(viewProvider);

    if ((static_cast<int>(mask) & static_cast<int>(SnapType::Point)) && (CrsId == 0 || VtId >= 0)) {
        if (CrsId == 0) {
            geoId = Sketcher::GeoEnum::RtPnt;
            posId = Sketcher::PointPos::start;
        }
        else if (VtId >= 0) {
            Obj->getGeoVertexIndex(VtId, geoId, posId);
        }

        snapPos.x = Obj->getPoint(geoId, posId).x;
        snapPos.y = Obj->getPoint(geoId, posId).y;
        return true;
    }
    else if (static_cast<int>(mask) & static_cast<int>(SnapType::Edge)) {
        if (CrsId == 1) {  // H_Axis
            snapPos.y = 0;
            // dont return true, allow grid snap to handle X coordinate
            return false;
        }
        else if (CrsId == 2) {  // V_Axis
            snapPos.x = 0;
            // dont return true, allow grid snap to handle Y coordinate
            return false;
        }
        else if (CrvId >= 0 || CrvId <= Sketcher::GeoEnum::RefExt) {  // Curves

            const Part::Geometry* geo = Obj->getGeometry(CrvId);

            Base::Vector3d pointToOverride(inputPos.x, inputPos.y, 0.);

            double pointParam = 0.0;
            auto curve = dynamic_cast<const Part::GeomCurve*>(geo);
            if (curve) {
                try {
                    curve->closestParameter(pointToOverride, pointParam);
                    pointToOverride = curve->pointAtParameter(pointParam);
                }
                catch (Base::CADKernelError& e) {
                    e.reportException();
                    return false;
                }

                // If it is a line, then we check if we need to snap to the middle.
                if (geo->is<Part::GeomLineSegment>()) {
                    const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geo);
                    snapToLineMiddle(pointToOverride, line);
                }

                // If it is an arc, then we check if we need to snap to the middle (not the center).
                if (geo->is<Part::GeomArcOfCircle>()) {
                    const Part::GeomArcOfCircle* arc = static_cast<const Part::GeomArcOfCircle*>(geo);
                    snapToArcMiddle(pointToOverride, arc);
                }

                snapPos.x = pointToOverride.x;
                snapPos.y = pointToOverride.y;

                return true;
            }
        }
    }
    return false;
}

bool SnapManager::snapToGrid(Base::Vector2d inputPos, Base::Vector2d& snapPos)
{
    // Snap Tolerance in pixels
    const double snapTol = viewProvider.getGridSize() / 5;

    snapPos = inputPos;

    double tmpX = inputPos.x, tmpY = inputPos.y;

    viewProvider.getClosestGridPoint(tmpX, tmpY);

    bool snapped = false;

    // Check if x within snap tolerance
    if (inputPos.x < tmpX + snapTol && inputPos.x > tmpX - snapTol) {
        snapPos.x = tmpX;  // Snap X Mouse Position
        snapped = true;
    }

    // Check if y within snap tolerance
    if (inputPos.y < tmpY + snapTol && inputPos.y > tmpY - snapTol) {
        snapPos.y = tmpY;  // Snap Y Mouse Position
        snapped = true;
    }

    return snapped;
}

bool SnapManager::snapToLineMiddle(Base::Vector3d& pointToOverride, const Part::GeomLineSegment* line)
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
        v += 2 * std::numbers::pi;
    }
    double angle = v - u;
    int revert = angle < std::numbers::pi ? 1 : -1;

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

Base::Vector2d SketcherGui::SnapManager::SnapHandle::compute(SnapType mask)
{
    if (!mgr) {
        return cursorPos;
    }
    return mgr->snap(cursorPos, mask);
}
