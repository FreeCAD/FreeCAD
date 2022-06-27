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


#ifndef SKETCHERGUI_DrawSketchHandlerEllipse_H
#define SKETCHERGUI_DrawSketchHandlerEllipse_H

#include <cmath>

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

#include "CircleEllipseConstructionMethod.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

/* Ellipse ==============================================================================*/
class DrawSketchHandlerEllipse;

using DrawSketchHandlerEllipseBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerEllipse,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerEllipse : public DrawSketchHandlerEllipseBase
{
    friend DrawSketchHandlerEllipseBase;

public:
    DrawSketchHandlerEllipse(ConstructionMethod constrMethod = ConstructionMethod::Center) :
        DrawSketchHandlerEllipseBase(constrMethod) {}
    virtual ~DrawSketchHandlerEllipse() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            if (constructionMethod() == ConstructionMethod::Center) {
                centerPoint = onSketchPos;
                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            else {
                apoapsis = onSketchPos;
                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            periapsis = onSketchPos;

            calculateMajorAxisParameters();

            try {
                CreateAndDrawShapeGeometry();
            }
            catch(const Base::ValueError &) {} // avoid polluting the error console with drawing exceptions.

            SbString text;
            double angle = GetPointAngle(centerPoint, onSketchPos);
            text.sprintf(" (%.1fR,%.1fdeg)", (float)firstRadius, (float)angle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            calculateThroughPointMinorAxisParameters(onSketchPos);

            try {
                CreateAndDrawShapeGeometry();
            }
            catch(const Base::ValueError &) {} // avoid polluting the error console with drawing exceptions.

            SbString text;
            text.sprintf(" (%.1fR,%.1fR)", (float)majorRadius, (float)minorRadius);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstraints[2]);
                return;
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch ellipse"));

            ellipseGeoId = getHighestCurveIndex() + 1;

            createShape(false);

            commandAddShapeGeometryAndConstraints();

            // in the exceptional event that this may lead to a circle, do not exposeInternalGeometry
            if(!ShapeGeometry.empty() && ShapeGeometry[0]->getTypeId() == Part::GeomEllipse::getClassTypeId())
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", ellipseGeoId);

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add ellipse: %s\n", e.what());
            Gui::Command::abortCommand();
            THROWM(Base::RuntimeError, "Tool execution aborted\n") // This prevents constraints from being applied on non existing geometry
        }
    }

    virtual void generateAutoConstraints() override {

        if (constructionMethod() == ConstructionMethod::Center) {

            auto & ac1 = sugConstraints[0];
            auto & ac2 = sugConstraints[1];
            auto & ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(ac1, ellipseGeoId, Sketcher::PointPos::mid);    // add auto constraints for the center point
            generateAutoConstraintsOnElement(ac2, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the edge
            generateAutoConstraintsOnElement(ac3, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the edge
        }
        else {

            auto & ac1 = sugConstraints[0];
            auto & ac2 = sugConstraints[1];
            auto & ac3 = sugConstraints[2];

            generateAutoConstraintsOnElement(ac1, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the first point
            generateAutoConstraintsOnElement(ac2, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the second point
            generateAutoConstraintsOnElement(ac3, ellipseGeoId, Sketcher::PointPos::none);   // add auto constraints for the edge
        }
        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry parameters are accurate
        // This is particularly important for adding widget mandated constraints.
        removeRedundantAutoConstraints();
    }

    virtual void createAutoConstraints() override {
        // execute python command to create autoconstraints
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
        sugConstraints[2].clear();
    }

    virtual void createShape(bool onlyeditoutline) override {
        Q_UNUSED(onlyeditoutline);

        ShapeGeometry.clear();

        Base::Vector2d majorAxis = firstAxis;
        majorRadius = firstRadius;

        if( state() == SelectMode::SeekSecond) {
            addEllipseToShapeGeometry(toVector3d(centerPoint), toVector3d(majorAxis), majorRadius, majorRadius*0.5, geometryCreationMode);
        }
        else { // SelectMode::SeekThird or SelectMode::End
            minorRadius = secondRadius;

            if (secondRadius > firstRadius) {
                majorAxis = secondAxis;
                majorRadius = secondRadius;
                minorRadius = firstRadius;
            }

            if (fabs(firstRadius - secondRadius) < Precision::Confusion())
                addCircleToShapeGeometry(toVector3d(centerPoint), firstRadius, geometryCreationMode);
            else
                addEllipseToShapeGeometry(toVector3d(centerPoint), toVector3d(majorAxis), majorRadius, minorRadius, geometryCreationMode);
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Ellipse";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center)
            return QString::fromLatin1("Sketcher_Pointer_Create_Ellipse");
        else // constructionMethod == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointEllipse");
    }

    void calculateMajorAxisParameters() {

        if (constructionMethod() == ConstructionMethod::ThreeRim)
            centerPoint = (apoapsis - periapsis) / 2 + periapsis;

        firstAxis = periapsis - centerPoint;
        firstRadius = firstAxis.Length();
    }

    void calculateThroughPointMinorAxisParameters(const Base::Vector2d & onSketchPos) {
        // we calculate the ellipse that will pass via the cursor as per de la Hire

        Base::Vector2d projx;
        projx.ProjectToLine(onSketchPos - centerPoint, firstAxis); // projection onto the major axis

        auto projy = onSketchPos - centerPoint - projx;

        auto lprojx = projx.Length(); // Px = a cos t
        auto lprojy = projy.Length(); // Py = b sin t

        double t = std::acos(lprojx/firstRadius);

        secondRadius = lprojy / std::sin(t); // b = Py / sin t

        secondAxis = projy.Normalize() * secondRadius;
    }

    void calculateMinorAxis(double minorradius) {
        //Find bPoint For that first we need the distance of onSketchPos to major axis.
        secondAxis = firstAxis.Perpendicular(false).Normalize() * minorRadius;
        secondRadius = minorradius;
    }

private:
    Base::Vector2d centerPoint, periapsis; // Center Mode SeekFirst and SeekSecond, 3PointMode SeekFirst
    Base::Vector2d apoapsis; // 3Point SeekSecond
    Base::Vector2d firstAxis, secondAxis;
    double firstRadius, secondRadius, majorRadius, minorRadius;
    int ellipseGeoId;
};

template <> auto DrawSketchHandlerEllipseBase::ToolWidgetManager::getState(int parameterindex) const {

    switch (parameterindex) {
    case WParameter::First:
    case WParameter::Second:
        return SelectMode::SeekFirst;
        break;
    case WParameter::Third:
    case WParameter::Fourth:
        return SelectMode::SeekSecond;
        break;
    case WParameter::Fifth:
        return SelectMode::SeekThird;
        break;
    case WParameter::Sixth:
        if(handler->constructionMethod() == ConstructionMethod::ThreeRim)
            return SelectMode::SeekThird;
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
        break;
    default:
        THROWM(Base::ValueError, "Parameter index without an associated machine state")
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::configureToolWidget() {

    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Center"), QStringLiteral("Axis endpoints and radius")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        syncConstructionMethodComboboxToHandler(); // in case the DSH was called with a specific construction method
    }

    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_ellipse", "x of center"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_ellipse", "y of center"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_ellipse", "First radius"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p3_ellipse", "Angle to HAxis"));
        toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p3_ellipse", "Second radius"));
    }
    else { // ThreeRim
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of first point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of first point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of second point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of second point"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of third point"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of third point"));
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->centerPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->centerPoint.y = value;
            break;
        case WParameter::Third:
        {
            auto radius = value;
            auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;

            dHandler->periapsis = dHandler->centerPoint + Base::Vector2d::FromPolar(radius, angle);

            dHandler->calculateMajorAxisParameters();
            break;
        }
        case WParameter::Fourth:
        {
            auto angle = value * M_PI / 180;
            auto radius = toolWidget->getParameter(WParameter::Third);

            dHandler->periapsis = dHandler->centerPoint + Base::Vector2d::FromPolar(radius, angle);

            dHandler->calculateMajorAxisParameters();
            break;
        }
        case WParameter::Fifth:
        {
             auto minorradius = value;

             dHandler->calculateMinorAxis(minorradius);
        }
        }

    }
    else { // ThreeRim
        switch (parameterindex) {
        case WParameter::First:
            dHandler->apoapsis.x = value;
            break;
        case WParameter::Second:
            dHandler->apoapsis.y = value;
            break;
        case WParameter::Third:
            dHandler->periapsis.x = value;
            dHandler->calculateMajorAxisParameters();
            break;
        case WParameter::Fourth:
            dHandler->periapsis.y = value;
            dHandler->calculateMajorAxisParameters();
            break;
        case WParameter::Fifth:
        {
            auto px = value;
            auto py = toolWidget->getParameter(WParameter::Sixth);
            dHandler->calculateThroughPointMinorAxisParameters(Base::Vector2d(px,py));
            break;
        }
        case WParameter::Sixth:
        {
            auto px = toolWidget->getParameter(WParameter::Fifth);
            auto py = value;
            dHandler->calculateThroughPointMinorAxisParameters(Base::Vector2d(px,py));
            break;
        }

        }
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            double length = (onSketchPos - dHandler->centerPoint).Length();
            if (toolWidget->isParameterSet(WParameter::Third)) {
                length = toolWidget->getParameter(WParameter::Third);
                if (length < Precision::Confusion())
                    return;

                Base::Vector2d v = onSketchPos - dHandler->centerPoint;
                if( v.x < Precision::Confusion() && v.y < Precision::Confusion())
                    v.x = 1; // if direction cannot be determined, default to (1,0)

                onSketchPos = dHandler->centerPoint + v * length / v.Length();
            }

            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                onSketchPos.x = dHandler->centerPoint.x + cos(angle) * length;
                onSketchPos.y = dHandler->centerPoint.y + sin(angle) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third))
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);

            if (toolWidget->isParameterSet(WParameter::Fourth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                auto minorradius = toolWidget->getParameter(WParameter::Fifth);
                onSketchPos = dHandler->centerPoint +(dHandler->periapsis - dHandler->centerPoint).Perpendicular(true).Normalize() * minorradius;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth))
                onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

            if (toolWidget->isParameterSet(WParameter::Sixth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {

            auto v = onSketchPos - dHandler->centerPoint;
            if (!toolWidget->isParameterSet(WParameter::Third)) {
                toolWidget->updateVisualValue(WParameter::Third, v.Length());
            }

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, v.Length() > 0 ? v.Angle() * 180 / M_PI:0, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            auto v = onSketchPos - dHandler->centerPoint;
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, v.Length());
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) &&
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->setState(SelectMode::SeekThird);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {

                handler->setState(SelectMode::End);
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth) &&
                toolWidget->isParameterSet(WParameter::Sixth)) {

                handler->setState(SelectMode::End);
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::addConstraints() {
    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        int firstCurve = dHandler->ellipseGeoId;

        auto x0 = toolWidget->getParameter(WParameter::First);
        auto y0 = toolWidget->getParameter(WParameter::Second);
        auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;

        auto x0set = toolWidget->isParameterSet(WParameter::First);
        auto y0set = toolWidget->isParameterSet(WParameter::Second);
        auto firstRadiusSet = toolWidget->isParameterSet(WParameter::Third);
        auto angleSet = toolWidget->isParameterSet(WParameter::Fourth);
        auto secondRadiusSet = toolWidget->isParameterSet(WParameter::Fifth);

        using namespace Sketcher;

        if(!handler->ShapeGeometry.empty() && handler->ShapeGeometry[0]->getTypeId() == Part::GeomEllipse::getClassTypeId()) {

            int firstLine = firstCurve + 1;     // this is always the major axis
            int secondLine = firstCurve + 2;    // this is always the minor axis

            // NOTE: Because mouse positions are enforced by the widget, it is not possible to use the last radius > first radius when
            // widget parameters are enforced. Then firstLine always goes with firstRadiusSet.

            auto constraintx0 = [&]() {
                ConstraintToAttachment(GeoElementId(firstCurve,PointPos::mid), GeoElementId::VAxis, x0, handler->sketchgui->getObject());
            };

            auto constrainty0 = [&]() {
                ConstraintToAttachment(GeoElementId(firstCurve,PointPos::mid), GeoElementId::HAxis, y0,  handler->sketchgui->getObject());
            };

            auto constraintFirstRadius = [&]() {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    firstCurve, 3, firstLine, 1, dHandler->firstRadius);
            };

            auto constraintSecondRadius = [&]() {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    firstCurve, 3, secondLine, 1, dHandler->secondRadius);
            };

            auto constraintAngle = [&]() {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                    firstLine, angle);
            };

            // NOTE: if AutoConstraints is empty, we can add constraints directly without any diagnose. No diagnose was run.
            if(handler->AutoConstraints.empty()) {
                if(x0set)
                    constraintx0();

                if(y0set)
                    constrainty0();


                //this require to show internal geometry.
                if (firstRadiusSet)
                    constraintFirstRadius();

                //Todo: this makes the ellipse 'jump' because it's doing a 180 degree turn before applying asked angle. Probably because start and end points of line are not in the correct direction.
                if (angleSet)
                    constraintAngle();

                if (secondRadiusSet)
                    constraintSecondRadius();

            }
            else { // There is a valid diagnose.
                auto centerpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter will always be set
                if(x0set && centerpointinfo.isXDoF()) {
                    constraintx0();

                    handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

                    centerpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid)); // get updated point position
                }

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter will always be set
                if(y0set && centerpointinfo.isYDoF()) {
                    constrainty0();

                    handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition
                }

                // Major axis (it is not a solver parameter in the solver implementation)

                int leftDoFs = handler->getLineDoFs(firstLine);

                if (firstRadiusSet && leftDoFs > 0) {
                    constraintFirstRadius();
                    handler->diagnoseWithAutoConstraints(); // It is not a normal line as it is constrained by the Ellipse, so we need to recalculate after radius addition
                    leftDoFs = handler->getLineDoFs(firstLine);
                }

                if (angleSet && leftDoFs > 0) {
                    constraintAngle();
                    handler->diagnoseWithAutoConstraints(); // It is not a normal line as it is constrained by the Ellipse, so we need to recalculate after radius addition
                }

                // Minor axis (it is a solver parameter in the solver implementation)

                auto edgeinfo = handler->getEdgeInfo(firstCurve);
                auto ellipse = static_cast<SolverGeometryExtension::Ellipse &>(edgeinfo);

                if(secondRadiusSet && ellipse.isMinorRadiusDoF()) {
                    constraintSecondRadius();
                }
            }
        }
        else { // it is a circle
            int firstCurve = handler->getHighestCurveIndex();

            auto x0 = toolWidget->getParameter(WParameter::First);
            auto y0 = toolWidget->getParameter(WParameter::Second);

            auto x0set = toolWidget->isParameterSet(WParameter::First);
            auto y0set = toolWidget->isParameterSet(WParameter::Second);
            auto radiusSet = toolWidget->isParameterSet(WParameter::Third);

            using namespace Sketcher;

            auto constraintx0 = [&]() {
                ConstraintToAttachment(GeoElementId(firstCurve,PointPos::mid), GeoElementId::VAxis, x0, handler->sketchgui->getObject());
            };

            auto constrainty0 = [&]() {
                ConstraintToAttachment(GeoElementId(firstCurve,PointPos::mid), GeoElementId::HAxis, y0,  handler->sketchgui->getObject());
            };

            auto constraintradius = [&]() {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                    firstCurve, dHandler->firstRadius);
            };

            // NOTE: if AutoConstraints is empty, we can add constraints directly without any diagnose. No diagnose was run.
            if(handler->AutoConstraints.empty()) {
                if(x0set)
                    constraintx0();

                if(y0set)
                    constrainty0();

                if(radiusSet)
                    constraintradius();
            }
            else { // There is a valid diagnose.
                auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid));

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter will always be set
                if(x0set && startpointinfo.isXDoF()) {
                    constraintx0();

                    handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition

                    startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::mid)); // get updated point position
                }

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter will always be set
                if(y0set && startpointinfo.isYDoF()) {
                    constrainty0();

                    handler->diagnoseWithAutoConstraints(); // ensure we have recalculated parameters after each constraint addition
                }

                auto edgeinfo = handler->getEdgeInfo(firstCurve);
                auto circle = static_cast<SolverGeometryExtension::Circle &>(edgeinfo);

                // if Autoconstraints is empty we do not have a diagnosed system and the parameter will always be set
                if(radiusSet && circle.isRadiusDoF()) {
                    constraintradius();
                }
            }

        }
    }
    //No constraint possible for 3 rim ellipse.
}


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerEllipse_H

