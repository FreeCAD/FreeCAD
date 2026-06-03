// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <algorithm>
#include <cmath>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>

#include <QGuiApplication>
#include <QPainter>
#include <QTimer>

#include <Inventor/events/SoKeyboardEvent.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Sketcher/App/PythonConverter.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "CommandConstraints.h"
#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


using namespace SketcherGui;
using namespace Sketcher;

/************************************ Attorney *******************************************/

inline void ViewProviderSketchDrawSketchHandlerAttorney::
    setConstraintSelectability(ViewProviderSketch& vp, bool enabled /*= true*/)
{
    vp.setConstraintSelectability(enabled);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setPositionText(
    ViewProviderSketch& vp,
    const Base::Vector2d& Pos,
    const SbString& txt
)
{
    vp.setPositionText(Pos, txt);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setPositionText(
    ViewProviderSketch& vp,
    const Base::Vector2d& Pos
)
{
    vp.setPositionText(Pos);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::resetPositionText(ViewProviderSketch& vp)
{
    vp.resetPositionText();
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(
    ViewProviderSketch& vp,
    const std::vector<Base::Vector2d>& EditCurve
)
{
    vp.drawEdit(EditCurve);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(
    ViewProviderSketch& vp,
    const std::list<std::vector<Base::Vector2d>>& list
)
{
    vp.drawEdit(list);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawLineExtensionAutoConstraintHint(
    ViewProviderSketch& vp,
    const std::vector<Base::Vector2d>& HintCurve
)
{
    vp.drawLineExtensionAutoConstraintHint(HintCurve);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawParallelPerpendicularHint(
    ViewProviderSketch& vp,
    const std::vector<Base::Vector2d>& HintLines
)
{
    vp.drawParallelPerpendicularHint(HintLines);
}

inline bool ViewProviderSketchDrawSketchHandlerAttorney::isLineExtensionAutoConstraintHintVisible(
    const ViewProviderSketch& vp,
    const std::vector<Base::Vector2d>& HintCurve
)
{
    return vp.isLineExtensionAutoConstraintHintVisible(HintCurve);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawEditMarkers(
    ViewProviderSketch& vp,
    const std::vector<Base::Vector2d>& EditMarkers,
    unsigned int augmentationlevel
)
{
    vp.drawEditMarkers(EditMarkers, augmentationlevel);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setAxisPickStyle(ViewProviderSketch& vp, bool on)
{
    vp.setAxisPickStyle(on);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::moveCursorToSketchPoint(
    ViewProviderSketch& vp,
    Base::Vector2d point
)
{
    vp.moveCursorToSketchPoint(point);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::ensureFocus(ViewProviderSketch& vp)
{
    vp.ensureFocus();
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::preselectAtPoint(
    ViewProviderSketch& vp,
    Base::Vector2d point
)
{
    vp.preselectAtPoint(point);
}

inline int ViewProviderSketchDrawSketchHandlerAttorney::getPreselectPoint(const ViewProviderSketch& vp)
{
    return vp.getPreselectPoint();
}

inline int ViewProviderSketchDrawSketchHandlerAttorney::getPreselectCurve(const ViewProviderSketch& vp)
{
    return vp.getPreselectCurve();
}

inline int ViewProviderSketchDrawSketchHandlerAttorney::getPreselectCross(const ViewProviderSketch& vp)
{
    return vp.getPreselectCross();
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setAngleSnapping(
    ViewProviderSketch& vp,
    bool enable,
    Base::Vector2d referencePoint
)
{
    vp.setAngleSnapping(enable, referencePoint);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::moveConstraint(
    ViewProviderSketch& vp,
    int constNum,
    const Base::Vector2d& toPos,
    OffsetMode offset
)
{
    vp.moveConstraint(constNum, toPos, offset);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::signalToolChanged(
    const ViewProviderSketch& vp,
    const std::string& toolname
)
{
    vp.signalToolChanged(toolname);
}

/**************************** CurveConverter **********************************************/

CurveConverter::CurveConverter()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View"
        );
        hGrp->Attach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the exception is not propagated
        Base::Console().developerError("CurveConverter", "Malformed parameter string: %s\n", e.what());
    }

    updateCurvedEdgeCountSegmentsParameter();
}

CurveConverter::~CurveConverter()
{
    // Do not detach from the parameter group.
    // So far there is only a single static instance of CurveConverter inside
    // DrawSketchHandler::drawEdit. This static instance will be destroyed after
    // the main() function has been exited so that any attempt to access the
    // parameter managers is undefined behaviour. See issue #13622.
}

std::vector<Base::Vector2d> CurveConverter::toVector2D(const Part::Geometry* geometry)
{
    std::vector<Base::Vector2d> vector2d;

    auto emplaceasvector2d = [&vector2d](const Base::Vector3d& point) {
        vector2d.emplace_back(point.x, point.y);
    };

    auto isperiodicconic = geometry->is<Part::GeomCircle>() || geometry->is<Part::GeomEllipse>();
    auto isbounded = geometry->isDerivedFrom<Part::GeomBoundedCurve>();

    if (geometry->is<Part::GeomLineSegment>()) {  // add a line
        auto geo = static_cast<const Part::GeomLineSegment*>(geometry);

        emplaceasvector2d(geo->getStartPoint());
        emplaceasvector2d(geo->getEndPoint());
    }
    else if (isperiodicconic || isbounded) {

        auto geo = static_cast<const Part::GeomConic*>(geometry);

        double segment = (geo->getLastParameter() - geo->getFirstParameter())
            / curvedEdgeCountSegments;

        for (int i = 0; i < curvedEdgeCountSegments; i++) {
            emplaceasvector2d(geo->value(geo->getFirstParameter() + i * segment));
        }

        // either close the curve for untrimmed conic or set the last point for bounded curves
        emplaceasvector2d(isperiodicconic ? geo->value(0) : geo->value(geo->getLastParameter()));
    }

    return vector2d;
}

std::list<std::vector<Base::Vector2d>> CurveConverter::toVector2DList(
    const std::vector<Part::Geometry*>& geometries
)
{
    std::list<std::vector<Base::Vector2d>> list;

    for (const auto& geo : geometries) {
        list.push_back(toVector2D(geo));
    }

    return list;
}

void CurveConverter::updateCurvedEdgeCountSegmentsParameter()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );
    int stdcountsegments = hGrp->GetInt("SegmentsPerGeometry", 50);

    // value cannot be smaller than 6
    if (stdcountsegments < 6) {
        stdcountsegments = 6;
    }

    curvedEdgeCountSegments = stdcountsegments;
};

/** Observer for parameter group. */
void CurveConverter::OnChange(Base::Subject<const char*>& rCaller, const char* sReason)
{
    (void)rCaller;

    if (strcmp(sReason, "SegmentsPerGeometry") == 0) {
        updateCurvedEdgeCountSegmentsParameter();
    }
}

/**************************** DrawSketchHandler *******************************************/


//**************************************************************************
// Construction/Destruction

DrawSketchHandler::DrawSketchHandler()
    : Gui::ToolHandler()
    , sketchgui(nullptr)
{}

DrawSketchHandler::~DrawSketchHandler()
{
    delete hoverTimer;
}

std::string DrawSketchHandler::getToolName() const
{
    return "DSH_None";
}

std::unique_ptr<QWidget> DrawSketchHandler::createWidget() const
{
    return nullptr;
}

bool DrawSketchHandler::isWidgetVisible() const
{
    return false;
};

bool DrawSketchHandler::isConstructionMode() const
{
    return sketchgui->isConstructionMode();
}

const char* DrawSketchHandler::constructionModeAsBooleanText()
{
    return sketchgui->isConstructionMode() ? "True" : "False";
}

QPixmap DrawSketchHandler::getToolIcon() const
{
    return QPixmap();
}

QString DrawSketchHandler::getToolWidgetText() const
{
    return QString();
}


void DrawSketchHandler::activate(ViewProviderSketch* vp)
{
    sketchgui = vp;

    if (!Gui::ToolHandler::activate()) {
        sketchgui->purgeHandler();
    }
}
void DrawSketchHandler::setSketchGui(ViewProviderSketch* vp)
{
    sketchgui = vp;
}

void DrawSketchHandler::deactivate()
{
    Gui::ToolHandler::deactivate();
    ViewProviderSketchDrawSketchHandlerAttorney::setConstraintSelectability(*sketchgui, true);

    // clear temporary Curve and Markers from the scenograph
    clearEdit();
    clearEditMarkers();
    clearLineExtensionAutoConstraintHintDrawing();
    resetParallelPerpendicularHint();
    resetPositionText();
    setAngleSnapping(false);

    ViewProviderSketchDrawSketchHandlerAttorney::signalToolChanged(*sketchgui, "DSH_None");
}

void DrawSketchHandler::preActivated()
{
    this->signalToolChanged();
    ViewProviderSketchDrawSketchHandlerAttorney::setConstraintSelectability(*sketchgui, false);
}

void DrawSketchHandler::cancelCurrentAction()
{
    // the default behaviour is to quit - specific handler categories may
    // override this behaviour, for example to implement a continuous mode
    quit();
}

void DrawSketchHandler::registerPressedKey(bool pressed, int key)
{
    if (key == SoKeyboardEvent::ESCAPE && !pressed) {
        cancelCurrentAction();
    }
}

void DrawSketchHandler::pressRightButton(Base::Vector2d /*onSketchPos*/)
{
    cancelCurrentAction();
}


void DrawSketchHandler::quit()
{
    assert(sketchgui);

    Gui::Selection().rmvSelectionGate();
    Gui::Selection().rmvPreselect();

    sketchgui->purgeHandler();
}

void DrawSketchHandler::toolWidgetChanged(QWidget* newwidget)
{
    toolwidget = newwidget;
    onWidgetChanged();
}

//**************************************************************************
// Helpers

int DrawSketchHandler::getHighestVertexIndex()
{
    return sketchgui->getSketchObject()->getHighestVertexIndex();
}

int DrawSketchHandler::getHighestCurveIndex()
{
    return sketchgui->getSketchObject()->getHighestCurveIndex();
}

std::vector<QPixmap> DrawSketchHandler::suggestedConstraintsPixmaps(
    std::vector<AutoConstraint>& suggestedConstraints
)
{
    std::vector<QPixmap> pixmaps;
    // Iterate through AutoConstraints types and get their pixmaps
    for (auto& autoCstr : suggestedConstraints) {
        QString iconType;
        switch (autoCstr.Type) {
            case Horizontal:
                iconType = QStringLiteral("Constraint_Horizontal");
                break;
            case Vertical:
                iconType = QStringLiteral("Constraint_Vertical");
                break;
            case Coincident:
                iconType = QStringLiteral("Constraint_PointOnPoint");
                break;
            case PointOnObject:
                iconType = QStringLiteral("Constraint_PointOnObject");
                break;
            case Symmetric:
                iconType = QStringLiteral("Constraint_Symmetric");
                break;
            case Tangent:
                iconType = QStringLiteral("Constraint_Tangent");
                break;
            case Perpendicular:
                iconType = QStringLiteral("Constraint_Perpendicular");
                break;
            case Parallel:
                iconType = QStringLiteral("Constraint_Parallel");
                break;
            default:
                break;
        }
        if (!iconType.isEmpty()) {
            constexpr int iconWidth = 16;
            QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(
                iconType.toStdString().c_str(),
                QSize(iconWidth, iconWidth)
            );
            if (!icon.isNull()) {
                pixmaps.push_back(icon);
            }
        }
    }
    return pixmaps;
}

DrawSketchHandler::PreselectionData DrawSketchHandler::getPreselectionData() const
{
    SketchObject* obj = sketchgui->getSketchObject();

    // Extract preselection information (vertex, curve, cross)
    PreselectionData preSelData;
    int preSelPnt = getPreselectPoint();
    int preSelCrv = getPreselectCurve();
    int preSelCrs = getPreselectCross();

    if (preSelPnt != -1) {
        obj->getGeoVertexIndex(preSelPnt, preSelData.geoId, preSelData.posId);
    }
    else if (preSelCrv != -1) {
        const Part::Geometry* geom = obj->getGeometry(preSelCrv);
        if (geom) {
            preSelData.geoId = preSelCrv;
            if (geom->is<Part::GeomLineSegment>()) {
                auto* line = static_cast<const Part::GeomLineSegment*>(geom);
                preSelData.hitShapeDir = line->getEndPoint() - line->getStartPoint();
                preSelData.isLine = true;
            }
        }
    }
    else if (preSelCrs == 0) {
        preSelData.geoId = Sketcher::GeoEnum::RtPnt;
        preSelData.posId = PointPos::start;
    }
    else if (preSelCrs == 1) {
        preSelData.geoId = Sketcher::GeoEnum::HAxis;
        preSelData.hitShapeDir = Base::Vector3d(1, 0, 0);
        preSelData.isLine = true;
    }
    else if (preSelCrs == 2) {
        preSelData.geoId = Sketcher::GeoEnum::VAxis;
        preSelData.hitShapeDir = Base::Vector3d(0, 1, 0);
        preSelData.isLine = true;
    }
    return preSelData;
}

bool DrawSketchHandler::isLineCenterAutoConstraint(int GeoId, const Base::Vector2d& Pos) const
{
    SketchObject* obj = sketchgui->getSketchObject();

    auto* geo = obj->getGeometry(GeoId);
    if (geo->isDerivedFrom<Part::GeomLineSegment>()) {
        auto* line = static_cast<const Part::GeomLineSegment*>(geo);

        Base::Vector2d startPoint = toVector2d(line->getStartPoint());
        Base::Vector2d endPoint = toVector2d(line->getEndPoint());
        Base::Vector2d midPoint = (startPoint + endPoint) / 2;

        // Check if we are at middle of the line
        if ((Pos - midPoint).Length() < (endPoint - startPoint).Length() * 0.05) {
            return true;
        }
    }
    return false;
}

void DrawSketchHandler::seekPreselectionAutoConstraint(
    std::vector<AutoConstraint>& suggestedConstraints,
    const Base::Vector2d& Pos,
    const Base::Vector2d& Dir,
    AutoConstraint::TargetType type
)
{
    PreselectionData preSel = getPreselectionData();

    if (preSel.geoId != GeoEnum::GeoUndef) {
        // Currently only considers objects in current Sketcher
        AutoConstraint constr;
        constr.Type = Sketcher::None;
        constr.GeoId = preSel.geoId;
        constr.PosId = preSel.posId;
        if (type == AutoConstraint::VERTEX || type == AutoConstraint::VERTEX_NO_TANGENCY) {
            if (preSel.posId == PointPos::none) {
                bool lineCenter = isLineCenterAutoConstraint(preSel.geoId, Pos);
                constr.Type = lineCenter ? Sketcher::Symmetric : Sketcher::PointOnObject;
            }
            else {
                constr.Type = Sketcher::Coincident;
            }
        }
        else if (type == AutoConstraint::CURVE && preSel.posId != PointPos::none) {
            constr.Type = Sketcher::PointOnObject;
        }
        else if (type == AutoConstraint::CURVE && preSel.posId == PointPos::none) {
            constr.Type = Sketcher::Tangent;
        }

        if (constr.Type == Sketcher::Tangent && preSel.isLine) {
            if (Dir.Length() < 1e-8 || preSel.hitShapeDir.Length() < 1e-8) {
                return;  // Direction not set so return;
            }

            // We are hitting a line and have hitting vector information
            Base::Vector3d dir3d = Base::Vector3d(Dir.x, Dir.y, 0);
            double cosangle = dir3d.Normalize() * preSel.hitShapeDir.Normalize();

            // the angle between the line and the hitting direction are over around 6 degrees
            if (fabs(cosangle) > 0.995f) {
                return;
            }
        }

        if (constr.Type != Sketcher::None) {
            suggestedConstraints.push_back(constr);
        }
    }
}

double DrawSketchHandler::getAutoConstraintSearchDistance() const
{
    return 0.1 * sketchgui->getScaleFactor();
}

bool DrawSketchHandler::seekLineExtensionAutoConstraint(
    std::vector<AutoConstraint>& suggestedConstraints,
    const Base::Vector2d& Pos,
    AutoConstraint::TargetType type
)
{
    if (type != AutoConstraint::VERTEX && type != AutoConstraint::VERTEX_NO_TANGENCY) {
        return false;
    }

    for (const auto& constraint : suggestedConstraints) {
        if (constraint.Type == Sketcher::Coincident || constraint.Type == Sketcher::PointOnObject
            || constraint.Type == Sketcher::Symmetric) {
            return false;
        }
    }

    SketchObject* obj = sketchgui->getSketchObject();
    if (!obj) {
        return false;
    }

    constexpr double segmentStartParameter = 0.0;
    constexpr double segmentEndParameter = 1.0;

    const double searchDistance = getAutoConstraintSearchDistance();
    double bestDistanceSquared = searchDistance * searchDistance;
    int bestGeoId = GeoEnum::GeoUndef;
    Base::Vector2d bestAnchor;
    Base::Vector2d bestProjection;

    for (int geoId = 0; geoId <= getHighestCurveIndex(); ++geoId) {
        const Part::Geometry* geo = obj->getGeometry(geoId);
        if (!geo) {
            continue;
        }

        const auto* line = freecad_cast<const Part::GeomLineSegment*>(geo);
        if (!line) {
            continue;
        }

        const Base::Vector2d startPoint = toVector2d(line->getStartPoint());
        const Base::Vector2d endPoint = toVector2d(line->getEndPoint());
        const Base::Vector2d lineDirection = endPoint - startPoint;
        const double lineLengthSquared = lineDirection.Sqr();

        if (lineLengthSquared <= Precision::SquareConfusion()) {
            continue;
        }

        const Base::Vector2d cursorFromStart = Pos - startPoint;
        const double parameter = (cursorFromStart.x * lineDirection.x
                                  + cursorFromStart.y * lineDirection.y)
            / lineLengthSquared;

        if (parameter >= segmentStartParameter && parameter <= segmentEndParameter) {
            continue;
        }

        const Base::Vector2d projection = startPoint + parameter * lineDirection;
        const double distanceSquared = (Pos - projection).Sqr();
        if (distanceSquared > bestDistanceSquared) {
            continue;
        }

        bestDistanceSquared = distanceSquared;
        bestGeoId = geoId;
        bestAnchor = parameter < segmentStartParameter ? startPoint : endPoint;
        bestProjection = projection;
    }

    if (bestGeoId == GeoEnum::GeoUndef) {
        return false;
    }

    if (!isLineExtensionAutoConstraintHintVisible(bestAnchor, bestProjection)) {
        return false;
    }

    AutoConstraint constr;
    constr.Type = Sketcher::PointOnObject;
    constr.GeoId = bestGeoId;
    constr.PosId = PointPos::none;
    suggestedConstraints.push_back(constr);

    lineExtensionAutoConstraintHint.isValid = true;
    lineExtensionAutoConstraintHint.start = bestAnchor;
    lineExtensionAutoConstraintHint.end = bestProjection;

    return true;
}

void DrawSketchHandler::resetLineExtensionAutoConstraintHint()
{
    lineExtensionAutoConstraintHint = LineExtensionAutoConstraintHint();
}

void DrawSketchHandler::renderLineExtensionAutoConstraintHint() const
{
    if (!lineExtensionAutoConstraintHint.isValid) {
        clearLineExtensionAutoConstraintHintDrawing();
        return;
    }

    drawLineExtensionAutoConstraintHint(
        {lineExtensionAutoConstraintHint.start, lineExtensionAutoConstraintHint.end}
    );
}

bool DrawSketchHandler::isLineExtensionAutoConstraintHintVisible(
    const Base::Vector2d& start,
    const Base::Vector2d& end
) const
{
    return isLineExtensionAutoConstraintHintVisible(std::vector<Base::Vector2d> {start, end});
}

bool DrawSketchHandler::getLineExtensionAutoConstraintSnapPoint(Base::Vector2d& point) const
{
    if (!lineExtensionAutoConstraintHint.isValid) {
        return false;
    }

    point = lineExtensionAutoConstraintHint.end;
    return true;
}

bool DrawSketchHandler::seekAlignmentAutoConstraint(
    std::vector<AutoConstraint>& suggestedConstraints,
    const Base::Vector2d& Pos,
    const Base::Vector2d& Dir
)
{
    using std::numbers::pi;
    constexpr double angleDevRad = Base::toRadians<double>(2);

    AutoConstraint constr;
    constr.Type = Sketcher::None;
    constr.GeoId = GeoEnum::GeoUndef;
    constr.PosId = PointPos::none;
    double angle = std::abs(atan2(Dir.y, Dir.x));
    if (angle < angleDevRad || (pi - angle) < angleDevRad) {
        // Suggest horizontal constraint
        constr.Type = Sketcher::Horizontal;
    }
    else if (std::abs(angle - pi / 2) < angleDevRad) {
        // Suggest vertical constraint
        constr.Type = Sketcher::Vertical;
    }
    else if (parallelPerpendicularRefGeoId != GeoEnum::GeoUndef) {
        SketchObject* obj = sketchgui->getSketchObject();

        if (auto* line = obj->getGeometry<Part::GeomLineSegment>(parallelPerpendicularRefGeoId)) {
            Base::Vector2d lineDir = toVector2d(line->getEndPoint() - line->getStartPoint());

            if (fabs(lineDir.x) > Precision::Confusion() && fabs(lineDir.y) > Precision::Confusion()
                && lineDir.Sqr() > Precision::SquareConfusion()) {
                lineDir.Normalize();
                double lineAngle = atan2(lineDir.y, lineDir.x);
                angle = atan2(Dir.y, Dir.x);

                Sketcher::ConstraintType candidateConstraint = Sketcher::None;
                if (std::abs(lineAngle - angle) < angleDevRad
                    || std::abs(lineAngle - angle + pi) < angleDevRad
                    || std::abs(lineAngle - angle - pi) < angleDevRad) {
                    candidateConstraint = Sketcher::Parallel;
                }
                else if (std::abs(lineAngle - angle - 0.5 * pi) < angleDevRad
                         || std::abs(lineAngle - angle + 0.5 * pi) < angleDevRad) {
                    candidateConstraint = Sketcher::Perpendicular;
                }

                if (candidateConstraint != Sketcher::None) {
                    constr.Type = candidateConstraint;
                    constr.GeoId = parallelPerpendicularRefGeoId;
                    constr.PosId = PointPos::none;
                }
            }
        }
    }

    if (constr.Type != Sketcher::None) {
        suggestedConstraints.push_back(constr);
        return true;
    }
    return false;
}

bool DrawSketchHandler::seekTangentAutoConstraint(
    std::vector<AutoConstraint>& suggestedConstraints,
    const Base::Vector2d& Pos,
    const Base::Vector2d& Dir
)
{
    using std::numbers::pi;
    // This function does not handle endpoint tangencies.
    SketchObject* obj = sketchgui->getSketchObject();
    int tangId = GeoEnum::GeoUndef;
    PointPos tanPos = PointPos::none;

    // Do not consider if distance is more than that.
    // Decrease this value when a candidate is found.
    double tangDeviation = getAutoConstraintSearchDistance();

    // Get geometry list
    const std::vector<Part::Geometry*> geomlist = obj->getCompleteGeometry();

    Base::Vector3d tmpPos(Pos.x, Pos.y, 0.f);                    // Current cursor point
    Base::Vector3d tmpDir(Dir.x, Dir.y, 0.f);                    // Direction of line
    Base::Vector3d tmpStart(Pos.x - Dir.x, Pos.y - Dir.y, 0.f);  // Start point

    auto removeCoincidentConstraint = [&](int completeGeometryIndex, PointPos pos) {
        // The callers pass an index into getCompleteGeometry(); the stored
        // constraints use real GeoIds (negative for external geometry).
        int geoId = obj->getGeoIdFromCompleteGeometryIndex(completeGeometryIndex);
        std::erase_if(suggestedConstraints, [geoId, pos](const AutoConstraint& c) {
            return c.Type == Coincident && c.GeoId == geoId && c.PosId == pos;
        });
    };

    int i = -1;
    for (auto* geo : geomlist) {
        i++;

        if (geo->isDerivedFrom<Part::GeomCircle>()) {
            auto* circle = static_cast<const Part::GeomCircle*>(geo);
            Base::Vector3d center = circle->getCenter();
            double radius = circle->getRadius();

            // ignore if no touch (use dot product)
            if (tmpDir * (center - tmpPos) > 0 || tmpDir * (center - tmpStart) < 0) {
                continue;
            }

            Base::Vector3d projPnt(0.f, 0.f, 0.f);
            projPnt = projPnt.ProjectToLine(center - tmpPos, tmpDir);
            double projDist = std::abs(projPnt.Length() - radius);

            // Find if nearest
            if (projDist < tangDeviation) {
                tangId = i;
                tangDeviation = projDist;
            }
        }
        else if (geo->isDerivedFrom<Part::GeomEllipse>()) {
            auto* ellipse = static_cast<const Part::GeomEllipse*>(geo);
            Base::Vector3d center = ellipse->getCenter();

            double a = ellipse->getMajorRadius();
            double b = ellipse->getMinorRadius();
            Base::Vector3d majdir = ellipse->getMajorAxisDir();

            double cf = sqrt(a * a - b * b);

            Base::Vector3d focus1P = center + cf * majdir;
            Base::Vector3d focus2P = center - cf * majdir;

            Base::Vector3d norm = Base::Vector3d(Dir.y, -Dir.x).Normalize();

            double distancetoline = norm * (tmpPos - focus1P);  // distance focus1 to line

            // mirror of focus1 with respect to the line
            Base::Vector3d focus1PMirrored = focus1P + 2 * distancetoline * norm;

            double error = fabs((focus1PMirrored - focus2P).Length() - 2 * a);

            if (error < tangDeviation) {
                tangId = i;
                tangDeviation = error;
            }
        }
        else if (geo->isDerivedFrom<Part::GeomArcOfCircle>()) {
            auto* arc = static_cast<const Part::GeomArcOfCircle*>(geo);
            Base::Vector3d center = arc->getCenter();
            double radius = arc->getRadius();

            // ignore if no touch (use dot product)
            if (tmpDir * (center - tmpPos) > 0 || tmpDir * (center - tmpStart) < 0) {
                continue;
            }

            Base::Vector3d projPnt(0.f, 0.f, 0.f);
            projPnt = projPnt.ProjectToLine(center - tmpPos, tmpDir);
            double projDist = std::abs(projPnt.Length() - radius);

            if (projDist < tangDeviation) {
                Base::Vector3d start = arc->getStartPoint();
                Base::Vector3d end = arc->getEndPoint();

                if ((start - tmpPos).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::start;
                    tangId = i;
                    tangDeviation = projDist;

                    // There must be a coincident autoconstraint added before. So we remove it
                    removeCoincidentConstraint(tangId, tanPos);
                }
                else if ((start - tmpStart).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::start;
                    tangId = i;
                    tangDeviation = projDist;
                    // Coincident is added somewhere else so it has to be handled after the geo
                    // creation.
                }
                else if ((end - tmpPos).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::end;
                    tangId = i;
                    tangDeviation = projDist;

                    // There must be a coincident autoconstraint added before. So we remove it
                    removeCoincidentConstraint(tangId, tanPos);
                }
                else if ((end - tmpStart).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::end;
                    tangId = i;
                    tangDeviation = projDist;
                    // Coincident is added somewhere else so it has to be handled after the geo
                    // creation.
                }
                else {
                    double startAngle, endAngle;
                    arc->getRange(startAngle, endAngle, /*emulateCCW=*/true);

                    double angle = atan2(projPnt.y, projPnt.x);
                    while (angle < startAngle) {
                        angle += 2 * pi;  // Bring it to range of arc
                    }

                    // if the point is on correct side of arc
                    if (angle <= endAngle) {  // Now need to check only one side
                        tangId = i;
                        tangDeviation = projDist;
                    }
                }
            }
        }
        else if (geo->isDerivedFrom<Part::GeomArcOfEllipse>()) {
            auto* aoe = static_cast<const Part::GeomArcOfEllipse*>(geo);
            Base::Vector3d center = aoe->getCenter();

            double a = aoe->getMajorRadius();
            double b = aoe->getMinorRadius();
            Base::Vector3d majdir = aoe->getMajorAxisDir();

            double cf = sqrt(a * a - b * b);

            Base::Vector3d focus1P = center + cf * majdir;
            Base::Vector3d focus2P = center - cf * majdir;

            Base::Vector3d norm = Base::Vector3d(Dir.y, -Dir.x).Normalize();

            double distancetoline = norm * (tmpPos - focus1P);  // distance focus1 to line

            // mirror of focus1 with respect to the line
            Base::Vector3d focus1PMirrored = focus1P + 2 * distancetoline * norm;

            double error = fabs((focus1PMirrored - focus2P).Length() - 2 * a);

            if (error < tangDeviation) {
                Base::Vector3d start = aoe->getStartPoint();
                Base::Vector3d end = aoe->getEndPoint();

                if ((start - tmpPos).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::start;
                    tangId = i;
                    tangDeviation = error;
                    removeCoincidentConstraint(tangId, tanPos);
                }
                else if ((start - tmpStart).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::start;
                    tangId = i;
                    tangDeviation = error;
                }
                else if ((end - tmpPos).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::end;
                    tangId = i;
                    tangDeviation = error;
                    removeCoincidentConstraint(tangId, tanPos);
                }
                else if ((end - tmpStart).Sqr() < Precision::SquareConfusion()) {
                    tanPos = PointPos::end;
                    tangId = i;
                    tangDeviation = error;
                }
                else {
                    tangId = i;
                    tangDeviation = error;
                }
            }
        }
    }

    if (tangId != GeoEnum::GeoUndef) {
        AutoConstraint constr;
        constr.Type = Tangent;
        // tangId is an index into getCompleteGeometry(); convert to a GeoId
        // (negative for external geometry).
        constr.GeoId = obj->getGeoIdFromCompleteGeometryIndex(tangId);
        constr.PosId = tanPos;
        suggestedConstraints.push_back(constr);
        return true;
    }
    return false;
}

void DrawSketchHandler::openCommand(const std::string& name)
{
    currentTransactionID = sketchgui->getDocument()->openCommand(name.c_str());
}
void DrawSketchHandler::commitCommand()
{
    Gui::Command::commitCommand(currentTransactionID);
}
void DrawSketchHandler::abortCommand()
{
    Gui::Command::abortCommand(currentTransactionID);
}

int DrawSketchHandler::seekAutoConstraint(
    std::vector<AutoConstraint>& suggestedConstraints,
    const Base::Vector2d& Pos,
    const Base::Vector2d& Dir,
    AutoConstraint::TargetType type
)
{
    suggestedConstraints.clear();

    resetLineExtensionAutoConstraintHint();

    if (!sketchgui->Autoconstraints.getValue()) {
        return 0;  // If Autoconstraints property is not set quit
    }

    // Reference line hover-selection detection
    PreselectionData preselection = getPreselectionData();
    bool horOrVert = fabs(preselection.hitShapeDir.x) < Precision::Confusion()
        || fabs(preselection.hitShapeDir.y) < Precision::Confusion();
    if (preselection.isLine && !horOrVert && preselection.geoId != GeoEnum::GeoUndef) {
        if (preselection.geoId != lastHoveredGeoId) {
            lastHoveredGeoId = preselection.geoId;
            startHoverTimer();
        }
    }
    else {
        if (lastHoveredGeoId != GeoEnum::GeoUndef) {
            lastHoveredGeoId = GeoEnum::GeoUndef;
            stopHoverTimer();
        }
    }

    seekPreselectionAutoConstraint(suggestedConstraints, Pos, Dir, type);
    seekLineExtensionAutoConstraint(suggestedConstraints, Pos, type);

    if (Dir.Length() > 1e-8 && type != AutoConstraint::CURVE) {
        bool tangentCreated = false;
        if (type != AutoConstraint::VERTEX_NO_TANGENCY) {
            tangentCreated = seekTangentAutoConstraint(suggestedConstraints, Pos, Dir);
        }

        if (!tangentCreated) {
            // We don't check for alignment if there is already a tangency.
            seekAlignmentAutoConstraint(suggestedConstraints, Pos, Dir);
        }
    }

    return suggestedConstraints.size();
}

// TODO: Figure out and explain what it actually returns
bool DrawSketchHandler::generateOneAutoConstraintFromSuggestion(
    const AutoConstraint& ac,
    int geoId1,
    Sketcher::PointPos posId1,
    std::vector<std::unique_ptr<Sketcher::Constraint>>& autoConstraints
)
{
    int geoId2 = ac.GeoId;
    Sketcher::PointPos posId2 = ac.PosId;

    static const auto isStartOrEnd = [](const Sketcher::PointPos posId) {
        return posId == Sketcher::PointPos::start || posId == Sketcher::PointPos::end;
    };

    switch (ac.Type) {
        case Sketcher::Coincident: {
            if (posId1 == Sketcher::PointPos::none) {
                return true;
            }

            // find if there is already a matching tangency
            auto itOfTangentConstraint = autoConstraints.end();
            if (isStartOrEnd(posId1) && isStartOrEnd(posId2)) {
                itOfTangentConstraint = std::ranges::find(
                    autoConstraints,
                    std::tuple {Sketcher::Tangent, geoId1, geoId2},
                    [](const auto& ace) { return std::tuple {ace->Type, ace->First, ace->Second}; }
                );
            }

            if (itOfTangentConstraint != autoConstraints.end()) {
                // modify tangency to endpoint-to-endpoint
                (*itOfTangentConstraint)->FirstPos = posId1;
                (*itOfTangentConstraint)->SecondPos = posId2;
            }
            else {
                auto c = std::make_unique<Sketcher::Constraint>();
                c->Type = Sketcher::Coincident;
                c->First = geoId1;
                c->FirstPos = posId1;
                c->Second = geoId2;
                c->SecondPos = posId2;
                autoConstraints.push_back(std::move(c));
            }
        } break;
        case Sketcher::PointOnObject: {
            if (posId1 == Sketcher::PointPos::none) {
                // Auto constraining an edge so swap parameters
                std::swap(geoId1, geoId2);
                std::swap(posId1, posId2);
            }

            auto itOfTangentConstraint = autoConstraints.end();
            if (isStartOrEnd(posId1)) {
                itOfTangentConstraint = std::ranges::find_if(autoConstraints, [&](const auto& ace) {
                    return ace->Type == Sketcher::Tangent && ace->involvesGeoId(geoId1)
                        && ace->involvesGeoId(geoId2);
                });
            }

            // if tangency, convert to point-to-edge tangency
            if (itOfTangentConstraint != autoConstraints.end()) {
                if ((*itOfTangentConstraint)->First != geoId1) {
                    std::swap((*itOfTangentConstraint)->Second, (*itOfTangentConstraint)->First);
                }

                (*itOfTangentConstraint)->FirstPos = posId1;
            }
            else {
                auto c = std::make_unique<Sketcher::Constraint>();
                c->Type = Sketcher::PointOnObject;
                c->First = geoId1;
                c->FirstPos = posId1;
                c->Second = geoId2;
                autoConstraints.push_back(std::move(c));
            }
        } break;
        case Sketcher::Symmetric: {
            auto c = std::make_unique<Sketcher::Constraint>();
            c->Type = Sketcher::Symmetric;
            c->First = geoId2;
            c->FirstPos = Sketcher::PointPos::start;
            c->Second = geoId2;
            c->SecondPos = Sketcher::PointPos::end;
            c->Third = geoId1;
            c->ThirdPos = posId1;
            autoConstraints.push_back(std::move(c));
        } break;
        // In special case of Horizontal/Vertical constraint, geoId2 is normally
        // unused and should be 'Constraint::GeoUndef' However it can be used as a
        // way to require the function to apply these constraints on another
        // geometry In this case the caller as to set geoId2, then it will be used
        // as target instead of geoId2
        case Sketcher::Horizontal:
        case Sketcher::Vertical: {
            auto c = std::make_unique<Sketcher::Constraint>();
            c->Type = ac.Type;
            c->First = (geoId2 != Sketcher::GeoEnum::GeoUndef ? geoId2 : geoId1);
            autoConstraints.push_back(std::move(c));
        } break;
        case Sketcher::Perpendicular: {
            auto c = std::make_unique<Sketcher::Constraint>();
            c->Type = Sketcher::Perpendicular;
            c->First = geoId1;
            c->Second = geoId2;
            autoConstraints.push_back(std::move(c));
        } break;
        case Sketcher::Parallel: {
            auto c = std::make_unique<Sketcher::Constraint>();
            c->Type = Sketcher::Parallel;
            c->First = geoId1;
            c->Second = geoId2;
            autoConstraints.push_back(std::move(c));
        } break;
        case Sketcher::Tangent: {
            Sketcher::SketchObject* Obj = sketchgui->getObject<Sketcher::SketchObject>();

            const Part::Geometry* geom1 = Obj->getGeometry(geoId1);
            const Part::Geometry* geom2 = Obj->getGeometry(geoId2);
            if (!geom1 || !geom2) {
                return false;
            }

            // 2026.01.16: Do not use swap as it did before or it breaks resultCoincident.
            // NOTE: Temporarily deactivated : ellipse tangency support using construction elements
            if (geom1->is<Part::GeomEllipse>()
                && (geom2->is<Part::GeomConic>() || geom2->is<Part::GeomArcOfConic>())) {
                // makeTangentToEllipseviaNewPoint(
                //     Obj,
                //     static_cast<const Part::GeomEllipse*>(geom1),
                //     geom2,
                //     geoId1,
                //     geoId2);
                return false;
            }
            else if (
                geom2->is<Part::GeomEllipse>()
                && (geom1->is<Part::GeomConic>() || geom1->is<Part::GeomArcOfConic>())
            ) {
                // makeTangentToEllipseviaNewPoint(
                //     Obj,
                //     static_cast<const Part::GeomEllipse*>(geom2),
                //     geom1,
                //     geoId2,
                //     geoId1);
                return false;
            }
            else if (
                geom1->is<Part::GeomArcOfEllipse>()
                && (geom2->is<Part::GeomConic>() || geom2->is<Part::GeomArcOfConic>())
            ) {
                // makeTangentToArcOfEllipseviaNewPoint(
                //     Obj,
                //     static_cast<const Part::GeomArcOfEllipse*>(geom1),
                //     geom2,
                //     geoId1,
                //     geoId2);
                return false;
            }
            else if (
                geom2->is<Part::GeomArcOfEllipse>()
                && (geom1->is<Part::GeomConic>() || geom1->is<Part::GeomArcOfConic>())
            ) {
                // makeTangentToArcOfEllipseviaNewPoint(
                //     Obj,
                //     static_cast<const Part::GeomArcOfEllipse*>(geom2),
                //     geom1,
                //     geoId2,
                //     geoId1);
                return false;
            }

            auto resultCoincident = std::ranges::find_if(autoConstraints, [&](const auto& ace) {
                return ace->Type == Sketcher::Coincident && ace->First == geoId1
                    && ace->Second == geoId2;
            });

            auto resultPointOnObject = std::ranges::find_if(autoConstraints, [&](const auto& ace) {
                return ace->Type == Sketcher::PointOnObject && ace->involvesGeoId(geoId1)
                    && ace->involvesGeoId(geoId2);
            });

            if (resultCoincident != autoConstraints.end()
                && isStartOrEnd((*resultCoincident)->FirstPos)
                && isStartOrEnd((*resultCoincident)->SecondPos)) {
                // endpoint-to-endpoint tangency
                (*resultCoincident)->Type = Sketcher::Tangent;
            }
            else if (
                resultPointOnObject != autoConstraints.end()
                && isStartOrEnd((*resultPointOnObject)->FirstPos)
            ) {
                // endpoint-to-edge tangency
                (*resultPointOnObject)->Type = Sketcher::Tangent;
            }
            else if (
                resultCoincident != autoConstraints.end()
                && (*resultCoincident)->FirstPos == Sketcher::PointPos::mid
                && (*resultCoincident)->SecondPos == Sketcher::PointPos::mid && geom1 && geom2
                && (geom1->is<Part::GeomCircle>() || geom1->is<Part::GeomArcOfCircle>())
                && (geom2->is<Part::GeomCircle>() || geom2->is<Part::GeomArcOfCircle>())
            ) {
                // equality
                auto c = std::make_unique<Sketcher::Constraint>();
                c->Type = Sketcher::Equal;
                c->First = geoId1;
                c->Second = geoId2;
                autoConstraints.push_back(std::move(c));
            }
            else {  // regular edge to edge tangency
                auto c = std::make_unique<Sketcher::Constraint>();
                c->Type = Sketcher::Tangent;
                c->First = geoId1;
                c->Second = geoId2;
                autoConstraints.push_back(std::move(c));
            }
        } break;
        default:
            break;
    }

    return true;
}

bool DrawSketchHandler::filterRedundantAutoConstraints(
    std::vector<std::unique_ptr<Sketcher::Constraint>>& autoConstraints
)
{
    if (autoConstraints.empty()) {
        return true;
    }

    auto sketchobject = getSketchObject();

    auto constraints = toPointerVector(autoConstraints);

    // Allows a diagnose with the new autoconstraints as if they were part of the sketchobject,
    // but WITHOUT adding them to the sketchobject..
    sketchobject->diagnoseAdditionalConstraints(constraints);

    if (sketchobject->getLastHasRedundancies()) {
        Base::Console().message(
            sketchobject->getFullLabel(),
            QT_TRANSLATE_NOOP("Notifications", "Autoconstraints cause redundancy. Removing them") "\n"
        );

        auto lastsketchconstraintindex = sketchobject->Constraints.getSize() - 1;

        auto redundants = sketchobject->getLastRedundant();  // redundants is always sorted

        for (int index = redundants.size() - 1; index >= 0; index--) {
            int redundantconstraintindex = redundants[index] - 1;
            if (redundantconstraintindex > lastsketchconstraintindex) {
                int removeindex = redundantconstraintindex - lastsketchconstraintindex - 1;
                autoConstraints.erase(std::next(autoConstraints.begin(), removeindex));
            }
            else {
                return false;
            }
        }

        // NOTE: If we removed all redundants in the list, then at this moment there are no
        // redundants anymore
    }

    // This can happen if OVP generated constraints and autoconstraints are conflicting
    // For instance : https://github.com/FreeCAD/FreeCAD/issues/17722
    if (sketchobject->getLastHasConflicts()) {
        auto lastsketchconstraintindex = sketchobject->Constraints.getSize() - 1;

        auto conflicting = sketchobject->getLastConflicting();

        for (int index = conflicting.size() - 1; index >= 0; index--) {
            int conflictingIndex = conflicting[index] - 1;
            if (conflictingIndex > lastsketchconstraintindex) {
                int removeindex = conflictingIndex - lastsketchconstraintindex - 1;
                autoConstraints.erase(std::next(autoConstraints.begin(), removeindex));
            }
        }
    }

    return true;
}

void DrawSketchHandler::addGeneratedAutoConstraints(
    const std::vector<std::unique_ptr<Sketcher::Constraint>>& autoConstraints
)
{
    auto constraints = toPointerVector(autoConstraints);

    Gui::Command::doCommand(
        Gui::Command::Doc,
        Sketcher::PythonConverter::convert(Gui::Command::getObjectCmd(sketchgui->getObject()), constraints)
            .c_str()
    );
}

void DrawSketchHandler::createAutoConstraints(
    const std::vector<AutoConstraint>& autoConstrs,
    int geoId1,
    Sketcher::PointPos posId1,
    bool createowncommand /*= true*/
)
{
    if (!sketchgui->Autoconstraints.getValue()) {
        return;  // If Autoconstraints property is not set quit
    }

    if (autoConstrs.empty()) {
        return;
    }

    if (createowncommand) {
        // Open the Command
        openCommand(QT_TRANSLATE_NOOP("Command", "Add Auto-Constraints"));
    }

    // Iterate through constraints
    for (auto& cstr : autoConstrs) {
        int geoId2 = cstr.GeoId;

        switch (cstr.Type) {
            case Sketcher::Coincident: {
                if (posId1 == Sketcher::PointPos::none) {
                    continue;
                }
                // If the auto constraint has a point create a coincident otherwise it is an
                // edge on a point
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                    geoId1,
                    static_cast<int>(posId1),
                    cstr.GeoId,
                    static_cast<int>(cstr.PosId)
                );
            } break;
            case Sketcher::PointOnObject: {
                Sketcher::PointPos posId2 = cstr.PosId;
                if (posId1 == Sketcher::PointPos::none) {
                    // Auto constraining an edge so swap parameters
                    std::swap(geoId1, geoId2);
                    std::swap(posId1, posId2);
                }

                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    geoId1,
                    static_cast<int>(posId1),
                    geoId2
                );
            } break;
            case Sketcher::Symmetric: {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Symmetric',%d,1,%d,2,%d,%d)) ",
                    geoId2,
                    geoId2,
                    geoId1,
                    static_cast<int>(posId1)
                );
            } break;
                // In special case of Horizontal/Vertical constraint, geoId2 is normally unused
                // and should be 'Constraint::GeoUndef' However it can be used as a way to
                // require the function to apply these constraints on another geometry In this
                // case the caller as to set geoId2, then it will be used as target instead of
                // geoId2
            case Sketcher::Horizontal: {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Horizontal',%d)) ",
                    geoId2 != GeoEnum::GeoUndef ? geoId2 : geoId1
                );
            } break;
            case Sketcher::Vertical: {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Vertical',%d)) ",
                    geoId2 != GeoEnum::GeoUndef ? geoId2 : geoId1
                );
            } break;
            case Sketcher::Perpendicular: {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Perpendicular',%d, %d)) ",
                    geoId1,
                    geoId2
                );
            } break;
            case Sketcher::Parallel: {
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Parallel',%d, %d)) ",
                    geoId1,
                    geoId2
                );
            } break;
            case Sketcher::Tangent: {
                Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

                const Part::Geometry* geom1 = Obj->getGeometry(geoId1);
                const Part::Geometry* geom2 = Obj->getGeometry(cstr.GeoId);

                // ellipse tangency support using construction elements (lines)
                if (geom1 && geom2
                    && (geom1->is<Part::GeomEllipse>() || geom2->is<Part::GeomEllipse>())) {

                    if (!geom1->is<Part::GeomEllipse>()) {
                        std::swap(geoId1, geoId2);
                    }

                    // geoId1 is the ellipse
                    geom1 = Obj->getGeometry(geoId1);
                    geom2 = Obj->getGeometry(geoId2);

                    if (geom2->is<Part::GeomEllipse>() || geom2->is<Part::GeomArcOfEllipse>()
                        || geom2->is<Part::GeomCircle>() || geom2->is<Part::GeomArcOfCircle>()) {
                        // in all these cases an intermediate element is needed
                        bool success = makeTangentToEllipseviaNewPoint(
                            Obj,
                            static_cast<const Part::GeomEllipse*>(geom1),
                            geom2,
                            geoId1,
                            geoId2
                        );

                        if (createowncommand) {
                            closeAndRecompute(currentTransactionID, !success, Obj);
                        }
                        else {
                            tryAutoRecompute(Obj);
                        }

                        return;
                    }
                }

                // arc of ellipse tangency support using external elements
                if (geom1 && geom2
                    && (geom1->is<Part::GeomArcOfEllipse>() || geom2->is<Part::GeomArcOfEllipse>())) {

                    if (!geom1->is<Part::GeomArcOfEllipse>()) {
                        std::swap(geoId1, geoId2);
                    }

                    // geoId1 is the arc of ellipse
                    geom1 = Obj->getGeometry(geoId1);
                    geom2 = Obj->getGeometry(geoId2);

                    if (geom2->is<Part::GeomArcOfEllipse>() || geom2->is<Part::GeomCircle>()
                        || geom2->is<Part::GeomArcOfCircle>()) {
                        // in all these cases an intermediate element is needed
                        bool success = makeTangentToArcOfEllipseviaNewPoint(
                            Obj,
                            static_cast<const Part::GeomArcOfEllipse*>(geom1),
                            geom2,
                            geoId1,
                            geoId2
                        );

                        if (createowncommand) {
                            closeAndRecompute(currentTransactionID, !success, Obj);
                        }
                        else {
                            tryAutoRecompute(Obj);
                        }

                        return;
                    }
                }

                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addConstraint(Sketcher.Constraint('Tangent',%d, %d)) ",
                    geoId1,
                    cstr.GeoId
                );
            } break;
            default:
                break;
        }

        if (createowncommand) {
            commitCommand();
        }
        // Gui::Command::updateActive(); // There is already an recompute in each command
        // creation, this is redundant.
    }
}

int DrawSketchHandler::seekAndRenderAutoConstraint(
    std::vector<AutoConstraint>& suggestedConstraints,
    const Base::Vector2d& Pos,
    const Base::Vector2d& Dir,
    AutoConstraint::TargetType type
)
{
    const int constraintCount = seekAutoConstraint(suggestedConstraints, Pos, Dir, type);
    renderLineExtensionAutoConstraintHint();

    if (constraintCount) {
        renderSuggestConstraintsCursor(suggestedConstraints);
    }
    else {
        applyCursor();
    }

    return suggestedConstraints.size();
}

void DrawSketchHandler::renderSuggestConstraintsCursor(std::vector<AutoConstraint>& suggestedConstraints)
{
    std::vector<QPixmap> pixmaps = suggestedConstraintsPixmaps(suggestedConstraints);
    if (pixmaps.empty()) {
        applyCursor();
        return;
    }
    addCursorTail(pixmaps);
}

void DrawSketchHandler::setPositionText(const Base::Vector2d& Pos, const SbString& text)
{
    ViewProviderSketchDrawSketchHandlerAttorney::setPositionText(*sketchgui, Pos, text);
}


void DrawSketchHandler::setPositionText(const Base::Vector2d& Pos)
{
    ViewProviderSketchDrawSketchHandlerAttorney::setPositionText(*sketchgui, Pos);
}

void DrawSketchHandler::resetPositionText()
{
    ViewProviderSketchDrawSketchHandlerAttorney::resetPositionText(*sketchgui);
}

void DrawSketchHandler::drawEdit(const std::vector<Base::Vector2d>& EditCurve) const
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(*sketchgui, EditCurve);
}

void DrawSketchHandler::drawEdit(const std::list<std::vector<Base::Vector2d>>& list) const
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(*sketchgui, list);
}

void DrawSketchHandler::drawEdit(const std::vector<Part::Geometry*>& geometries) const
{
    static CurveConverter c;

    auto list = c.toVector2DList(geometries);

    drawEdit(list);
}

void DrawSketchHandler::drawLineExtensionAutoConstraintHint(
    const std::vector<Base::Vector2d>& HintCurve
) const
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawLineExtensionAutoConstraintHint(
        *sketchgui,
        HintCurve
    );
}

bool DrawSketchHandler::isLineExtensionAutoConstraintHintVisible(
    const std::vector<Base::Vector2d>& HintCurve
) const
{
    return ViewProviderSketchDrawSketchHandlerAttorney::isLineExtensionAutoConstraintHintVisible(
        *sketchgui,
        HintCurve
    );
}

bool DrawSketchHandler::getStartPointOfCurrentSegment(Base::Vector2d& point) const
{
    return false;
}

void DrawSketchHandler::drawParallelPerpendicularHint(const std::vector<Base::Vector2d>& HintLines) const
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawParallelPerpendicularHint(*sketchgui, HintLines);
}

void DrawSketchHandler::resetParallelPerpendicularHint()
{
    parallelPerpendicularRefGeoId = GeoEnum::GeoUndef;
    lastHoveredGeoId = GeoEnum::GeoUndef;
    stopHoverTimer();
    clearParallelPerpendicularHintDrawing();
}

void DrawSketchHandler::clearParallelPerpendicularHintDrawing() const
{
    drawParallelPerpendicularHint(std::vector<Base::Vector2d>());
}

void DrawSketchHandler::renderParallelPerpendicularHint() const
{
    if (parallelPerpendicularRefGeoId == GeoEnum::GeoUndef) {
        clearParallelPerpendicularHintDrawing();
        return;
    }

    SketchObject* obj = sketchgui->getSketchObject();
    Base::Vector2d startPoint;
    if (!obj || !getStartPointOfCurrentSegment(startPoint)) {
        clearParallelPerpendicularHintDrawing();
        return;
    }

    auto* line = obj->getGeometry<Part::GeomLineSegment>(parallelPerpendicularRefGeoId);
    if (!line) {
        clearParallelPerpendicularHintDrawing();
        return;
    }

    Base::Vector3d lineStart = line->getStartPoint();
    Base::Vector3d lineEnd = line->getEndPoint();
    Base::Vector2d lineDir = toVector2d(lineEnd - lineStart);

    if (lineDir.Sqr() <= Precision::SquareConfusion()) {
        clearParallelPerpendicularHintDrawing();
        return;
    }

    lineDir.Normalize();
    Base::Vector2d perpDir(-lineDir.y, lineDir.x);

    double halfLength = 1000.0 * sketchgui->getScaleFactor();

    Base::Vector2d p1 = startPoint - halfLength * lineDir;
    Base::Vector2d p2 = startPoint + halfLength * lineDir;
    Base::Vector2d p3 = startPoint - halfLength * perpDir;
    Base::Vector2d p4 = startPoint + halfLength * perpDir;

    drawParallelPerpendicularHint({p1, p2, p3, p4});
}

void DrawSketchHandler::startHoverTimer()
{
    if (!hoverTimer) {
        hoverTimer = new QTimer();
        hoverTimer->setSingleShot(true);
        QObject::connect(hoverTimer, &QTimer::timeout, [this]() { onHoverTimeout(); });
    }
    hoverTimer->start(400);
}

void DrawSketchHandler::stopHoverTimer()
{
    if (hoverTimer) {
        hoverTimer->stop();
    }
}

void DrawSketchHandler::onHoverTimeout()
{
    if (lastHoveredGeoId != GeoEnum::GeoUndef) {
        parallelPerpendicularRefGeoId = lastHoveredGeoId;
        renderParallelPerpendicularHint();
    }
}

void DrawSketchHandler::clearEdit() const
{
    drawEdit(std::vector<Base::Vector2d>());
}

void DrawSketchHandler::clearLineExtensionAutoConstraintHintDrawing() const
{
    drawLineExtensionAutoConstraintHint(std::vector<Base::Vector2d>());
}

void DrawSketchHandler::clearEditMarkers() const
{
    drawEditMarkers(std::vector<Base::Vector2d>());
}

void DrawSketchHandler::drawPositionAtCursor(const Base::Vector2d& position)
{
    setPositionText(position);
}

void DrawSketchHandler::drawDirectionAtCursor(const Base::Vector2d& position, const Base::Vector2d& origin)
{
    if (!showCursorCoords()) {
        return;
    }

    float length = (position - origin).Length();
    float angle = (position - origin).GetAngle(Base::Vector2d(1.f, 0.f));

    SbString text;
    std::string lengthString = lengthToDisplayFormat(length, 1);
    std::string angleString = angleToDisplayFormat(angle * 180.0 / std::numbers::pi, 1);
    text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
    setPositionText(position, text);
}

void DrawSketchHandler::drawWidthHeightAtCursor(
    const Base::Vector2d& position,
    const double val1,
    const double val2
)
{
    if (!showCursorCoords()) {
        return;
    }

    SbString text;
    std::string val1String = lengthToDisplayFormat(val1, 1);
    std::string val2String = lengthToDisplayFormat(val2, 1);
    text.sprintf(" (%s x %s)", val1String.c_str(), val2String.c_str());
    setPositionText(position, text);
}

void DrawSketchHandler::drawDoubleAtCursor(const Base::Vector2d& position, const double val, Base::Unit unit)
{
    if (!showCursorCoords()) {
        return;
    }

    SbString text;
    std::string doubleString = unit == Base::Unit::Length
        ? lengthToDisplayFormat(val, 1)
        : angleToDisplayFormat(Base::toDegrees(val), 1);
    text.sprintf(" (%s)", doubleString.c_str());
    setPositionText(position, text);
}

std::unique_ptr<QWidget> DrawSketchHandler::createToolWidget() const
{
    return createWidget();  // NVI
}

bool DrawSketchHandler::isToolWidgetVisible() const
{
    return isWidgetVisible();  // NVI
}

QPixmap DrawSketchHandler::getToolWidgetHeaderIcon() const
{
    return getToolIcon();
}

QString DrawSketchHandler::getToolWidgetHeaderText() const
{
    return getToolWidgetText();
}

void DrawSketchHandler::drawEditMarkers(
    const std::vector<Base::Vector2d>& EditMarkers,
    unsigned int augmentationlevel
) const
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawEditMarkers(
        *sketchgui,
        EditMarkers,
        augmentationlevel
    );
}

void DrawSketchHandler::setAxisPickStyle(bool on)
{
    ViewProviderSketchDrawSketchHandlerAttorney::setAxisPickStyle(*sketchgui, on);
}

void DrawSketchHandler::moveCursorToSketchPoint(Base::Vector2d point)
{
    ViewProviderSketchDrawSketchHandlerAttorney::moveCursorToSketchPoint(*sketchgui, point);
}

void DrawSketchHandler::ensureFocus()
{
    ViewProviderSketchDrawSketchHandlerAttorney::ensureFocus(*sketchgui);
}

void DrawSketchHandler::preselectAtPoint(Base::Vector2d point)
{
    ViewProviderSketchDrawSketchHandlerAttorney::preselectAtPoint(*sketchgui, point);
}

int DrawSketchHandler::getPreselectPoint() const
{
    return ViewProviderSketchDrawSketchHandlerAttorney::getPreselectPoint(*sketchgui);
}

int DrawSketchHandler::getPreselectCurve() const
{
    return ViewProviderSketchDrawSketchHandlerAttorney::getPreselectCurve(*sketchgui);
}

int DrawSketchHandler::getPreselectCross() const
{
    return ViewProviderSketchDrawSketchHandlerAttorney::getPreselectCross(*sketchgui);
}

Sketcher::SketchObject* DrawSketchHandler::getSketchObject()
{
    return sketchgui->getSketchObject();
}

void DrawSketchHandler::setAngleSnapping(bool enable, Base::Vector2d referencePoint)
{
    ViewProviderSketchDrawSketchHandlerAttorney::setAngleSnapping(*sketchgui, enable, referencePoint);
}

void DrawSketchHandler::moveConstraint(int constNum, const Base::Vector2d& toPos, OffsetMode offset)
{
    ViewProviderSketchDrawSketchHandlerAttorney::moveConstraint(*sketchgui, constNum, toPos, offset);
}

void DrawSketchHandler::signalToolChanged() const
{
    ViewProviderSketchDrawSketchHandlerAttorney::signalToolChanged(*sketchgui, this->getToolName());
}
