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

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerBSpline;

namespace ConstructionMethods
{
enum class BSplineConstructionMethod
{
    ControlPoints,
    Knots,
    End  // Must be the last one
};
}  // namespace ConstructionMethods

using DSHBSplineController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerBSpline,
    /*SelectModeT*/ StateMachines::TwoSeekEnd,
    /*PAutoConstraintSize =*/2,
    /*OnViewParametersT =*/OnViewParameters<4, 4>,  // NOLINT
    /*WidgetParametersT =*/WidgetParameters<1, 1>,  // NOLINT
    /*WidgetCheckboxesT =*/WidgetCheckboxes<1, 1>,  // NOLINT
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
    ConstructionMethods::BSplineConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHBSplineControllerBase = DSHBSplineController::ControllerBase;

using DrawSketchHandlerBSplineBase = DrawSketchControllableHandler<DSHBSplineController>;


class DrawSketchHandlerBSpline: public DrawSketchHandlerBSplineBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerBSpline)

    friend DSHBSplineController;
    friend DSHBSplineControllerBase;

public:
    explicit DrawSketchHandlerBSpline(
        ConstructionMethod constrMethod = ConstructionMethod::ControlPoints,
        bool periodic = false
    )
        : DrawSketchHandlerBSplineBase(constrMethod)
        , SplineDegree(3)
        , periodic(periodic)
        , prevCursorPosition(Base::Vector2d())
        , resetSeekSecond(false) {};
    ~DrawSketchHandlerBSpline() override = default;

    void activated() override
    {
        DrawSketchHandlerBSplineBase::activated();
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Sketch B-Spline"));
    }

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;

        switch (state()) {
            case SelectMode::SeekFirst: {
                toolWidgetManager.drawPositionAtCursor(onSketchPos);

                seekAndRenderAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            case SelectMode::SeekSecond: {
                toolWidgetManager.drawDirectionAtCursor(onSketchPos, getLastPoint());

                try {
                    CreateAndDrawShapeGeometry();
                }
                catch (const Base::ValueError&) {
                }  // equal points while hovering raise an objection that can be safely ignored

                seekAndRenderAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f));
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        if (geoIds.size() == 1) {
            // if we just have one point and we can not close anything
            Gui::Command::abortCommand();
            return;
        }

        try {
            if (constructionMethod() == ConstructionMethod::ControlPoints) {
                createShape(false);

                commandAddShapeGeometryAndConstraints();

                int currentgeoid = getHighestCurveIndex();

                // autoconstraints were added to the circles of the poles or knots, which is ok
                // because they must go to the right position, or the user will freak-out if they
                // appear out of the autoconstrained position. However, autoconstraints on the first
                // and last pole/knot, in normal non-periodic b-splines (with appropriate endpoint
                // knot multiplicity) as the ones created by this tool are intended for the b-spline
                // endpoints, and not for the poles/knots, so here we retrieve any autoconstraint on
                // those poles/knots center and mangle it to the endpoint.
                if (!periodic) {
                    for (auto& constr : sketchgui->getSketchObject()->Constraints.getValues()) {
                        if (constr->First == geoIds[0]
                            && constr->FirstPos == Sketcher::PointPos::mid) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::start;
                        }
                        else if (constr->First == geoIds.back()
                                 && constr->FirstPos == Sketcher::PointPos::mid) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::end;
                        }
                    }
                }

                // Constraint pole/knot circles to B-spline.
                std::stringstream cstream;
                cstream << "conList = []\n";

                for (size_t i = 0; i < geoIds.size(); i++) {
                    cstream << "conList.append(Sketcher.Constraint('InternalAlignment:Sketcher::"
                               "BSplineControlPoint',"
                            << geoIds[0] + i << "," << static_cast<int>(Sketcher::PointPos::mid)
                            << "," << currentgeoid << "," << i << "))\n";
                }

                cstream << Gui::Command::getObjectCmd(sketchgui->getObject())
                        << ".addConstraint(conList)\n"
                        << "del conList\n";

                Gui::Command::doCommand(Gui::Command::Doc, cstream.str().c_str());

                // for showing the knots on creation
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            else {
                int myDegree = 3;

                if (!periodic) {
                    // FIXME: This is hardcoded until degree can be changed
                    multiplicities.front() = myDegree + 1;
                    multiplicities.back() = myDegree + 1;
                }

                std::vector<std::stringstream> streams;

                // Create subsets of points between C0 knots.
                // The first point
                streams.emplace_back();
                streams.back() << "App.Vector(" << points.front().x << "," << points.front().y
                               << "),";
                // Middle points
                for (size_t i = 1; i < points.size() - 1; ++i) {
                    streams.back() << "App.Vector(" << points[i].x << "," << points[i].y << "),";
                    if (multiplicities[i] >= myDegree) {
                        streams.emplace_back();
                        streams.back() << "App.Vector(" << points[i].x << "," << points[i].y << "),";
                    }
                }
                // The last point
                streams.back() << "App.Vector(" << points.back().x << "," << points.back().y << "),";

                // Note the plural of plurals. Each element is a separate sequence.
                std::vector<std::string> controlpointses;
                controlpointses.reserve(streams.size());
                for (auto& stream : streams) {
                    controlpointses.emplace_back(stream.str());


                    auto& controlpoints = controlpointses.back();

                    // remove last comma and add brackets
                    int index = controlpoints.rfind(',');
                    controlpoints.resize(index);

                    controlpoints.insert(0, 1, '[');
                    controlpoints.append(1, ']');
                }

                // With just 3 points provided OCCT gives a quadratic spline where
                // the middle point is NOT a knot. This needs to be treated differently.
                // FIXME: Decide whether to force a knot or not.
                std::vector<bool> isBetweenC0Points(points.size(), false);
                for (size_t i = 1; i < points.size() - 1; ++i) {
                    if (multiplicities[i - 1] >= myDegree && multiplicities[i + 1] >= myDegree) {
                        isBetweenC0Points[i] = true;
                    }
                }

                int currentgeoid = getHighestCurveIndex();

                // TODO: Bypass this for when there are no C0 knots
                // Create B-spline in pieces between C0 knots
                Gui::Command::runCommand(Gui::Command::Gui, "_finalbsp_poles = []");
                Gui::Command::runCommand(Gui::Command::Gui, "_finalbsp_knots = []");
                Gui::Command::runCommand(Gui::Command::Gui, "_finalbsp_mults = []");
                Gui::Command::runCommand(Gui::Command::Gui, "_bsps = []");
                for (auto& controlpoints : controlpointses) {
                    // TODO: variable degrees?
                    QString cmdstr = QStringLiteral(
                                         "_bsps.append(Part.BSplineCurve())\n"
                                         "_bsps[-1].interpolate(%1, PeriodicFlag=%2)\n"
                                         "_bsps[-1].increaseDegree(%3)"
                    )
                                         .arg(QString::fromLatin1(controlpoints.c_str()))
                                         .arg(QString::fromLatin1(periodic ? "True" : "False"))
                                         .arg(myDegree);
                    Gui::Command::runCommand(Gui::Command::Gui, cmdstr.toLatin1());
                    // Adjust internal knots here (raise multiplicity)
                    // How this contributes to the final B-spline
                    if (controlpoints == controlpointses.front()) {
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_poles.extend(_bsps[-1].getPoles())"
                        );
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_knots.extend(_bsps[-1].getKnots())"
                        );
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_mults.extend(_bsps[-1].getMultiplicities())"
                        );
                    }
                    else {
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_poles.extend(_bsps[-1].getPoles()[1:])"
                        );
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_knots.extend([_finalbsp_knots[-1] + i "
                            "for i in _bsps[-1].getKnots()[1:]])"
                        );
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_mults[-1] = 3"
                        );  // FIXME: Hardcoded
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_mults.extend(_bsps[-1].getMultiplicities()[1:])"
                        );
                    }
                }

                // {"poles", "mults", "knots", "periodic", "degree", "weights", "CheckRational",
                // NULL};
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.BSplineCurve"
                    "(_finalbsp_poles,_finalbsp_mults,_finalbsp_knots,%s,%d,None,False),%s)",
                    periodic ? "True" : "False",
                    myDegree,
                    constructionModeAsBooleanText()
                );
                currentgeoid++;

                // TODO: Confirm we do not need to delete individual elements
                Gui::Command::runCommand(Gui::Command::Gui, "del(_bsps)\n");
                Gui::Command::runCommand(Gui::Command::Gui, "del(_finalbsp_poles)\n");
                Gui::Command::runCommand(Gui::Command::Gui, "del(_finalbsp_knots)\n");
                Gui::Command::runCommand(Gui::Command::Gui, "del(_finalbsp_mults)\n");

                // autoconstraints were added to the knots, which is ok because they must go to the
                // right position, or the user will freak-out if they appear out of the
                // autoconstrained position. However, autoconstraints on the first and last knot, in
                // non-periodic b-splines (with appropriate endpoint knot multiplicity) as the ones
                // created by this tool are intended for the b-spline endpoints, and not for the
                // knots, so here we retrieve any autoconstraint on those knots and mangle it to the
                // endpoint.
                if (!periodic) {
                    for (auto& constr : sketchgui->getSketchObject()->Constraints.getValues()) {
                        if (constr->First == geoIds[0]
                            && constr->FirstPos == Sketcher::PointPos::start) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::start;
                        }
                        else if (constr->First == geoIds.back()
                                 && constr->FirstPos == Sketcher::PointPos::start) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::end;
                        }
                    }
                }

                // Constraint knots to B-spline.
                std::stringstream cstream;

                cstream << "conList = []\n";

                int knotNumber = 0;
                for (size_t i = 0; i < geoIds.size(); i++) {
                    if (isBetweenC0Points[i]) {
                        // Constraint point on curve
                        cstream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                << geoIds[0] + i << ","
                                << static_cast<int>(Sketcher::PointPos::start) << ","
                                << currentgeoid << "))\n";
                    }
                    else {
                        cstream << "conList.append(Sketcher.Constraint('InternalAlignment:Sketcher:"
                                   ":BSplineKnotPoint',"
                                << geoIds[0] + i << ","
                                << static_cast<int>(Sketcher::PointPos::start) << ","
                                << currentgeoid << "," << knotNumber << "))\n";
                        // NOTE: Assume here that the spline shape doesn't change on increasing knot
                        // multiplicity.
                        // Change the knot multiplicity here because the user asked and it's not C0
                        // NOTE: The knot number here has to be provided in the OCCT ordering.
                        if (multiplicities[i] > 1 && multiplicities[i] < myDegree) {
                            Gui::cmdAppObjectArgs(
                                sketchgui->getObject(),
                                "modifyBSplineKnotMultiplicity(%d, %d, %d) ",
                                currentgeoid,
                                knotNumber + 1,
                                multiplicities[i] - 1
                            );
                        }
                        knotNumber++;
                    }
                }

                cstream << Gui::Command::getObjectCmd(sketchgui->getObject())
                        << ".addConstraint(conList)\n";
                cstream << "del conList\n";

                Gui::Command::doCommand(Gui::Command::Doc, cstream.str().c_str());

                // for showing the rest of internal geometry on creation
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Error creating B-spline")
            );
            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(sketchgui->getSketchObject());

            return;
        }
    }

    void generateAutoConstraints() override
    {
        // The auto constraints are already generated in canGoToNextMode


        // Ensure temporary autoconstraints do not generate a redundancy and that the geometry
        // parameters are accurate This is particularly important for adding widget mandated
        // constraints.
        removeRedundantAutoConstraints();
    }

    void createAutoConstraints() override
    {
        // execute python command to create autoconstraints
        createGeneratedAutoConstraints(true);

        sugConstraints[0].clear();
        sugConstraints[1].clear();
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        using State = std::pair<ConstructionMethod, SelectMode>;
        using enum Gui::InputHint::UserInput;

        const Gui::InputHint switchModeHint {tr("%1 switch mode"), {KeyM}};

        return Gui::lookupHints<State>(
            {constructionMethod(), state()},
            {
                // ControlPoints method
                {.state = {ConstructionMethod::ControlPoints, SelectMode::SeekFirst},
                 .hints =
                     {
                         {tr("%1 pick first control point"), {MouseLeft}},
                         switchModeHint,
                         {tr("%1 + degree"), {KeyU}},
                         {tr("%1 - degree"), {KeyJ}},
                     }},
                {.state = {ConstructionMethod::ControlPoints, SelectMode::SeekSecond},
                 .hints =
                     {
                         {tr("%1 pick next control point"), {MouseLeft}},
                         {tr("%1 finish B-spline"), {MouseRight}},
                         switchModeHint,
                         {tr("%1 + degree"), {KeyU}},
                         {tr("%1 - degree"), {KeyJ}},
                     }},

                // Knots method
                {.state = {ConstructionMethod::Knots, SelectMode::SeekFirst},
                 .hints =
                     {
                         {tr("%1 pick first knot"), {MouseLeft}},
                         switchModeHint,
                         {tr("%1 toggle periodic"), {KeyR}},
                     }},
                {.state = {ConstructionMethod::Knots, SelectMode::SeekSecond},
                 .hints =
                     {
                         {tr("%1 pick next knot"), {MouseLeft}},
                         {tr("%1 finish B-spline"), {MouseRight}},
                         switchModeHint,
                         {tr("%1 toggle periodic"), {KeyR}},
                     }},
            });
    }

    std::string getToolName() const override
    {
        return "DSH_BSpline";
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (constructionMethod() == ConstructionMethod::ControlPoints) {
            if (periodic) {
                return QStringLiteral("Sketcher_Pointer_Create_Periodic_BSpline");
            }
            else {
                return QStringLiteral("Sketcher_Pointer_Create_BSpline");
            }
        }
        else {
            if (periodic) {
                return QStringLiteral("Sketcher_Pointer_Create_Periodic_BSplineByInterpolation");
            }
            else {
                return QStringLiteral("Sketcher_Pointer_Create_BSplineByInterpolation");
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
        return Gui::BitmapFactory().pixmap("Sketcher_CreateBSpline");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("B-Spline Parameters"));
    }

    bool canGoToNextMode() override
    {
        Sketcher::PointPos pointPos = constructionMethod() == ConstructionMethod::ControlPoints
            ? Sketcher::PointPos::mid
            : Sketcher::PointPos::start;
        if (state() == SelectMode::SeekFirst) {
            // insert point for pole/knot, defer internal alignment constraining.
            if (!addPos()) {
                return false;
            }

            // add auto constraints on pole/knot
            auto& ac0 = sugConstraints[0];
            generateAutoConstraintsOnElement(ac0, geoIds.back(), pointPos);

            sketchgui->getSketchObject()->solve();
        }
        else if (state() == SelectMode::SeekSecond) {
            // Prevent adding a new point if it's coincident with the last one.
            if (!points.empty()
                && (prevCursorPosition - getLastPoint()).Length() < Precision::Confusion()) {
                return false;
            }

            // We stay in SeekSecond unless the user closed the bspline.
            bool isClosed = false;

            // check if coincident with first pole/knot
            for (auto& ac : sugConstraints.back()) {
                if (ac.Type == Sketcher::Coincident) {
                    if (ac.GeoId == geoIds[0]) {
                        isClosed = true;
                    }
                    else {
                        // The coincidence with first point may be indirect
                        const auto coincidents
                            = sketchgui->getSketchObject()->getAllCoincidentPoints(ac.GeoId, ac.PosId);
                        if (coincidents.find(geoIds[0]) != coincidents.end()) {
                            isClosed = true;
                        }
                    }
                }
            }

            if (isClosed) {
                if (periodic) {  // if periodic we do not need the last pole/knot
                    return true;
                }
            }
            else {
                setAngleSnapping(true, getLastPoint());
                resetSeekSecond = true;
            }

            // insert circle point for pole/knot, defer internal alignment constraining.
            if (!addPos()) {
                return false;
            }

            // add auto constraints on pole/knot
            auto& ac1 = sugConstraints[1];
            generateAutoConstraintsOnElement(ac1, geoIds.back(), pointPos);
            sugConstraintsBackup.push_back(std::move(ac1));
            ac1.clear();

            return isClosed;
        }
        return true;
    }

    void angleSnappingControl() override
    {
        if (state() == SelectMode::SeekSecond && !points.empty()) {
            setAngleSnapping(true, getLastPoint());
        }
        else {
            setAngleSnapping(false);
        }
    }

    void quit() override
    {
        // We must see if we need to create a B-spline before cancelling everything

        if (state() == SelectMode::SeekSecond) {
            if (geoIds.size() > 1) {
                // create B-spline from existing poles/knots
                setState(SelectMode::End);
                finish();
            }
            else {
                // We don't want to finish() as that'll create auto-constraints
                handleContinuousMode();
            }
        }
        else {
            DrawSketchHandler::quit();
        }
    }

    void rightButtonOrEsc() override
    {
        quit();
    }

    void onReset() override
    {
        Gui::Command::abortCommand();
        tryAutoRecomputeIfNotSolve(sketchgui->getSketchObject());
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Sketch B-Spline"));

        SplineDegree = 3;
        geoIds.clear();
        points.clear();
        multiplicities.clear();
        sugConstraintsBackup.clear();

        toolWidgetManager.resetControls();
    }

    void undoLastPoint()
    {
        // can only delete last pole/knot if it exists
        if (state() != SelectMode::SeekSecond) {
            return;
        }

        // if only first pole/knot exists it's equivalent to canceling current spline
        if (geoIds.size() == 1) {
            // this also exits b-spline creation if continuous mode is off
            quit();
            return;
        }

        // reverse the steps of press/release button
        try {
            // already ensured that CurrentConstraint == EditCurve.size() > 1
            const int delGeoId = geoIds.back();
            const auto& constraints = sketchgui->getSketchObject()->Constraints.getValues();
            for (int i = constraints.size() - 1; i >= 0; --i) {
                if (delGeoId == constraints[i]->First || delGeoId == constraints[i]->Second
                    || delGeoId == constraints[i]->Third) {
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "delConstraint(%d)", i);
                }
            }

            // Remove pole/knot
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometry(%d)", delGeoId);

            sketchgui->getSketchObject()->solve();

            geoIds.pop_back();
            points.pop_back();
            multiplicities.pop_back();
            distances.pop_back();

            updateDataAndDrawToPosition(prevCursorPosition);
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Error deleting last pole/knot")
            );
            // some commands might have already deleted some constraints/geometries but not
            // others
            Gui::Command::abortCommand();

            sketchgui->getSketchObject()->solve();

            return;
        }
    }

private:
    size_t SplineDegree;
    bool periodic;
    Base::Vector2d prevCursorPosition;
    std::vector<Base::Vector2d> points;
    std::vector<int> multiplicities;
    std::vector<int> geoIds;
    std::vector<bool> isBetweenC0Points;
    std::vector<double> distances;
    bool resetSeekSecond;

    std::vector<std::vector<AutoConstraint>> sugConstraintsBackup;

    bool addPos()
    {
        addToVectors();
        return addGeometry(getLastPoint(), geoIds.back(), points.size() == 1);
    }

    void addToVectors()
    {
        points.push_back(prevCursorPosition);
        multiplicities.push_back(1);
        geoIds.push_back(getHighestCurveIndex() + 1);
        if (geoIds.size() != distances.size()) {
            distances.push_back(-1);
        }
    }

    bool addGeometry(Base::Vector2d pos, int geoId, bool firstPoint)
    {
        try {
            Gui::cmdAppObjectArgs(
                sketchgui->getObject(),
                constructionMethod() == ConstructionMethod::ControlPoints
                    ? "addGeometry(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),10),True)"
                    : "addGeometry(Part.Point(App.Vector(%f,%f,0)),True)",
                pos.x,
                pos.y
            );


            if (constructionMethod() == ConstructionMethod::ControlPoints) {
                if (firstPoint) {  // First pole defaults to 1.0 weight
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addConstraint(Sketcher.Constraint('Weight',%d,%f)) ",
                        geoId,
                        1.0
                    );
                }
                else {
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
                        geoIds[0],
                        geoId
                    );
                }
            }
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Error adding B-spline pole/knot")
            );

            Gui::Command::abortCommand();

            sketchgui->getSketchObject()->solve();

            return false;
        }
        return true;
    }

    void changeConstructionMethode()
    {
        // Restart the command
        Gui::Command::abortCommand();
        tryAutoRecomputeIfNotSolve(sketchgui->getSketchObject());
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Sketch B-Spline"));

        // Restore keyboard focus after command restart
        if (Gui::Document* doc = Gui::Application::Instance->activeDocument()) {
            if (Gui::MDIView* mdi = doc->getActiveView()) {
                mdi->setFocus();
            }
        }

        // Add the necessary alignment geometries and constraints
        for (size_t i = 0; i < geoIds.size(); ++i) {
            addGeometry(points[i], geoIds[i], i == 0);
        }

        // reapply the auto-constraints
        Sketcher::PointPos pointPos = constructionMethod() == ConstructionMethod::ControlPoints
            ? Sketcher::PointPos::mid
            : Sketcher::PointPos::start;
        if (!geoIds.empty()) {
            generateAutoConstraintsOnElement(sugConstraints[0], geoIds[0], pointPos);
        }

        size_t i = 1;
        for (auto& ac : sugConstraintsBackup) {
            if (i < geoIds.size()) {
                generateAutoConstraintsOnElement(ac, geoIds[i], pointPos);
            }
            ++i;
        }
    }

    Base::Vector2d getLastPoint()
    {
        return points.empty() ? Base::Vector2d() : points.back();
    }

    void createShape(bool onlyeditoutline) override
    {
        ShapeGeometry.clear();

        std::vector<Base::Vector3d> bsplinePoints3D;
        for (auto& point : points) {
            bsplinePoints3D.emplace_back(point.x, point.y, 0.0);
        }

        double len = (prevCursorPosition - getLastPoint()).Length();
        if (onlyeditoutline && (points.empty() || len >= Precision::Confusion())) {
            bsplinePoints3D.emplace_back(prevCursorPosition.x, prevCursorPosition.y, 0.0);
        }
        if (bsplinePoints3D.size() < 2) {
            return;
        }

        if (constructionMethod() == ConstructionMethod::ControlPoints) {
            size_t vSize = bsplinePoints3D.size();
            size_t maxDegree = vSize - (periodic ? 0 : 1);
            size_t degree = std::min(maxDegree, SplineDegree);

            std::vector<double> weights(vSize, 1.0);
            std::vector<double> knots;
            std::vector<int> mults;
            if (!periodic) {
                for (size_t i = 0; i < vSize - degree + 1; ++i) {
                    knots.push_back(i);
                }
                mults.resize(vSize - degree + 1, 1);
                mults.front() = degree + 1;
                mults.back() = degree + 1;
            }
            else {
                for (size_t i = 0; i < vSize + 1; ++i) {
                    knots.push_back(i);
                }
                mults.resize(vSize + 1, 1);
            }

            auto bSpline = std::make_unique<Part::GeomBSplineCurve>(
                bsplinePoints3D,
                weights,
                knots,
                mults,
                degree,
                periodic
            );
            bSpline->setPoles(bsplinePoints3D);
            Sketcher::GeometryFacade::setConstruction(bSpline.get(), isConstructionMode());
            ShapeGeometry.emplace_back(std::move(bSpline));
        }
        else {
            try {
                std::vector<gp_Pnt> editCurveForOCCT;
                editCurveForOCCT.reserve(bsplinePoints3D.size());
                for (auto& p : bsplinePoints3D) {
                    editCurveForOCCT.emplace_back(p.x, p.y, 0.0);
                }

                // TODO: This maybe optimized by storing the spline as an attribute.
                auto bSpline = std::make_unique<Part::GeomBSplineCurve>();
                bSpline.get()->interpolate(editCurveForOCCT, periodic);

                Sketcher::GeometryFacade::setConstruction(bSpline.get(), isConstructionMode());
                ShapeGeometry.emplace_back(std::move(bSpline));
            }
            catch (const Standard_Failure&) {
                // Since it happens very frequently that the interpolation fails
                // it's sufficient to report this as log message to avoid to pollute
                // the output window
                Base::Console().log(std::string("drawBSplineToPosition"), "interpolation failed\n");
            }
        }
    }
};

template<>
auto DSHBSplineControllerBase::getState(int labelindex) const
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
            THROWM(Base::ValueError, "Label index without an associated machine state")
    }
}

template<>
void DSHBSplineController::firstKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    toolWidget->setParameterWithoutPassingFocus(WParameter::First, value + 1);
}

template<>
void DSHBSplineController::secondKeyShortcut()
{
    auto value = toolWidget->getParameter(WParameter::First);
    if (value > 1.0) {  // NOLINT
        toolWidget->setParameterWithoutPassingFocus(WParameter::First, value - 1);
    }
}

template<>
void DSHBSplineController::thirdKeyShortcut()
{
    auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
    toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
}

template<>
void DSHBSplineController::fourthKeyShortcut()
{
    handler->undoLastPoint();
}

template<>
void DSHBSplineController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        toolWidget->setNoticeVisible(true);
        toolWidget->setNoticeText(
            QApplication::translate("TaskSketcherTool_c1_bspline", "Press F to undo last point.")
        );

        QStringList names = {
            QApplication::translate("Sketcher_CreateBSpline", "From control points"),
            QApplication::translate("Sketcher_CreateBSpline", "From knots")
        };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_bspline", "Periodic (R)")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_bspline", "Create a periodic B-spline.")
        );
        syncCheckboxToHandler(WCheckbox::FirstBox, handler->periodic);

        if (isConstructionMode()) {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline_Constr")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSplineByInterpolation_Constr")
            );
            toolWidget->setCheckboxIcon(
                WCheckbox::FirstBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline_Constr")
            );
        }
        else {
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                0,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline")
            );
            toolWidget->setComboboxItemIcon(
                WCombobox::FirstCombo,
                1,
                Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSplineByInterpolation")
            );
            toolWidget->setCheckboxIcon(
                WCheckbox::FirstBox,
                Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline")
            );
        }

        toolWidget->setParameterLabel(
            WParameter::First,
            QApplication::translate("ToolWidgetManager_p4", "Degree (+'U'/ -'J')")
        );
        toolWidget->configureParameterUnit(WParameter::First, Base::Unit());
        toolWidget->configureParameterMin(WParameter::First, 1.0);  // NOLINT
        toolWidget->configureParameterMax(WParameter::First, Geom_BSplineCurve::MaxDegree());
        toolWidget->configureParameterDecimals(WParameter::First, 0);
    }

    if (handler->constructionMethod() == ConstructionMethod::ControlPoints) {
        toolWidget->setParameter(WParameter::First, handler->SplineDegree);
        toolWidget->setParameterVisible(WParameter::First, true);
    }
    else {
        // We still set the value in case user change of mode.
        toolWidget->setParameterWithoutPassingFocus(WParameter::First, handler->SplineDegree);
        toolWidget->setParameterVisible(WParameter::First, false);
    }

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
void DSHBSplineController::adaptDrawingToParameterChange(int parameterindex, double value)
{
    switch (parameterindex) {
        case WParameter::First:
            handler->SplineDegree = std::max(1, static_cast<int>(value));
            break;
    }
}

template<>
void DSHBSplineController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            handler->periodic = value;
            break;
    }

    handler->updateCursor();
}

template<>
void DSHBSplineControllerBase::doEnforceControlParameters(Base::Vector2d& onSketchPos)
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

            if (handler->resetSeekSecond) {
                handler->resetSeekSecond = false;
                unsetOnViewParameter(thirdParam.get());
                unsetOnViewParameter(fourthParam.get());
                setFocusToOnViewParameter(OnViewParameter::Third);
                return;
            }

            Base::Vector2d prevPoint = handler->getLastPoint();

            Base::Vector2d dir = onSketchPos - prevPoint;
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

                onSketchPos = prevPoint + length * dir.Normalize();
                if (handler->geoIds.size() == handler->distances.size()) {
                    handler->distances.push_back(length);
                }
                else {
                    // update in case it changed
                    handler->distances[handler->distances.size() - 1] = length;
                }
            }

            if (fourthParam->isSet) {
                double angle = Base::toRadians(fourthParam->getValue());
                onSketchPos.x = prevPoint.x + cos(angle) * length;
                onSketchPos.y = prevPoint.y + sin(angle) * length;
            }

            if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing
                && (onSketchPos - prevPoint).Length() < Precision::Confusion()) {
                unsetOnViewParameter(thirdParam.get());
                unsetOnViewParameter(fourthParam.get());
            }
        } break;
        default:
            break;
    }
}

template<>
void DSHBSplineController::adaptParameters(Base::Vector2d onSketchPos)
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

            Base::Vector2d prevPoint;
            if (!handler->points.empty()) {
                prevPoint = handler->getLastPoint();
            }

            Base::Vector3d start = toVector3d(prevPoint);
            Base::Vector3d end = toVector3d(onSketchPos);
            Base::Vector3d vec = end - start;

            if (!thirdParam->isSet) {
                setOnViewParameterValue(OnViewParameter::Third, vec.Length());
            }

            double range = (onSketchPos - prevPoint).Angle();
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
void DSHBSplineController::computeNextDrawSketchHandlerMode()
{
    switch (handler->state()) {
        case SelectMode::SeekFirst: {
            auto& firstParam = onViewParameters[OnViewParameter::First];
            auto& secondParam = onViewParameters[OnViewParameter::Second];

            if (firstParam->hasFinishedEditing || secondParam->hasFinishedEditing) {
                double x = firstParam->getValue();
                double y = secondParam->getValue();
                handler->onButtonPressed(Base::Vector2d(x, y));
            }
        } break;
        case SelectMode::SeekSecond: {
            auto& thirdParam = onViewParameters[OnViewParameter::Third];
            auto& fourthParam = onViewParameters[OnViewParameter::Fourth];

            if (thirdParam->hasFinishedEditing && fourthParam->hasFinishedEditing) {
                handler->canGoToNextMode();  // its not going to next mode

                unsetOnViewParameter(thirdParam.get());
                unsetOnViewParameter(fourthParam.get());
            }
        } break;
        default:
            break;
    }
}


template<>
void DSHBSplineController::doConstructionMethodChanged()
{
    handler->changeConstructionMethode();

    syncConstructionMethodComboboxToHandler();
    bool byCtrlPoints = handler->constructionMethod() == ConstructionMethod::ControlPoints;
    toolWidget->setParameterVisible(WParameter::First, byCtrlPoints);

    handler->updateHint();
}


template<>
bool DSHBSplineControllerBase::resetOnConstructionMethodeChanged()
{
    return false;
}


template<>
void DSHBSplineController::addConstraints()
{

    App::DocumentObject* obj = handler->sketchgui->getObject();

    int firstCurve = handler->geoIds[0];

    Sketcher::PointPos pPos = handler->constructionMethod() == ConstructionMethod::ControlPoints
        ? Sketcher::PointPos::mid
        : Sketcher::PointPos::start;

    auto x0 = onViewParameters[OnViewParameter::First]->getValue();
    auto y0 = onViewParameters[OnViewParameter::Second]->getValue();

    auto x0set = onViewParameters[OnViewParameter::First]->isSet;
    auto y0set = onViewParameters[OnViewParameter::Second]->isSet;

    using namespace Sketcher;

    auto constraintToOrigin = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, pPos), GeoElementId::RtPnt, x0, obj);
    };

    auto constraintx0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, pPos), GeoElementId::VAxis, x0, obj);
    };

    auto constrainty0 = [&]() {
        ConstraintToAttachment(GeoElementId(firstCurve, pPos), GeoElementId::HAxis, y0, obj);
    };

    auto constraintlengths = [&](bool checkDof) {
        for (size_t i = 0; i < handler->geoIds.size() - 1; ++i) {
            bool dofOk = true;
            if (checkDof) {
                handler->diagnoseWithAutoConstraints();
                auto p1info = handler->getPointInfo(GeoElementId(handler->geoIds[i], pPos));
                auto p2info = handler->getPointInfo(GeoElementId(handler->geoIds[i + 1], pPos));

                int DoFs = p1info.getDoFs();
                DoFs += p2info.getDoFs();
                dofOk = DoFs > 0;
            }

            if (handler->distances[i + 1] > 0 && dofOk) {
                Gui::cmdAppObjectArgs(
                    obj,
                    "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                    handler->geoIds[i],
                    static_cast<int>(pPos),
                    handler->geoIds[i + 1],
                    static_cast<int>(pPos),
                    handler->distances[i + 1]
                );
            }
        }
    };


    if (handler->AutoConstraints.empty()) {  // No valid diagnosis. Every constraint can be added.

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            constraintToOrigin();
        }
        else {
            if (x0set) {
                constraintx0();
            }

            if (y0set) {
                constrainty0();
            }
        }

        constraintlengths(false);
    }
    else {  // Valid diagnosis. Must check which constraints may be added.
        auto startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));

        if (x0set && startpointinfo.isXDoF()) {
            constraintx0();

            // ensure we have recalculated parameters after each constraint addition
            handler->diagnoseWithAutoConstraints();

            // get updated point position
            startpointinfo = handler->getPointInfo(GeoElementId(firstCurve, PointPos::start));
        }

        if (y0set && startpointinfo.isYDoF()) {
            constrainty0();
        }

        constraintlengths(true);
    }
}

}  // namespace SketcherGui
