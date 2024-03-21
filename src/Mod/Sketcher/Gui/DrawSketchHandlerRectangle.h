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

#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerRectangle;

namespace ConstructionMethods
{

enum class RectangleConstructionMethod
{
    Diagonal,
    CenterAndCorner,
    ThreePoints,
    CenterAnd3Points,
    End  // Must be the last one
};

}

using DSHRectangleController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerRectangle,
    StateMachines::FiveSeekEnd,
    /*PAutoConstraintSize =*/3,
    /*OnViewParametersT =*/OnViewParameters<6, 6, 8, 8>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<0, 0, 0, 0>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2, 2, 2>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1, 1, 1>,  // NOLINT
    ConstructionMethods::RectangleConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHRectangleControllerBase = DSHRectangleController::ControllerBase;

using DrawSketchHandlerRectangleBase = DrawSketchControllableHandler<DSHRectangleController>;


class DrawSketchHandlerRectangle: public DrawSketchHandlerRectangleBase
{
    // Allow specialisations of controllers access to private members
    friend DSHRectangleController;
    friend DSHRectangleControllerBase;

public:
    DrawSketchHandlerRectangle(ConstructionMethod constrMethod = ConstructionMethod::Diagonal,
                               bool roundcorners = false,
                               bool frame = false)
        : DrawSketchHandlerRectangleBase(constrMethod)
        , roundCorners(roundcorners)
        , makeFrame(frame)
        , cornersReversed(false)
        , radius(0.0)
        , length(0.0)
        , width(0.0)
        , thickness(0.)
        , radiusFrame(0.0)
        , angle(0.0)
        , angle123(0.0)
        , angle412(0.0)
        , firstCurve(Sketcher::GeoEnum::GeoUndef)
        , constructionPointOneId(Sketcher::GeoEnum::GeoUndef)
        , constructionPointTwoId(Sketcher::GeoEnum::GeoUndef)
        , constructionPointThreeId(Sketcher::GeoEnum::GeoUndef)
        , centerPointId(Sketcher::GeoEnum::GeoUndef)
        , side(0)
    {}

    ~DrawSketchHandlerRectangle() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                if (constructionMethod() == ConstructionMethod::Diagonal
                    || constructionMethod() == ConstructionMethod::ThreePoints) {
                    corner1 = onSketchPos;
                }
                else {  //(constructionMethod == ConstructionMethod::CenterAndCorner)
                    center = onSketchPos;
                }

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            } break;
            case SelectMode::SeekSecond: {
                if (constructionMethod() == ConstructionMethod::Diagonal) {
                    toolWidgetManager.drawDirectionAtCursor(onSketchPos, corner1);

                    // Note : we swap corner2 and 4 to make sure the corners are CCW.
                    // making things easier down the line.
                    corner3 = onSketchPos;
                    if ((corner3.x - corner1.x) * (corner3.y - corner1.y) > 0) {
                        corner2 = Base::Vector2d(onSketchPos.x, corner1.y);
                        corner4 = Base::Vector2d(corner1.x, onSketchPos.y);
                        cornersReversed = false;
                    }
                    else {
                        corner4 = Base::Vector2d(onSketchPos.x, corner1.y);
                        corner2 = Base::Vector2d(corner1.x, onSketchPos.y);
                        cornersReversed = true;
                    }
                    angle123 = M_PI / 2;
                    angle412 = M_PI / 2;
                }
                else if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                    toolWidgetManager.drawDirectionAtCursor(onSketchPos, center);

                    corner1 = center - (onSketchPos - center);
                    corner3 = onSketchPos;
                    if (Base::sgn(corner3.x - corner1.x) * Base::sgn(corner3.y - corner1.y) > 0) {
                        corner2 = Base::Vector2d(onSketchPos.x, corner1.y);
                        corner4 = Base::Vector2d(corner1.x, onSketchPos.y);
                        cornersReversed = false;
                    }
                    else {
                        corner4 = Base::Vector2d(onSketchPos.x, corner1.y);
                        corner2 = Base::Vector2d(corner1.x, onSketchPos.y);
                        cornersReversed = true;
                    }
                    angle123 = M_PI / 2;
                    angle412 = M_PI / 2;
                }
                else if (constructionMethod() == ConstructionMethod::ThreePoints) {
                    toolWidgetManager.drawDirectionAtCursor(onSketchPos, corner1);

                    corner2 = onSketchPos;
                    Base::Vector2d perpendicular;
                    perpendicular.x = -(corner2 - corner1).y;
                    perpendicular.y = (corner2 - corner1).x;
                    corner3 = corner2 + perpendicular;
                    corner4 = corner1 + perpendicular;
                    angle123 = M_PI / 2;
                    angle412 = M_PI / 2;
                    corner2Initial = corner2;
                    side = getPointSideOfVector(corner3, corner2 - corner1, corner1);
                }
                else {
                    toolWidgetManager.drawDirectionAtCursor(onSketchPos, center);

                    corner1 = onSketchPos;
                    corner3 = center - (onSketchPos - center);
                    Base::Vector2d perpendicular;
                    perpendicular.x = -(onSketchPos - center).y;
                    perpendicular.y = (onSketchPos - center).x;
                    corner2 = center + perpendicular;
                    corner4 = center - perpendicular;
                    angle123 = M_PI / 2;
                    angle412 = M_PI / 2;
                    side = getPointSideOfVector(corner2, corner3 - corner1, corner1);
                }

                if (roundCorners) {
                    length = (corner2 - corner1).Length();
                    width = (corner4 - corner1).Length();
                    radius = std::min(length, width) / 6;  // NOLINT
                }
                else {
                    radius = 0.;
                }

                try {
                    CreateAndDrawShapeGeometry();

                    toolWidgetManager.drawWidthHeightAtCursor(onSketchPos, length, width);
                }
                catch (const Base::ValueError&) {
                }  // equal points while hovering raise an objection that can be safely ignored

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.0, 0.0))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            } break;
            case SelectMode::SeekThird: {
                if (constructionMethod() == ConstructionMethod::Diagonal
                    || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                    if (roundCorners) {
                        calculateRadius(onSketchPos);
                        toolWidgetManager.drawDoubleAtCursor(onSketchPos, radius);
                    }
                    else {  // Normal rectangle with frame.
                        calculateThickness(onSketchPos);
                        toolWidgetManager.drawDoubleAtCursor(onSketchPos, thickness);
                    }
                }
                else if (constructionMethod() == ConstructionMethod::ThreePoints) {
                    corner2 = corner2Initial;
                    corner3 = onSketchPos;
                    if (side == getPointSideOfVector(corner3, corner2 - corner1, corner1)) {
                        corner4 = corner1 + (corner3 - corner2);
                        cornersReversed = false;
                    }
                    else {
                        corner4 = corner2;
                        corner2 = corner1 + (corner3 - corner4);
                        cornersReversed = true;
                    }
                    Base::Vector2d a = corner1 - corner2;
                    Base::Vector2d b = corner3 - corner2;
                    if (fabs((sqrt(a.x * a.x + a.y * a.y) * sqrt(b.x * b.x + b.y * b.y)))
                        > Precision::Confusion()) {
                        angle123 =
                            acos((a.x * b.x + a.y * b.y)
                                 / (sqrt(a.x * a.x + a.y * a.y) * sqrt(b.x * b.x + b.y * b.y)));
                    }
                    angle412 = M_PI - angle123;
                    if (roundCorners) {
                        radius = std::min(length, width) / 6  // NOLINT
                            * std::min(sqrt(1 - cos(angle412) * cos(angle412)),
                                       sqrt(1 - cos(angle123) * cos(angle123)));
                    }
                    else {
                        radius = 0.;
                    }

                    toolWidgetManager.drawWidthHeightAtCursor(onSketchPos, length, width);
                }
                else {
                    corner2 = onSketchPos;
                    corner4 = center - (onSketchPos - center);
                    cornersReversed = false;
                    if (side != getPointSideOfVector(corner2, corner3 - corner1, corner1)) {
                        corner4 = onSketchPos;
                        corner2 = center - (onSketchPos - center);
                        cornersReversed = true;
                    }
                    Base::Vector2d a = corner4 - corner1;
                    Base::Vector2d b = corner2 - corner1;
                    if (fabs((sqrt(a.x * a.x + a.y * a.y) * sqrt(b.x * b.x + b.y * b.y)))
                        > Precision::Confusion()) {
                        angle412 =
                            acos((a.x * b.x + a.y * b.y)
                                 / (sqrt(a.x * a.x + a.y * a.y) * sqrt(b.x * b.x + b.y * b.y)));
                    }
                    angle123 = M_PI - angle412;
                    if (roundCorners) {
                        radius = std::min(length, width) / 6  // NOLINT
                            * std::min(sqrt(1 - cos(angle412) * cos(angle412)),
                                       sqrt(1 - cos(angle123) * cos(angle123)));
                    }
                    else {
                        radius = 0.;
                    }

                    toolWidgetManager.drawWidthHeightAtCursor(onSketchPos, length, width);
                }

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch (const Base::ValueError&) {
                }  // equal points while hovering raise an objection that can be safely ignored

                if ((constructionMethod() == ConstructionMethod::ThreePoints
                     || constructionMethod() == ConstructionMethod::CenterAnd3Points)
                    && seekAutoConstraint(sugConstraints[2],
                                          onSketchPos,
                                          Base::Vector2d(0.0, 0.0))) {
                    renderSuggestConstraintsCursor(sugConstraints[2]);
                    return;
                }
            } break;
            case SelectMode::SeekFourth: {
                if (constructionMethod() == ConstructionMethod::Diagonal
                    || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                    calculateThickness(onSketchPos);
                    toolWidgetManager.drawDoubleAtCursor(onSketchPos, thickness);
                }
                else {
                    if (roundCorners) {
                        calculateRadius(onSketchPos);
                        toolWidgetManager.drawDoubleAtCursor(onSketchPos, radius);
                    }
                    else {
                        calculateThickness(onSketchPos);
                        toolWidgetManager.drawDoubleAtCursor(onSketchPos, thickness);
                    }
                }

                CreateAndDrawShapeGeometry();
            } break;
            case SelectMode::SeekFifth: {
                calculateThickness(onSketchPos);
                toolWidgetManager.drawDoubleAtCursor(onSketchPos, thickness);

                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            firstCurve = getHighestCurveIndex() + 1;

            createShape(false);

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch box"));

            commandAddShapeGeometryAndConstraints();

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add box"));

            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError,
                   QT_TRANSLATE_NOOP(
                       "Notifications",
                       "Tool execution aborted") "\n")  // This prevents constraints from being
                                                        // applied on non existing geometry
        }

        thickness = 0.;
    }

    void generateAutoConstraints() override
    {

        if (constructionMethod() == ConstructionMethod::Diagonal) {
            // add auto constraints at the start of the first side
            if (radius > Precision::Confusion()) {
                if (!sugConstraints[0].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[0],
                                                     constructionPointOneId,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[1].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     constructionPointTwoId,
                                                     Sketcher::PointPos::start);
                }
            }
            else {
                if (!sugConstraints[0].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[0],
                                                     firstCurve,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[1].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     firstCurve + 1,
                                                     Sketcher::PointPos::end);
                }
            }
        }
        else if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
            // add auto constraints at center
            if (!sugConstraints[0].empty()) {
                generateAutoConstraintsOnElement(sugConstraints[0],
                                                 centerPointId,
                                                 Sketcher::PointPos::start);
            }

            // add auto constraints for the line segment end
            if (!sugConstraints[1].empty()) {
                if (radius > Precision::Confusion()) {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     constructionPointOneId,
                                                     Sketcher::PointPos::start);
                }
                else {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     firstCurve + 1,
                                                     Sketcher::PointPos::end);
                }
            }
        }
        else if (constructionMethod() == ConstructionMethod::ThreePoints) {
            if (radius > Precision::Confusion()) {
                if (!sugConstraints[0].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[0],
                                                     constructionPointOneId,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[1].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     constructionPointTwoId,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[2].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[2],
                                                     constructionPointThreeId,
                                                     Sketcher::PointPos::start);
                }
            }
            else {
                if (!sugConstraints[0].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[0],
                                                     firstCurve,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[1].empty()) {
                    if (!cornersReversed) {
                        generateAutoConstraintsOnElement(sugConstraints[1],
                                                         firstCurve + 1,
                                                         Sketcher::PointPos::start);
                    }
                    else {
                        generateAutoConstraintsOnElement(sugConstraints[1],
                                                         firstCurve + 3,
                                                         Sketcher::PointPos::start);
                    }
                }

                if (!sugConstraints[2].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[2],
                                                     firstCurve + 2,
                                                     Sketcher::PointPos::start);
                }
            }
        }
        else if (constructionMethod() == ConstructionMethod::CenterAnd3Points) {
            // add auto constraints at center
            if (!sugConstraints[0].empty()) {
                generateAutoConstraintsOnElement(sugConstraints[0],
                                                 centerPointId,
                                                 Sketcher::PointPos::start);
            }

            // add auto constraints for the line segment end
            if (radius > Precision::Confusion()) {
                if (!sugConstraints[1].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     constructionPointOneId,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[2].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[2],
                                                     constructionPointTwoId,
                                                     Sketcher::PointPos::start);
                }
            }
            else {
                if (!sugConstraints[1].empty()) {
                    generateAutoConstraintsOnElement(sugConstraints[1],
                                                     firstCurve,
                                                     Sketcher::PointPos::start);
                }

                if (!sugConstraints[2].empty()) {
                    if (!cornersReversed) {
                        generateAutoConstraintsOnElement(sugConstraints[2],
                                                         firstCurve + 1,
                                                         Sketcher::PointPos::start);
                    }
                    else {
                        generateAutoConstraintsOnElement(sugConstraints[2],
                                                         firstCurve + 3,
                                                         Sketcher::PointPos::start);
                    }
                }
            }
        }

        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry
        // parameters are accurate This is particularly important for adding widget mandated
        // constraints.
        removeRedundantAutoConstraints();
    }

    void createAutoConstraints() override
    {
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
    }

    std::string getToolName() const override
    {
        return "DSH_Rectangle";
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (!roundCorners && !makeFrame) {
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QString::fromLatin1("Sketcher_Pointer_Create_Box_Center");
            }
            else if (constructionMethod() == ConstructionMethod::ThreePoints) {
                return QString::fromLatin1("Sketcher_Pointer_Create_Box_3Points");
            }
            else if (constructionMethod() == ConstructionMethod::CenterAnd3Points) {
                return QString::fromLatin1("Sketcher_Pointer_Create_Box_3Points_Center");
            }
            else {
                return QString::fromLatin1("Sketcher_Pointer_Create_Box");
            }
        }
        else if (roundCorners && !makeFrame) {
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QString::fromLatin1("Sketcher_Pointer_Oblong_Center");
            }
            else {
                return QString::fromLatin1("Sketcher_Pointer_Oblong");
            }
        }
        else if (!roundCorners && makeFrame) {
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QString::fromLatin1("Sketcher_Pointer_Create_Frame_Center");
            }
            else {
                return QString::fromLatin1("Sketcher_Pointer_Create_Frame");
            }
        }
        else {  // both roundCorners and makeFrame
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QString::fromLatin1("Sketcher_Pointer_Oblong_Frame_Center");
            }
            else {
                return QString::fromLatin1("Sketcher_Pointer_Oblong_Frame");
            }
        }
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_CreateRectangle");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Rectangle parameters"));
    }

    void angleSnappingControl() override
    {
        if ((constructionMethod() == ConstructionMethod::ThreePoints)
            && state() == SelectMode::SeekSecond) {
            setAngleSnapping(true, corner1);
        }
        else if ((constructionMethod() == ConstructionMethod::CenterAnd3Points)
                 && state() == SelectMode::SeekSecond) {
            setAngleSnapping(true, center);
        }
        else if ((constructionMethod() == ConstructionMethod::ThreePoints)
                 && state() == SelectMode::SeekThird) {
            setAngleSnapping(true, cornersReversed ? corner4 : corner2);
        }
        else if ((constructionMethod() == ConstructionMethod::CenterAnd3Points)
                 && state() == SelectMode::SeekThird) {
            setAngleSnapping(true, corner1);
        }

        else {
            setAngleSnapping(false);
        }
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond
            && (length < Precision::Confusion() || width < Precision::Confusion())) {
            return false;
        }

        return true;
    }

    // reimplement because if not radius then it's 2 steps
    void onButtonPressed(Base::Vector2d onSketchPos) override
    {
        this->updateDataAndDrawToPosition(onSketchPos);

        if (canGoToNextMode()) {
            if (constructionMethod() == ConstructionMethod::Diagonal
                || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (state() == SelectMode::SeekSecond && !roundCorners && !makeFrame) {
                    setState(SelectMode::End);
                }
                else if ((state() == SelectMode::SeekThird && roundCorners && !makeFrame)
                         || (state() == SelectMode::SeekThird && !roundCorners && makeFrame)) {
                    setState(SelectMode::End);
                }
                else if (state() == SelectMode::SeekFourth) {
                    setState(SelectMode::End);
                }
                else {
                    this->moveToNextMode();
                }
            }
            else {
                if (state() == SelectMode::SeekThird && !roundCorners && !makeFrame) {
                    setState(SelectMode::End);
                }
                else if ((state() == SelectMode::SeekFourth && roundCorners && !makeFrame)
                         || (state() == SelectMode::SeekFourth && !roundCorners && makeFrame)) {
                    setState(SelectMode::End);
                }
                else {
                    this->moveToNextMode();
                }
            }
        }
    }

private:
    Base::Vector2d center, corner1, corner2, corner3, corner4, frameCorner1, frameCorner2,
        frameCorner3, frameCorner4, corner2Initial;
    Base::Vector3d center1, center2, center3, center4;
    bool roundCorners, makeFrame, cornersReversed;
    double radius, length, width, thickness, radiusFrame, angle, angle123, angle412;
    int firstCurve, constructionPointOneId, constructionPointTwoId, constructionPointThreeId,
        centerPointId, side;

    void createShape(bool onlyeditoutline) override
    {

        ShapeGeometry.clear();

        Base::Vector2d vecL = corner2 - corner1;
        Base::Vector2d vecW = corner4 - corner1;
        length = vecL.Length();
        width = vecW.Length();
        angle = vecL.Angle();
        if (length > Precision::Confusion() && width > Precision::Confusion()
            && fmod(fabs(angle123), M_PI) > Precision::Confusion()) {
            vecL = vecL / length;
            vecW = vecW / width;
            double end = angle - M_PI / 2;
            double L1 = radius;
            double L2 = radius;
            if (cos(angle123 / 2) != 1 && cos(angle412 / 2) != 1) {
                L1 = radius / sqrt(1 - cos(angle123 / 2) * cos(angle123 / 2));
                L2 = radius / sqrt(1 - cos(angle412 / 2) * cos(angle412 / 2));
            }

            addLineToShapeGeometry(toVector3d(corner1 + vecL * L2 * cos(angle412 / 2)),
                                   toVector3d(corner2 - vecL * L1 * cos(angle123 / 2)),
                                   isConstructionMode());
            addLineToShapeGeometry(toVector3d(corner2 + vecW * L1 * cos(angle123 / 2)),
                                   toVector3d(corner3 - vecW * L2 * cos(angle412 / 2)),
                                   isConstructionMode());
            addLineToShapeGeometry(toVector3d(corner3 - vecL * L2 * cos(angle412 / 2)),
                                   toVector3d(corner4 + vecL * L1 * cos(angle123 / 2)),
                                   isConstructionMode());
            addLineToShapeGeometry(toVector3d(corner4 - vecW * L1 * cos(angle123 / 2)),
                                   toVector3d(corner1 + vecW * L2 * cos(angle412 / 2)),
                                   isConstructionMode());

            if (roundCorners && radius > Precision::Confusion()) {
                // center points required later for special case of round corner frame with
                // radiusFrame = 0.
                Base::Vector2d b1 = (vecL + vecW) / (vecL + vecW).Length();
                Base::Vector2d b2 = (vecL - vecW) / (vecL - vecW).Length();
                center1 = toVector3d(corner1 + b1 * L2);
                center2 = toVector3d(corner2 - b2 * L1);
                center3 = toVector3d(corner3 - b1 * L2);
                center4 = toVector3d(corner4 + b2 * L1);

                addArcToShapeGeometry(center1,
                                      end - M_PI + angle412,
                                      end,
                                      radius,
                                      isConstructionMode());
                addArcToShapeGeometry(center2,
                                      end,
                                      end - M_PI - angle123,
                                      radius,
                                      isConstructionMode());
                addArcToShapeGeometry(center3,
                                      end + angle412,
                                      end - M_PI,
                                      radius,
                                      isConstructionMode());
                addArcToShapeGeometry(center4,
                                      end - M_PI,
                                      end - angle123,
                                      radius,
                                      isConstructionMode());
            }

            if (makeFrame && state() != SelectMode::SeekSecond
                && fabs(thickness) > Precision::Confusion()) {
                if (radius < Precision::Confusion()) {
                    radiusFrame = 0.;
                }
                else {
                    radiusFrame = radius + thickness;
                    if (radiusFrame < 0.) {
                        radiusFrame = 0.;
                    }
                }

                Base::Vector2d vecLF = frameCorner2 - frameCorner1;
                Base::Vector2d vecWF = frameCorner4 - frameCorner1;
                double lengthF = vecLF.Length();
                double widthF = vecWF.Length();

                double L1F = 0.;
                double L2F = 0.;
                if (radius > Precision::Confusion()) {
                    L1F = L1 * radiusFrame / radius;
                    L2F = L2 * radiusFrame / radius;
                }

                addLineToShapeGeometry(
                    toVector3d(frameCorner1 + vecLF / lengthF * L2F * cos(angle412 / 2)),
                    toVector3d(frameCorner2 - vecLF / lengthF * L1F * cos(angle123 / 2)),
                    isConstructionMode());
                addLineToShapeGeometry(
                    toVector3d(frameCorner2 + vecWF / widthF * L1F * cos(angle123 / 2)),
                    toVector3d(frameCorner3 - vecWF / widthF * L2F * cos(angle412 / 2)),
                    isConstructionMode());
                addLineToShapeGeometry(
                    toVector3d(frameCorner3 - vecLF / lengthF * L2F * cos(angle412 / 2)),
                    toVector3d(frameCorner4 + vecLF / lengthF * L1F * cos(angle123 / 2)),
                    isConstructionMode());
                addLineToShapeGeometry(
                    toVector3d(frameCorner4 - vecWF / widthF * L1F * cos(angle123 / 2)),
                    toVector3d(frameCorner1 + vecWF / widthF * L2F * cos(angle412 / 2)),
                    isConstructionMode());

                if (roundCorners && radiusFrame > Precision::Confusion()) {
                    Base::Vector2d b1 = (vecL + vecW) / (vecL + vecW).Length();
                    Base::Vector2d b2 = (vecL - vecW) / (vecL - vecW).Length();

                    addArcToShapeGeometry(toVector3d(frameCorner1 + b1 * L2F),
                                          end - M_PI + angle412,
                                          end,
                                          radiusFrame,
                                          isConstructionMode());
                    addArcToShapeGeometry(toVector3d(frameCorner2 - b2 * L1F),
                                          end,
                                          end - M_PI - angle123,
                                          radiusFrame,
                                          isConstructionMode());
                    addArcToShapeGeometry(toVector3d(frameCorner3 - b1 * L2F),
                                          end + angle412,
                                          end - M_PI,
                                          radiusFrame,
                                          isConstructionMode());
                    addArcToShapeGeometry(toVector3d(frameCorner4 + b2 * L1F),
                                          end - M_PI,
                                          end - angle123,
                                          radiusFrame,
                                          isConstructionMode());
                }
            }

            if (!onlyeditoutline) {
                ShapeConstraints.clear();
                Sketcher::ConstraintType typeA = Sketcher::Horizontal;
                Sketcher::ConstraintType typeB = Sketcher::Vertical;
                if (Base::sgn(corner3.x - corner1.x) * Base::sgn(corner3.y - corner1.y) < 0) {
                    typeA = Sketcher::Vertical;
                    typeB = Sketcher::Horizontal;
                }

                if (radius > Precision::Confusion()) {

                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve,
                                          Sketcher::PointPos::start,
                                          firstCurve + 4,  // NOLINT
                                          Sketcher::PointPos::end);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve,
                                          Sketcher::PointPos::end,
                                          firstCurve + 5,  // NOLINT
                                          Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve + 1,  // NOLINT
                                          Sketcher::PointPos::start,
                                          firstCurve + 5,  // NOLINT
                                          Sketcher::PointPos::end);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve + 1,  // NOLINT
                                          Sketcher::PointPos::end,
                                          firstCurve + 6,  // NOLINT
                                          Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve + 2,  // NOLINT
                                          Sketcher::PointPos::start,
                                          firstCurve + 6,  // NOLINT
                                          Sketcher::PointPos::end);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve + 2,  // NOLINT
                                          Sketcher::PointPos::end,
                                          firstCurve + 7,  // NOLINT
                                          Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve + 3,  // NOLINT
                                          Sketcher::PointPos::start,
                                          firstCurve + 7,  // NOLINT
                                          Sketcher::PointPos::end);
                    addToShapeConstraints(Sketcher::Tangent,
                                          firstCurve + 3,  // NOLINT
                                          Sketcher::PointPos::end,
                                          firstCurve + 4,  // NOLINT
                                          Sketcher::PointPos::start);

                    if (fabs(angle) < Precision::Confusion()
                        || constructionMethod() == ConstructionMethod::Diagonal
                        || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                        addToShapeConstraints(typeA, firstCurve);
                        addToShapeConstraints(typeA, firstCurve + 2);
                        addToShapeConstraints(typeB, firstCurve + 1);
                        addToShapeConstraints(typeB, firstCurve + 3);
                    }
                    else {
                        addToShapeConstraints(Sketcher::Parallel,
                                              firstCurve,
                                              Sketcher::PointPos::none,
                                              firstCurve + 2);
                        addToShapeConstraints(Sketcher::Parallel,
                                              firstCurve + 1,
                                              Sketcher::PointPos::none,
                                              firstCurve + 3);
                        if (fabs(angle123 - M_PI / 2) < Precision::Confusion()) {
                            addToShapeConstraints(Sketcher::Perpendicular,
                                                  firstCurve,
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 1);
                        }
                    }
                    addToShapeConstraints(Sketcher::Equal,
                                          firstCurve + 4,  // NOLINT
                                          Sketcher::PointPos::none,
                                          firstCurve + 5);  // NOLINT
                    addToShapeConstraints(Sketcher::Equal,
                                          firstCurve + 5,  // NOLINT
                                          Sketcher::PointPos::none,
                                          firstCurve + 6);  // NOLINT
                    addToShapeConstraints(Sketcher::Equal,
                                          firstCurve + 6,  // NOLINT
                                          Sketcher::PointPos::none,
                                          firstCurve + 7);  // NOLINT

                    if (fabs(thickness) > Precision::Confusion()) {
                        if (radiusFrame
                            < Precision::Confusion()) {  // case inner rectangle is normal rectangle

                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 8,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 9,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 9,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 10,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 10,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 11,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 11,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 8,  // NOLINT
                                                  Sketcher::PointPos::start);

                            if (fabs(angle) < Precision::Confusion()
                                || constructionMethod() == ConstructionMethod::Diagonal
                                || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                                addToShapeConstraints(typeA, firstCurve + 8);   // NOLINT
                                addToShapeConstraints(typeA, firstCurve + 10);  // NOLINT
                                addToShapeConstraints(typeB, firstCurve + 9);   // NOLINT
                                addToShapeConstraints(typeB, firstCurve + 11);  // NOLINT
                            }
                            else {
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 8,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve + 10);  // NOLINT
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 9,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve + 11);  // NOLINT
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 8,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve);
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 9,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve + 1);  // NOLINT
                            }

                            // add construction lines +12, +13, +14, +15
                            addLineToShapeGeometry(
                                center1,
                                Base::Vector3d(frameCorner1.x, frameCorner1.y, 0.),
                                true);
                            addLineToShapeGeometry(
                                center2,
                                Base::Vector3d(frameCorner2.x, frameCorner2.y, 0.),
                                true);
                            addLineToShapeGeometry(
                                center3,
                                Base::Vector3d(frameCorner3.x, frameCorner3.y, 0.),
                                true);
                            addLineToShapeGeometry(
                                center4,
                                Base::Vector3d(frameCorner4.x, frameCorner4.y, 0.),
                                true);

                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 12,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 4,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 12,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 8,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 13,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 5,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 13,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 9,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 14,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 6,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 14,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 10,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 15,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 7,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 15,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 11,  // NOLINT
                                                  Sketcher::PointPos::start);

                            addToShapeConstraints(Sketcher::Perpendicular,
                                                  firstCurve + 12,  // NOLINT
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 13);  // NOLINT
                            addToShapeConstraints(Sketcher::Perpendicular,
                                                  firstCurve + 13,  // NOLINT
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 14);  // NOLINT
                            addToShapeConstraints(Sketcher::Perpendicular,
                                                  firstCurve + 14,  // NOLINT
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 15);  // NOLINT
                        }
                        else {  // case inner rectangle is rounded rectangle
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 8,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 12,  // NOLINT
                                                  Sketcher::PointPos::end);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 8,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 13,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 9,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 13,  // NOLINT
                                                  Sketcher::PointPos::end);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 9,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 14,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 10,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 14,  // NOLINT
                                                  Sketcher::PointPos::end);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 10,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 15,  // NOLINT
                                                  Sketcher::PointPos::start);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 11,  // NOLINT
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 15,  // NOLINT
                                                  Sketcher::PointPos::end);
                            addToShapeConstraints(Sketcher::Tangent,
                                                  firstCurve + 11,  // NOLINT
                                                  Sketcher::PointPos::end,
                                                  firstCurve + 12,  // NOLINT
                                                  Sketcher::PointPos::start);

                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 4,  // NOLINT
                                                  Sketcher::PointPos::mid,
                                                  firstCurve + 12,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 5,  // NOLINT
                                                  Sketcher::PointPos::mid,
                                                  firstCurve + 13,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 6,  // NOLINT
                                                  Sketcher::PointPos::mid,
                                                  firstCurve + 14,  // NOLINT
                                                  Sketcher::PointPos::mid);
                            addToShapeConstraints(Sketcher::Coincident,
                                                  firstCurve + 7,  // NOLINT
                                                  Sketcher::PointPos::mid,
                                                  firstCurve + 15,  // NOLINT
                                                  Sketcher::PointPos::mid);

                            if (fabs(angle) < Precision::Confusion()
                                || constructionMethod() == ConstructionMethod::Diagonal
                                || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                                addToShapeConstraints(typeA, firstCurve + 8);   // NOLINT
                                addToShapeConstraints(typeA, firstCurve + 10);  // NOLINT
                                addToShapeConstraints(typeB, firstCurve + 9);   // NOLINT
                            }
                            else {
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 8,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve + 10);  // NOLINT
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 9,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve + 11);  // NOLINT
                                addToShapeConstraints(Sketcher::Parallel,
                                                      firstCurve + 8,  // NOLINT
                                                      Sketcher::PointPos::none,
                                                      firstCurve);
                            }
                        }
                    }

                    if (constructionMethod() == ConstructionMethod::ThreePoints) {
                        if (fabs(thickness) > Precision::Confusion()) {
                            constructionPointOneId = firstCurve + 16;    // NOLINT
                            constructionPointTwoId = firstCurve + 17;    // NOLINT
                            constructionPointThreeId = firstCurve + 18;  // NOLINT
                        }
                        else {
                            constructionPointOneId = firstCurve + 8;     // NOLINT
                            constructionPointTwoId = firstCurve + 9;     // NOLINT
                            constructionPointThreeId = firstCurve + 10;  // NOLINT
                        }

                        addPointToShapeGeometry(Base::Vector3d(corner1.x, corner1.y, 0.), true);
                        if (!cornersReversed) {
                            addPointToShapeGeometry(Base::Vector3d(corner2.x, corner2.y, 0.), true);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 1);
                        }
                        else {
                            addPointToShapeGeometry(Base::Vector3d(corner4.x, corner4.y, 0.), true);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 2);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 3);
                        }
                        addPointToShapeGeometry(Base::Vector3d(corner3.x, corner3.y, 0.), true);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 3);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointThreeId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 1);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointThreeId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 2);
                    }
                    else if (constructionMethod() == ConstructionMethod::CenterAnd3Points) {
                        if (fabs(thickness) > Precision::Confusion()) {
                            constructionPointOneId = firstCurve + 16;  // NOLINT
                            constructionPointTwoId = firstCurve + 17;  // NOLINT
                            centerPointId = firstCurve + 18;           // NOLINT
                        }
                        else {
                            constructionPointOneId = firstCurve + 8;  // NOLINT
                            constructionPointTwoId = firstCurve + 9;  // NOLINT
                            centerPointId = firstCurve + 10;          // NOLINT
                        }

                        addPointToShapeGeometry(Base::Vector3d(corner1.x, corner1.y, 0.), true);
                        if (!cornersReversed) {
                            addPointToShapeGeometry(Base::Vector3d(corner2.x, corner2.y, 0.), true);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 1);
                        }
                        else {
                            addPointToShapeGeometry(Base::Vector3d(corner4.x, corner4.y, 0.), true);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 2);
                            addToShapeConstraints(Sketcher::PointOnObject,
                                                  constructionPointTwoId,
                                                  Sketcher::PointPos::start,
                                                  firstCurve + 3);
                        }
                        addPointToShapeGeometry(Base::Vector3d(center.x, center.y, 0.), true);
                        addToShapeConstraints(Sketcher::Symmetric,
                                              firstCurve + 2,
                                              Sketcher::PointPos::start,
                                              firstCurve,
                                              Sketcher::PointPos::start,
                                              centerPointId,
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 3);  // NOLINT
                    }
                    else if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                        if (fabs(thickness) > Precision::Confusion()) {
                            constructionPointOneId = firstCurve + 16;  // NOLINT
                            centerPointId = firstCurve + 17;           // NOLINT
                        }
                        else {
                            constructionPointOneId = firstCurve + 8;  // NOLINT
                            centerPointId = firstCurve + 9;           // NOLINT
                        }

                        addPointToShapeGeometry(Base::Vector3d(corner3.x, corner3.y, 0.), true);
                        addPointToShapeGeometry(Base::Vector3d(center.x, center.y, 0.), true);
                        addToShapeConstraints(Sketcher::Symmetric,
                                              firstCurve + 2,
                                              Sketcher::PointPos::start,
                                              firstCurve,
                                              Sketcher::PointPos::start,
                                              centerPointId,
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 1);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 2);
                    }
                    else {
                        if (fabs(thickness) > Precision::Confusion()) {
                            constructionPointOneId = firstCurve + 16;  // NOLINT
                            constructionPointTwoId = firstCurve + 17;  // NOLINT
                        }
                        else {
                            constructionPointOneId = firstCurve + 8;  // NOLINT
                            constructionPointTwoId = firstCurve + 9;  // NOLINT
                        }

                        addPointToShapeGeometry(Base::Vector3d(corner1.x, corner1.y, 0.), true);
                        addPointToShapeGeometry(Base::Vector3d(corner3.x, corner3.y, 0.), true);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointOneId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 3);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointTwoId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 1);
                        addToShapeConstraints(Sketcher::PointOnObject,
                                              constructionPointTwoId,
                                              Sketcher::PointPos::start,
                                              firstCurve + 2);
                    }
                }
                else {  // cases of normal rectangles and normal frames
                    addToShapeConstraints(Sketcher::Coincident,
                                          firstCurve,
                                          Sketcher::PointPos::end,
                                          firstCurve + 1,
                                          Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident,
                                          firstCurve + 1,
                                          Sketcher::PointPos::end,
                                          firstCurve + 2,
                                          Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident,
                                          firstCurve + 2,
                                          Sketcher::PointPos::end,
                                          firstCurve + 3,
                                          Sketcher::PointPos::start);
                    addToShapeConstraints(Sketcher::Coincident,
                                          firstCurve + 3,
                                          Sketcher::PointPos::end,
                                          firstCurve,
                                          Sketcher::PointPos::start);
                    if (fabs(angle) < Precision::Confusion()
                        || constructionMethod() == ConstructionMethod::Diagonal
                        || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                        addToShapeConstraints(typeA, firstCurve);
                        addToShapeConstraints(typeA, firstCurve + 2);
                        addToShapeConstraints(typeB, firstCurve + 1);
                        addToShapeConstraints(typeB, firstCurve + 3);
                    }
                    else {
                        addToShapeConstraints(Sketcher::Parallel,
                                              firstCurve,
                                              Sketcher::PointPos::none,
                                              firstCurve + 2);
                        addToShapeConstraints(Sketcher::Parallel,
                                              firstCurve + 1,
                                              Sketcher::PointPos::none,
                                              firstCurve + 3);
                        if (fabs(angle123 - M_PI / 2) < Precision::Confusion()) {
                            addToShapeConstraints(Sketcher::Perpendicular,
                                                  firstCurve,
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 1);
                        }
                    }

                    if (fabs(thickness) > Precision::Confusion()) {
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 4,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 5,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 5,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 6,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 6,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 7,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 7,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 4,  // NOLINT
                                              Sketcher::PointPos::start);

                        if (fabs(angle) < Precision::Confusion()
                            || constructionMethod() == ConstructionMethod::Diagonal
                            || constructionMethod() == ConstructionMethod::CenterAndCorner) {
                            addToShapeConstraints(typeA, firstCurve + 4);  // NOLINT
                            addToShapeConstraints(typeA, firstCurve + 6);  // NOLINT
                            addToShapeConstraints(typeB, firstCurve + 5);  // NOLINT
                            addToShapeConstraints(typeB, firstCurve + 7);  // NOLINT
                        }
                        else {
                            addToShapeConstraints(Sketcher::Parallel,
                                                  firstCurve + 4,  // NOLINT
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 6);  // NOLINT
                            addToShapeConstraints(Sketcher::Parallel,
                                                  firstCurve + 5,  // NOLINT
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 7);  // NOLINT
                            addToShapeConstraints(Sketcher::Parallel,
                                                  firstCurve,
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 4);  // NOLINT
                            addToShapeConstraints(Sketcher::Parallel,
                                                  firstCurve + 1,  // NOLINT
                                                  Sketcher::PointPos::none,
                                                  firstCurve + 5);  // NOLINT
                        }

                        // add construction lines
                        addLineToShapeGeometry(Base::Vector3d(corner1.x, corner1.y, 0.),
                                               Base::Vector3d(frameCorner1.x, frameCorner1.y, 0.),
                                               true);
                        addLineToShapeGeometry(Base::Vector3d(corner2.x, corner2.y, 0.),
                                               Base::Vector3d(frameCorner2.x, frameCorner2.y, 0.),
                                               true);
                        addLineToShapeGeometry(Base::Vector3d(corner3.x, corner3.y, 0.),
                                               Base::Vector3d(frameCorner3.x, frameCorner3.y, 0.),
                                               true);
                        addLineToShapeGeometry(Base::Vector3d(corner4.x, corner4.y, 0.),
                                               Base::Vector3d(frameCorner4.x, frameCorner4.y, 0.),
                                               true);

                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 8,  // NOLINT
                                              Sketcher::PointPos::start,
                                              firstCurve,
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 8,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 4,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 9,  // NOLINT
                                              Sketcher::PointPos::start,
                                              firstCurve + 1,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 9,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 5,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 10,  // NOLINT
                                              Sketcher::PointPos::start,
                                              firstCurve + 2,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 10,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 6,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 11,  // NOLINT
                                              Sketcher::PointPos::start,
                                              firstCurve + 3,  // NOLINT
                                              Sketcher::PointPos::start);
                        addToShapeConstraints(Sketcher::Coincident,
                                              firstCurve + 11,  // NOLINT
                                              Sketcher::PointPos::end,
                                              firstCurve + 7,  // NOLINT
                                              Sketcher::PointPos::start);

                        addToShapeConstraints(Sketcher::Perpendicular,
                                              firstCurve + 8,  // NOLINT
                                              Sketcher::PointPos::none,
                                              firstCurve + 9);  // NOLINT
                        addToShapeConstraints(Sketcher::Perpendicular,
                                              firstCurve + 9,  // NOLINT
                                              Sketcher::PointPos::none,
                                              firstCurve + 10);  // NOLINT
                        addToShapeConstraints(Sketcher::Perpendicular,
                                              firstCurve + 10,  // NOLINT
                                              Sketcher::PointPos::none,
                                              firstCurve + 11);  // NOLINT
                    }

                    if (constructionMethod() == ConstructionMethod::CenterAndCorner
                        || constructionMethod() == ConstructionMethod::CenterAnd3Points) {
                        if (fabs(thickness) > Precision::Confusion()) {
                            centerPointId = firstCurve + 12;  // NOLINT
                        }
                        else {
                            centerPointId = firstCurve + 4;  // NOLINT
                        }

                        addPointToShapeGeometry(Base::Vector3d(center.x, center.y, 0.), true);
                        addToShapeConstraints(Sketcher::Symmetric,
                                              firstCurve + 2,  // NOLINT
                                              Sketcher::PointPos::start,
                                              firstCurve,
                                              Sketcher::PointPos::start,
                                              centerPointId,
                                              Sketcher::PointPos::start);
                    }
                }
            }
        }
    }

    int getPointSideOfVector(Base::Vector2d pointToCheck,
                             Base::Vector2d separatingVector,
                             Base::Vector2d pointOnVector)
    {
        Base::Vector2d secondPointOnVec = pointOnVector + separatingVector;
        double d = (pointToCheck.x - pointOnVector.x) * (secondPointOnVec.y - pointOnVector.y)
            - (pointToCheck.y - pointOnVector.y) * (secondPointOnVec.x - pointOnVector.x);
        if (abs(d) < Precision::Confusion()) {
            return 0;
        }
        else if (d < 0) {
            return -1;
        }
        else {
            return 1;
        }
    }

    void calculateRadius(Base::Vector2d onSketchPos)
    {
        Base::Vector2d u = (corner2 - corner1) / (corner2 - corner1).Length();
        Base::Vector2d v = (corner4 - corner1) / (corner4 - corner1).Length();
        Base::Vector2d e = onSketchPos - corner1;
        double du = (v.y * e.x - v.x * e.y) / (u.x * v.y - u.y * v.x);
        double dv = (-u.y * e.x + u.x * e.y) / (u.x * v.y - u.y * v.x);

        if (-Precision::Confusion() < du && du < 0) {
            du = 0.0;
        }
        if (-Precision::Confusion() < dv && dv < 0) {
            dv = 0.0;
        }

        if (du < 0.0 || du > length || dv < 0.0 || dv > width) {
            radius = 0.;
        }
        else {
            if (du < length - du && dv < width - dv) {
                radius = (du + dv
                          + std::max(2 * sqrt(du * dv) * sin(angle412 / 2),
                                     -2 * sqrt(du * dv) * sin(angle412 / 2)))
                    * tan(angle412 / 2);
            }
            else if (du > length - du && dv < width - dv) {
                du = length - du;
                radius = (du + dv
                          + std::max(2 * sqrt(du * dv) * sin(angle123 / 2),
                                     -2 * sqrt(du * dv) * sin(angle123 / 2)))
                    * tan(angle123 / 2);
            }
            else if (du < length - du && dv > width - dv) {
                dv = width - dv;
                radius = (du + dv
                          + std::max(2 * sqrt(du * dv) * sin(angle123 / 2),
                                     -2 * sqrt(du * dv) * sin(angle123 / 2)))
                    * tan(angle123 / 2);
            }
            else {
                du = length - du;
                dv = width - dv;
                radius = (du + dv
                          + std::max(2 * sqrt(du * dv) * sin(angle412 / 2),
                                     -2 * sqrt(du * dv) * sin(angle412 / 2)))
                    * tan(angle412 / 2);
            }
            radius = std::min(
                radius,
                std::min(length * 0.999, width * 0.999)  // NOLINT
                    / (cos(angle412 / 2) / sqrt(1 - cos(angle412 / 2) * cos(angle412 / 2))
                       + cos(angle123 / 2) / sqrt(1 - cos(angle123 / 2) * cos(angle123 / 2))));
        }
    }

    void calculateThickness(Base::Vector2d onSketchPos)
    {

        Base::Vector2d u = (corner2 - corner1) / (corner2 - corner1).Length();
        Base::Vector2d v = (corner4 - corner1) / (corner4 - corner1).Length();
        Base::Vector2d e = onSketchPos - corner1;
        double obliqueThickness = 0.;
        double du = (v.y * e.x - v.x * e.y) / (u.x * v.y - u.y * v.x);
        double dv = (-u.y * e.x + u.x * e.y) / (u.x * v.y - u.y * v.x);
        if (du > 0 && du < length && !(dv > 0 && dv < width)) {
            obliqueThickness = std::min(fabs(dv), fabs(width - dv));
        }
        else if (dv > 0 && dv < width && !(du > 0 && du < length)) {
            obliqueThickness = std::min(fabs(du), fabs(length - du));
        }
        else if (du > 0 && du < length && dv > 0 && dv < width) {
            obliqueThickness = -std::min(std::min(fabs(du), fabs(length - du)),
                                         std::min(fabs(dv), fabs(width - dv)));
        }
        else {
            obliqueThickness = std::max(std::min(fabs(du), fabs(length - du)),
                                        std::min(fabs(dv), fabs(width - dv)));
        }


        frameCorner1 = corner1 - u * obliqueThickness - v * obliqueThickness;
        frameCorner2 = corner2 + u * obliqueThickness - v * obliqueThickness;
        frameCorner3 = corner3 + u * obliqueThickness + v * obliqueThickness;
        frameCorner4 = corner4 - u * obliqueThickness + v * obliqueThickness;

        thickness = obliqueThickness * sin(angle412);
    }
};

template<>
auto DSHRectangleControllerBase::getState(int labelindex) const
{
    if (handler->constructionMethod() == ConstructionMethod::Diagonal
        || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
        switch (labelindex) {
            case OnViewParameter::First:
            case OnViewParameter::Second:
                return SelectMode::SeekFirst;
                break;
            case OnViewParameter::Third:
            case OnViewParameter::Fourth:
                return SelectMode::SeekSecond;
                break;
            case OnViewParameter::Fifth:
                if (handler->roundCorners) {
                    return SelectMode::SeekThird;
                }
                else {
                    return SelectMode::End;
                }
                break;
            case OnViewParameter::Sixth:
                if (handler->makeFrame) {
                    if (!handler->roundCorners) {
                        return SelectMode::SeekThird;
                    }
                    else {
                        return SelectMode::SeekFourth;
                    }
                }
                else {
                    return SelectMode::End;
                }
                break;
            default:
                THROWM(Base::ValueError, "Parameter index without an associated machine state")
        }
    }
    else {
        switch (labelindex) {
            case OnViewParameter::First:
            case OnViewParameter::Second:
                return SelectMode::SeekFirst;
                break;
            case OnViewParameter::Third:
            case OnViewParameter::Fourth:
                return SelectMode::SeekSecond;
                break;
            case OnViewParameter::Fifth:
            case OnViewParameter::Sixth:
                return SelectMode::SeekThird;
                break;
            case OnViewParameter::Seventh:
                if (handler->roundCorners) {
                    return SelectMode::SeekFourth;
                }
                else {
                    return SelectMode::End;
                }
                break;
            case OnViewParameter::Eighth:
                if (handler->makeFrame) {
                    if (!handler->roundCorners) {
                        return SelectMode::SeekFourth;
                    }
                    else {
                        return SelectMode::SeekFifth;
                    }
                }
                else {
                    return SelectMode::End;
                }
                break;
            default:
                THROWM(Base::ValueError, "Parameter index without an associated machine state")
        }
    }
}

template<>
void DSHRectangleController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {
            QApplication::translate("TaskSketcherTool_c1_rectangle", "Corner, width, height"),
            QApplication::translate("TaskSketcherTool_c1_rectangle", "Center, width, height"),
            QApplication::translate("TaskSketcherTool_c1_rectangle", "3 corners"),
            QApplication::translate("TaskSketcherTool_c1_rectangle", "Center, 2 corners")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_rectangle", "Rounded corners (U)"));
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_rectangle",
                                    "Create a rectangle with rounded corners."));
        syncCheckboxToHandler(WCheckbox::FirstBox, handler->roundCorners);

        toolWidget->setCheckboxLabel(
            WCheckbox::SecondBox,
            QApplication::translate("TaskSketcherTool_c2_rectangle", "Frame (J)"));
        toolWidget->setCheckboxToolTip(
            WCheckbox::SecondBox,
            QApplication::translate("TaskSketcherTool_c2_rectangle",
                                    "Create two rectangles with a constant offset."));
        syncCheckboxToHandler(WCheckbox::SecondBox, handler->makeFrame);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center_Constr"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                2,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle3Points_Constr"));
            toolWidget->setComboboxItemIcon(WCombobox::FirstCombo,
                                            3,
                                            Gui::BitmapFactory().iconFromTheme(
                                                "Sketcher_CreateRectangle3Points_Center_Constr"));

            toolWidget->setCheckboxIcon(
                WCheckbox::FirstBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong_Constr"));
            toolWidget->setCheckboxIcon(
                WCheckbox::SecondBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFrame_Constr"));
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                2,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle3Points"));
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                3,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle3Points_Center"));

            toolWidget->setCheckboxIcon(
                WCheckbox::FirstBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong"));
            toolWidget->setCheckboxIcon(WCheckbox::SecondBox,
                                        Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFrame"));
        }
    }

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);

    if (handler->constructionMethod() == ConstructionMethod::Diagonal
        || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
        onViewParameters[OnViewParameter::Third]->setLabelType(
            Gui::SoDatumLabel::DISTANCEX,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Fourth]->setLabelType(
            Gui::SoDatumLabel::DISTANCEY,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Fifth]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Sixth]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning);
    }
    else if (handler->constructionMethod() == ConstructionMethod::ThreePoints) {
        onViewParameters[OnViewParameter::Third]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Fourth]->setLabelType(
            Gui::SoDatumLabel::ANGLE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Fifth]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Sixth]->setLabelType(
            Gui::SoDatumLabel::ANGLE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Seventh]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Eighth]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning);
    }
    else if (handler->constructionMethod() == ConstructionMethod::CenterAnd3Points) {
        onViewParameters[OnViewParameter::Third]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
        onViewParameters[OnViewParameter::Fourth]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
        onViewParameters[OnViewParameter::Fifth]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Sixth]->setLabelType(
            Gui::SoDatumLabel::ANGLE,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Seventh]->setLabelType(
            Gui::SoDatumLabel::RADIUS,
            Gui::EditableDatumLabel::Function::Dimensioning);
        onViewParameters[OnViewParameter::Eighth]->setLabelType(
            Gui::SoDatumLabel::DISTANCE,
            Gui::EditableDatumLabel::Function::Dimensioning);
    }
}

template<>
void DSHRectangleController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    Q_UNUSED(checkboxindex);

    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            handler->roundCorners = value;
            break;
        case WCheckbox::SecondBox:
            handler->makeFrame = value;
            break;
    }

    handler->updateCursor();
}

template<>
void DSHRectangleControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::First]->getValue();
            }

            if (onViewParameters[OnViewParameter::Second]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Second]->getValue();
            }
        } break;
        case SelectMode::SeekSecond: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (onViewParameters[OnViewParameter::Third]->isSet) {
                    double length = onViewParameters[OnViewParameter::Third]->getValue();
                    if (fabs(length) < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                        return;
                    }

                    if (handler->constructionMethod() == ConstructionMethod::Diagonal) {
                        int sign = (onSketchPos.x - handler->corner1.x) >= 0 ? 1 : -1;
                        onSketchPos.x = handler->corner1.x + sign * length;
                    }
                    else {
                        onSketchPos.x = handler->center.x + length / 2;
                    }
                }
                if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                    double width = onViewParameters[OnViewParameter::Fourth]->getValue();
                    if (fabs(width) < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
                        return;
                    }

                    if (handler->constructionMethod() == ConstructionMethod::Diagonal) {
                        int sign = (onSketchPos.y - handler->corner1.y) >= 0 ? 1 : -1;
                        onSketchPos.y = handler->corner1.y + sign * width;
                    }
                    else {
                        onSketchPos.y = handler->center.y + width / 2;
                    }
                }
            }
            else if (handler->constructionMethod() == ConstructionMethod::ThreePoints) {
                Base::Vector2d dir = onSketchPos - handler->corner1;
                if (dir.Length() < Precision::Confusion()) {
                    dir.x = 1.0;  // if direction null, default to (1,0)
                }
                double length = dir.Length();

                if (onViewParameters[OnViewParameter::Third]->isSet) {
                    length = onViewParameters[OnViewParameter::Third]->getValue();
                    if (length < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                        return;
                    }

                    onSketchPos = handler->corner1 + length * dir.Normalize();
                }

                if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                    double angle =
                        Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
                    onSketchPos.x = handler->corner1.x + cos(angle) * length;
                    onSketchPos.y = handler->corner1.y + sin(angle) * length;
                }
            }
            else {
                if (onViewParameters[OnViewParameter::Third]->isSet) {
                    onSketchPos.x = onViewParameters[OnViewParameter::Third]->getValue();
                }

                if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                    onSketchPos.y = onViewParameters[OnViewParameter::Fourth]->getValue();
                }
                if (onViewParameters[OnViewParameter::Third]->isSet
                    && onViewParameters[OnViewParameter::Fourth]->isSet
                    && (onSketchPos - handler->center).Length() < Precision::Confusion()) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Third].get());
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Fourth].get());
                }
            }
        } break;
        case SelectMode::SeekThird: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (handler->roundCorners) {
                    if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                        Base::Vector2d vecL = (handler->corner2 - handler->corner1).Normalize();
                        onSketchPos = handler->corner1
                            + vecL * onViewParameters[OnViewParameter::Fifth]->getValue();
                    }
                }
                else {
                    if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                        double thickness = onViewParameters[OnViewParameter::Sixth]->getValue();
                        if (thickness <= -std::min(handler->width, handler->length) / 2) {
                            unsetOnViewParameter(onViewParameters[OnViewParameter::Sixth].get());
                            return;
                        }

                        Base::Vector2d u = (handler->corner2 - handler->corner1).Normalize();
                        Base::Vector2d v = (handler->corner4 - handler->corner1).Normalize();
                        onSketchPos = handler->corner1 - u * thickness - v * thickness;
                    }
                }
            }
            else if (handler->constructionMethod() == ConstructionMethod::ThreePoints) {
                Base::Vector2d dir = onSketchPos - handler->corner2Initial;
                if (dir.Length() < Precision::Confusion()) {
                    dir.x = 1.0;  // if direction null, default to (1,0)
                }
                double width = dir.Length();

                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    width = onViewParameters[OnViewParameter::Fifth]->getValue();
                    if (width < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                        return;
                    }

                    onSketchPos = handler->corner2Initial + width * dir.Normalize();
                }
                if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                    double angle =
                        Base::toRadians(onViewParameters[OnViewParameter::Sixth]->getValue());
                    if (fmod(angle, M_PI) < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Sixth].get());
                        return;
                    }

                    int sign1 =
                        handler->getPointSideOfVector(onSketchPos,
                                                      handler->corner2Initial - handler->corner1,
                                                      handler->corner1);

                    int sign = handler->side != sign1 ? 1 : -1;

                    double angle123 =
                        (handler->corner2Initial - handler->corner1).Angle() + M_PI + sign * angle;

                    onSketchPos.x = handler->corner2Initial.x + cos(angle123) * width;
                    onSketchPos.y = handler->corner2Initial.y + sin(angle123) * width;
                }
            }
            else {
                Base::Vector2d dir = onSketchPos - handler->corner1;
                if (dir.Length() < Precision::Confusion()) {
                    dir.x = 1.0;  // if direction null, default to (1,0)
                }
                double width = dir.Length();
                if (onViewParameters[OnViewParameter::Fifth]->isSet) {
                    width = onViewParameters[OnViewParameter::Fifth]->getValue();
                    if (width < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Fifth].get());
                        return;
                    }

                    onSketchPos = handler->corner1 + width * dir.Normalize();
                }
                if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                    double c =
                        Base::toRadians(onViewParameters[OnViewParameter::Sixth]->getValue());
                    if (fmod(c, M_PI) < Precision::Confusion()) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Sixth].get());
                        return;
                    }

                    double a = asin(width * sin(M_PI - c)
                                    / (handler->corner3 - handler->corner1).Length());

                    int sign1 = handler->getPointSideOfVector(onSketchPos,
                                                              handler->corner3 - handler->corner1,
                                                              handler->corner1);

                    int sign = handler->side != sign1 ? 1 : -1;

                    double angle = (handler->center - handler->corner1).Angle() + sign * (c - a);

                    onSketchPos.x = handler->corner1.x + cos(angle) * width;
                    onSketchPos.y = handler->corner1.y + sin(angle) * width;
                }
            }
        } break;
        case SelectMode::SeekFourth: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {

                if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                    double thickness = onViewParameters[OnViewParameter::Sixth]->getValue();
                    if (thickness <= -std::min(handler->width, handler->length) / 2) {
                        unsetOnViewParameter(onViewParameters[OnViewParameter::Sixth].get());
                        return;
                    }

                    Base::Vector2d u = (handler->corner2 - handler->corner1).Normalize();
                    Base::Vector2d v = (handler->corner4 - handler->corner1).Normalize();
                    onSketchPos = handler->corner1 - u * thickness - v * thickness;
                }
            }
            else {
                if (handler->roundCorners) {
                    if (onViewParameters[OnViewParameter::Seventh]->isSet) {
                        double angleToUse = handler->angle412 / 2;
                        if (handler->constructionMethod() == ConstructionMethod::CenterAnd3Points) {
                            angleToUse = handler->angle123 / 2;
                        }
                        Base::Vector2d vecL =
                            (handler->corner2Initial - handler->corner1).Normalize();
                        double L2 = onViewParameters[OnViewParameter::Seventh]->getValue()
                            / sqrt(1 - cos(angleToUse) * cos(angleToUse));
                        onSketchPos = handler->corner1 + vecL * L2 * cos(angleToUse);
                    }
                }
                else {
                    if (onViewParameters[OnViewParameter::Eighth]->isSet) {
                        double thickness = onViewParameters[OnViewParameter::Eighth]->getValue();
                        if (thickness <= -std::min(handler->width, handler->length) / 2) {
                            unsetOnViewParameter(onViewParameters[OnViewParameter::Eighth].get());
                            return;
                        }

                        Base::Vector2d u = (handler->corner2 - handler->corner1).Normalize();
                        Base::Vector2d v = (handler->corner4 - handler->corner1).Normalize();
                        onSketchPos = handler->corner1 - u * thickness - v * thickness;
                    }
                }
            }
        } break;
        case SelectMode::SeekFifth: {
            if (onViewParameters[OnViewParameter::Eighth]->isSet) {
                double thickness = onViewParameters[OnViewParameter::Eighth]->getValue();
                if (thickness <= -std::min(handler->width, handler->length) / 2) {
                    unsetOnViewParameter(onViewParameters[OnViewParameter::Eighth].get());
                    return;
                }

                Base::Vector2d u = (handler->corner2 - handler->corner1).Normalize();
                Base::Vector2d v = (handler->corner4 - handler->corner1).Normalize();
                onSketchPos = handler->corner1 - u * thickness - v * thickness;
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHRectangleController::adaptParameters(Base::Vector2d onSketchPos)
{

    // If checkboxes need synchronisation (they were changed by the DSH, e.g. by using 'M' to switch
    // construction method), synchronise them and return.
    if (syncCheckboxToHandler(WCheckbox::FirstBox, handler->roundCorners)) {
        return;
    }

    if (syncCheckboxToHandler(WCheckbox::SecondBox, handler->makeFrame)) {
        return;
    }

    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (!onViewParameters[OnViewParameter::First]->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!onViewParameters[OnViewParameter::Second]->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            onViewParameters[OnViewParameter::First]->setLabelAutoDistanceReverse(!sameSign);
            onViewParameters[OnViewParameter::Second]->setLabelAutoDistanceReverse(sameSign);
            onViewParameters[OnViewParameter::First]->setPoints(Base::Vector3d(),
                                                                toVector3d(onSketchPos));
            onViewParameters[OnViewParameter::Second]->setPoints(Base::Vector3d(),
                                                                 toVector3d(onSketchPos));
        } break;
        case SelectMode::SeekSecond: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    double length = handler->cornersReversed ? handler->width : handler->length;
                    setOnViewParameterValue(OnViewParameter::Third, length);
                }

                if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                    double width = handler->cornersReversed ? handler->length : handler->width;
                    setOnViewParameterValue(OnViewParameter::Fourth, width);
                }

                Base::Vector3d start = toVector3d(handler->corner1);
                Base::Vector3d vec = toVector3d(onSketchPos) - start;
                bool sameSign = vec.x * vec.y > 0.;

                onViewParameters[OnViewParameter::Third]->setLabelAutoDistanceReverse(sameSign);
                onViewParameters[OnViewParameter::Fourth]->setLabelAutoDistanceReverse(!sameSign);

                onViewParameters[OnViewParameter::Third]->setPoints(
                    start,
                    toVector3d(handler->cornersReversed ? handler->corner4 : handler->corner2));
                onViewParameters[OnViewParameter::Fourth]->setPoints(
                    start,
                    toVector3d(handler->cornersReversed ? handler->corner2 : handler->corner4));
            }
            else if (handler->constructionMethod() == ConstructionMethod::ThreePoints) {
                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, handler->length);
                }

                onViewParameters[OnViewParameter::Third]->setPoints(toVector3d(handler->corner4),
                                                                    toVector3d(handler->corner3));

                if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth,
                                            Base::toDegrees(handler->angle),
                                            Base::Unit::Angle);
                }

                onViewParameters[OnViewParameter::Fourth]->setPoints(toVector3d(handler->corner1),
                                                                     Base::Vector3d());
                onViewParameters[OnViewParameter::Fourth]->setLabelRange(
                    (handler->corner2 - handler->corner1).Angle());
            }
            else {
                if (!onViewParameters[OnViewParameter::Third]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Third, onSketchPos.x);
                }

                if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fourth, onSketchPos.y);
                }

                bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
                onViewParameters[OnViewParameter::Third]->setLabelAutoDistanceReverse(!sameSign);
                onViewParameters[OnViewParameter::Fourth]->setLabelAutoDistanceReverse(sameSign);
                onViewParameters[OnViewParameter::Third]->setPoints(Base::Vector3d(),
                                                                    toVector3d(onSketchPos));
                onViewParameters[OnViewParameter::Fourth]->setPoints(Base::Vector3d(),
                                                                     toVector3d(onSketchPos));
            }
        } break;
        case SelectMode::SeekThird: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (handler->roundCorners) {
                    if (!onViewParameters[OnViewParameter::Fifth]->isSet) {
                        setOnViewParameterValue(OnViewParameter::Fifth, handler->radius);
                    }

                    Base::Vector3d center = handler->center3;
                    Base::Vector3d end = toVector3d(handler->corner3);

                    if (handler->radius != 0.0) {
                        Base::Vector3d vec = (end - center).Normalize();
                        end = center + vec * handler->radius;
                    }

                    onViewParameters[OnViewParameter::Fifth]->setPoints(center, end);
                }
                else {
                    if (!onViewParameters[OnViewParameter::Sixth]->isSet) {
                        setOnViewParameterValue(OnViewParameter::Sixth, handler->thickness);
                    }

                    Base::Vector3d start = toVector3d(handler->corner3);
                    Base::Vector3d end =
                        Base::Vector3d(handler->corner3.x, handler->frameCorner3.y, 0.0);
                    onViewParameters[OnViewParameter::Sixth]->setPoints(start, end);
                }
            }
            else {
                onViewParameters[OnViewParameter::Third]->setPoints(toVector3d(handler->corner4),
                                                                    toVector3d(handler->corner3));

                bool threePoints = handler->constructionMethod() == ConstructionMethod::ThreePoints;
                bool notReversed = threePoints && !handler->cornersReversed;

                if (!onViewParameters[OnViewParameter::Fifth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Fifth,
                                            notReversed ? handler->width : handler->length);
                }

                Base::Vector3d start = toVector3d(handler->corner1);
                onViewParameters[OnViewParameter::Fifth]->setLabelAutoDistanceReverse(!notReversed);
                onViewParameters[OnViewParameter::Fifth]->setPoints(
                    start,
                    toVector3d(notReversed ? handler->corner4 : handler->corner2));


                if (!onViewParameters[OnViewParameter::Sixth]->isSet) {
                    if (threePoints) {
                        double val = Base::toDegrees(handler->angle123);
                        setOnViewParameterValue(OnViewParameter::Sixth, val, Base::Unit::Angle);
                    }
                    else {
                        double val = Base::toDegrees(handler->angle412);
                        setOnViewParameterValue(OnViewParameter::Sixth, val, Base::Unit::Angle);
                    }
                }

                if (threePoints) {
                    onViewParameters[OnViewParameter::Sixth]->setPoints(
                        toVector3d(notReversed ? handler->corner2 : handler->corner4),
                        Base::Vector3d());
                    double startAngle = notReversed ? (handler->corner3 - handler->corner2).Angle()
                                                    : (handler->corner1 - handler->corner4).Angle();
                    onViewParameters[OnViewParameter::Sixth]->setLabelStartAngle(startAngle);
                    onViewParameters[OnViewParameter::Sixth]->setLabelRange(handler->angle123);
                }
                else {
                    onViewParameters[OnViewParameter::Sixth]->setPoints(
                        toVector3d(handler->corner1),
                        Base::Vector3d());
                    double startAngle = (handler->corner2 - handler->corner1).Angle();
                    onViewParameters[OnViewParameter::Sixth]->setLabelStartAngle(startAngle);
                    onViewParameters[OnViewParameter::Sixth]->setLabelRange(handler->angle412);
                }
            }

        } break;
        case SelectMode::SeekFourth: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (!onViewParameters[OnViewParameter::Sixth]->isSet) {
                    setOnViewParameterValue(OnViewParameter::Sixth, handler->thickness);
                }

                Base::Vector3d start = toVector3d(handler->corner3);
                Base::Vector3d end =
                    Base::Vector3d(handler->corner3.x, handler->frameCorner3.y, 0.0);
                onViewParameters[OnViewParameter::Sixth]->setPoints(start, end);
            }
            else {
                if (handler->roundCorners) {
                    if (!onViewParameters[OnViewParameter::Seventh]->isSet) {
                        setOnViewParameterValue(OnViewParameter::Seventh, handler->radius);
                    }

                    Base::Vector3d center = handler->center3;
                    Base::Vector3d end = toVector3d(handler->corner3);

                    if (handler->radius != 0.0) {
                        Base::Vector3d vec = (end - center).Normalize();
                        end = center + vec * handler->radius;
                    }

                    onViewParameters[OnViewParameter::Seventh]->setPoints(center, end);
                }
                else {
                    if (!onViewParameters[OnViewParameter::Eighth]->isSet) {
                        setOnViewParameterValue(OnViewParameter::Eighth, handler->thickness);
                    }

                    Base::Vector3d start = toVector3d(handler->corner3);
                    Base::Vector3d vec =
                        toVector3d((handler->corner3 - handler->corner2).Normalize());
                    Base::Vector3d end = start + handler->thickness * vec;
                    onViewParameters[OnViewParameter::Eighth]->setPoints(start, end);
                }
            }
        } break;
        case SelectMode::SeekFifth: {
            if (!onViewParameters[OnViewParameter::Eighth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Eighth, handler->thickness);
            }

            Base::Vector3d start = toVector3d(handler->corner3);
            Base::Vector3d vec = toVector3d((handler->corner3 - handler->corner2).Normalize());
            Base::Vector3d end = start + handler->thickness * vec;
            onViewParameters[OnViewParameter::Eighth]->setPoints(start, end);
        } break;
        default:
            break;
    }
}

template<>
void DSHRectangleController::doChangeDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            if (onViewParameters[OnViewParameter::First]->isSet
                && onViewParameters[OnViewParameter::Second]->isSet) {

                handler->setState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            if (onViewParameters[OnViewParameter::Third]->isSet
                && onViewParameters[OnViewParameter::Fourth]->isSet) {

                if (handler->roundCorners || handler->makeFrame
                    || handler->constructionMethod() == ConstructionMethod::ThreePoints
                    || handler->constructionMethod() == ConstructionMethod::CenterAnd3Points) {

                    handler->setState(SelectMode::SeekThird);
                }
                else {
                    handler->setState(SelectMode::End);
                }
            }
        } break;
        case SelectMode::SeekThird: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (handler->roundCorners && onViewParameters[OnViewParameter::Fifth]->isSet) {

                    if (handler->makeFrame) {
                        handler->setState(SelectMode::SeekFourth);
                    }
                    else {
                        handler->setState(SelectMode::End);
                    }
                }
                else if (handler->makeFrame && onViewParameters[OnViewParameter::Sixth]->isSet) {

                    handler->setState(SelectMode::End);
                }
            }
            else {
                if (onViewParameters[OnViewParameter::Fifth]->isSet
                    && onViewParameters[OnViewParameter::Sixth]->isSet) {
                    if (handler->roundCorners || handler->makeFrame) {
                        handler->setState(SelectMode::SeekFourth);
                    }
                    else {
                        handler->setState(SelectMode::End);
                    }
                }
            }
        } break;
        case SelectMode::SeekFourth: {
            if (handler->constructionMethod() == ConstructionMethod::Diagonal
                || handler->constructionMethod() == ConstructionMethod::CenterAndCorner) {
                if (onViewParameters[OnViewParameter::Sixth]->isSet) {
                    handler->setState(SelectMode::End);
                }
            }
            else {
                if (handler->roundCorners && onViewParameters[OnViewParameter::Seventh]->isSet) {

                    if (handler->makeFrame) {
                        handler->setState(SelectMode::SeekFifth);
                    }
                    else {
                        handler->setState(SelectMode::End);
                    }
                }
                else if (handler->makeFrame && onViewParameters[OnViewParameter::Eighth]->isSet) {
                    handler->setState(SelectMode::End);
                }
            }
        } break;
        case SelectMode::SeekFifth: {
            if (handler->makeFrame && onViewParameters[OnViewParameter::Eighth]->isSet) {
                handler->setState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHRectangleController::addConstraints()
{
    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->firstCurve;

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();
    auto length = onViewParameters[OnViewParameter::Third]->getValue();
    auto width = onViewParameters[OnViewParameter::Fourth]->getValue();
    auto radius = onViewParameters[OnViewParameter::Fifth]->getValue();
    auto thickness = onViewParameters[OnViewParameter::Sixth]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto lengthSet = onViewParameters[OnViewParameter::Third]->isSet;
    auto widthSet = onViewParameters[OnViewParameter::Fourth]->isSet;
    auto radiusSet = onViewParameters[OnViewParameter::Fifth]->isSet;
    auto thicknessSet = onViewParameters[OnViewParameter::Sixth]->isSet;

    auto angle = Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
    auto innerAngle = Base::toRadians(onViewParameters[OnViewParameter::Sixth]->getValue());

    auto angleSet = onViewParameters[OnViewParameter::Fourth]->isSet;
    auto innerAngleSet = onViewParameters[OnViewParameter::Sixth]->isSet;

    if (handler->constructionMethod() == ConstructionMethod::ThreePoints
        || handler->constructionMethod() == ConstructionMethod::CenterAnd3Points) {
        width = onViewParameters[OnViewParameter::Fifth]->getValue();
        radius = onViewParameters[OnViewParameter::Seventh]->getValue();
        thickness = onViewParameters[OnViewParameter::Eighth]->getValue();

        widthSet = onViewParameters[OnViewParameter::Fifth]->isSet;
        radiusSet = onViewParameters[OnViewParameter::Seventh]->isSet;
        thicknessSet = onViewParameters[OnViewParameter::Eighth]->isSet;
    }

    using namespace Sketcher;

    int firstPointId = firstCurve;
    if (handler->constructionMethod() == ConstructionMethod::Diagonal
        || handler->constructionMethod() == ConstructionMethod::ThreePoints) {
        if (handler->radius > Precision::Confusion()) {
            firstPointId = handler->constructionPointOneId;
        }
    }
    else {
        firstPointId = handler->centerPointId;
    }

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstPointId, PointPos::start),
                               GeoElementId::VAxis,
                               x0,
                               obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstPointId, PointPos::start),
                               GeoElementId::HAxis,
                               y0,
                               obj);
    };

    auto constraintlength = [&]() {
        int curveId = handler->cornersReversed ? firstCurve : firstCurve + 1;

        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                              curveId,
                              1,
                              curveId + 2,
                              2,
                              fabs(length));
    };

    auto constraintwidth = [&]() {
        int curveId = handler->cornersReversed ? firstCurve + 1 : firstCurve;

        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                              curveId,
                              1,
                              curveId + 2,
                              2,
                              fabs(width));
    };

    // NOTE: if AutoConstraints is empty, we can add constraints directly without any diagnose. No
    // diagnose was run.
    if (handler->AutoConstraints.empty()) {
        if (x0set) {
            constraintx0();
        }

        if (y0set) {
            constrainty0();
        }

        if (lengthSet) {
            constraintlength();
        }

        if (widthSet) {
            constraintwidth();
        }
    }
    else {  // There is a valid diagnose.
        auto firstpointinfo = handler->getPointInfo(GeoElementId(firstPointId, PointPos::start));

        if (x0set && firstpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            firstpointinfo = handler->getPointInfo(
                GeoElementId(firstPointId, PointPos::start));  // get updated point position
        }

        if (y0set && firstpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
        }

        if (lengthSet) {
            int curveId = handler->cornersReversed ? firstCurve : firstCurve + 1;
            auto startpointinfo = handler->getPointInfo(GeoElementId(curveId, PointPos::start));
            auto endpointinfo = handler->getPointInfo(GeoElementId(curveId + 2, PointPos::end));

            int DoFs = startpointinfo.getDoFs();
            DoFs += endpointinfo.getDoFs();

            if (DoFs > 0) {
                constraintlength();
            }

            handler->diagnoseWithAutoConstraints();
        }

        if (widthSet) {
            int curveId = handler->cornersReversed ? firstCurve + 1 : firstCurve;
            auto startpointinfo = handler->getPointInfo(GeoElementId(curveId, PointPos::start));
            auto endpointinfo = handler->getPointInfo(GeoElementId(curveId + 2, PointPos::end));

            int DoFs = startpointinfo.getDoFs();
            DoFs += endpointinfo.getDoFs();

            if (DoFs > 0) {
                constraintwidth();
            }
        }
    }

    // NOTE: As of today, there are no autoconstraints on the radius or on the frame thickness,
    // therefore, they are necessarily constrainable were applicable.

    if (handler->constructionMethod() == ConstructionMethod::ThreePoints
        || handler->constructionMethod() == ConstructionMethod::CenterAnd3Points) {

        if (angleSet) {
            if (fabs(angle - M_PI) < Precision::Confusion()
                || fabs(angle + M_PI) < Precision::Confusion()
                || fabs(angle) < Precision::Confusion()) {
                Gui::cmdAppObjectArgs(obj,
                                      "addConstraint(Sketcher.Constraint('Horizontal',%d)) ",
                                      firstCurve);
            }
            else if (fabs(angle - M_PI / 2) < Precision::Confusion()
                     || fabs(angle + M_PI / 2) < Precision::Confusion()) {
                Gui::cmdAppObjectArgs(obj,
                                      "addConstraint(Sketcher.Constraint('Vertical',%d)) ",
                                      firstCurve);
            }
            else {
                Gui::cmdAppObjectArgs(obj,
                                      "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                                      Sketcher::GeoEnum::HAxis,
                                      firstCurve,
                                      angle);
            }
        }
        if (innerAngleSet) {
            if (fabs(innerAngle - M_PI / 2)
                > Precision::Confusion()) {  // if 90? then perpendicular already created.
                if (handler->constructionMethod() == ConstructionMethod::ThreePoints) {
                    Gui::cmdAppObjectArgs(
                        obj,
                        "addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
                        firstCurve + 1,
                        1,
                        firstCurve,
                        2,
                        innerAngle);
                }
                else {
                    Gui::cmdAppObjectArgs(
                        obj,
                        "addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
                        firstCurve,
                        1,
                        firstCurve + 3,
                        2,
                        innerAngle);
                }
            }
        }
    }

    if (radiusSet && radius > Precision::Confusion()) {
        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                              firstCurve + 5,  // NOLINT
                              radius);
    }

    if (thicknessSet) {
        Gui::cmdAppObjectArgs(obj,
                              "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f)) ",
                              firstCurve + (handler->roundCorners == true ? 8 : 4),  // NOLINT
                              1,
                              firstCurve,
                              fabs(thickness));
    }
}


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerRectangle_H
