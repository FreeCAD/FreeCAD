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


#ifndef SKETCHERGUI_DrawSketchHandlerPolygon_H
#define SKETCHERGUI_DrawSketchHandlerPolygon_H

#include <QApplication>

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

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

using DSHPolygonController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerPolygon,
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

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, centerPoint);

                firstCorner = onSketchPos;

                CreateAndDrawShapeGeometry();

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
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
            Gui::Command::doCommand(Gui::Command::Doc,
                                    "import ProfileLib.RegularPolygon\n"
                                    "ProfileLib.RegularPolygon.makeRegularPolygon(%s,%i,App.Vector("
                                    "%f,%f,0),App.Vector(%f,%f,0),%s)",
                                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),
                                    numberOfCorners,
                                    centerPoint.x,
                                    centerPoint.y,
                                    firstCorner.x,
                                    firstCorner.y,
                                    constructionModeAsBooleanText());

            Gui::Command::commitCommand();

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(sketchgui,
                             QT_TRANSLATE_NOOP("Notifications", "Error"),
                             QT_TRANSLATE_NOOP("Notifications", "Failed to add polygon"));

            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError,
                   QT_TRANSLATE_NOOP(
                       "Notifications",
                       "Tool execution aborted") "\n")  // This prevents constraints from being
                                                        // applied on non existing geometry
        }
    }

    void generateAutoConstraints() override
    {
        // add auto constraints at the center of the polygon
        int circlegeoid = getHighestCurveIndex();
        int lastsidegeoid = getHighestCurveIndex() - 1;
        if (sugConstraints[0].size() > 0) {
            generateAutoConstraintsOnElement(sugConstraints[0],
                                             circlegeoid,
                                             Sketcher::PointPos::mid);
        }

        // add auto constraints to the last side of the polygon
        if (sugConstraints[1].size() > 0) {
            generateAutoConstraintsOnElement(sugConstraints[1],
                                             lastsidegeoid,
                                             Sketcher::PointPos::end);
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
        return QString::fromLatin1("Sketcher_Pointer_Regular_Polygon");
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
        return QString(QObject::tr("Polygon parameters"));
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

        double angleOfSeparation = 2.0 * M_PI / static_cast<double>(numberOfCorners);  // NOLINT
        double cos_v = cos(angleOfSeparation);
        double sin_v = sin(angleOfSeparation);

        double rx = dV.x;
        double ry = dV.y;

        for (int i = 1; i <= static_cast<int>(numberOfCorners); i++) {
            const double old_rx = rx;
            rx = cos_v * rx - sin_v * ry;
            ry = cos_v * ry + sin_v * old_rx;
            Base::Vector2d newCorner = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
            addLineToShapeGeometry(toVector3d(prevCorner),
                                   toVector3d(newCorner),
                                   isConstructionMode());
            prevCorner = newCorner;
        }
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
    toolWidget->setParameterWithoutPassingFocus(OnViewParameter::First, value + 1);
}

template<>
void DSHPolygonController::secondKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    if (value > 3.0) {  // NOLINT
        toolWidget->setParameterWithoutPassingFocus(OnViewParameter::First, value - 1);
    }
}

template<>
void DSHPolygonController::configureToolWidget()
{

    toolWidget->setParameterLabel(OnViewParameter::First,
                                  QApplication::translate("ToolWidgetManager_p4", "Sides 'U'/'J'"));
    toolWidget->setParameter(OnViewParameter::First,
                             handler->numberOfCorners);  // unconditionally set
    toolWidget->configureParameterUnit(OnViewParameter::First, Base::Unit());
    toolWidget->configureParameterMin(OnViewParameter::First, 3.0);  // NOLINT
    // We set a reasonable max to avoid the spinbox from being very large
    toolWidget->configureParameterMax(OnViewParameter::First, 9999.0);  // NOLINT
    toolWidget->configureParameterDecimals(OnViewParameter::First, 0);

    onViewParameters[OnViewParameter::First]->setLabelType(Gui::SoDatumLabel::DISTANCEX);
    onViewParameters[OnViewParameter::Second]->setLabelType(Gui::SoDatumLabel::DISTANCEY);
    onViewParameters[OnViewParameter::Third]->setLabelType(
        Gui::SoDatumLabel::DISTANCE,
        Gui::EditableDatumLabel::Function::Dimensioning);
    onViewParameters[OnViewParameter::Fourth]->setLabelType(
        Gui::SoDatumLabel::ANGLE,
        Gui::EditableDatumLabel::Function::Dimensioning);
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
            if (onViewParameters[OnViewParameter::First]->isSet) {
                onSketchPos.x = onViewParameters[OnViewParameter::First]->getValue();
            }

            if (onViewParameters[OnViewParameter::Second]->isSet) {
                onSketchPos.y = onViewParameters[OnViewParameter::Second]->getValue();
            }
        } break;
        case SelectMode::SeekSecond: {
            Base::Vector2d dir = onSketchPos - handler->centerPoint;
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

                onSketchPos = handler->centerPoint + length * dir.Normalize();
            }

            if (onViewParameters[OnViewParameter::Fourth]->isSet) {
                double angle =
                    Base::toRadians(onViewParameters[OnViewParameter::Fourth]->getValue());
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
            Base::Vector3d start = toVector3d(handler->centerPoint);
            Base::Vector3d end = toVector3d(handler->firstCorner);
            Base::Vector3d vec = end - start;

            if (!onViewParameters[OnViewParameter::Third]->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, vec.Length());
            }

            double range = (handler->firstCorner - handler->centerPoint).Angle();
            if (!onViewParameters[OnViewParameter::Fourth]->isSet) {
                setOnViewParameterValue(OnViewParameter::Fourth,
                                        Base::toDegrees(range),
                                        Base::Unit::Angle);
            }

            onViewParameters[OnViewParameter::Third]->setPoints(start, end);
            onViewParameters[OnViewParameter::Fourth]->setPoints(start, Base::Vector3d());
            onViewParameters[OnViewParameter::Fourth]->setLabelRange(range);

        } break;
        default:
            break;
    }
}

template<>
void DSHPolygonController::doChangeDrawSketchHandlerMode()
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

                handler->setState(SelectMode::End);
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
        ConstraintToAttachment(GeoElementId(lastCurve, PointPos::mid),
                               GeoElementId::VAxis,
                               x0,
                               handler->sketchgui->getObject());
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(lastCurve, PointPos::mid),
                               GeoElementId::HAxis,
                               y0,
                               handler->sketchgui->getObject());
    };

    auto constraintradius = [&]() {
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(),
                              "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                              lastCurve,
                              radius);
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
                GeoElementId(lastCurve, PointPos::mid));  // get updated point position
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


#endif  // SKETCHERGUI_DrawSketchHandlerPolygon_H
