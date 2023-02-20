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

#include <QInputDialog>
#include <Inventor/events/SoKeyboardEvent.h>

#include "GeometryCreationMode.h"


namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

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

    virtual ~DrawSketchHandlerBSplineByInterpolation() = default;

    /// modes
    enum SELECT_MODE {
        STATUS_SEEK_FIRST_POINT,
        STATUS_SEEK_ADDITIONAL_POINTS,
        STATUS_CLOSE
    };

    // TODO: this kind of behavior will be useful in a superclass
    // when LMB is pressed it's a transitional state so some undos can't be done
    // (like delete last knot)
    enum MOUSE_PRESS_MODE {
        MOUSE_PRESSED,
        MOUSE_NOT_PRESSED
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;

        if (Mode==STATUS_SEEK_FIRST_POINT) {
            setPositionText(onSketchPos);

            if (seekAutoConstraint(sugConstr.back(), onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr.back());
                return;
            }
        }
        else if (Mode==STATUS_SEEK_ADDITIONAL_POINTS) {

            drawControlPolygonToPosition(onSketchPos);

            drawCursorToPosition(onSketchPos);

            if (seekAutoConstraint(sugConstr.back(), onSketchPos, Base::Vector2d(0.f,0.f))) {
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

            Mode = STATUS_SEEK_ADDITIONAL_POINTS;

            // insert circle point for knot, defer internal alignment constraining.
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Knot Point"));

                //Add knot
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Point(App.Vector(%f,%f,0)),True)",
                                      BSplineKnots.back().x, BSplineKnots.back().y);

                knotGeoIds.push_back(getHighestCurveIndex());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                return false;
            }

            //Gui::Command::commitCommand();

            //static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

            // add auto constraints on knot
            if (!sugConstr.back().empty()) {
                createAutoConstraints(sugConstr.back(), knotGeoIds.back(), Sketcher::PointPos::mid, false);
            }

            static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

            addSugConstraint();

        }
        else if (Mode == STATUS_SEEK_ADDITIONAL_POINTS) {
            BSplineKnots.push_back(onSketchPos);

            // check if coincident with first knot
            for(auto & ac : sugConstr.back()) {
                if( ac.Type == Sketcher::Coincident && ac.GeoId == knotGeoIds[0] && ac.PosId == Sketcher::PointPos::start ) {
                    IsClosed = true;
                }
            }

            if (IsClosed) {
                Mode = STATUS_CLOSE;

                if (ConstrMethod == 1) { // if periodic we do not need the last pole
                    BSplineKnots.pop_back();
                    sugConstr.pop_back();

                    return true;
                }
            }

            // insert point for knot, defer internal alignment constraining.
            try {

                //Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Pole circle"));

                //Add pole
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Point(App.Vector(%f,%f,0)),True)",
                                      BSplineKnots.back().x,BSplineKnots.back().y);

                knotGeoIds.push_back(getHighestCurveIndex());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                return false;
            }

            // add auto constraints on knot
            if (!sugConstr.back().empty()) {
                createAutoConstraints(sugConstr.back(), knotGeoIds.back(), Sketcher::PointPos::mid, false);
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
        if (SoKeyboardEvent::D == key && pressed) {
            SplineDegree = QInputDialog::getInt(
                Gui::getMainWindow(),
                QObject::tr("B-Spline Degree"),
                QObject::tr("Define B-Spline Degree, between 1 and %1:")
                .arg(QString::number(Geom_BSplineCurve::MaxDegree())),
                SplineDegree, 1, Geom_BSplineCurve::MaxDegree(), 1);
            // FIXME: Pressing Esc here also finishes the B-Spline creation.
            // The user may only want to exit the dialog.
        }
        // On pressing Backspace delete last knot
        else if (SoKeyboardEvent::BACKSPACE == key && pressed) {
            // when mouse is pressed we are in a transitional state so don't mess with it
            if (MOUSE_PRESSED == MousePressMode)
                return;

            // can only delete last knot if it exists
            if (STATUS_SEEK_FIRST_POINT == Mode ||
                STATUS_CLOSE == Mode)
                return;

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
                const auto& constraints = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->Constraints.getValues();
                for (int i = constraints.size() - 1; i >= 0; --i) {
                    if (delGeoId == constraints[i]->First ||
                        delGeoId == constraints[i]->Second ||
                        delGeoId == constraints[i]->Third)
                        Gui::cmdAppObjectArgs(sketchgui->getObject(), "delConstraint(%d)", i);
                }

                // Remove knot
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometry(%d)", delGeoId);

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                knotGeoIds.pop_back();
                BSplineKnots.pop_back();

                // last entry is kept, as it corresponds to the current knot, but the one corresponding to the erased knot is removed
                sugConstr.erase(std::prev(std::prev(sugConstr.end())));


                // run this in the end to draw lines and position text
                drawControlPolygonToPosition(prevCursorPosition);
                drawCursorToPosition(prevCursorPosition);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                // some commands might have already deleted some constraints/geometries but not others
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                return;
            }
        }
        // TODO: On pressing, say, W, modify last pole's weight
        // TODO: On pressing, say, M, modify next knot's multiplicity

        return;
    }

    void quit() override
    {
        // We must see if we need to create a B-spline before cancelling everything
        // and now just like any other Handler,

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

        bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

        if (knotGeoIds.size() > 1) {
            // create B-spline from existing knots
            Mode=STATUS_CLOSE;
            finishCommand(Base::Vector2d(0.f,0.f));
        }
        else if(knotGeoIds.size() == 1) {
            // if we just have one point and we can not close anything, then cancel this creation but continue according to continuous mode
            //sketchgui->getDocument()->undo(1);

            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            if(!continuousMode){
                DrawSketchHandler::quit();
            }
            else {
                // This code disregards existing data and enables the continuous creation mode.
                resetHandlerState();
            }
        }
        else { // we have no data (CurrentConstraint == 0) so user when right-clicking really wants to exit
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

        eraseEditCurve();

        addSugConstraint();

        IsClosed = false;
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Create_BSpline");
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

    void drawCursorToPosition(Base::Vector2d position)
    {
        if (!BSplineKnots.empty()) {
            float length = (position - BSplineKnots.back()).Length();
            float angle = (position - BSplineKnots.back()).GetAngle(Base::Vector2d(1.f,0.f));

            if (showCursorCoords()) {
                SbString text;
                std::string lengthString = lengthToDisplayFormat(length, 1);
                std::string angleString = angleToDisplayFormat((angle != -FLOAT_MAX) ? angle * 180 / M_PI : 0, 1);
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
        if (Mode==STATUS_CLOSE) {
            // TODO: make modifications for construction by knots
            unsetCursor();
            resetPositionText();

            std::stringstream stream;
            std::string controlpoints;

            // TODO: for knots compute new control points and use those instead
            for (auto & knot : BSplineKnots) {
                stream << "App.Vector(" << knot.x << "," << knot.y << "),";
            }

            controlpoints = stream.str();

            // remove last comma and add brackets
            int index = controlpoints.rfind(',');
            controlpoints.resize(index);

            controlpoints.insert(0,1,'[');
            controlpoints.append(1,']');

            int currentgeoid = getHighestCurveIndex();

            // TODO: Adjust this or remove condition if needed
            unsigned int maxDegree = 1;

            try {
                //Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add B-spline curve"));

                // {"poles", "mults", "knots", "periodic", "degree", "weights", "CheckRational", NULL};
                // FIXME: can we get by without naming the variable?
                // TODO: variable degrees?
                QString cmdstr = QString::fromLatin1("_bsp = Part.BSplineCurve()\n"
                                                     "_bsp.interpolate(%1, PeriodicFlag=%2)\n"
                    ).arg(QString::fromLatin1(controlpoints.c_str()),
                          QString::fromLatin1(ConstrMethod == 0 ?"False":"True"));
                Gui::Command::runCommand(Gui::Command::Gui, cmdstr.toLatin1());
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(_bsp,%s)",
                                      geometryCreationMode==Construction?"True":"False");
                Gui::Command::runCommand(Gui::Command::Gui, "del(_bsp)\n");

                currentgeoid++;

                // autoconstraints were added to the circles of the poles, which is ok because they must go to the
                // right position, or the user will freak-out if they appear out of the autoconstrained position.
                // However, autoconstraints on the first and last knot, in normal non-periodic b-splines (with appropriate endpoint knot multiplicity)
                // as the ones created by this tool are intended for the b-spline endpoints, and not for the knots,
                // so here we retrieve any autoconstraint on those knots' center and mangle it to the endpoint.
                // TODO: this will be done to the first and last knots instead for ConstrMethod==2
                if (ConstrMethod == 0) {
                    for(auto & constr : static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->Constraints.getValues()) {
                        if(constr->First == knotGeoIds[0] && constr->FirstPos == Sketcher::PointPos::start) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::start;
                        }
                        else if(constr->First == knotGeoIds.back() && constr->FirstPos == Sketcher::PointPos::start) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::end;
                        }
                    }
                }

                // TODO: Change this for construction by knots
                // Constraint pole circles to B-spline.
                std::stringstream cstream;

                cstream << "conList = []\n";

                for (size_t i = 0; i < knotGeoIds.size(); i++) {
                    cstream << "conList.append(Sketcher.Constraint('InternalAlignment:Sketcher::BSplineKnotPoint'," << knotGeoIds[0] + i
                        << "," << static_cast<int>(Sketcher::PointPos::start) << "," << currentgeoid << "," << i << "))\n";
                }

                cstream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
                cstream << "del conList\n";

                Gui::Command::doCommand(Gui::Command::Doc, cstream.str().c_str());

                // for showing the knots on creation
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                return false;
            }

            Gui::Command::commitCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

            if(continuousMode){
                // This code enables the continuous creation mode.
                resetHandlerState();

                drawCursorToPosition(position);

                /* It is ok not to call to purgeHandler
                 * in continuous creation mode because the
                 * handler is destroyed by the quit() method on pressing the
                 * right button of the mouse */
            }
            else{
                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
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

    // suggested autoconstraints for knots.
    // A new one must be added e.g. using addSugConstraint() before adding a new knot.
    std::vector<std::vector<AutoConstraint>> sugConstr;

    int ConstrMethod;
    unsigned int SplineDegree;
    bool IsClosed;
    std::vector<int> knotGeoIds;
    Base::Vector2d prevCursorPosition;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerBSplineByInterpolation_H
