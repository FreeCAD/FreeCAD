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

#ifndef SKETCHERGUI_DrawSketchHandlerRectangle_H
#define SKETCHERGUI_DrawSketchHandlerRectangle_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerBox: public DrawSketchHandler
{
public:
    enum ConstructionMethod
    {
        Diagonal,
        CenterAndCorner
    };

    explicit DrawSketchHandlerBox(ConstructionMethod constrMethod = Diagonal)
        : Mode(STATUS_SEEK_First)
        , EditCurve(5)
        , constructionMethod(constrMethod)
    {}
    ~DrawSketchHandlerBox() override
    {}

    /// mode table
    enum BoxMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_End
    };

public:
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
            if (constructionMethod == Diagonal) {
                float dx = onSketchPos.x - EditCurve[0].x;
                float dy = onSketchPos.y - EditCurve[0].y;
                if (showCursorCoords()) {
                    SbString text;
                    std::string dxString = lengthToDisplayFormat(dx, 1);
                    std::string dyString = lengthToDisplayFormat(dy, 1);
                    text.sprintf(" (%s x %s)", dxString.c_str(), dyString.c_str());
                    setPositionText(onSketchPos, text);
                }

                EditCurve[2] = onSketchPos;
                EditCurve[1] = Base::Vector2d(onSketchPos.x, EditCurve[0].y);
                EditCurve[3] = Base::Vector2d(EditCurve[0].x, onSketchPos.y);
            }
            else if (constructionMethod == CenterAndCorner) {
                float dx = onSketchPos.x - center.x;
                float dy = onSketchPos.y - center.y;
                if (showCursorCoords()) {
                    SbString text;
                    std::string dxString = lengthToDisplayFormat(dx, 1);
                    std::string dyString = lengthToDisplayFormat(dy, 1);
                    text.sprintf(" (%s x %s)", dxString.c_str(), dyString.c_str());
                    setPositionText(onSketchPos, text);
                }

                EditCurve[0] = center - (onSketchPos - center);
                EditCurve[1] = Base::Vector2d(EditCurve[0].x, onSketchPos.y);
                EditCurve[2] = onSketchPos;
                EditCurve[3] = Base::Vector2d(onSketchPos.x, EditCurve[0].y);
                EditCurve[4] = EditCurve[0];
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.0, 0.0))) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        applyCursor();
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {
            if (constructionMethod == Diagonal) {
                EditCurve[0] = onSketchPos;
                EditCurve[4] = onSketchPos;
            }
            else if (constructionMethod == CenterAndCorner) {
                center = onSketchPos;
            }

            Mode = STATUS_SEEK_Second;
        }
        else {
            if (constructionMethod == Diagonal) {
                EditCurve[2] = onSketchPos;
                EditCurve[1] = Base::Vector2d(onSketchPos.x, EditCurve[0].y);
                EditCurve[3] = Base::Vector2d(EditCurve[0].x, onSketchPos.y);
                drawEdit(EditCurve);
                Mode = STATUS_End;
            }
            else if (constructionMethod == CenterAndCorner) {
                EditCurve[0] = center - (onSketchPos - center);
                EditCurve[1] = Base::Vector2d(EditCurve[0].x, onSketchPos.y);
                EditCurve[2] = onSketchPos;
                EditCurve[3] = Base::Vector2d(onSketchPos.x, EditCurve[0].y);
                EditCurve[4] = EditCurve[0];
                drawEdit(EditCurve);
                Mode = STATUS_End;
            }
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

            try {
                if (constructionMethod == Diagonal) {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch box"));
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "geoList = []\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "%s.addGeometry(geoList,%s)\n"
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "%s.addConstraint(conList)\n"
                        "del geoList, conList\n",
                        EditCurve[0].x,
                        EditCurve[0].y,
                        EditCurve[1].x,
                        EditCurve[1].y,  // line 1
                        EditCurve[1].x,
                        EditCurve[1].y,
                        EditCurve[2].x,
                        EditCurve[2].y,  // line 2
                        EditCurve[2].x,
                        EditCurve[2].y,
                        EditCurve[3].x,
                        EditCurve[3].y,  // line 3
                        EditCurve[3].x,
                        EditCurve[3].y,
                        EditCurve[0].x,
                        EditCurve[0].y,                                              // line 4
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),  // the sketch
                        geometryCreationMode == Construction
                            ? "True"
                            : "False",  // geometry as construction or not
                        firstCurve,
                        firstCurve + 1,  // coincident1
                        firstCurve + 1,
                        firstCurve + 2,  // coincident2
                        firstCurve + 2,
                        firstCurve + 3,  // coincident3
                        firstCurve + 3,
                        firstCurve,                                                   // coincident4
                        firstCurve,                                                   // horizontal1
                        firstCurve + 2,                                               // horizontal2
                        firstCurve + 1,                                               // vertical1
                        firstCurve + 3,                                               // vertical2
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str());  // the sketch

                    Gui::Command::commitCommand();
                }
                else if (constructionMethod == CenterAndCorner) {
                    Gui::Command::openCommand(
                        QT_TRANSLATE_NOOP("Command", "Add centered sketch box"));
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "geoList = []\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)))"
                        "\n"
                        "geoList.append(Part.Point(App.Vector(%f,%f,0)))\n"
                        "%s.addGeometry(geoList,%s)\n"
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Symmetric',%i,2,%i,1,%i,1))\n"
                        "%s.addConstraint(conList)\n"
                        "del geoList, conList\n",
                        EditCurve[0].x,
                        EditCurve[0].y,
                        EditCurve[1].x,
                        EditCurve[1].y,  // line 1
                        EditCurve[1].x,
                        EditCurve[1].y,
                        EditCurve[2].x,
                        EditCurve[2].y,  // line 2
                        EditCurve[2].x,
                        EditCurve[2].y,
                        EditCurve[3].x,
                        EditCurve[3].y,  // line 3
                        EditCurve[3].x,
                        EditCurve[3].y,
                        EditCurve[0].x,
                        EditCurve[0].y,  // line 4
                        center.x,
                        center.y,                                                    // center point
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),  // the sketch
                        geometryCreationMode == Construction
                            ? "True"
                            : "False",  // geometry as construction or not
                        firstCurve,
                        firstCurve + 1,  // coincident1
                        firstCurve + 1,
                        firstCurve + 2,  // coincident2
                        firstCurve + 2,
                        firstCurve + 3,  // coincident3
                        firstCurve + 3,
                        firstCurve,      // coincident4
                        firstCurve + 1,  // horizontal1
                        firstCurve + 3,  // horizontal2
                        firstCurve,      // vertical1
                        firstCurve + 2,  // vertical2
                        firstCurve + 1,
                        firstCurve,
                        firstCurve + 4,                                               // Symmetric
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str());  // the sketch

                    Gui::Command::commitCommand();
                }
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add box"));

                Gui::Command::abortCommand();
            }

            if (constructionMethod == Diagonal) {
                // add auto constraints at the start of the first side
                if (!sugConstr1.empty()) {
                    createAutoConstraints(sugConstr1,
                                          getHighestCurveIndex() - 3,
                                          Sketcher::PointPos::start);
                    sugConstr1.clear();
                }

                // add auto constraints at the end of the second side
                if (!sugConstr2.empty()) {
                    createAutoConstraints(sugConstr2,
                                          getHighestCurveIndex() - 2,
                                          Sketcher::PointPos::end);
                    sugConstr2.clear();
                }
            }
            else if (constructionMethod == CenterAndCorner) {
                // add auto constraints at the start of the first side
                if (!sugConstr1.empty()) {
                    createAutoConstraints(sugConstr1,
                                          getHighestCurveIndex(),
                                          Sketcher::PointPos::start);
                    sugConstr1.clear();
                }

                // add auto constraints at the end of the second side
                if (!sugConstr2.empty()) {
                    createAutoConstraints(sugConstr2,
                                          getHighestCurveIndex() - 3,
                                          Sketcher::PointPos::end);
                    sugConstr2.clear();
                }
            }

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(5);
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
        }
        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_Box");
    }

protected:
    BoxMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
    ConstructionMethod constructionMethod;
    Base::Vector2d center;
};

class DrawSketchHandlerOblong: public DrawSketchHandler
{
public:
    DrawSketchHandlerOblong()
        : Mode(STATUS_SEEK_First)
        , lengthX(0)
        , lengthY(0)
        , radius(0)
        , signX(1)
        , signY(1)
        , EditCurve(37)
    {}
    ~DrawSketchHandlerOblong() override
    {}
    /// mode table
    enum BoxMode
    {
        STATUS_SEEK_First,  /**< enum value ----. */
        STATUS_SEEK_Second, /**< enum value ----. */
        STATUS_End
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
            float distanceX = onSketchPos.x - StartPos.x;
            float distanceY = onSketchPos.y - StartPos.y;

            lengthX = distanceX;
            lengthY = distanceY;
            signX = Base::sgn(distanceX);
            signY = Base::sgn(distanceY);
            if (fabs(distanceX) > fabs(distanceY)) {
                radius = fabs(distanceY)
                    / 4;  // we use a fourth of the smaller distance as default radius
            }
            else {
                radius = fabs(distanceX) / 4;
            }

            // we draw the lines with 36 segments, 8 for each arc and 4 lines
            // draw the arcs
            for (int i = 0; i < 8; i++) {
                // calculate the x,y positions forming the arc
                double angle = i * M_PI / 16.0;
                double x_i = -radius * sin(angle);
                double y_i = -radius * cos(angle);
                // we are drawing clockwise starting with the arc that is besides StartPos
                if (signX == signY) {
                    EditCurve[i] = Base::Vector2d(StartPos.x + signX * (radius + x_i),
                                                  StartPos.y + signY * (radius + y_i));
                    EditCurve[9 + i] =
                        Base::Vector2d(StartPos.x + signY * (radius + y_i),
                                       StartPos.y + lengthY - signX * (radius + x_i));
                    EditCurve[18 + i] =
                        Base::Vector2d(StartPos.x + lengthX - signX * (radius + x_i),
                                       StartPos.y + lengthY - signY * (radius + y_i));
                    EditCurve[27 + i] =
                        Base::Vector2d(StartPos.x + lengthX - signY * (radius + y_i),
                                       StartPos.y + signX * (radius + x_i));
                }
                else {
                    EditCurve[i] = Base::Vector2d(StartPos.x - signY * (radius + y_i),
                                                  StartPos.y - signX * (radius + x_i));
                    EditCurve[9 + i] = Base::Vector2d(StartPos.x + lengthX - signX * (radius + x_i),
                                                      StartPos.y + signY * (radius + y_i));
                    EditCurve[18 + i] =
                        Base::Vector2d(StartPos.x + lengthX + signY * (radius + y_i),
                                       StartPos.y + lengthY + signX * (radius + x_i));
                    EditCurve[27 + i] =
                        Base::Vector2d(StartPos.x + signX * (radius + x_i),
                                       StartPos.y + lengthY - signY * (radius + y_i));
                }
            }
            // draw the lines
            if (signX == signY) {
                EditCurve[8] = Base::Vector2d(StartPos.x, StartPos.y + (signY * radius));
                EditCurve[17] = Base::Vector2d(StartPos.x + (signX * radius), StartPos.y + lengthY);
                EditCurve[26] =
                    Base::Vector2d(StartPos.x + lengthX, StartPos.y + lengthY - (signY * radius));
                EditCurve[35] = Base::Vector2d(StartPos.x + lengthX - (signX * radius), StartPos.y);
            }
            else {
                EditCurve[8] = Base::Vector2d(StartPos.x + (signX * radius), StartPos.y);
                EditCurve[17] = Base::Vector2d(StartPos.x + lengthX, StartPos.y + (signY * radius));
                EditCurve[26] =
                    Base::Vector2d(StartPos.x + lengthX - (signX * radius), StartPos.y + lengthY);
                EditCurve[35] = Base::Vector2d(StartPos.x, StartPos.y + lengthY - (signY * radius));
            }
            // close the curve
            EditCurve[36] = EditCurve[0];

            if (showCursorCoords()) {
                SbString text;
                std::string radiusString = lengthToDisplayFormat(radius, 1);
                std::string xString = lengthToDisplayFormat(lengthX, 1);
                std::string yString = lengthToDisplayFormat(lengthY, 1);
                text.sprintf("  (R%s X%s Y%s)",
                             radiusString.c_str(),
                             xString.c_str(),
                             yString.c_str());
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f, 0.f))) {
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
            EndPos = onSketchPos;
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
            // first determine the angles for the first arc
            double start = 0;
            double end = M_PI / 2;
            if (signX > 0 && signY > 0) {
                start = -2 * end;
                end = -1 * end;
            }
            else if (signX > 0 && signY < 0) {
                start = end;
                end = 2 * end;
            }
            else if (signX < 0 && signY > 0) {
                start = -1 * end;
                end = 0;
            }

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add rounded rectangle"));
                Gui::Command::doCommand(
                    Gui::Command::Doc,
                    // syntax for arcs: Part.ArcOfCircle(Part.Circle(center, axis, radius),
                    // startangle, endangle)
                    "geoList = []\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), "
                    "App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.LineSegment(App.Vector(%f, %f, 0), App.Vector(%f, %f, "
                    "0)))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), "
                    "App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.LineSegment(App.Vector(%f, %f, 0), App.Vector(%f, %f, "
                    "0)))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), "
                    "App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.LineSegment(App.Vector(%f, %f, 0), App.Vector(%f, %f, "
                    "0)))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), "
                    "App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.LineSegment(App.Vector(%f, %f, 0), App.Vector(%f, %f, "
                    "0)))\n"
                    "%s.addGeometry(geoList, %s)\n"
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 2))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 2))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 2))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 2))\n"
                    "conList.append(Sketcher.Constraint('Horizontal', %i))\n"
                    "conList.append(Sketcher.Constraint('Horizontal', %i))\n"
                    "conList.append(Sketcher.Constraint('Vertical', %i))\n"
                    "conList.append(Sketcher.Constraint('Vertical', %i))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "%s.addConstraint(conList)\n"
                    "del geoList, conList\n",
                    StartPos.x + (signX * radius),
                    StartPos.y + (signY * radius),  // center of the  arc 1
                    radius,
                    start,
                    end,  // start and end angle of arc1
                    EditCurve[8].x,
                    EditCurve[8].y,
                    EditCurve[9].x,
                    EditCurve[9].y,  // line 1
                    signX == signY
                        ? StartPos.x + (signX * radius)
                        : StartPos.x + lengthX - (signX * radius),  // center of the arc 2
                    signX == signY ? StartPos.y + lengthY - (signY * radius)
                                   : StartPos.y + (signY * radius),
                    radius,
                    // start and end angle of arc 2
                    // the logic is that end is start + M_PI / 2 and start is the previous end -
                    // M_PI
                    end - M_PI,
                    end - 0.5 * M_PI,
                    EditCurve[17].x,
                    EditCurve[17].y,
                    EditCurve[18].x,
                    EditCurve[18].y,  // line 2
                    StartPos.x + lengthX - (signX * radius),
                    StartPos.y + lengthY - (signY * radius),  // center of the arc 3
                    radius,
                    end - 1.5 * M_PI,
                    end - M_PI,
                    EditCurve[26].x,
                    EditCurve[26].y,
                    EditCurve[27].x,
                    EditCurve[27].y,  // line 3
                    signX == signY ? StartPos.x + lengthX - (signX * radius)
                                   : StartPos.x + (signX * radius),  // center of the arc 4
                    signX == signY ? StartPos.y + (signY * radius)
                                   : StartPos.y + lengthY - (signY * radius),
                    radius,
                    end - 2 * M_PI,
                    end - 1.5 * M_PI,
                    EditCurve[35].x,
                    EditCurve[35].y,
                    EditCurve[36].x,
                    EditCurve[36].y,                                             // line 4
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),  // the sketch
                    geometryCreationMode == Construction
                        ? "True"
                        : "False",  // geometry as construction or not
                    firstCurve,
                    firstCurve + 1,  // tangent 1
                    firstCurve + 1,
                    firstCurve + 2,  // tangent 2
                    firstCurve + 2,
                    firstCurve + 3,  // tangent 3
                    firstCurve + 3,
                    firstCurve + 4,  // tangent 4
                    firstCurve + 4,
                    firstCurve + 5,  // tangent 5
                    firstCurve + 5,
                    firstCurve + 6,  // tangent 6
                    firstCurve + 6,
                    firstCurve + 7,  // tangent 7
                    firstCurve + 7,
                    firstCurve,                                        // tangent 8
                    signX == signY ? firstCurve + 3 : firstCurve + 1,  // horizontal constraint
                    signX == signY ? firstCurve + 7 : firstCurve + 5,  // horizontal constraint
                    signX == signY ? firstCurve + 1 : firstCurve + 3,  // vertical constraint
                    signX == signY ? firstCurve + 5 : firstCurve + 7,  // vertical constraint
                    firstCurve,
                    firstCurve + 2,  // equal  1
                    firstCurve + 2,
                    firstCurve + 4,  // equal  2
                    firstCurve + 4,
                    firstCurve + 6,                                               // equal  3
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str());  // the sketch

                // not all users want these extra points, some power users find them unnecessary
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Sketcher");
                auto showExtraPoints = hGrp->GetBool("RoundRectangleSuggConstraints", true);
                if (showExtraPoints) {
                    // now add construction geometry - two points used to take suggested constraints
                    Gui::Command::doCommand(
                        Gui::Command::Doc,
                        "geoList = []\n"
                        "geoList.append(Part.Point(App.Vector(%f, %f, 0)))\n"
                        "geoList.append(Part.Point(App.Vector(%f, %f, 0)))\n"
                        "%s.addGeometry(geoList, True)\n"  // geometry as construction
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "%s.addConstraint(conList)\n"
                        "del geoList, conList\n",
                        StartPos.x,
                        StartPos.y,  // point at StartPos
                        EndPos.x,
                        EndPos.y,  // point at EndPos
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),  // the sketch
                        firstCurve + 8,
                        firstCurve + 1,  // point on object constraint
                        firstCurve + 8,
                        firstCurve + 7,  // point on object constraint
                        firstCurve + 9,
                        firstCurve + 3,  // point on object constraint
                        firstCurve + 9,
                        firstCurve + 5,  // point on object constraint
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str());  // the sketch
                }

                Gui::Command::commitCommand();

                // add auto constraints at the StartPos auxiliary point
                if (!sugConstr1.empty()) {
                    createAutoConstraints(sugConstr1,
                                          getHighestCurveIndex() - 1,
                                          Sketcher::PointPos::start);
                    sugConstr1.clear();
                }

                // add auto constraints at the EndPos auxiliary point
                if (!sugConstr2.empty()) {
                    createAutoConstraints(sugConstr2,
                                          getHighestCurveIndex(),
                                          Sketcher::PointPos::start);
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(
                    sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Failed to add rounded rectangle"));

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
                EditCurve.resize(37);
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
        }
        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Oblong");
    }

protected:
    BoxMode Mode;
    Base::Vector2d StartPos, EndPos;
    double lengthX, lengthY, radius;
    float signX, signY;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1, sugConstr2;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerRectangle_H
