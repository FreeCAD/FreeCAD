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
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
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


using namespace SketcherGui;
using namespace Sketcher;


//**************************************************************************
// Construction/Destruction

DrawSketchHandler::DrawSketchHandler()
        : sketchgui(0)
{

}

DrawSketchHandler::~DrawSketchHandler()
{

}

void DrawSketchHandler::quit(void)
{
    assert(sketchgui);
    sketchgui->drawEdit(std::vector<Base::Vector2D>());
    resetPositionText();

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
                                          const Base::Vector2D& Pos, const Base::Vector2D& Dir,
                                          AutoConstraint::TargetType type)
{
    suggestedConstraints.clear();

    if (!sketchgui->Autoconstraints.getValue())
        return 0; // If Autoconstraints property is not set quit

    // Get Preselection
    int preSelPnt = sketchgui->getPreselectPoint();
    int preSelCrv = sketchgui->getPreselectCurve();
    int preSelCrs = sketchgui->getPreselectCross();
    int GeoId = Constraint::GeoUndef;
    Sketcher::PointPos PosId = Sketcher::none;
    if (preSelPnt != -1)
        sketchgui->getSketchObject()->getGeoVertexIndex(preSelPnt, GeoId, PosId);
    else if (preSelCrv != -1)
        GeoId = preSelCrv;
    else if (preSelCrs == 0) { // root point
        GeoId = -1;
        PosId = Sketcher::start;
    }
    else if (preSelCrs == 1) // x axis
        GeoId = -1;
    else if (preSelCrs == 2) // y axis
        GeoId = -2;

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

        if (constr.Type != Sketcher::None)
            suggestedConstraints.push_back(constr);
    }

    if (Dir.Length() < 1e-8)
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
    double angle = std::abs(atan2(Dir.fY, Dir.fX));
    if (angle < angleDevRad || (M_PI - angle) < angleDevRad )
        // Suggest horizontal constraint
        constr.Type = Sketcher::Horizontal;
    else if (std::abs(angle - M_PI_2) < angleDevRad)
        // Suggest vertical constraint
        constr.Type = Sketcher::Vertical;

    if (constr.Type != Sketcher::None)
        suggestedConstraints.push_back(constr);
 
    // Find if there are tangent constraints (currently arcs and circles)
    // FIXME needs to consider when zooming out?
    const double tangDeviation = 2.;

    int tangId = Constraint::GeoUndef;
    double smlTangDist = 1e15f;

    // Get geometry list
    const std::vector<Part::Geometry *> geomlist = sketchgui->getSketchObject()->getCompleteGeometry();

    // Iterate through geometry
    int i = 0;
    for (std::vector<Part::Geometry *>::const_iterator it=geomlist.begin(); it != geomlist.end(); ++it, i++) {

        if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>((*it));

            Base::Vector3d center = circle->getCenter();
            Base::Vector3d tmpPos(Pos.fX, Pos.fY, 0.f);

            double radius = circle->getRadius();

            Base::Vector3d projPnt(0.f, 0.f, 0.f);
            projPnt = projPnt.ProjToLine(center - tmpPos, Base::Vector3d(Dir.fX, Dir.fY));
            double projDist = projPnt.Length();

            if ( (projDist < radius + tangDeviation ) && (projDist > radius - tangDeviation)) {
                // Find if nearest
                if (projDist < smlTangDist) {
                    tangId = i;
                    smlTangDist = projDist;
                }
            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>((*it));

            Base::Vector3d center = arc->getCenter();
            double radius = arc->getRadius();

            Base::Vector3d projPnt(0.f, 0.f, 0.f);
            Base::Vector3d tmpPos(Pos.fX, Pos.fY, 0.f);

            projPnt = projPnt.ProjToLine(center - tmpPos, Base::Vector3d(Dir.fX, Dir.fY));
            double projDist = projPnt.Length();

            if ( projDist < radius + tangDeviation && projDist > radius - tangDeviation) {
                double startAngle, endAngle;
                arc->getRange(startAngle, endAngle);

                projPnt += center;
                double angle = atan2(projPnt.y, projPnt.x);

                // if the pnt is on correct side of arc and find if nearest
                if ((angle > startAngle && angle < endAngle) &&
                    (projDist < smlTangDist) ) {
                    tangId = i;
                    smlTangDist = projDist;
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
                                              int geoId1, Sketcher::PointPos posId1)
{
    if (!sketchgui->Autoconstraints.getValue())
        return; // If Autoconstraints property is not set quit

    if (autoConstrs.size() > 0) {
        // Open the Command
        Gui::Command::openCommand("Add auto constraints");

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
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%i, %i)) "
                                        ,sketchgui->getObject()->getNameInDocument()
                                        ,geoId1, it->GeoId
                                       );
                } break;
            }

            Gui::Command::commitCommand();
            Gui::Command::updateActive();
        }
    }
}

void DrawSketchHandler::renderSuggestConstraintsCursor(std::vector<AutoConstraint> &suggestedConstraints)
{
    // Auto Constrait icon size in px
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
            iconType = QString::fromAscii("Constraint_Horizontal");
            break;
        case Vertical:
            iconType = QString::fromAscii("Constraint_Vertical");
            break;
        case Coincident:
            iconType = QString::fromAscii("Constraint_PointOnPoint");
            break;
        case PointOnObject:
            iconType = QString::fromAscii("Constraint_PointOnObject");
            break;
        case Tangent:
            iconType = QString::fromAscii("Constraint_Tangent");
            break;
        }

        QPixmap icon = Gui::BitmapFactory().pixmap(iconType.toAscii()).scaledToWidth(iconSize);
        qp.drawPixmap(QPoint(baseIcon.width() + i * iconSize, baseIcon.height() - iconSize), icon);
    }

    qp.end(); // Finish painting

    // Create the new cursor with the icon.
    QPoint p=actCursor.hotSpot();
    QCursor newCursor(newIcon, p.x(), p.y());
    applyCursor(newCursor);
}

void DrawSketchHandler::setPositionText(const Base::Vector2D &Pos, const SbString &text)
{
    sketchgui->setPositionText(Pos, text);
}


void DrawSketchHandler::setPositionText(const Base::Vector2D &Pos)
{
    sketchgui->setPositionText(Pos);
}

void DrawSketchHandler::resetPositionText(void)
{
    sketchgui->resetPositionText();
}
