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
# include <Standard_math.hxx>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoFont.h>
# include <QPainter>
# include <cmath>
#endif  // #ifndef _PreComp_

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Macro.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/View3DInventor.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "ViewProviderSketch.h"
#include "CommandConstraints.h"


using namespace SketcherGui;
using namespace Sketcher;


//**************************************************************************
// Construction/Destruction

DrawSketchHandler::DrawSketchHandler() : sketchgui(0) {}

DrawSketchHandler::~DrawSketchHandler() {}

void DrawSketchHandler::quit(void)
{
    assert(sketchgui);
    sketchgui->drawEdit(std::vector<Base::Vector2d>());
    resetPositionText();

    Gui::Selection().rmvSelectionGate();
    Gui::Selection().rmvPreselect();

    unsetCursor();
    sketchgui->purgeHandler();
}

//**************************************************************************
// Helpers

int DrawSketchHandler::getHighestVertexIndex(void)
{
    return sketchgui->getSketchObject()->getHighestVertexIndex();
}

int DrawSketchHandler::getHighestCurveIndex(void)
{
    return sketchgui->getSketchObject()->getHighestCurveIndex();
}

void DrawSketchHandler::setCursor(const QPixmap &p,int x,int y)
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();

        oldCursor = viewer->getWidget()->cursor();
        QCursor cursor(p, x, y);
        actCursor = cursor;

        viewer->getWidget()->setCursor(cursor);
    }
}

void DrawSketchHandler::applyCursor(void)
{
    applyCursor(actCursor);
}

void DrawSketchHandler::applyCursor(QCursor &newCursor)
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewer->getWidget()->setCursor(newCursor);
    }
}

void DrawSketchHandler::unsetCursor(void)
{
    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewer->getWidget()->setCursor(oldCursor);
    }
}

int DrawSketchHandler::seekAutoConstraint(std::vector<AutoConstraint> &suggestedConstraints,
                                          const Base::Vector2d& Pos, const Base::Vector2d& Dir,
                                          AutoConstraint::TargetType type)
{
    suggestedConstraints.clear();

    if (!sketchgui->Autoconstraints.getValue())
        return 0; // If Autoconstraints property is not set quit

    Base::Vector3d hitShapeDir = Base::Vector3d(0,0,0); // direction of hit shape (if it is a line, the direction of the line)

    // Get Preselection
    int preSelPnt = sketchgui->getPreselectPoint();
    int preSelCrv = sketchgui->getPreselectCurve();
    int preSelCrs = sketchgui->getPreselectCross();
    int GeoId = Constraint::GeoUndef;
    Sketcher::PointPos PosId = Sketcher::none;
    if (preSelPnt != -1)
        sketchgui->getSketchObject()->getGeoVertexIndex(preSelPnt, GeoId, PosId);
    else if (preSelCrv != -1){
        GeoId = preSelCrv;
        const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(GeoId);

        if(geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()){
            const Part::GeomLineSegment *line = static_cast<const Part::GeomLineSegment *>(geom);
            hitShapeDir= line->getEndPoint()-line->getStartPoint();
        }

    }
    else if (preSelCrs == 0) { // root point
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = Sketcher::start;
    }
    else if (preSelCrs == 1){ // x axis
        GeoId = Sketcher::GeoEnum::HAxis;
        hitShapeDir = Base::Vector3d(1,0,0);

    }
    else if (preSelCrs == 2){ // y axis
        GeoId = Sketcher::GeoEnum::VAxis;
        hitShapeDir = Base::Vector3d(0,1,0);
    }

    if (GeoId != Constraint::GeoUndef) {
        // Currently only considers objects in current Sketcher
        AutoConstraint constr;
        constr.Type = Sketcher::None;
        constr.GeoId = GeoId;
        constr.PosId = PosId;
        if (type == AutoConstraint::VERTEX && PosId != Sketcher::none)
            constr.Type = Sketcher::Coincident;
        else if (type == AutoConstraint::CURVE && PosId != Sketcher::none)
            constr.Type = Sketcher::PointOnObject;
        else if (type == AutoConstraint::VERTEX && PosId == Sketcher::none)
            constr.Type = Sketcher::PointOnObject;
        else if (type == AutoConstraint::CURVE && PosId == Sketcher::none)
            constr.Type = Sketcher::Tangent;

        if(constr.Type == Sketcher::Tangent && Dir.Length() > 1e-8 && hitShapeDir.Length() > 1e-8) { // We are hitting a line and have hitting vector information
            Base::Vector3d dir3d = Base::Vector3d(Dir.x,Dir.y,0);
            double cosangle=dir3d.Normalize()*hitShapeDir.Normalize();

            // the angle between the line and the hitting direction are over around 6 degrees (it is substantially parallel)
            // or if it is an sketch axis (that can not move to accommodate to the shape), then only if it is around 6 degrees with the normal (around 84 degrees)
            if (fabs(cosangle) < 0.995f || ((GeoId==Sketcher::GeoEnum::HAxis || GeoId==Sketcher::GeoEnum::VAxis) && fabs(cosangle) < 0.1))
                suggestedConstraints.push_back(constr);


            return suggestedConstraints.size();
        }

        if (constr.Type != Sketcher::None)
            suggestedConstraints.push_back(constr);
    }

    if (Dir.Length() < 1e-8 || type == AutoConstraint::CURVE)
        // Direction not set so return;
        return suggestedConstraints.size();

    // Suggest vertical and horizontal constraints

    // Number of Degree of deviation from horizontal or vertical lines
    const double angleDev = 2;
    const double angleDevRad = angleDev *  M_PI / 180.;

    AutoConstraint constr;
    constr.Type = Sketcher::None;
    constr.GeoId = Constraint::GeoUndef;
    constr.PosId = Sketcher::none;
    double angle = std::abs(atan2(Dir.y, Dir.x));
    if (angle < angleDevRad || (M_PI - angle) < angleDevRad )
        // Suggest horizontal constraint
        constr.Type = Sketcher::Horizontal;
    else if (std::abs(angle - M_PI_2) < angleDevRad)
        // Suggest vertical constraint
        constr.Type = Sketcher::Vertical;

    if (constr.Type != Sketcher::None)
        suggestedConstraints.push_back(constr);

    // Find if there are tangent constraints (currently arcs and circles)

    int tangId = Constraint::GeoUndef;

    // Do not consider if distance is more than that.
    // Decrease this value when a candidate is found.
    double tangDeviation = 0.1 * sketchgui->getScaleFactor();

    // Get geometry list
    const std::vector<Part::Geometry *> geomlist = sketchgui->getSketchObject()->getCompleteGeometry();

    Base::Vector3d tmpPos(Pos.x, Pos.y, 0.f);                 // Current cursor point
    Base::Vector3d tmpDir(Dir.x, Dir.y, 0.f);                 // Direction of line
    Base::Vector3d tmpStart(Pos.x-Dir.x, Pos.y-Dir.y, 0.f);  // Start point

    // Iterate through geometry
    int i = 0;
    for (std::vector<Part::Geometry *>::const_iterator it=geomlist.begin(); it != geomlist.end(); ++it, i++) {

        if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>((*it));

            Base::Vector3d center = circle->getCenter();

            double radius = circle->getRadius();

            // ignore if no touch (use dot product)
            if(tmpDir * (center-tmpPos) > 0 || tmpDir * (center-tmpStart) < 0)
                continue;

            Base::Vector3d projPnt(0.f, 0.f, 0.f);
            projPnt = projPnt.ProjectToLine(center - tmpPos, tmpDir);
            double projDist = std::abs(projPnt.Length() - radius);

            // Find if nearest
            if (projDist < tangDeviation) {
                tangId = i;
                tangDeviation = projDist;
            }

        } else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {

            const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>((*it));

            Base::Vector3d center = ellipse->getCenter();

            double a = ellipse->getMajorRadius();
            double b = ellipse->getMinorRadius();
            Base::Vector3d majdir = ellipse->getMajorAxisDir();

            double cf = sqrt(a*a - b*b);

            Base::Vector3d focus1P = center + cf * majdir;
            Base::Vector3d focus2P = center - cf * majdir;

            Base::Vector3d norm = Base::Vector3d(Dir.y,-Dir.x).Normalize();

            double distancetoline = norm*(tmpPos - focus1P); // distance focus1 to line

            Base::Vector3d focus1PMirrored = focus1P + 2*distancetoline*norm; // mirror of focus1 with respect to the line

            double error = fabs((focus1PMirrored-focus2P).Length() - 2*a);

            if ( error< tangDeviation) {
                    tangId = i;
                    tangDeviation = error;
            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>((*it));

            Base::Vector3d center = arc->getCenter();
            double radius = arc->getRadius();

            // ignore if no touch (use dot product)
            if(tmpDir * (center-tmpPos) > 0 || tmpDir * (center-tmpStart) < 0)
                continue;

            Base::Vector3d projPnt(0.f, 0.f, 0.f);
            projPnt = projPnt.ProjectToLine(center - tmpPos, tmpDir);
            double projDist = std::abs(projPnt.Length() - radius);

            if (projDist < tangDeviation) {
                double startAngle, endAngle;
                arc->getRange(startAngle, endAngle, /*emulateCCW=*/true);

                double angle = atan2(projPnt.y, projPnt.x);
                while(angle < startAngle)
                    angle += 2*D_PI;         // Bring it to range of arc

                // if the point is on correct side of arc
                if (angle <= endAngle) {     // Now need to check only one side
                    tangId = i;
                    tangDeviation = projDist;
                }
            }
        } else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>((*it));

            Base::Vector3d center = aoe->getCenter();

            double a = aoe->getMajorRadius();
            double b = aoe->getMinorRadius();
            Base::Vector3d majdir = aoe->getMajorAxisDir();

            double cf = sqrt(a*a - b*b);

            Base::Vector3d focus1P = center + cf * majdir;
            Base::Vector3d focus2P = center - cf * majdir;

            Base::Vector3d norm = Base::Vector3d(Dir.y,-Dir.x).Normalize();

            double distancetoline = norm*(tmpPos - focus1P); // distance focus1 to line

            Base::Vector3d focus1PMirrored = focus1P + 2*distancetoline*norm; // mirror of focus1 with respect to the line

            double error = fabs((focus1PMirrored-focus2P).Length() - 2*a);

            if ( error< tangDeviation ) {
                    tangId = i;
                    tangDeviation = error;
            }

            if (error < tangDeviation) {
                double startAngle, endAngle;
                aoe->getRange(startAngle, endAngle, /*emulateCCW=*/true);

                double angle = Base::fmod(
                    atan2(-aoe->getMajorRadius()*((tmpPos.x-center.x)*majdir.y-(tmpPos.y-center.y)*majdir.x),
                                aoe->getMinorRadius()*((tmpPos.x-center.x)*majdir.x+(tmpPos.y-center.y)*majdir.y)
                    )- startAngle, 2.f*M_PI);

                while(angle < startAngle)
                    angle += 2*D_PI;         // Bring it to range of arc

                // if the point is on correct side of arc
                if (angle <= endAngle) {     // Now need to check only one side
                    tangId = i;
                    tangDeviation = error;
                }
            }
        }
    }

    if (tangId != Constraint::GeoUndef) {
        if (tangId > getHighestCurveIndex()) // external Geometry
            tangId = getHighestCurveIndex() - tangId;
        // Suggest vertical constraint
        constr.Type = Tangent;
        constr.GeoId = tangId;
        constr.PosId = Sketcher::none;
        suggestedConstraints.push_back(constr);
    }

    return suggestedConstraints.size();
}

void DrawSketchHandler::createAutoConstraints(const std::vector<AutoConstraint> &autoConstrs,
                                              int geoId1, Sketcher::PointPos posId1, bool createowncommand /*= true*/)
{
    if (!sketchgui->Autoconstraints.getValue())
        return; // If Autoconstraints property is not set quit

    if (autoConstrs.size() > 0) {

        if(createowncommand) {
            // Open the Command
            Gui::Command::openCommand("Add auto constraints");
        }

        // Iterate through constraints
        std::vector<AutoConstraint>::const_iterator it = autoConstrs.begin();
        for (; it != autoConstrs.end(); ++it) {
            switch (it->Type)
            {
            case Sketcher::Coincident: {
                if (posId1 == Sketcher::none)
                    continue;
                // If the auto constraint has a point create a coincident otherwise it is an edge on a point
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%i,%i,%i,%i)) "
                                        ,sketchgui->getObject()->getNameInDocument()
                                        ,geoId1, posId1, it->GeoId, it->PosId
                                        );
                } break;
            case Sketcher::PointOnObject: {
                int geoId2 = it->GeoId;
                Sketcher::PointPos posId2 = it->PosId;
                if (posId1 == Sketcher::none) {
                    // Auto constraining an edge so swap parameters
                    std::swap(geoId1,geoId2);
                    std::swap(posId1,posId2);
                }

                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%i,%i,%i)) "
                                        ,sketchgui->getObject()->getNameInDocument()
                                        ,geoId1, posId1, geoId2
                                       );
                } break;
            case Sketcher::Horizontal: {

                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Horizontal',%i)) "
                ,sketchgui->getObject()->getNameInDocument()
                ,geoId1
                );

                } break;
            case Sketcher::Vertical: {

                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Vertical',%i)) "
                ,sketchgui->getObject()->getNameInDocument()
                ,geoId1
                );

                } break;
            case Sketcher::Tangent: {
                Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());

                const Part::Geometry *geom1 = Obj->getGeometry(geoId1);
                const Part::Geometry *geom2 = Obj->getGeometry(it->GeoId);

                int geoId2 = it->GeoId;

                // ellipse tangency support using construction elements (lines)
                if( geom1 && geom2 &&
                    ( geom1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() )){

                    if(geom1->getTypeId() != Part::GeomEllipse::getClassTypeId())
                        std::swap(geoId1,geoId2);

                    // geoId1 is the ellipse
                    geom1 = Obj->getGeometry(geoId1);
                    geom2 = Obj->getGeometry(geoId2);

                    if( geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {
                        // in all these cases an intermediate element is needed
                        makeTangentToEllipseviaNewPoint(Obj,
                                                        static_cast<const Part::GeomEllipse *>(geom1),
                                                        geom2, geoId1, geoId2);
                        return;
                    }
                }

                // arc of ellipse tangency support using external elements
                if( geom1 && geom2 &&
                    ( geom1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() )){

                    if(geom1->getTypeId() != Part::GeomArcOfEllipse::getClassTypeId())
                        std::swap(geoId1,geoId2);

                    // geoId1 is the arc of ellipse
                    geom1 = Obj->getGeometry(geoId1);
                    geom2 = Obj->getGeometry(geoId2);

                    if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                        geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {
                        // in all these cases an intermediate element is needed
                        makeTangentToArcOfEllipseviaNewPoint(Obj,
                                                             static_cast<const Part::GeomArcOfEllipse *>(geom1), geom2, geoId1, geoId2);
                        return;
                    }
                }

                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%i, %i)) "
                                        ,sketchgui->getObject()->getNameInDocument()
                                        ,geoId1, it->GeoId
                                       );
                } break;
            default:
                break;
            }

            if(createowncommand) {
                Gui::Command::commitCommand();
            }
            //Gui::Command::updateActive(); // There is already an recompute in each command creation, this is redundant.
        }
    }
}

void DrawSketchHandler::renderSuggestConstraintsCursor(std::vector<AutoConstraint> &suggestedConstraints)
{
    // Auto Constraint icon size in px
    int iconSize = 16;

    // Create a pixmap that will contain icon and each autoconstraint icon
    QPixmap baseIcon = actCursor.pixmap();
    QPixmap newIcon(baseIcon.width() + suggestedConstraints.size() * iconSize,
                    baseIcon.height());
    newIcon.fill(Qt::transparent);

    QPainter qp;
    qp.begin(&newIcon);

    qp.drawPixmap(0,0, baseIcon);

    // Iterate through AutoConstraints type and add icons to the cursor pixmap
    std::vector<AutoConstraint>::iterator it=suggestedConstraints.begin();
    int i = 0;
    for (; it != suggestedConstraints.end(); ++it, i++) {
        QString iconType;
        switch (it->Type)
        {
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
            QPixmap icon = Gui::BitmapFactory().pixmap(iconType.toLatin1()).scaledToWidth(iconSize);
            qp.drawPixmap(QPoint(baseIcon.width() + i * iconSize, baseIcon.height() - iconSize), icon);
        }
    }

    qp.end(); // Finish painting

    // Create the new cursor with the icon.
    QPoint p=actCursor.hotSpot();
    QCursor newCursor(newIcon, p.x(), p.y());
    applyCursor(newCursor);
}

void DrawSketchHandler::setPositionText(const Base::Vector2d &Pos, const SbString &text)
{
    sketchgui->setPositionText(Pos, text);
}


void DrawSketchHandler::setPositionText(const Base::Vector2d &Pos)
{
    sketchgui->setPositionText(Pos);
}

void DrawSketchHandler::resetPositionText(void)
{
    sketchgui->resetPositionText();
}
