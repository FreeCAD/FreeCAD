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

#include <QGuiApplication>
#include <QPainter>
#endif  // #ifndef _PreComp_

// clang-format off
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
// clang-format on

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "CommandConstraints.h"
#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


using namespace SketcherGui;
using namespace Sketcher;

/************************************ Attorney *******************************************/

inline void
ViewProviderSketchDrawSketchHandlerAttorney::setConstraintSelectability(ViewProviderSketch& vp,
                                                                        bool enabled /*= true*/)
{
    vp.setConstraintSelectability(enabled);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setPositionText(ViewProviderSketch& vp,
                                                                         const Base::Vector2d& Pos,
                                                                         const SbString& txt)
{
    vp.setPositionText(Pos, txt);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setPositionText(ViewProviderSketch& vp,
                                                                         const Base::Vector2d& Pos)
{
    vp.setPositionText(Pos);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::resetPositionText(ViewProviderSketch& vp)
{
    vp.resetPositionText();
}

inline void
ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(ViewProviderSketch& vp,
                                                      const std::vector<Base::Vector2d>& EditCurve)
{
    vp.drawEdit(EditCurve);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(
    ViewProviderSketch& vp,
    const std::list<std::vector<Base::Vector2d>>& list)
{
    vp.drawEdit(list);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::drawEditMarkers(
    ViewProviderSketch& vp,
    const std::vector<Base::Vector2d>& EditMarkers,
    unsigned int augmentationlevel)
{
    vp.drawEditMarkers(EditMarkers, augmentationlevel);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::setAxisPickStyle(ViewProviderSketch& vp,
                                                                          bool on)
{
    vp.setAxisPickStyle(on);
}

inline void
ViewProviderSketchDrawSketchHandlerAttorney::moveCursorToSketchPoint(ViewProviderSketch& vp,
                                                                     Base::Vector2d point)
{
    vp.moveCursorToSketchPoint(point);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::preselectAtPoint(ViewProviderSketch& vp,
                                                                          Base::Vector2d point)
{
    vp.preselectAtPoint(point);
}

inline int
ViewProviderSketchDrawSketchHandlerAttorney::getPreselectPoint(const ViewProviderSketch& vp)
{
    return vp.getPreselectPoint();
}

inline int
ViewProviderSketchDrawSketchHandlerAttorney::getPreselectCurve(const ViewProviderSketch& vp)
{
    return vp.getPreselectCurve();
}

inline int
ViewProviderSketchDrawSketchHandlerAttorney::getPreselectCross(const ViewProviderSketch& vp)
{
    return vp.getPreselectCross();
}

inline void
ViewProviderSketchDrawSketchHandlerAttorney::setAngleSnapping(ViewProviderSketch& vp,
                                                              bool enable,
                                                              Base::Vector2d referencePoint)
{
    vp.setAngleSnapping(enable, referencePoint);
}

inline void ViewProviderSketchDrawSketchHandlerAttorney::moveConstraint(ViewProviderSketch& vp,
                                                                        int constNum,
                                                                        const Base::Vector2d& toPos)
{
    vp.moveConstraint(constNum, toPos);
}


/**************************** CurveConverter **********************************************/

CurveConverter::CurveConverter()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrp->Attach(this);
    }
    catch (const Base::ValueError& e) {  // ensure that if parameter strings are not well-formed,
                                         // the exception is not propagated
        Base::Console().DeveloperError("CurveConverter",
                                       "Malformed parameter string: %s\n",
                                       e.what());
    }

    updateCurvedEdgeCountSegmentsParameter();
}

CurveConverter::~CurveConverter()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrp->Detach(this);
    }
    catch (const Base::ValueError&
               e) {  // ensure that if parameter strings are not well-formed, the program is not
                     // terminated when calling the noexcept destructor.
        Base::Console().DeveloperError("CurveConverter",
                                       "Malformed parameter string: %s\n",
                                       e.what());
    }
}

std::vector<Base::Vector2d> CurveConverter::toVector2D(const Part::Geometry* geometry)
{
    std::vector<Base::Vector2d> vector2d;

    const auto type = geometry->getTypeId();

    auto emplaceasvector2d = [&vector2d](const Base::Vector3d& point) {
        vector2d.emplace_back(point.x, point.y);
    };

    auto isconic = type.isDerivedFrom(Part::GeomConic::getClassTypeId());
    auto isbounded = type.isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId());

    if (type == Part::GeomLineSegment::getClassTypeId()) {  // add a line
        auto geo = static_cast<const Part::GeomLineSegment*>(geometry);

        emplaceasvector2d(geo->getStartPoint());
        emplaceasvector2d(geo->getEndPoint());
    }
    else if (isconic || isbounded) {

        auto geo = static_cast<const Part::GeomConic*>(geometry);

        double segment =
            (geo->getLastParameter() - geo->getFirstParameter()) / curvedEdgeCountSegments;

        for (int i = 0; i < curvedEdgeCountSegments; i++) {
            emplaceasvector2d(geo->value(geo->getFirstParameter() + i * segment));
        }

        // either close the curve for untrimmed conic or set the last point for bounded curves
        emplaceasvector2d(isconic ? geo->value(0) : geo->value(geo->getLastParameter()));
    }

    return vector2d;
}

std::list<std::vector<Base::Vector2d>>
CurveConverter::toVector2DList(const std::vector<Part::Geometry*>& geometries)
{
    std::list<std::vector<Base::Vector2d>> list;

    for (const auto& geo : geometries) {
        list.push_back(toVector2D(geo));
    }

    return list;
}

void CurveConverter::updateCurvedEdgeCountSegmentsParameter()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
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
    : sketchgui(nullptr)
{}

DrawSketchHandler::~DrawSketchHandler()
{}

QString DrawSketchHandler::getCrosshairCursorSVGName() const
{
    return QString::fromLatin1("None");
}

void DrawSketchHandler::activate(ViewProviderSketch* vp)
{
    sketchgui = vp;

    // save the cursor at the time the DSH is activated
    auto* view = dynamic_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow());

    if (view) {
        Gui::View3DInventorViewer* viewer = dynamic_cast<Gui::View3DInventor*>(view)->getViewer();
        oldCursor = viewer->getWidget()->cursor();

        updateCursor();

        this->preActivated();
        this->activated();
    }
    else {
        sketchgui->purgeHandler();
    }
}

void DrawSketchHandler::deactivate()
{
    this->deactivated();
    this->postDeactivated();
    ViewProviderSketchDrawSketchHandlerAttorney::setConstraintSelectability(*sketchgui, true);

    // clear temporary Curve and Markers from the scenograph
    drawEdit(std::vector<Base::Vector2d>());
    drawEditMarkers(std::vector<Base::Vector2d>());
    resetPositionText();
    unsetCursor();
    setAngleSnapping(false);
}

void DrawSketchHandler::preActivated()
{
    ViewProviderSketchDrawSketchHandlerAttorney::setConstraintSelectability(*sketchgui, false);
}

void DrawSketchHandler::quit()
{
    assert(sketchgui);

    Gui::Selection().rmvSelectionGate();
    Gui::Selection().rmvPreselect();

    sketchgui->purgeHandler();
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

unsigned long DrawSketchHandler::getCrosshairColor()
{
    unsigned long color = 0xFFFFFFFF;  // white
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    color = hGrp->GetUnsigned("CursorCrosshairColor", color);
    // from rgba to rgb
    color = (color >> 8) & 0xFFFFFF;
    return color;
}

void DrawSketchHandler::setCrosshairCursor(const QString& svgName)
{
    const unsigned long defaultCrosshairColor = 0xFFFFFF;
    unsigned long color = getCrosshairColor();
    auto colorMapping = std::map<unsigned long, unsigned long>();
    colorMapping[defaultCrosshairColor] = color;
    // hot spot of all SVG icons should be 8,8 for 32x32 size (16x16 for 64x64)
    int hotX = 8;
    int hotY = 8;
    setSvgCursor(svgName, hotX, hotY, colorMapping);
}

void DrawSketchHandler::setCrosshairCursor(const char* svgName)
{
    QString cursorName = QString::fromLatin1(svgName);
    setCrosshairCursor(cursorName);
}

void DrawSketchHandler::setSvgCursor(const QString& cursorName,
                                     int x,
                                     int y,
                                     const std::map<unsigned long, unsigned long>& colorMapping)
{
    // The Sketcher_Pointer_*.svg icons have a default size of 64x64. When directly creating
    // them with a size of 32x32 they look very bad.
    // As a workaround the icons are created with 64x64 and afterwards the pixmap is scaled to
    // 32x32. This workaround is only needed if pRatio is equal to 1.0
    //
    qreal pRatio = devicePixelRatio();
    bool isRatioOne = (pRatio == 1.0);
    qreal defaultCursorSize = isRatioOne ? 64 : 32;
    qreal hotX = x;
    qreal hotY = y;
#if !defined(Q_OS_WIN32) && !defined(Q_OS_MAC)
    if (qGuiApp->platformName() == QLatin1String("xcb")) {
        hotX *= pRatio;
        hotY *= pRatio;
    }
#endif
    qreal cursorSize = defaultCursorSize * pRatio;

    QPixmap pointer = Gui::BitmapFactory().pixmapFromSvg(cursorName.toStdString().c_str(),
                                                         QSizeF(cursorSize, cursorSize),
                                                         colorMapping);
    if (isRatioOne) {
        pointer = pointer.scaled(32, 32);
    }
    pointer.setDevicePixelRatio(pRatio);
    setCursor(pointer, hotX, hotY, false);
}

void DrawSketchHandler::setCursor(const QPixmap& p, int x, int y, bool autoScale)
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();

        QCursor cursor;
        QPixmap p1(p);
        // TODO remove autoScale after all cursors are SVG-based
        if (autoScale) {
            qreal pRatio = viewer->devicePixelRatio();
            int newWidth = p.width() * pRatio;
            int newHeight = p.height() * pRatio;
            p1 = p1.scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            p1.setDevicePixelRatio(pRatio);
            qreal hotX = x;
            qreal hotY = y;
#if !defined(Q_OS_WIN32) && !defined(Q_OS_MAC)
            if (qGuiApp->platformName() == QLatin1String("xcb")) {
                hotX *= pRatio;
                hotY *= pRatio;
            }
#endif
            cursor = QCursor(p1, hotX, hotY);
        }
        else {
            // already scaled
            cursor = QCursor(p1, x, y);
        }

        actCursor = cursor;
        actCursorPixmap = p1;

        viewer->getWidget()->setCursor(cursor);
    }
}

void DrawSketchHandler::addCursorTail(std::vector<QPixmap>& pixmaps)
{
    // Create a pixmap that will contain icon and each autoconstraint icon
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        QPixmap baseIcon = QPixmap(actCursorPixmap);
        baseIcon.setDevicePixelRatio(actCursorPixmap.devicePixelRatio());
        qreal pixelRatio = baseIcon.devicePixelRatio();
        // cursor size in device independent pixels
        qreal baseCursorWidth = baseIcon.width();
        qreal baseCursorHeight = baseIcon.height();

        int tailWidth = 0;
        for (auto const& p : pixmaps) {
            tailWidth += p.width();
        }

        int newIconWidth = baseCursorWidth + tailWidth;
        int newIconHeight = baseCursorHeight;

        QPixmap newIcon(newIconWidth, newIconHeight);
        newIcon.fill(Qt::transparent);

        QPainter qp;
        qp.begin(&newIcon);

        qp.drawPixmap(QPointF(0, 0),
                      baseIcon.scaled(baseCursorWidth * pixelRatio,
                                      baseCursorHeight * pixelRatio,
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation));

        // Iterate through pixmaps and them to the cursor pixmap
        std::vector<QPixmap>::iterator pit = pixmaps.begin();
        int i = 0;
        qreal currentIconX = baseCursorWidth;
        qreal currentIconY;

        for (; pit != pixmaps.end(); ++pit, i++) {
            QPixmap icon = *pit;
            currentIconY = baseCursorHeight - icon.height();
            qp.drawPixmap(QPointF(currentIconX, currentIconY), icon);
            currentIconX += icon.width();
        }

        qp.end();  // Finish painting

        // Create the new cursor with the icon.
        QPoint p = actCursor.hotSpot();
        newIcon.setDevicePixelRatio(pixelRatio);
        QCursor newCursor(newIcon, p.x(), p.y());
        applyCursor(newCursor);
    }
}

void DrawSketchHandler::updateCursor()
{
    auto cursorstring = getCrosshairCursorSVGName();

    if (cursorstring != QString::fromLatin1("None")) {
        setCrosshairCursor(cursorstring);
    }
}

void DrawSketchHandler::applyCursor()
{
    applyCursor(actCursor);
}

void DrawSketchHandler::applyCursor(QCursor& newCursor)
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewer->getWidget()->setCursor(newCursor);
    }
}

void DrawSketchHandler::unsetCursor()
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewer->getWidget()->setCursor(oldCursor);
    }
}

qreal DrawSketchHandler::devicePixelRatio()
{
    qreal pixelRatio = 1;
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        pixelRatio = viewer->devicePixelRatio();
    }
    return pixelRatio;
}

std::vector<QPixmap>
DrawSketchHandler::suggestedConstraintsPixmaps(std::vector<AutoConstraint>& suggestedConstraints)
{
    std::vector<QPixmap> pixmaps;
    // Iterate through AutoConstraints types and get their pixmaps
    std::vector<AutoConstraint>::iterator it = suggestedConstraints.begin();
    int i = 0;
    for (; it != suggestedConstraints.end(); ++it, i++) {
        QString iconType;
        switch (it->Type) {
            case Horizontal:
                iconType = QString::fromLatin1("Constraint_Horizontal");
                break;
            case Vertical:
                iconType = QString::fromLatin1("Constraint_Vertical");
                break;
            case Coincident:
                iconType = QString::fromLatin1("Constraint_PointOnPoint");
                break;
            case PointOnObject:
                iconType = QString::fromLatin1("Constraint_PointOnObject");
                break;
            case Tangent:
                iconType = QString::fromLatin1("Constraint_Tangent");
                break;
            default:
                break;
        }
        if (!iconType.isEmpty()) {
            qreal pixelRatio = 1;
            Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
            if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer =
                    static_cast<Gui::View3DInventor*>(view)->getViewer();
                pixelRatio = viewer->devicePixelRatio();
            }
            int iconWidth = 16 * pixelRatio;
            QPixmap icon = Gui::BitmapFactory().pixmapFromSvg(iconType.toStdString().c_str(),
                                                              QSize(iconWidth, iconWidth));
            pixmaps.push_back(icon);
        }
    }
    return pixmaps;
}

int DrawSketchHandler::seekAutoConstraint(std::vector<AutoConstraint>& suggestedConstraints,
                                          const Base::Vector2d& Pos,
                                          const Base::Vector2d& Dir,
                                          AutoConstraint::TargetType type)
{
    suggestedConstraints.clear();

    if (!sketchgui->Autoconstraints.getValue()) {
        return 0;  // If Autoconstraints property is not set quit
    }

    Base::Vector3d hitShapeDir =
        Base::Vector3d(0,
                       0,
                       0);  // direction of hit shape (if it is a line, the direction of the line)

    // Get Preselection
    int preSelPnt = getPreselectPoint();
    int preSelCrv = getPreselectCurve();
    int preSelCrs = getPreselectCross();
    int GeoId = GeoEnum::GeoUndef;

    Sketcher::PointPos PosId = Sketcher::PointPos::none;

    if (preSelPnt != -1) {
        sketchgui->getSketchObject()->getGeoVertexIndex(preSelPnt, GeoId, PosId);
    }
    else if (preSelCrv != -1) {
        const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(preSelCrv);

        // ensure geom exists in case object was called before preselection is updated
        if (geom) {
            GeoId = preSelCrv;
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* line = static_cast<const Part::GeomLineSegment*>(geom);
                hitShapeDir = line->getEndPoint() - line->getStartPoint();
            }
        }
    }
    else if (preSelCrs == 0) {  // root point
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = Sketcher::PointPos::start;
    }
    else if (preSelCrs == 1) {  // x axis
        GeoId = Sketcher::GeoEnum::HAxis;
        hitShapeDir = Base::Vector3d(1, 0, 0);
    }
    else if (preSelCrs == 2) {  // y axis
        GeoId = Sketcher::GeoEnum::VAxis;
        hitShapeDir = Base::Vector3d(0, 1, 0);
    }

    if (GeoId != GeoEnum::GeoUndef) {
        // Currently only considers objects in current Sketcher
        AutoConstraint constr;
        constr.Type = Sketcher::None;
        constr.GeoId = GeoId;
        constr.PosId = PosId;
        if ((type == AutoConstraint::VERTEX || type == AutoConstraint::VERTEX_NO_TANGENCY)
            && PosId != Sketcher::PointPos::none) {
            constr.Type = Sketcher::Coincident;
        }
        else if (type == AutoConstraint::CURVE && PosId != Sketcher::PointPos::none) {
            constr.Type = Sketcher::PointOnObject;
        }
        else if ((type == AutoConstraint::VERTEX || type == AutoConstraint::VERTEX_NO_TANGENCY)
                 && PosId == Sketcher::PointPos::none) {
            constr.Type = Sketcher::PointOnObject;
        }
        else if (type == AutoConstraint::CURVE && PosId == Sketcher::PointPos::none) {
            constr.Type = Sketcher::Tangent;
        }

        if (constr.Type == Sketcher::Tangent && Dir.Length() > 1e-8
            && hitShapeDir.Length()
                > 1e-8) {  // We are hitting a line and have hitting vector information
            Base::Vector3d dir3d = Base::Vector3d(Dir.x, Dir.y, 0);
            double cosangle = dir3d.Normalize() * hitShapeDir.Normalize();

            // the angle between the line and the hitting direction are over around 6 degrees (it is
            // substantially parallel) or if it is an sketch axis (that can not move to accommodate
            // to the shape), then only if it is around 6 degrees with the normal (around 84
            // degrees)
            if (fabs(cosangle) < 0.995f
                || ((GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis)
                    && fabs(cosangle) < 0.1)) {
                suggestedConstraints.push_back(constr);
            }


            return suggestedConstraints.size();
        }

        if (constr.Type != Sketcher::None) {
            suggestedConstraints.push_back(constr);
        }
    }

    if (Dir.Length() < 1e-8 || type == AutoConstraint::CURVE) {
        // Direction not set so return;
        return suggestedConstraints.size();
    }

    // Suggest vertical and horizontal constraints

    // Number of Degree of deviation from horizontal or vertical lines
    const double angleDev = 2;
    const double angleDevRad = angleDev * M_PI / 180.;

    AutoConstraint constr;
    constr.Type = Sketcher::None;
    constr.GeoId = GeoEnum::GeoUndef;
    constr.PosId = Sketcher::PointPos::none;
    double angle = std::abs(atan2(Dir.y, Dir.x));
    if (angle < angleDevRad || (M_PI - angle) < angleDevRad) {
        // Suggest horizontal constraint
        constr.Type = Sketcher::Horizontal;
    }
    else if (std::abs(angle - M_PI_2) < angleDevRad) {
        // Suggest vertical constraint
        constr.Type = Sketcher::Vertical;
    }

    if (constr.Type != Sketcher::None) {
        suggestedConstraints.push_back(constr);
    }

    // Do not seek for tangent if we are actually building a primitive
    if (type == AutoConstraint::VERTEX_NO_TANGENCY) {
        return suggestedConstraints.size();
    }

    // Find if there are tangent constraints (currently arcs and circles)

    int tangId = GeoEnum::GeoUndef;

    // Do not consider if distance is more than that.
    // Decrease this value when a candidate is found.
    double tangDeviation = 0.1 * sketchgui->getScaleFactor();

    // Get geometry list
    const std::vector<Part::Geometry*> geomlist =
        sketchgui->getSketchObject()->getCompleteGeometry();

    Base::Vector3d tmpPos(Pos.x, Pos.y, 0.f);                    // Current cursor point
    Base::Vector3d tmpDir(Dir.x, Dir.y, 0.f);                    // Direction of line
    Base::Vector3d tmpStart(Pos.x - Dir.x, Pos.y - Dir.y, 0.f);  // Start point

    // Iterate through geometry
    int i = 0;
    for (std::vector<Part::Geometry*>::const_iterator it = geomlist.begin(); it != geomlist.end();
         ++it, i++) {

        if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>((*it));

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
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {

            const Part::GeomEllipse* ellipse = static_cast<const Part::GeomEllipse*>((*it));

            Base::Vector3d center = ellipse->getCenter();

            double a = ellipse->getMajorRadius();
            double b = ellipse->getMinorRadius();
            Base::Vector3d majdir = ellipse->getMajorAxisDir();

            double cf = sqrt(a * a - b * b);

            Base::Vector3d focus1P = center + cf * majdir;
            Base::Vector3d focus2P = center - cf * majdir;

            Base::Vector3d norm = Base::Vector3d(Dir.y, -Dir.x).Normalize();

            double distancetoline = norm * (tmpPos - focus1P);  // distance focus1 to line

            Base::Vector3d focus1PMirrored =
                focus1P + 2 * distancetoline * norm;  // mirror of focus1 with respect to the line

            double error = fabs((focus1PMirrored - focus2P).Length() - 2 * a);

            if (error < tangDeviation) {
                tangId = i;
                tangDeviation = error;
            }
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle* arc = static_cast<const Part::GeomArcOfCircle*>((*it));

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
                double startAngle, endAngle;
                arc->getRange(startAngle, endAngle, /*emulateCCW=*/true);

                double angle = atan2(projPnt.y, projPnt.x);
                while (angle < startAngle) {
                    angle += 2 * D_PI;  // Bring it to range of arc
                }

                // if the point is on correct side of arc
                if (angle <= endAngle) {  // Now need to check only one side
                    tangId = i;
                    tangDeviation = projDist;
                }
            }
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            const Part::GeomArcOfEllipse* aoe = static_cast<const Part::GeomArcOfEllipse*>((*it));

            Base::Vector3d center = aoe->getCenter();

            double a = aoe->getMajorRadius();
            double b = aoe->getMinorRadius();
            Base::Vector3d majdir = aoe->getMajorAxisDir();

            double cf = sqrt(a * a - b * b);

            Base::Vector3d focus1P = center + cf * majdir;
            Base::Vector3d focus2P = center - cf * majdir;

            Base::Vector3d norm = Base::Vector3d(Dir.y, -Dir.x).Normalize();

            double distancetoline = norm * (tmpPos - focus1P);  // distance focus1 to line

            Base::Vector3d focus1PMirrored =
                focus1P + 2 * distancetoline * norm;  // mirror of focus1 with respect to the line

            double error = fabs((focus1PMirrored - focus2P).Length() - 2 * a);

            if (error < tangDeviation) {
                tangId = i;
                tangDeviation = error;
            }

            if (error < tangDeviation) {
                double startAngle, endAngle;
                aoe->getRange(startAngle, endAngle, /*emulateCCW=*/true);

                double angle = Base::fmod(
                    atan2(
                        -aoe->getMajorRadius()
                            * ((tmpPos.x - center.x) * majdir.y - (tmpPos.y - center.y) * majdir.x),
                        aoe->getMinorRadius()
                            * ((tmpPos.x - center.x) * majdir.x + (tmpPos.y - center.y) * majdir.y))
                        - startAngle,
                    2.f * M_PI);

                while (angle < startAngle) {
                    angle += 2 * D_PI;  // Bring it to range of arc
                }

                // if the point is on correct side of arc
                if (angle <= endAngle) {  // Now need to check only one side
                    tangId = i;
                    tangDeviation = error;
                }
            }
        }
    }

    if (tangId != GeoEnum::GeoUndef) {
        if (tangId > getHighestCurveIndex()) {  // external Geometry
            tangId = getHighestCurveIndex() - tangId;
        }
        // Suggest vertical constraint
        constr.Type = Tangent;
        constr.GeoId = tangId;
        constr.PosId = Sketcher::PointPos::none;
        suggestedConstraints.push_back(constr);
    }

    return suggestedConstraints.size();
}

void DrawSketchHandler::createAutoConstraints(const std::vector<AutoConstraint>& autoConstrs,
                                              int geoId1,
                                              Sketcher::PointPos posId1,
                                              bool createowncommand /*= true*/)
{
    if (!sketchgui->Autoconstraints.getValue()) {
        return;  // If Autoconstraints property is not set quit
    }

    if (!autoConstrs.empty()) {

        if (createowncommand) {
            // Open the Command
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add auto constraints"));
        }

        // Iterate through constraints
        std::vector<AutoConstraint>::const_iterator it = autoConstrs.begin();
        for (; it != autoConstrs.end(); ++it) {
            int geoId2 = it->GeoId;

            switch (it->Type) {
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
                        it->GeoId,
                        static_cast<int>(it->PosId));
                } break;
                case Sketcher::PointOnObject: {
                    Sketcher::PointPos posId2 = it->PosId;
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
                        geoId2);
                } break;
                    // In special case of Horizontal/Vertical constraint, geoId2 is normally unused
                    // and should be 'Constraint::GeoUndef' However it can be used as a way to
                    // require the function to apply these constraints on another geometry In this
                    // case the caller as to set geoId2, then it will be used as target instead of
                    // geoId2
                case Sketcher::Horizontal: {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addConstraint(Sketcher.Constraint('Horizontal',%d)) ",
                                          geoId2 != GeoEnum::GeoUndef ? geoId2 : geoId1);
                } break;
                case Sketcher::Vertical: {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addConstraint(Sketcher.Constraint('Vertical',%d)) ",
                                          geoId2 != GeoEnum::GeoUndef ? geoId2 : geoId1);
                } break;
                case Sketcher::Tangent: {
                    Sketcher::SketchObject* Obj =
                        static_cast<Sketcher::SketchObject*>(sketchgui->getObject());

                    const Part::Geometry* geom1 = Obj->getGeometry(geoId1);
                    const Part::Geometry* geom2 = Obj->getGeometry(it->GeoId);

                    // ellipse tangency support using construction elements (lines)
                    if (geom1 && geom2
                        && (geom1->getTypeId() == Part::GeomEllipse::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomEllipse::getClassTypeId())) {

                        if (geom1->getTypeId() != Part::GeomEllipse::getClassTypeId()) {
                            std::swap(geoId1, geoId2);
                        }

                        // geoId1 is the ellipse
                        geom1 = Obj->getGeometry(geoId1);
                        geom2 = Obj->getGeometry(geoId2);

                        if (geom2->getTypeId() == Part::GeomEllipse::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomCircle::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            // in all these cases an intermediate element is needed
                            makeTangentToEllipseviaNewPoint(
                                Obj,
                                static_cast<const Part::GeomEllipse*>(geom1),
                                geom2,
                                geoId1,
                                geoId2);
                            return;
                        }
                    }

                    // arc of ellipse tangency support using external elements
                    if (geom1 && geom2
                        && (geom1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())) {

                        if (geom1->getTypeId() != Part::GeomArcOfEllipse::getClassTypeId()) {
                            std::swap(geoId1, geoId2);
                        }

                        // geoId1 is the arc of ellipse
                        geom1 = Obj->getGeometry(geoId1);
                        geom2 = Obj->getGeometry(geoId2);

                        if (geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomCircle::getClassTypeId()
                            || geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            // in all these cases an intermediate element is needed
                            makeTangentToArcOfEllipseviaNewPoint(
                                Obj,
                                static_cast<const Part::GeomArcOfEllipse*>(geom1),
                                geom2,
                                geoId1,
                                geoId2);
                            return;
                        }
                    }

                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "addConstraint(Sketcher.Constraint('Tangent',%d, %d)) ",
                                          geoId1,
                                          it->GeoId);
                } break;
                default:
                    break;
            }

            if (createowncommand) {
                Gui::Command::commitCommand();
            }
            // Gui::Command::updateActive(); // There is already an recompute in each command
            // creation, this is redundant.
        }
    }
}

void DrawSketchHandler::renderSuggestConstraintsCursor(
    std::vector<AutoConstraint>& suggestedConstraints)
{
    std::vector<QPixmap> pixmaps = suggestedConstraintsPixmaps(suggestedConstraints);
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

void DrawSketchHandler::drawEdit(const std::vector<Base::Vector2d>& EditCurve)
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(*sketchgui, EditCurve);
}

void DrawSketchHandler::drawEdit(const std::list<std::vector<Base::Vector2d>>& list)
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawEdit(*sketchgui, list);
}

void DrawSketchHandler::drawEdit(const std::vector<Part::Geometry*>& geometries)
{
    static CurveConverter c;

    auto list = c.toVector2DList(geometries);

    drawEdit(list);
}

void DrawSketchHandler::drawPositionAtCursor(const Base::Vector2d& position)
{
    setPositionText(position);
}

void DrawSketchHandler::drawDirectionAtCursor(const Base::Vector2d& position,
                                              const Base::Vector2d& origin)
{
    float length = (position - origin).Length();
    float angle = (position - origin).GetAngle(Base::Vector2d(1.f, 0.f));

    if (showCursorCoords()) {
        SbString text;
        std::string lengthString = lengthToDisplayFormat(length, 1);
        std::string angleString = angleToDisplayFormat(angle * 180.0 / M_PI, 1);
        text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
        setPositionText(position, text);
    }
}

void DrawSketchHandler::drawEditMarkers(const std::vector<Base::Vector2d>& EditMarkers,
                                        unsigned int augmentationlevel)
{
    ViewProviderSketchDrawSketchHandlerAttorney::drawEditMarkers(*sketchgui,
                                                                 EditMarkers,
                                                                 augmentationlevel);
}

void DrawSketchHandler::setAxisPickStyle(bool on)
{
    ViewProviderSketchDrawSketchHandlerAttorney::setAxisPickStyle(*sketchgui, on);
}

void DrawSketchHandler::moveCursorToSketchPoint(Base::Vector2d point)
{
    ViewProviderSketchDrawSketchHandlerAttorney::moveCursorToSketchPoint(*sketchgui, point);
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
    ViewProviderSketchDrawSketchHandlerAttorney::setAngleSnapping(*sketchgui,
                                                                  enable,
                                                                  referencePoint);
}

void DrawSketchHandler::moveConstraint(int constNum, const Base::Vector2d& toPos)
{
    ViewProviderSketchDrawSketchHandlerAttorney::moveConstraint(*sketchgui, constNum, toPos);
}
