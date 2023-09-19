/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHERGUI_DrawSketchHandlerSlot_H
#define SKETCHERGUI_DrawSketchHandlerSlot_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerSlot: public DrawSketchHandler
{
public:
    DrawSketchHandlerSlot()
        : Mode(STATUS_SEEK_First)
        , SnapMode(SNAP_MODE_Free)
        , SnapDir(SNAP_DIR_Horz)
        , dx(0)
        , dy(0)
        , r(0)
        , EditCurve(35)
    {}
    ~DrawSketchHandlerSlot() override
    {}
    /// mode table
    enum BoxMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_End
    };

    enum SNAP_MODE
    {
        SNAP_MODE_Free,
        SNAP_MODE_Straight
    };

    enum SNAP_DIR
    {
        SNAP_DIR_Horz,
        SNAP_DIR_Vert
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {

        if (Mode == STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode == STATUS_SEEK_Second) {
            dx = onSketchPos.x - StartPos.x;
            dy = onSketchPos.y - StartPos.y;

            if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
                SnapMode = SNAP_MODE_Straight;
            }
            else {
                SnapMode = SNAP_MODE_Free;
            }

            double a = 0;
            double rev = 0;
            if (fabs(dx) > fabs(dy)) {
                r = fabs(dx) / 4;
                rev = Base::sgn(dx);
                SnapDir = SNAP_DIR_Horz;
                if (SnapMode == SNAP_MODE_Straight) {
                    dy = 0;
                }
            }
            else {
                r = fabs(dy) / 4;
                a = 8;
                rev = Base::sgn(dy);
                SnapDir = SNAP_DIR_Vert;
                if (SnapMode == SNAP_MODE_Straight) {
                    dx = 0;
                }
            }

            // draw the arcs with each 16 segments
            for (int i = 0; i < 17; i++) {
                // first get the position at the arc
                // if a is 0, the end points of the arc are at the y-axis, if it is 8, they are on
                // the x-axis
                double angle = (i + a) * M_PI / 16.0;
                double rx = -r * rev * sin(angle);
                double ry = r * rev * cos(angle);
                // now apply the rotation matrix according to the angle between StartPos and
                // onSketchPos
                if (!(dx == 0 || dy == 0)) {
                    double rotAngle = atan(dy / dx);
                    if (a > 0) {
                        rotAngle = -atan(dx / dy);
                    }
                    double rxRot = rx * cos(rotAngle) - ry * sin(rotAngle);
                    double ryRot = rx * sin(rotAngle) + ry * cos(rotAngle);
                    rx = rxRot;
                    ry = ryRot;
                }
                EditCurve[i] = Base::Vector2d(StartPos.x + rx, StartPos.y + ry);
                EditCurve[17 + i] = Base::Vector2d(StartPos.x + dx - rx, StartPos.y + dy - ry);
            }
            EditCurve[34] = EditCurve[0];

            if (showCursorCoords()) {
                SbString text;
                std::string rString = lengthToDisplayFormat(r, 1);
                std::string sqrtString = lengthToDisplayFormat(sqrt(dx * dx + dy * dy), 1);
                text.sprintf("  (R%s L%s))", rString.c_str(), sqrtString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2,
                                   onSketchPos,
                                   Base::Vector2d(dx, dy),
                                   AutoConstraint::VERTEX_NO_TANGENCY)) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            StartPos = onSketchPos;
            Mode = STATUS_SEEK_Second;
        }
        else {
            Mode = STATUS_End;
        }
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End) {
            unsetCursor();
            resetPositionText();

            int firstCurve = getHighestCurveIndex() + 1;
            // add the geometry to the sketch
            // first determine the rotation angle for the first arc
            double start, end;
            if (fabs(dx) > fabs(dy)) {
                if (dx > 0) {
                    start = 0.5 * M_PI;
                    end = 1.5 * M_PI;
                }
                else {
                    start = 1.5 * M_PI;
                    end = 0.5 * M_PI;
                }
            }
            else {
                if (dy > 0) {
                    start = -M_PI;
                    end = 0;
                }
                else {
                    start = 0;
                    end = -M_PI;
                }
            }

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add slot"));

                AutoConstraint lastCons = {Sketcher::None,
                                           Sketcher::GeoEnum::GeoUndef,
                                           Sketcher::PointPos::none};

                if (!sugConstr2.empty()) {
                    lastCons = sugConstr2.back();
                }

                ostringstream snapCon = ostringstream("");
                if (SnapMode == SNAP_MODE_Straight) {
                    snapCon << "conList.append(Sketcher.Constraint('";
                    if (SnapDir == SNAP_DIR_Horz) {
                        snapCon << "Horizontal";
                    }
                    else {
                        snapCon << "Vertical";
                    }
                    snapCon << "'," << firstCurve + 2 << "))\n";

                    // If horizontal/vertical already applied because of snap, do not duplicate with
                    // Autocontraint
                    if (lastCons.Type == Sketcher::Horizontal
                        || lastCons.Type == Sketcher::Vertical) {
                        sugConstr2.pop_back();
                    }
                }
                else {
                    // If horizontal/vertical Autoconstraint suggested, applied it on first line
                    // (rather than last arc)
                    if (lastCons.Type == Sketcher::Horizontal
                        || lastCons.Type == Sketcher::Vertical) {
                        sugConstr2.back().GeoId = firstCurve + 2;
                    }
                }

                Gui::Command::doCommand(
                    Gui::Command::Doc,
                    "geoList = []\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), "
                    "App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), "
                    "App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.LineSegment(App.Vector(%f, %f, 0), App.Vector(%f, %f, "
                    "0)))\n"
                    "geoList.append(Part.LineSegment(App.Vector(%f, %f, 0), App.Vector(%f, %f, "
                    "0)))\n"
                    "%s.addGeometry(geoList, %s)\n"
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "%s"
                    "%s.addConstraint(conList)\n"
                    "del geoList, conList\n",
                    StartPos.x,
                    StartPos.y,  // center of the arc1
                    r,           // radius arc1
                    start,
                    end,  // start and end angle of arc1
                    StartPos.x + dx,
                    StartPos.y + dy,  // center of the arc2
                    r,                // radius arc2
                    end,
                    end + M_PI,  // start and end angle of arc2
                    EditCurve[16].x,
                    EditCurve[16].y,
                    EditCurve[17].x,
                    EditCurve[17].y,  // line1
                    EditCurve[33].x,
                    EditCurve[33].y,
                    EditCurve[34].x,
                    EditCurve[34].y,                                             // line2
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),  // the sketch
                    geometryCreationMode == Construction
                        ? "True"
                        : "False",  // geometry as construction or not
                    firstCurve,
                    firstCurve + 2,  // tangent1
                    firstCurve + 2,
                    firstCurve + 1,  // tangent2
                    firstCurve + 1,
                    firstCurve + 3,  // tangent3
                    firstCurve + 3,
                    firstCurve,  // tangent4
                    firstCurve,
                    firstCurve + 1,         // equal constraint
                    snapCon.str().c_str(),  // horizontal/vertical constraint if snapping
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str());  // the sketch

                Gui::Command::commitCommand();

                // add auto constraints at the center of the first arc
                if (!sugConstr1.empty()) {
                    createAutoConstraints(sugConstr1,
                                          getHighestCurveIndex() - 3,
                                          Sketcher::PointPos::mid);
                    sugConstr1.clear();
                }

                // add auto constraints at the center of the second arc
                if (!sugConstr2.empty()) {
                    createAutoConstraints(sugConstr2,
                                          getHighestCurveIndex() - 2,
                                          Sketcher::PointPos::mid);
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add slot"));

                Gui::Command::abortCommand();

                tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(35);
                applyCursor();
                /* this is ok not to call to purgeHandler
                 * in continuous creation mode because the
                 * handler is destroyed by the quit() method on pressing the
                 * right button of the mouse */
            }
            else {
                sketchgui->purgeHandler();  // no code after this line, Handler get deleted in
                                            // ViewProvider
            }
            SnapMode = SNAP_MODE_Straight;
        }
        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Slot");
    }

protected:
    BoxMode Mode;
    SNAP_MODE SnapMode;
    SNAP_DIR SnapDir;
    Base::Vector2d StartPos;
    double dx, dy, r;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerSlot_H
