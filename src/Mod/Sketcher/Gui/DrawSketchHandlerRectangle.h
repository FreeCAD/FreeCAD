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
#include <Gui/InputHint.h>
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
    std::list<Gui::InputHint> getToolHints() const override
    {
        using State = std::pair<ConstructionMethod, SelectMode>;
        using enum Gui::InputHint::UserInput;

        const auto switchHint =
            Gui::InputHint {.message = QObject::tr("%1 switch mode"), .sequences = {KeyM}};

        return Gui::lookupHints<State>(
            {constructionMethod(), state()},
            {
                // Diagonal method
                {.state = {ConstructionMethod::Diagonal, SelectMode::SeekFirst},
                 .hints =
                     {
                         {QObject::tr("%1 pick first corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::Diagonal, SelectMode::SeekSecond},
                 .hints =
                     {
                         {QObject::tr("%1 pick opposite corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::Diagonal, SelectMode::SeekThird},
                 .hints =
                     {
                         {QObject::tr("%1 set corner radius or frame thickness"), {MouseMove}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::Diagonal, SelectMode::SeekFourth},
                 .hints =
                     {
                         {QObject::tr("%1 set frame thickness"), {MouseMove}},
                         switchHint,
                     }},

                // CenterAndCorner method
                {.state = {ConstructionMethod::CenterAndCorner, SelectMode::SeekFirst},
                 .hints =
                     {
                         {QObject::tr("%1 pick center"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::CenterAndCorner, SelectMode::SeekSecond},
                 .hints =
                     {
                         {QObject::tr("%1 pick corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::CenterAndCorner, SelectMode::SeekThird},
                 .hints =
                     {
                         {QObject::tr("%1 set corner radius or frame thickness"), {MouseMove}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::CenterAndCorner, SelectMode::SeekFourth},
                 .hints =
                     {
                         {QObject::tr("%1 set frame thickness"), {MouseMove}},
                         switchHint,
                     }},

                // ThreePoints method
                {.state = {ConstructionMethod::ThreePoints, SelectMode::SeekFirst},
                 .hints =
                     {
                         {QObject::tr("%1 pick first corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::ThreePoints, SelectMode::SeekSecond},
                 .hints =
                     {
                         {QObject::tr("%1 pick second corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::ThreePoints, SelectMode::SeekThird},
                 .hints =
                     {
                         {QObject::tr("%1 pick third corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::ThreePoints, SelectMode::SeekFourth},
                 .hints =
                     {
                         {QObject::tr("%1 set corner radius or frame thickness"), {MouseMove}},
                         switchHint,
                     }},

                // CenterAnd3Points method
                {.state = {ConstructionMethod::CenterAnd3Points, SelectMode::SeekFirst},
                 .hints =
                     {
                         {QObject::tr("%1 pick center"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::CenterAnd3Points, SelectMode::SeekSecond},
                 .hints =
                     {
                         {QObject::tr("%1 pick first corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::CenterAnd3Points, SelectMode::SeekThird},
                 .hints =
                     {
                         {QObject::tr("%1 pick second corner"), {MouseLeft}},
                         switchHint,
                     }},
                {.state = {ConstructionMethod::CenterAnd3Points, SelectMode::SeekFourth},
                 .hints =
                     {
                         {QObject::tr("%1 set corner radius or frame thickness"), {MouseMove}},
                         switchHint,
                     }},
            });
    }

private:
    struct HintEntry
    {
        ConstructionMethods::RectangleConstructionMethod method;
        SelectMode state;
        std::list<Gui::InputHint> hints;
    };

    using HintTable = std::vector<HintEntry>;

    static Gui::InputHint switchModeHint();
    static HintTable getRectangleHintTable();
    static std::list<Gui::InputHint>
    lookupRectangleHints(ConstructionMethods::RectangleConstructionMethod method, SelectMode state);
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        using std::numbers::pi;

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

                seekAndRenderAutoConstraint(sugConstraints[0],
                                            onSketchPos,
                                            Base::Vector2d(0.f, 0.f));
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
                    angle123 = pi / 2;
                    angle412 = pi / 2;
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
                    angle123 = pi / 2;
                    angle412 = pi / 2;
                }
                else if (constructionMethod() == ConstructionMethod::ThreePoints) {
                    toolWidgetManager.drawDirectionAtCursor(onSketchPos, corner1);

                    corner2 = onSketchPos;
                    Base::Vector2d perpendicular;
                    perpendicular.x = -(corner2 - corner1).y;
                    perpendicular.y = (corner2 - corner1).x;
                    corner3 = corner2 + perpendicular;
                    corner4 = corner1 + perpendicular;
                    angle123 = pi / 2;
                    angle412 = pi / 2;
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
                    angle123 = pi / 2;
                    angle412 = pi / 2;
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

                seekAndRenderAutoConstraint(sugConstraints[1],
                                            onSketchPos,
                                            Base::Vector2d(0.0, 0.0));
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
                    angle412 = pi - angle123;
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
                    angle123 = pi - angle412;
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
                     || constructionMethod() == ConstructionMethod::CenterAnd3Points)) {
                    seekAndRenderAutoConstraint(sugConstraints[2],
                                                onSketchPos,
                                                Base::Vector2d(0.0, 0.0));
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
                return QStringLiteral("Sketcher_Pointer_Create_Box_Center");
            }
            else if (constructionMethod() == ConstructionMethod::ThreePoints) {
                return QStringLiteral("Sketcher_Pointer_Create_Box_3Points");
            }
            else if (constructionMethod() == ConstructionMethod::CenterAnd3Points) {
                return QStringLiteral("Sketcher_Pointer_Create_Box_3Points_Center");
            }
            else {
                return QStringLiteral("Sketcher_Pointer_Create_Box");
            }
        }
        else if (roundCorners && !makeFrame) {
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QStringLiteral("Sketcher_Pointer_Oblong_Center");
            }
            else {
                return QStringLiteral("Sketcher_Pointer_Oblong");
            }
        }
        else if (!roundCorners && makeFrame) {
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QStringLiteral("Sketcher_Pointer_Create_Frame_Center");
            }
            else {
                return QStringLiteral("Sketcher_Pointer_Create_Frame");
            }
        }
        else {  // both roundCorners and makeFrame
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                return QStringLiteral("Sketcher_Pointer_Oblong_Frame_Center");
            }
            else {
                return QStringLiteral("Sketcher_Pointer_Oblong_Frame");
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

    void onReset() override
    {
        thickness = 0.;
        toolWidgetManager.resetControls();
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
        if (length < Precision::Confusion() || width < Precision::Confusion()
            || fmod(fabs(angle123), std::numbers::pi) < Precision::Confusion()) {
            return;
        }

        vecL = vecL / length;
        vecW = vecW / width;
        double L1 = radius;
        double L2 = radius;
        if (cos(angle123 / 2) != 1 && cos(angle412 / 2) != 1) {
            L1 = radius / sqrt(1 - cos(angle123 / 2) * cos(angle123 / 2));
            L2 = radius / sqrt(1 - cos(angle412 / 2) * cos(angle412 / 2));
        }

        createFirstRectangleGeometries(vecL, vecW, L1, L2);

        bool thicknessNotZero = fabs(thickness) > Precision::Confusion();
        bool negThicknessEqualRadius = fabs(radius + thickness) < Precision::Confusion();
        if (makeFrame && state() != SelectMode::SeekSecond && thicknessNotZero) {
            createSecondRectangleGeometries(vecL, vecW, L1, L2);
        }

        if (!onlyeditoutline) {
            ShapeConstraints.clear();

            if (radius > Precision::Confusion()) {
                finishOblongCreation(thicknessNotZero, negThicknessEqualRadius);
            }
            else {  // cases of normal rectangles and normal frames
                finishRectangleCreation(thicknessNotZero);
            }
        }
    }

    void
    createFirstRectangleGeometries(Base::Vector2d vecL, Base::Vector2d vecW, double L1, double L2)
    {
        createFirstRectangleLines(vecL, vecW, L1, L2);

        if (roundCorners && radius > Precision::Confusion()) {
            createFirstRectangleFillets(vecL, vecW, L1, L2);
        }
    }

    void createFirstRectangleLines(Base::Vector2d vecL, Base::Vector2d vecW, double L1, double L2)
    {
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
    }

    void createFirstRectangleFillets(Base::Vector2d vecL, Base::Vector2d vecW, double L1, double L2)
    {
        using std::numbers::pi;

        // center points required later for special case of round corner frame with
        // radiusFrame = 0.
        double end = angle - pi / 2;

        Base::Vector2d b1 = (vecL + vecW) / (vecL + vecW).Length();
        Base::Vector2d b2 = (vecL - vecW) / (vecL - vecW).Length();
        center1 = toVector3d(corner1 + b1 * L2);
        center2 = toVector3d(corner2 - b2 * L1);
        center3 = toVector3d(corner3 - b1 * L2);
        center4 = toVector3d(corner4 + b2 * L1);

        addArcToShapeGeometry(center1, end - pi + angle412, end, radius, isConstructionMode());
        addArcToShapeGeometry(center2, end, end - pi - angle123, radius, isConstructionMode());
        addArcToShapeGeometry(center3, end + angle412, end - pi, radius, isConstructionMode());
        addArcToShapeGeometry(center4, end - pi, end - angle123, radius, isConstructionMode());
    }

    void
    createSecondRectangleGeometries(Base::Vector2d vecL, Base::Vector2d vecW, double L1, double L2)
    {
        using std::numbers::pi;

        double end = angle - pi / 2;

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

        addLineToShapeGeometry(toVector3d(frameCorner1 + vecLF / lengthF * L2F * cos(angle412 / 2)),
                               toVector3d(frameCorner2 - vecLF / lengthF * L1F * cos(angle123 / 2)),
                               isConstructionMode());
        addLineToShapeGeometry(toVector3d(frameCorner2 + vecWF / widthF * L1F * cos(angle123 / 2)),
                               toVector3d(frameCorner3 - vecWF / widthF * L2F * cos(angle412 / 2)),
                               isConstructionMode());
        addLineToShapeGeometry(toVector3d(frameCorner3 - vecLF / lengthF * L2F * cos(angle412 / 2)),
                               toVector3d(frameCorner4 + vecLF / lengthF * L1F * cos(angle123 / 2)),
                               isConstructionMode());
        addLineToShapeGeometry(toVector3d(frameCorner4 - vecWF / widthF * L1F * cos(angle123 / 2)),
                               toVector3d(frameCorner1 + vecWF / widthF * L2F * cos(angle412 / 2)),
                               isConstructionMode());

        if (roundCorners && radiusFrame > Precision::Confusion()) {
            Base::Vector2d b1 = (vecL + vecW) / (vecL + vecW).Length();
            Base::Vector2d b2 = (vecL - vecW) / (vecL - vecW).Length();

            addArcToShapeGeometry(toVector3d(frameCorner1 + b1 * L2F),
                                  end - pi + angle412,
                                  end,
                                  radiusFrame,
                                  isConstructionMode());
            addArcToShapeGeometry(toVector3d(frameCorner2 - b2 * L1F),
                                  end,
                                  end - pi - angle123,
                                  radiusFrame,
                                  isConstructionMode());
            addArcToShapeGeometry(toVector3d(frameCorner3 - b1 * L2F),
                                  end + angle412,
                                  end - pi,
                                  radiusFrame,
                                  isConstructionMode());
            addArcToShapeGeometry(toVector3d(frameCorner4 + b2 * L1F),
                                  end - pi,
                                  end - angle123,
                                  radiusFrame,
                                  isConstructionMode());
        }
    }


    void finishOblongCreation(bool thicknessNotZero, bool negThicknessEqualRadius)
    {
        addTangentCoincidences(firstCurve);

        addAlignmentConstraints();

        addArcEqualities();

        if (thicknessNotZero) {
            // There are 3 cases possible:
            // 1 - Thickness is negative and -thickness == radius.
            // In this case the inner rectangle is a normal rectangle and its corner
            // match the centers of the outer arcs.
            // 2 - Thickness is negative and  radius < -thickness.
            // In this case it's a normal rectangle but we need construction
            // lines to constraint it.
            // 3 - Thickness is either positive or negative and radius > -thickness.
            // In this case the second rectangle is also round-cornered
            if (radiusFrame < Precision::Confusion()) {
                addRectangleCoincidences(firstCurve + 8);  // NOLINT

                // Case 1
                if (negThicknessEqualRadius) {
                    finishOblongFrameCase1();
                }
                else {
                    finishOblongFrameCase2();
                }
            }
            else {  // case 3: inner rectangle is rounded rectangle
                finishOblongFrameCase3();
            }
        }

        if (constructionMethod() == ConstructionMethod::ThreePoints) {
            finishOblongThreePoints(thicknessNotZero, negThicknessEqualRadius);
        }
        else if (constructionMethod() == ConstructionMethod::CenterAnd3Points) {
            finishOblongCenterAnd3Points(thicknessNotZero, negThicknessEqualRadius);
        }
        else if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
            finishOblongCenterAndCorner(thicknessNotZero, negThicknessEqualRadius);
        }
        else {
            finishOblongDiagonal(thicknessNotZero, negThicknessEqualRadius);
        }
    }

    void finishOblongFrameCase1()
    {
        // In this case the corners of the second rectangle are coincident
        // with the centers of the arcs of the first rectangle.
        addToShapeConstraints(Sketcher::Coincident,
                              firstCurve + 8,  // NOLINT
                              Sketcher::PointPos::start,
                              firstCurve + 4,  // NOLINT
                              Sketcher::PointPos::mid);
        addToShapeConstraints(Sketcher::Coincident,
                              firstCurve + 9,  // NOLINT
                              Sketcher::PointPos::start,
                              firstCurve + 5,  // NOLINT
                              Sketcher::PointPos::mid);
        addToShapeConstraints(Sketcher::Coincident,
                              firstCurve + 10,  // NOLINT
                              Sketcher::PointPos::start,
                              firstCurve + 6,  // NOLINT
                              Sketcher::PointPos::mid);
        addToShapeConstraints(Sketcher::Coincident,
                              firstCurve + 11,  // NOLINT
                              Sketcher::PointPos::start,
                              firstCurve + 7,  // NOLINT
                              Sketcher::PointPos::mid);
    }

    void finishOblongFrameCase2()
    {
        // case 2: add construction lines +12, +13, +14, +15

        addFrameAlignmentConstraints(firstCurve + 8);

        addLineToShapeGeometry(center1, Base::Vector3d(frameCorner1.x, frameCorner1.y, 0.), true);
        addLineToShapeGeometry(center2, Base::Vector3d(frameCorner2.x, frameCorner2.y, 0.), true);
        addLineToShapeGeometry(center3, Base::Vector3d(frameCorner3.x, frameCorner3.y, 0.), true);
        addLineToShapeGeometry(center4, Base::Vector3d(frameCorner4.x, frameCorner4.y, 0.), true);

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

    void finishOblongFrameCase3()
    {
        addTangentCoincidences(firstCurve + 8);

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

        addFrameAlignmentConstraints(firstCurve + 8, false);
    }

    void finishOblongThreePoints(bool thicknessNotZero, bool negThicknessEqualRadius)
    {
        if (thicknessNotZero) {
            if (negThicknessEqualRadius) {
                constructionPointOneId = firstCurve + 12;    // NOLINT
                constructionPointTwoId = firstCurve + 13;    // NOLINT
                constructionPointThreeId = firstCurve + 14;  // NOLINT
            }
            else {
                constructionPointOneId = firstCurve + 16;    // NOLINT
                constructionPointTwoId = firstCurve + 17;    // NOLINT
                constructionPointThreeId = firstCurve + 18;  // NOLINT
            }
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

    void finishOblongCenterAnd3Points(bool thicknessNotZero, bool negThicknessEqualRadius)
    {
        if (thicknessNotZero) {
            if (negThicknessEqualRadius) {
                constructionPointOneId = firstCurve + 12;  // NOLINT
                constructionPointTwoId = firstCurve + 13;  // NOLINT
                centerPointId = firstCurve + 14;           // NOLINT
            }
            else {
                constructionPointOneId = firstCurve + 16;  // NOLINT
                constructionPointTwoId = firstCurve + 17;  // NOLINT
                centerPointId = firstCurve + 18;           // NOLINT
            }
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

    void finishOblongCenterAndCorner(bool thicknessNotZero, bool negThicknessEqualRadius)
    {
        if (thicknessNotZero) {
            if (negThicknessEqualRadius) {
                constructionPointOneId = firstCurve + 12;  // NOLINT
                centerPointId = firstCurve + 13;           // NOLINT
            }
            else {
                constructionPointOneId = firstCurve + 16;  // NOLINT
                centerPointId = firstCurve + 17;           // NOLINT
            }
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

    void finishOblongDiagonal(bool thicknessNotZero, bool negThicknessEqualRadius)
    {
        if (thicknessNotZero) {
            if (negThicknessEqualRadius) {
                constructionPointOneId = firstCurve + 12;  // NOLINT
                constructionPointTwoId = firstCurve + 13;  // NOLINT
            }
            else {
                constructionPointOneId = firstCurve + 16;  // NOLINT
                constructionPointTwoId = firstCurve + 17;  // NOLINT
            }
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

    void addTangentCoincidences(int geoId)
    {
        addToShapeConstraints(Sketcher::Tangent,
                              geoId,
                              Sketcher::PointPos::start,
                              geoId + 4,  // NOLINT
                              Sketcher::PointPos::end);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId,
                              Sketcher::PointPos::end,
                              geoId + 5,  // NOLINT
                              Sketcher::PointPos::start);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId + 1,  // NOLINT
                              Sketcher::PointPos::start,
                              geoId + 5,  // NOLINT
                              Sketcher::PointPos::end);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId + 1,  // NOLINT
                              Sketcher::PointPos::end,
                              geoId + 6,  // NOLINT
                              Sketcher::PointPos::start);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId + 2,  // NOLINT
                              Sketcher::PointPos::start,
                              geoId + 6,  // NOLINT
                              Sketcher::PointPos::end);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId + 2,  // NOLINT
                              Sketcher::PointPos::end,
                              geoId + 7,  // NOLINT
                              Sketcher::PointPos::start);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId + 3,  // NOLINT
                              Sketcher::PointPos::start,
                              geoId + 7,  // NOLINT
                              Sketcher::PointPos::end);
        addToShapeConstraints(Sketcher::Tangent,
                              geoId + 3,  // NOLINT
                              Sketcher::PointPos::end,
                              geoId + 4,  // NOLINT
                              Sketcher::PointPos::start);
    }

    void addArcEqualities()
    {
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
    }

    void finishRectangleCreation(bool thicknessNotZero)
    {
        addRectangleCoincidences(firstCurve);

        addAlignmentConstraints();

        if (thicknessNotZero) {
            finishRectangleFrameCreation();
        }

        if (constructionMethod() == ConstructionMethod::CenterAndCorner
            || constructionMethod() == ConstructionMethod::CenterAnd3Points) {
            finishCenteredRectangleCreation(thicknessNotZero);
        }
    }

    void addRectangleCoincidences(int geoId)
    {
        addToShapeConstraints(Sketcher::Coincident,
                              geoId,
                              Sketcher::PointPos::end,
                              geoId + 1,
                              Sketcher::PointPos::start);
        addToShapeConstraints(Sketcher::Coincident,
                              geoId + 1,
                              Sketcher::PointPos::end,
                              geoId + 2,
                              Sketcher::PointPos::start);
        addToShapeConstraints(Sketcher::Coincident,
                              geoId + 2,
                              Sketcher::PointPos::end,
                              geoId + 3,
                              Sketcher::PointPos::start);
        addToShapeConstraints(Sketcher::Coincident,
                              geoId + 3,
                              Sketcher::PointPos::end,
                              geoId,
                              Sketcher::PointPos::start);
    }

    void addAlignmentConstraints()
    {
        Sketcher::ConstraintType typeA = Sketcher::Horizontal;
        Sketcher::ConstraintType typeB = Sketcher::Vertical;
        if (Base::sgn(corner3.x - corner1.x) * Base::sgn(corner3.y - corner1.y) < 0) {
            typeA = Sketcher::Vertical;
            typeB = Sketcher::Horizontal;
        }

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
            if (fabs(angle123 - std::numbers::pi / 2) < Precision::Confusion()) {
                addToShapeConstraints(Sketcher::Perpendicular,
                                      firstCurve,
                                      Sketcher::PointPos::none,
                                      firstCurve + 1);
            }
        }
    }

    void finishRectangleFrameCreation()
    {
        addRectangleCoincidences(firstCurve + 4);

        addFrameAlignmentConstraints(firstCurve + 4);

        addRectangleFrameConstructionLines();
    }

    void addFrameAlignmentConstraints(int geoId, bool addLast = true)
    {
        Sketcher::ConstraintType typeA = Sketcher::Horizontal;
        Sketcher::ConstraintType typeB = Sketcher::Vertical;
        if (Base::sgn(corner3.x - corner1.x) * Base::sgn(corner3.y - corner1.y) < 0) {
            typeA = Sketcher::Vertical;
            typeB = Sketcher::Horizontal;
        }

        if (fabs(angle) < Precision::Confusion()
            || constructionMethod() == ConstructionMethod::Diagonal
            || constructionMethod() == ConstructionMethod::CenterAndCorner) {
            addToShapeConstraints(typeA, geoId);      // NOLINT
            addToShapeConstraints(typeA, geoId + 2);  // NOLINT
            addToShapeConstraints(typeB, geoId + 1);  // NOLINT
            if (addLast) {
                addToShapeConstraints(typeB, geoId + 3);  // NOLINT
            }
        }
        else {
            addToShapeConstraints(Sketcher::Parallel,
                                  geoId,  // NOLINT
                                  Sketcher::PointPos::none,
                                  geoId + 2);  // NOLINT
            addToShapeConstraints(Sketcher::Parallel,
                                  geoId + 1,  // NOLINT
                                  Sketcher::PointPos::none,
                                  geoId + 3);  // NOLINT
            addToShapeConstraints(Sketcher::Parallel,
                                  firstCurve,
                                  Sketcher::PointPos::none,
                                  geoId);  // NOLINT
            if (addLast) {
                addToShapeConstraints(Sketcher::Parallel,
                                      firstCurve + 1,  // NOLINT
                                      Sketcher::PointPos::none,
                                      geoId + 1);  // NOLINT
            }
        }
    }

    void addRectangleFrameConstructionLines()
    {
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

    void finishCenteredRectangleCreation(bool thicknessNotZero)
    {
        if (thicknessNotZero) {
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
    }
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerRectangle_H
