// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/InputHint.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "GeometryCreationMode.h"
#include "Utils.h"

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "SketcherRegularPolygonDialog.h"

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerPolygon;

using DSHPolygonController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerPolygon,
    StateMachines::TwoSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<4>,
    /*WidgetParametersT =*/WidgetParameters<1>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

using DSHPolygonControllerBase = DSHPolygonController::ControllerBase;

using DrawSketchHandlerPolygonBase = DrawSketchControllableHandler<DSHPolygonController>;


class DrawSketchHandlerPolygon: public DrawSketchHandlerPolygonBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerPolygon)

    friend DSHPolygonController;
    friend DSHPolygonControllerBase;

public:
    explicit DrawSketchHandlerPolygon(int corners = 6)  // NOLINT
        : numberOfCorners(corners)
        , radius(0.0)
    {}
    ~DrawSketchHandlerPolygon() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                centerPoint = onSketchPos;

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, centerPoint);

                firstCorner = onSketchPos;

                CreateAndDrawShapeGeometry();

                seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        unsetCursor();
        resetPositionText();
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add polygon"));

        try {
            Gui::Command::doCommand(
                Gui::Command::Doc,
                "import ProfileLib.RegularPolygon\n"
                "ProfileLib.RegularPolygon.makeRegularPolygon(%s,%i,App.Vector("
                "%f,%f,0),App.Vector(%f,%f,0),%s)",
                Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),
                numberOfCorners,
                centerPoint.x,
                centerPoint.y,
                firstCorner.x,
                firstCorner.y,
                constructionModeAsBooleanText()
            );

            Gui::Command::commitCommand();

            tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add polygon")
            );

            Gui::Command::abortCommand();
            THROWM(
                Base::RuntimeError,
                QT_TRANSLATE_NOOP(
                    "Notifications",
                    "Tool execution aborted"
                ) "\n"
            )  // This prevents constraints from being
               // applied on non existing geometry
        }
    }

    void generateAutoConstraints() override
    {
        // add auto constraints at the center of the polygon
        int circlegeoid = getHighestCurveIndex();
        int lastsidegeoid = getHighestCurveIndex() - 1;
        if (sugConstraints[0].size() > 0) {
            generateAutoConstraintsOnElement(sugConstraints[0], circlegeoid, Sketcher::PointPos::mid);
        }

        // add auto constraints to the last side of the polygon
        if (sugConstraints[1].size() > 0) {
            generateAutoConstraintsOnElement(sugConstraints[1], lastsidegeoid, Sketcher::PointPos::end);
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
        return "DSH_Polygon";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Regular_Polygon");
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreateRegularPolygon");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Polygon Parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekSecond && radius < Precision::Confusion()) {
            // Prevent validation of null shape.
            return false;
        }

        return true;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond) {
            setAngleSnapping(true, centerPoint);
        }

        else {
            setAngleSnapping(false);
        }
    }

private:
    unsigned int numberOfCorners;
    Base::Vector2d centerPoint, firstCorner;
    double radius;

    void createShape(bool onlyeditoutline) override
    {
        Q_UNUSED(onlyeditoutline)

        ShapeGeometry.clear();

        Base::Vector2d prevCorner = firstCorner;
        Base::Vector2d dV = firstCorner - centerPoint;
        radius = dV.Length();

        if (radius < Precision::Confusion()) {
            return;
        }

        double angleOfSeparation = 2.0 * std::numbers::pi
            / static_cast<double>(numberOfCorners);  // NOLINT
        double cos_v = cos(angleOfSeparation);
        double sin_v = sin(angleOfSeparation);

        double rx = dV.x;
        double ry = dV.y;

        for (int i = 1; i <= static_cast<int>(numberOfCorners); i++) {
            const double old_rx = rx;
            rx = cos_v * rx - sin_v * ry;
            ry = cos_v * ry + sin_v * old_rx;
            Base::Vector2d newCorner = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
            addLineToShapeGeometry(toVector3d(prevCorner), toVector3d(newCorner), isConstructionMode());
            prevCorner = newCorner;
        }
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return Gui::lookupHints<SelectMode>(
            state(),
            {
                {.state = SelectMode::SeekFirst,
                 .hints =
                     {
                         {tr("%1 pick polygon center"), {MouseLeft}},
                         {tr("%1/%2 increase / decrease number of sides"), {KeyU, KeyJ}},
                     }},
                {.state = SelectMode::SeekSecond,
                 .hints =
                     {
                         {tr("%1 pick rotation and size"), {MouseMove}},
                         {tr("%1 confirm"), {MouseLeft}},
                         {tr("%1/%2 increase / decrease number of sides"), {KeyU, KeyJ}},
                     }},
            });
    }
};

template<>
auto DSHPolygonControllerBase::getState(int labelindex) const
{
    switch (labelindex) {
        case OnViewParameter::First:
        case OnViewParameter::Second:
            return SelectMode::SeekFirst;
            break;
        case OnViewParameter::Third:
        case OnViewParameter::Fourth:
            return SelectMode::SeekSecond;
            break;
        default:
            THROWM(Base::ValueError, "OnViewParameter index without an associated machine state")
    }
}

template<>
void DSHPolygonController::firstKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    toolWidget->setParameterWithoutPassingFocus(WParameter::First, value + 1);
}

template<>
void DSHPolygonController::secondKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    if (value > 3.0) {  // NOLINT
        toolWidget->setParameterWithoutPassingFocus(WParameter::First, value - 1);
    }
}

template<>
void DSHPolygonController::configureToolWidget()
{

    toolWidget->setParameterLabel(
        WParameter::First,
        QApplication::translate("ToolWidgetManager_p4", "Sides (+'U'/ -'J')")
    );
    toolWidget->setParameter(WParameter::First,
                             handler->numberOfCorners);  // unconditionally set
    toolWidget->configureParameterUnit(WParameter::First, Base::Unit());
    toolWidget->configureParameterMin(WParameter::First, 3.0);  // NOLINT
    // We set a reasonable max to avoid the spinbox from being very large
    toolWidget->configureParameterMax(WParameter::First, 9999.0);  // NOLINT
    toolWidget->configureParameterDecimals(WParameter::First, 0);

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning
    );
}

template<>
void DSHPolygonController::adaptDrawingToParameterChange(int parameterindex, double value)
{
    switch (parameterindex) {
        case WParameter::First:
            handler->numberOfCorners = std::max(3, static_cast<int>(value));
            break;
    }
}

template<>
void DSHPolygonControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->isSet) {
                onSketchPos.x = firstParam->getValue();
            }

            if (secondParam->isSet) {
                onSketchPos.y = secondParam->getValue();
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector2d dir = onSketchPos - handler->centerPoint;
            if (dir.Length() < Precision::Confusion()) {
                dir.x = 1.0;  // if direction null, default to (1,0)
            }
            double length = dir.Length();

            if (thirdParam->isSet) {
                length = thirdParam->getValue();
                if (length < Precision::Confusion() && thirdParam->hasFinishedEditing) {
                    unsetOnViewParameter(thirdParam.get());
                    return;
                }

                onSketchPos = handler->centerPoint + length * dir.Normalize();
            }

            if (fourthParam->isSet) {
                double angle = Base::toRadians(fourthParam->getValue());
                onSketchPos.x = handler->centerPoint.x + cos(angle) * length;
                onSketchPos.y = handler->centerPoint.y + sin(angle) * length;
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPolygonController::adaptParameters(Base::Vector2d onSketchPos)
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (!firstParam->isSet) {
                setOnViewParameterValue(OnViewParameter::First, onSketchPos.x);
            }

            if (!secondParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Second, onSketchPos.y);
            }

            bool sameSign = onSketchPos.x * onSketchPos.y > 0.;
            firstParam->setLabelAutoDistanceReverse(!sameSign);
            secondParam->setLabelAutoDistanceReverse(sameSign);
            firstParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
            secondParam->setPoints(Base::Vector3d(), toVector3d(onSketchPos));
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            Base::Vector3d start = toVector3d(handler->centerPoint);
            Base::Vector3d end = toVector3d(handler->firstCorner);
            Base::Vector3d vec = end - start;

            if (!thirdParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, vec.Length());
            }

            double range = (handler->firstCorner - handler->centerPoint).Angle();
            if (!fourthParam->isSet) {
                setOnViewParameterValue(
                    OnViewParameter::Fourth,
                    Base::toDegrees(range),
                    Base::Unit::Angle
                );
            }

            thirdParam->setPoints(start, end);
            fourthParam->setPoints(start, Base::Vector3d());
            fourthParam->setLabelRange(range);

        } break;
        default:
            break;
    }
}

template<>
void DSHPolygonController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing && secondParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::SeekSecond);
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing) {
                handler->setNextState(SelectMode::End);
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHPolygonController::addConstraints()
{
    int lastCurve = handler->getHighestCurveIndex();

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();
    auto radius = onViewParameters[OnViewParameter::Third]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;
    auto radiusSet = onViewParameters[OnViewParameter::Third]->isSet;

    using namespace Sketcher;

    auto constraintx0 = [&]() {
        ConstraintToAttachment(
            GeoElementId(lastCurve, PointPos::mid),
            GeoElementId::VAxis,
            x0,
            handler->sketchgui->getObject()
        );
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(
            GeoElementId(lastCurve, PointPos::mid),
            GeoElementId::HAxis,
            y0,
            handler->sketchgui->getObject()
        );
    };

    auto constraintradius = [&]() {
        Gui::cmdAppObjectArgs(
            handler->sketchgui->getObject(),
            "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            lastCurve,
            radius
        );
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

        if (radiusSet) {
            constraintradius();
        }
    }
    else {  // There is a valid diagnose.
        auto startpointinfo = handler->getPointInfo(GeoElementId(lastCurve, PointPos::mid));

        // if Autoconstraints is empty we do not have a diagnosed system and the parameter will
        // always be set
        if (x0set && startpointinfo.isXDoF()) {
            constraintx0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition

            startpointinfo = handler->getPointInfo(
                GeoElementId(lastCurve, PointPos::mid)
            );  // get updated point position
        }

        // if Autoconstraints is empty we do not have a diagnosed system and the parameter will
        // always be set
        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();

            handler->diagnoseWithAutoConstraints();  // ensure we have recalculated parameters after
                                                     // each constraint addition
        }

        auto edgeinfo = handler->getEdgeInfo(lastCurve);
        auto circle = static_cast<SolverGeometryExtension::Circle&>(edgeinfo);

        // if Autoconstraints is empty we do not have a diagnosed system and the parameter will
        // always be set
        if (radiusSet && circle.isRadiusDoF()) {
            constraintradius();
        }
    }
}

}  // namespace SketcherGui
