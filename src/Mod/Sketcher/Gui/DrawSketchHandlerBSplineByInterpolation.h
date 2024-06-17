/***************************************************************************
 *   Copyright (c) 2023 Ajinkya Dahale <dahale.a.p@gmail.com>              *
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

#ifndef SKETCHERGUI_DrawSketchHandlerBSplineByInterpolation_H
#define SKETCHERGUI_DrawSketchHandlerBSplineByInterpolation_H

#include <Inventor/events/SoKeyboardEvent.h>
#include <QInputDialog>

#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerBSplineByInterpolation: public DrawSketchHandler
{
public:
    explicit DrawSketchHandlerBSplineByInterpolation(int constructionMethod)
        : Mode(STATUS_SEEK_FIRST_POINT)
        , MousePressMode(MOUSE_NOT_PRESSED)
        , ConstrMethod(constructionMethod)
        , SplineDegree(3)
        , IsClosed(false)
    {
        addSugConstraint();
        applyCursor();
    }

    ~DrawSketchHandlerBSplineByInterpolation() override = default;

    /// modes
    enum SELECT_MODE
    {
        STATUS_SEEK_FIRST_POINT,
        STATUS_SEEK_ADDITIONAL_POINTS,
        STATUS_CLOSE
    };

    // TODO: this kind of behavior will be useful in a superclass
    // when LMB is pressed it's a transitional state so some undos can't be done
    // (like delete last knot)
    enum MOUSE_PRESS_MODE
    {
        MOUSE_PRESSED,
        MOUSE_NOT_PRESSED
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;

        if (Mode == STATUS_SEEK_FIRST_POINT) {
            setPositionText(onSketchPos);

            if (seekAutoConstraint(sugConstr.back(), onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr.back());
                return;
            }
        }
        else if (Mode == STATUS_SEEK_ADDITIONAL_POINTS) {

            drawBSplineToPosition(onSketchPos);

            drawCursorToPosition(onSketchPos);

            if (seekAutoConstraint(sugConstr.back(), onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstr.back());
                return;
            }
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;

        MousePressMode = MOUSE_PRESSED;

        if (Mode == STATUS_SEEK_FIRST_POINT) {
            BSplineKnots.push_back(onSketchPos);
            BSplineMults.push_back(1);  // NOTE: not strictly true for end-points

            Mode = STATUS_SEEK_ADDITIONAL_POINTS;

            // insert point for knot, defer internal alignment constraining.
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Knot Point"));

                // Add knot
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addGeometry(Part.Point(App.Vector(%f,%f,0)),True)",
                                      BSplineKnots.back().x,
                                      BSplineKnots.back().y);

                knotGeoIds.push_back(getHighestCurveIndex());
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Cannot add knot point"));
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject*>(sketchgui->getObject())->solve();

                return false;
            }

            // add auto constraints on knot
            if (!sugConstr.back().empty()) {
                createAutoConstraints(sugConstr.back(),
                                      knotGeoIds.back(),
                                      Sketcher::PointPos::start,
                                      false);
            }

            static_cast<Sketcher::SketchObject*>(sketchgui->getObject())->solve();

            addSugConstraint();
        }
        else if (Mode == STATUS_SEEK_ADDITIONAL_POINTS) {
            // check if coincidence issues with first or last added knot
            for (auto& ac : sugConstr.back()) {
                if (ac.Type == Sketcher::Coincident) {
                    if (ac.GeoId == knotGeoIds[0] && ac.PosId == Sketcher::PointPos::start) {
                        IsClosed = true;
                    }
                    else {
                        // The coincidence with first point may be indirect
                        const auto coincidents =
                            static_cast<Sketcher::SketchObject*>(sketchgui->getObject())
                                ->getAllCoincidentPoints(ac.GeoId, ac.PosId);
                        if (coincidents.find(knotGeoIds[0]) != coincidents.end()
                            && coincidents.at(knotGeoIds[0]) == Sketcher::PointPos::start) {
                            IsClosed = true;
                        }
                        else if (coincidents.find(knotGeoIds.back()) != coincidents.end()
                                 && coincidents.at(knotGeoIds.back())
                                     == Sketcher::PointPos::start) {
                            return true;  // OCCT doesn't allow consecutive points being coincident
                        }
                    }
                }
            }

            BSplineKnots.push_back(onSketchPos);
            BSplineMults.push_back(1);  // NOTE: not strictly true for end-points

            if (IsClosed) {
                Mode = STATUS_CLOSE;

                if (ConstrMethod == 1) {  // if periodic we do not need the last pole
                    BSplineKnots.pop_back();
                    sugConstr.pop_back();

                    return true;
                }
            }

            // insert point for knot, defer internal alignment constraining.
            try {


                // Add knot
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addGeometry(Part.Point(App.Vector(%f,%f,0)),True)",
                                      BSplineKnots.back().x,
                                      BSplineKnots.back().y);

                knotGeoIds.push_back(getHighestCurveIndex());
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(
                    sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Cannot add internal alignment points"));
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject*>(sketchgui->getObject())->solve();

                return false;
            }

            // add auto constraints on knot
            if (!sugConstr.back().empty()) {
                createAutoConstraints(sugConstr.back(),
                                      knotGeoIds.back(),
                                      Sketcher::PointPos::start,
                                      false);
            }

            if (!IsClosed) {
                addSugConstraint();
            }
        }
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;
        MousePressMode = MOUSE_NOT_PRESSED;

        return finishCommand(onSketchPos);
    }

    void registerPressedKey(bool pressed, int key) override
    {
        // if (SoKeyboardEvent::D == key && pressed) {
        //     SplineDegree = QInputDialog::getInt(
        //         Gui::getMainWindow(),
        //         QObject::tr("B-Spline Degree"),
        //         QObject::tr("Define B-Spline Degree, between 1 and %1:")
        //         .arg(QString::number(Geom_BSplineCurve::MaxDegree())),
        //         SplineDegree, 1, Geom_BSplineCurve::MaxDegree(), 1);
        //     // FIXME: Pressing Esc here also finishes the B-Spline creation.
        //     // The user may only want to exit the dialog.
        // }
        if (SoKeyboardEvent::M == key && pressed) {
            if (BSplineMults.size() > 1) {
                BSplineMults.back() = QInputDialog::getInt(
                    Gui::getMainWindow(),
                    QObject::tr("Set knot multiplicity"),
                    QObject::tr(
                        "Set knot multiplicity at the last point provided, between 1 and %1:"
                        "Note that multiplicity may be ignored under certain circumstances."
                        "Please refer to documentation for details")
                        .arg(QString::number(SplineDegree)),
                    BSplineMults.back(),
                    1,
                    SplineDegree,
                    1);
            }
            // FIXME: Pressing Esc here also finishes the B-Spline creation.
            // The user may only want to exit the dialog.
        }
        // On pressing Backspace delete last knot
        else if (SoKeyboardEvent::BACKSPACE == key && pressed) {
            // when mouse is pressed we are in a transitional state so don't mess with it
            if (MOUSE_PRESSED == MousePressMode) {
                return;
            }

            // can only delete last knot if it exists
            if (STATUS_SEEK_FIRST_POINT == Mode || STATUS_CLOSE == Mode) {
                return;
            }

            // if only first knot exists it's equivalent to canceling current spline
            if (knotGeoIds.size() == 1) {
                // this also exits b-spline creation if continuous mode is off
                this->quit();
                return;
            }

            // reverse the steps of press/release button
            try {
                // already ensured that CurrentConstraint == EditCurve.size() > 1
                const int delGeoId = knotGeoIds.back();
                const auto& constraints =
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject())
                        ->Constraints.getValues();
                for (int i = constraints.size() - 1; i >= 0; --i) {
                    if (delGeoId == constraints[i]->First || delGeoId == constraints[i]->Second
                        || delGeoId == constraints[i]->Third) {
                        Gui::cmdAppObjectArgs(sketchgui->getObject(), "delConstraint(%d)", i);
                    }
                }

                // Remove knot
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometry(%d)", delGeoId);

                static_cast<Sketcher::SketchObject*>(sketchgui->getObject())->solve();

                knotGeoIds.pop_back();
                BSplineKnots.pop_back();

                // last entry is kept, as it corresponds to the current knot, but the one
                // corresponding to the erased knot is removed
                sugConstr.erase(std::prev(std::prev(sugConstr.end())));


                // run this in the end to draw lines and position text
                drawBSplineToPosition(prevCursorPosition);
                drawCursorToPosition(prevCursorPosition);
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Error removing knot"));
                // some commands might have already deleted some constraints/geometries but not
                // others
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject*>(sketchgui->getObject())->solve();

                return;
            }
        }

        return;
    }

    void quit() override
    {
        // We must see if we need to create a B-spline before cancelling everything
        // and now just like any other Handler,

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");

        bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

        if (knotGeoIds.size() > 1) {
            // create B-spline from existing knots
            Mode = STATUS_CLOSE;
            finishCommand(Base::Vector2d(0.f, 0.f));
        }
        else if (knotGeoIds.size() == 1) {
            // if we just have one point and we can not close anything, then cancel this creation
            // but continue according to continuous mode
            // sketchgui->getDocument()->undo(1);

            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            if (!continuousMode) {
                DrawSketchHandler::quit();
            }
            else {
                // This code disregards existing data and enables the continuous creation mode.
                resetHandlerState();
            }
        }
        else {  // we have no data (CurrentConstraint == 0) so user when right-clicking really wants
                // to exit
            DrawSketchHandler::quit();
        }
    }

private:
    void resetHandlerState()
    {
        Mode = STATUS_SEEK_FIRST_POINT;
        applyCursor();

        SplineDegree = 3;

        sugConstr.clear();
        knotGeoIds.clear();
        BSplineKnots.clear();
        BSplineMults.clear();

        eraseEditCurve();

        addSugConstraint();

        IsClosed = false;
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (SketcherGui::DrawSketchHandlerBSplineByInterpolation::ConstrMethod == 1) {
            return QString::fromLatin1("Sketcher_Pointer_Create_Periodic_BSplineByInterpolation");
        }
        else {
            return QString::fromLatin1("Sketcher_Pointer_Create_BSplineByInterpolation");
        }
    }

    void addSugConstraint()
    {
        std::vector<AutoConstraint> sugConstr1;
        sugConstr.push_back(std::move(sugConstr1));
    }

    // NOTE: In this context, it is not a control polygon, but a 1-degree interpolation
    void drawControlPolygonToPosition(Base::Vector2d position)
    {
        std::vector<Base::Vector2d> editcurve(BSplineKnots);
        editcurve.push_back(position);

        drawEdit(editcurve);
    }

    void drawBSplineToPosition(Base::Vector2d position)
    {
        try {
            tryInterpolateSpline(position);
        }
        catch (const Standard_Failure&) {
            // Since it happens very frequently that the interpolation fails
            // it's sufficient to report this as log message to avoid to pollute
            // the output window
            Base::Console().Log(std::string("drawBSplineToPosition"), "interpolation failed\n");
        }
    }

    void tryInterpolateSpline(Base::Vector2d position)
    {
        std::vector<Base::Vector2d> editcurve(BSplineKnots);
        editcurve.push_back(position);

        std::vector<gp_Pnt> editCurveForOCCT;
        editCurveForOCCT.reserve(editcurve.size());
        for (auto& p : editcurve) {
            editCurveForOCCT.emplace_back(p.x, p.y, 0.0);
        }

        // TODO: This maybe optimized by storing the spline as an attribute.
        Part::GeomBSplineCurve editBSpline;
        editBSpline.interpolate(editCurveForOCCT, ConstrMethod != 0);

        std::vector<Part::Geometry*> editBSplines;
        editBSplines.push_back(&editBSpline);

        drawEdit(editBSplines);
    }

    void drawCursorToPosition(Base::Vector2d position)
    {
        if (!BSplineKnots.empty()) {
            float length = (position - BSplineKnots.back()).Length();
            float angle = (position - BSplineKnots.back()).GetAngle(Base::Vector2d(1.f, 0.f));

            if (showCursorCoords()) {
                SbString text;
                std::string lengthString = lengthToDisplayFormat(length, 1);
                std::string angleString =
                    angleToDisplayFormat((angle != -FLOAT_MAX) ? angle * 180 / M_PI : 0, 1);
                text.sprintf(" (%s, %s)", lengthString.c_str(), angleString.c_str());
                setPositionText(position, text);
            }
        }
    }

    void eraseEditCurve()
    {
        drawEdit(std::vector<Base::Vector2d>());
    }

    bool finishCommand(Base::Vector2d position)
    {
        if (Mode == STATUS_CLOSE) {
            unsetCursor();
            resetPositionText();

            unsigned int myDegree = 3;

            if (ConstrMethod == 0) {
                BSplineMults.front() =
                    myDegree + 1;  // FIXME: This is hardcoded until degree can be changed
                BSplineMults.back() =
                    myDegree + 1;  // FIXME: This is hardcoded until degree can be changed
            }

            std::vector<std::stringstream> streams;

            // Create subsets of points between C0 knots.
            // The first point
            streams.emplace_back();
            streams.back() << "App.Vector(" << BSplineKnots.front().x << ","
                           << BSplineKnots.front().y << "),";
            // Middle points
            for (size_t i = 1; i < BSplineKnots.size() - 1; ++i) {
                streams.back() << "App.Vector(" << BSplineKnots[i].x << "," << BSplineKnots[i].y
                               << "),";
                if (BSplineMults[i] >= myDegree) {
                    streams.emplace_back();
                    streams.back()
                        << "App.Vector(" << BSplineKnots[i].x << "," << BSplineKnots[i].y << "),";
                }
            }
            // The last point
            streams.back() << "App.Vector(" << BSplineKnots.back().x << "," << BSplineKnots.back().y
                           << "),";

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
            std::vector<bool> isBetweenC0Points(BSplineKnots.size(), false);
            for (size_t i = 1; i < BSplineKnots.size() - 1; ++i) {
                if (BSplineMults[i - 1] >= myDegree && BSplineMults[i + 1] >= myDegree) {
                    isBetweenC0Points[i] = true;
                }
            }

            int currentgeoid = getHighestCurveIndex();

            try {
                // TODO: Bypass this for when there are no C0 knots
                // Create B-spline in pieces between C0 knots
                Gui::Command::runCommand(Gui::Command::Gui, "_finalbsp_poles = []");
                Gui::Command::runCommand(Gui::Command::Gui, "_finalbsp_knots = []");
                Gui::Command::runCommand(Gui::Command::Gui, "_finalbsp_mults = []");
                Gui::Command::runCommand(Gui::Command::Gui, "_bsps = []");
                for (auto& controlpoints : controlpointses) {
                    // TODO: variable degrees?
                    QString cmdstr =
                        QString::fromLatin1("_bsps.append(Part.BSplineCurve())\n"
                                            "_bsps[-1].interpolate(%1, PeriodicFlag=%2)\n"
                                            "_bsps[-1].increaseDegree(%3)")
                            .arg(QString::fromLatin1(controlpoints.c_str()))
                            .arg(QString::fromLatin1(ConstrMethod == 0 ? "False" : "True"))
                            .arg(myDegree);
                    Gui::Command::runCommand(Gui::Command::Gui, cmdstr.toLatin1());
                    // Adjust internal knots here (raise multiplicity)
                    // How this contributes to the final B-spline
                    if (controlpoints == controlpointses.front()) {
                        Gui::Command::runCommand(Gui::Command::Gui,
                                                 "_finalbsp_poles.extend(_bsps[-1].getPoles())");
                        Gui::Command::runCommand(Gui::Command::Gui,
                                                 "_finalbsp_knots.extend(_bsps[-1].getKnots())");
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_mults.extend(_bsps[-1].getMultiplicities())");
                    }
                    else {
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_poles.extend(_bsps[-1].getPoles()[1:])");
                        Gui::Command::runCommand(Gui::Command::Gui,
                                                 "_finalbsp_knots.extend([_finalbsp_knots[-1] + i "
                                                 "for i in _bsps[-1].getKnots()[1:]])");
                        Gui::Command::runCommand(Gui::Command::Gui,
                                                 "_finalbsp_mults[-1] = 3");  // FIXME: Hardcoded
                        Gui::Command::runCommand(
                            Gui::Command::Gui,
                            "_finalbsp_mults.extend(_bsps[-1].getMultiplicities()[1:])");
                    }
                }

                // {"poles", "mults", "knots", "periodic", "degree", "weights", "CheckRational",
                // NULL};
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "addGeometry(Part.BSplineCurve"
                    "(_finalbsp_poles,_finalbsp_mults,_finalbsp_knots,%s,%d,None,False),%s)",
                    ConstrMethod == 0 ? "False" : "True",
                    myDegree,
                    constructionModeAsBooleanText());
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
                if (ConstrMethod == 0) {
                    for (auto& constr : static_cast<Sketcher::SketchObject*>(sketchgui->getObject())
                                            ->Constraints.getValues()) {
                        if (constr->First == knotGeoIds[0]
                            && constr->FirstPos == Sketcher::PointPos::start) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::start;
                        }
                        else if (constr->First == knotGeoIds.back()
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
                for (size_t i = 0; i < knotGeoIds.size(); i++) {
                    if (isBetweenC0Points[i]) {
                        // Constraint point on curve
                        cstream << "conList.append(Sketcher.Constraint('PointOnObject',"
                                << knotGeoIds[0] + i << ","
                                << static_cast<int>(Sketcher::PointPos::start) << ","
                                << currentgeoid << "))\n";
                    }
                    else {
                        cstream << "conList.append(Sketcher.Constraint('InternalAlignment:Sketcher:"
                                   ":BSplineKnotPoint',"
                                << knotGeoIds[0] + i << ","
                                << static_cast<int>(Sketcher::PointPos::start) << ","
                                << currentgeoid << "," << knotNumber << "))\n";
                        // NOTE: Assume here that the spline shape doesn't change on increasing knot
                        // multiplicity.
                        // Change the knot multiplicity here because the user asked and it's not C0
                        // NOTE: The knot number here has to be provided in the OCCT ordering.
                        if (BSplineMults[i] > 1 && BSplineMults[i] < myDegree) {
                            Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                                  "modifyBSplineKnotMultiplicity(%d, %d, %d) ",
                                                  currentgeoid,
                                                  knotNumber + 1,
                                                  BSplineMults[i] - 1);
                        }
                        knotNumber++;
                    }
                }

                cstream << Gui::Command::getObjectCmd(sketchgui->getObject())
                        << ".addConstraint(conList)\n";
                cstream << "del conList\n";

                Gui::Command::doCommand(Gui::Command::Doc, cstream.str().c_str());

                // for showing the rest of internal geometry on creation
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "exposeInternalGeometry(%d)",
                                      currentgeoid);
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Error creating B-spline"));
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Sketcher");
                bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

                if (continuousMode) {
                    // This code enables the continuous creation mode.
                    resetHandlerState();

                    drawCursorToPosition(position);

                    /* It is ok not to call to purgeHandler
                     * in continuous creation mode because the
                     * handler is destroyed by the quit() method on pressing the
                     * right button of the mouse */
                }
                else {
                    sketchgui->purgeHandler();  // no code after this line, Handler get deleted in
                                                // ViewProvider
                }

                return false;
            }

            Gui::Command::commitCommand();

            tryAutoRecomputeIfNotSolve(
                static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);

            if (continuousMode) {
                // This code enables the continuous creation mode.
                resetHandlerState();

                drawCursorToPosition(position);

                /* It is ok not to call to purgeHandler
                 * in continuous creation mode because the
                 * handler is destroyed by the quit() method on pressing the
                 * right button of the mouse */
            }
            else {
                sketchgui->purgeHandler();  // no code after this line, Handler get deleted in
                                            // ViewProvider
            }
        }
        else {
            drawCursorToPosition(position);
        }

        return true;
    }

protected:
    SELECT_MODE Mode;
    MOUSE_PRESS_MODE MousePressMode;

    // Stores position of the knots of the BSpline.
    std::vector<Base::Vector2d> BSplineKnots;
    std::vector<unsigned int> BSplineMults;

    // suggested autoconstraints for knots.
    // A new one must be added e.g. using addSugConstraint() before adding a new knot.
    std::vector<std::vector<AutoConstraint>> sugConstr;

    int ConstrMethod;
    unsigned int SplineDegree;
    bool IsClosed;
    std::vector<int> knotGeoIds;
    Base::Vector2d prevCursorPosition;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerBSplineByInterpolation_H
