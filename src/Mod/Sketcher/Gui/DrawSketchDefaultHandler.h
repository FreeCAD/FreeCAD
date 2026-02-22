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

#include <Inventor/events/SoKeyboardEvent.h>

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Gui/Command.h>

#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/PythonConverter.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "AutoConstraint.h"
#include "DrawSketchHandler.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"

#include "Utils.h"

namespace SketcherGui
{

/*********************** Ancillary classes for DrawSketch Hierarchy *******************************/

/** @name Basic State Machines  */
//@{

namespace StateMachines
{

enum class OneSeekEnd
{
    SeekFirst,
    End  // MUST be the last one
};

enum class TwoSeekEnd
{
    SeekFirst,
    SeekSecond,
    End  // MUST be the last one
};

enum class ThreeSeekEnd
{
    SeekFirst,
    SeekSecond,
    SeekThird,
    End  // MUST be the last one
};

enum class FourSeekEnd
{
    SeekFirst,
    SeekSecond,
    SeekThird,
    SeekFourth,
    End  // MUST be the last one
};

enum class FiveSeekEnd
{
    SeekFirst,
    SeekSecond,
    SeekThird,
    SeekFourth,
    SeekFifth,
    End  // MUST be the last one
};

enum class TwoSeekDoEnd
{
    SeekFirst,
    SeekSecond,
    Do,
    End  // MUST be the last one
};

}  // namespace StateMachines

//@}

/** @brief A state machine to encapsulate a state
 *
 * @details
 *
 * A template class for a state machine defined by template type SelectModeT,
 * automatically initialised to the first state, encapsulating the actual state,
 * and enabling to change the state, while generating a call to onModeChanged()
 * after every change.
 *
 * the getNextMode() returns the next mode in the state machine, unless it is in
 * End mode, in which End mode is returned.
 *
 * The goal of this class to sense navigation through the states of the time machine (for example
 * as triggered by one input method) and notifying of changes (for example to another input method
 * to update it appropriately).
 *
 * NOTE: Some basic state machines are provided. However, it is possible for the user to provide its
 * own customised time machine instead. The state machine provided MUST include a last state named
 * End.
 */
template<typename SelectModeT>
class StateMachine
{
public:
    StateMachine()
        : Mode(static_cast<SelectModeT>(0))
    {}
    virtual ~StateMachine()
    {}

protected:
    void setState(SelectModeT mode)
    {
        Mode = mode;
        onModeChanged();
    }

    void ensureState(SelectModeT mode)
    {
        if (Mode != mode) {
            Mode = mode;
            onModeChanged();
        }
    }

    /** Ensure the state machine is the provided mode
     * but only if the mode is an earlier state.
     *
     * This allows one to return to previous states (e.g.
     * for modification), only if that state has previously
     * been completed.
     */
    void ensureStateIfEarlier(SelectModeT mode)
    {
        if (Mode != mode) {
            if (mode < Mode) {
                Mode = mode;
                onModeChanged();
            }
        }
    }

    SelectModeT state() const
    {
        return Mode;
    }

    bool isState(SelectModeT state) const
    {
        return Mode == state;
    }

    void setNextState(std::optional<SelectModeT> nextState)
    {
        nextMode = nextState;
    }

    std::optional<SelectModeT> getNextState()
    {
        return nextMode;
    }

    void applyNextState()
    {
        if (nextMode) {
            auto next = std::move(*nextMode);
            nextMode = std::nullopt;
            setState(next);
        }
    }

    bool isFirstState() const
    {
        return Mode == (static_cast<SelectModeT>(0));
    }

    bool isLastState() const
    {
        return Mode == SelectModeT::End;
    }

    constexpr SelectModeT getFirstState() const
    {
        return static_cast<SelectModeT>(0);
    }

    SelectModeT computeNextMode() const
    {
        auto modeint = static_cast<int>(state());

        if (modeint < maxMode) {
            auto newmode = static_cast<SelectModeT>(modeint + 1);
            return newmode;
        }
        else {
            return SelectModeT::End;
        }
    }

    void moveToNextMode()
    {
        setState(computeNextMode());
    }

    void reset()
    {
        nextMode = std::nullopt;
        if (Mode != static_cast<SelectModeT>(0)) {
            setState(static_cast<SelectModeT>(0));
        }
    }

    virtual bool onModeChanged()
    {
        return true;
    };

private:
    SelectModeT Mode;
    std::optional<SelectModeT> nextMode;
    static const constexpr int maxMode = static_cast<int>(SelectModeT::End);
};


namespace ConstructionMethods
{

enum class DefaultConstructionMethod
{
    End  // Must be the last one
};

}  // namespace ConstructionMethods

/** @brief An encapsulation of a construction method
 *
 * @details
 *
 * A template class for a construction method defined by template type ConstructionMethodT.
 *
 * A construction method is an alternative UI sequence of steps or input parameters to achieve a
 * goal.For example, for creation of a circle, point center + radius is a possible construction
 * method. Three points on the rim is another possible construction method.
 *
 * The construction method is automatically initialised to the first or default construction method.
 * It senses construction method change, triggering onModeChanged() after every change.
 *
 * the getNextMethod() returns the next construction method, in a round-robin fashion.
 *
 * NOTE: A construction method enum defining the different construction methods must be provided and
 * the last item shall be ConstructionMethodT::End.
 */
template<typename ConstructionMethodT>
class ConstructionMethodMachine
{
public:
    ConstructionMethodMachine(
        ConstructionMethodT constructionmethod = static_cast<ConstructionMethodT>(0)
    )
        : ConstructionMode(constructionmethod)
    {}
    virtual ~ConstructionMethodMachine()
    {}

protected:
    void setConstructionMethod(ConstructionMethodT mode)
    {
        ConstructionMode = mode;
        onConstructionMethodChanged();
    }

    ConstructionMethodT constructionMethod() const
    {
        return ConstructionMode;
    }

    bool isConstructionMethod(ConstructionMethodT state) const
    {
        return ConstructionMode == state;
    }

    void resetConstructionMode()
    {
        ConstructionMode = static_cast<ConstructionMethodT>(0);
    }

    void initConstructionMethod(ConstructionMethodT mode)
    {
        ConstructionMode = mode;
    }

    // Cyclically iterate construction methods
    ConstructionMethodT getNextMethod() const
    {
        auto modeint = static_cast<int>(ConstructionMode);


        if (modeint < (maxMode - 1)) {
            auto newmode = static_cast<ConstructionMethodT>(modeint + 1);
            return newmode;
        }
        else {
            return static_cast<ConstructionMethodT>(0);
        }
    }

    void iterateToNextConstructionMethod()
    {
        if (ConstructionMethodsCount() > 1) {
            setConstructionMethod(getNextMethod());
        }
    }

    virtual void onConstructionMethodChanged() {};

    static constexpr int ConstructionMethodsCount()
    {
        return maxMode;
    }

private:
    ConstructionMethodT ConstructionMode;
    static const constexpr int maxMode = static_cast<int>(ConstructionMethodT::End);
};

/** @brief A DrawSketchHandler template for geometry creation.
 *
 * @details
 * A state machine DrawSketchHandler providing:
 * - generic initialisation including setting the cursor
 * - structured command finalisation
 * - handling of continuous creation mode
 *
 * This class is intended to be used by instantiating the template with a new DSH type, and
 * then derive the new type from the instantiated template. This allows one to inherit all
 * the functionality, have direct access to all handler members, while allowing the DSH creator to
 * add additional data members and functions (and avoiding extensive usage of macros).
 *
 * Example:
 *
 * class DrawSketchHandlerPoint;
 *
 * using DrawSketchHandlerPointBase =
 *     DrawSketchDefaultHandler<DrawSketchHandlerPoint,
 *                              StateMachines::OneSeekEnd,
 *                              1>;
 *
 * class DrawSketchHandlerPoint: public DrawSketchHandlerPointBase
 * {
 *     ...
 * }
 *
 * This approach enables seamless upgrading to a default or custom widget DSH.
 *
 * This class provides an NVI interface for extension.
 *
 * Question 1: Do I need to use this handler or derive from this handler to make a new handler?
 *
 * No, you do not NEED to. But you are encouraged to. Structuring a handler following this NVI,
 * apart from savings in amount of code typed, enables a much easier and less verbose implementation
 * of a handler using a default widget (toolwidget).
 *
 * For handlers using a custom widget it will also help by structuring the code in a way consistent
 * with other handlers. It will result in an easier to maintain code.
 *
 * Question 2: I want to use the default widget, do I need to use this handler or derive from this
 * handler?
 *
 * You should use DrawSketchDefaultWidgetHandler instead. However, both classes use the same
 * interface, so if you derive from this class when implementing your handler and then decide to use
 * the tool widget, all you have to do is to change the base class from DrawSketchDefaultHandler to
 * DrawSketchDefaultWidgetHandler. Then you will have to implement the code that is exclusively
 * necessary for the default widget to work.
 */
template<
    typename HandlerT,            // A type for which the handler template is instantiated
    typename SelectModeT,         // The state machine defining the states that the handle iterates
    int PInitAutoConstraintSize,  // The initial size of the AutoConstraint>
    typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod>
class DrawSketchDefaultHandler: public DrawSketchHandler,
                                public StateMachine<SelectModeT>,
                                public ConstructionMethodMachine<ConstructionMethodT>
{
public:
    DrawSketchDefaultHandler(
        ConstructionMethodT constructionmethod = static_cast<ConstructionMethodT>(0)
    )
        : ConstructionMethodMachine<ConstructionMethodT>(constructionmethod)
        , sugConstraints(PInitAutoConstraintSize)
        , avoidRedundants(true)
        , continuousMode(true)
    {
        applyCursor();
    }

    ~DrawSketchDefaultHandler() override
    {}

    /** @name public DrawSketchHandler interface
     * NOTE: Not intended to be specialised. It calls some functions intended to be
     * overridden/specialised instead.
     */
    //@{
    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        updateDataAndDrawToPosition(snapHandle.compute());
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {

        onButtonPressed(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        finish();
        return true;
    }

    void registerPressedKey(bool pressed, int key) override
    {
        if (key == SoKeyboardEvent::M && pressed && !this->isLastState()) {
            this->iterateToNextConstructionMethod();
        }
        else if (key == SoKeyboardEvent::ESCAPE && pressed) {
            rightButtonOrEsc();
        }
    }

    void pressRightButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        rightButtonOrEsc();
    }

    virtual void rightButtonOrEsc()
    {
        if (this->isFirstState()) {
            quit();
        }
        else {
            handleContinuousMode();
        }
    }

    //@}


protected:
    using SelectMode = SelectModeT;
    using ModeStateMachine = StateMachine<SelectModeT>;
    using ConstructionMethod = ConstructionMethodT;
    using ConstructionMachine = ConstructionMethodMachine<ConstructionMethodT>;

    /** @name functions NOT intended for specialisation or to be hidden
        These functions define a predefined structure and are extendable using NVI.
        1. Call them from your handle
        2. Do not hide them or otherwise redefine them UNLESS YOU HAVE A REALLY GOOD REASON TO
        3. EXTEND their functionality, if needed, using the NVI interface (or if you do not need to
       derive, by specialising these functions).*/
    //@{
    /** @brief This function finalises the creation operation. It works only if the state machine is
     * in state End.
     *
     * @details
     * The functionality need to be provided by extending these virtual private functions:
     * 1. executeCommands() : Must be provided with the Commands to create the geometry
     * 2. generateAutoConstraints() : When using AutoConstraints vector, this function populates the
     * AutoConstraints vector, ensuring no redundant autoconstraints
     * 2. beforeCreateAutoConstraints() : Enables derived classes to define specific actions before
     * executeCommands and createAutoConstraints (optional).
     * 3. createAutoConstraints() : Must be provided with the commands to create autoconstraints
     *
     * It recomputes if not solves and handles continuous mode automatically
     *
     * It returns true if the handler has been purged.
     */
    bool finish()
    {
        if (this->isState(SelectMode::End)) {
            unsetCursor();
            resetPositionText();

            try {
                executeCommands();

                if (sugConstraints.size() > 0) {
                    beforeCreateAutoConstraints();

                    generateAutoConstraints();

                    createAutoConstraints();
                }
            }
            catch (const Base::RuntimeError& e) {
                // RuntimeError exceptions inside of the block above must provide a translatable
                // message. It is reported both to developer (report view) and user (notifications
                // area).
                Base::Console().error(e.what());
            }

            // Keep the recompute separate so that everything is drawn even if execution fails
            // partially
            try {
                tryAutoRecomputeIfNotSolve(sketchgui->getSketchObject());
            }
            catch (const Base::RuntimeError& e) {
                // RuntimeError exceptions inside of the block above must provide a translatable
                // message. It is reported both to developer (report view) and user (notifications
                // area).
                Base::Console().error(e.what());
            }
            return handleContinuousMode();
        }
        return false;
    }

    /** @brief This function resets the handler to the initial state.
     *
     * @details
     * The functionality can be extended using these virtual private function:
     * 1. onReset() : Any further initialisation applicable to your handler
     *
     * It clears the edit curve, resets the state machine and resizes edit curve and autoconstraints
     * to initial size. Reapplies the cursor bitmap.
     */
    void reset()
    {
        clearEdit();

        for (auto& ac : sugConstraints) {
            ac.clear();
        }

        AutoConstraints.clear();
        ShapeGeometry.clear();
        ShapeConstraints.clear();

        onReset();

        ModeStateMachine::reset();

        applyCursor();
    }

    /** @brief This function handles the geometry continuous mode.
     *
     * @details
     * The functionality can be extended using the virtual private function called from reset(),
     * namely:
     * 1. onReset() : Any further initialisation applicable to your handler
     *
     * It performs all the operations in reset().
     */
    bool handleContinuousMode()
    {
        if (continuousMode) {
            // This code enables the continuous creation mode.
            reset();
            // It is ok not to call to purgeHandler in continuous creation mode because the
            // handler is destroyed by the quit() method on pressing the right button of the mouse
            return false;
        }
        else {
            sketchgui->purgeHandler();  // no code after, Handler get deleted in ViewProvider
            return true;
        }
    }
    //@}

private:
    /** @name functions are intended to be overridden/specialised to extend basic functionality
        NVI interface. See documentation of the functions above.*/
    //@{
    virtual void onReset()
    {}
    virtual void executeCommands()
    {}
    virtual void generateAutoConstraints()
    {}
    virtual void beforeCreateAutoConstraints()
    {}
    virtual void createAutoConstraints()
    {}

    void onConstructionMethodChanged() override {};

    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    };

    virtual void angleSnappingControl()
    {}

    // function intended to populate ShapeGeometry and ShapeConstraints
    virtual void createShape(bool onlyeditoutline)
    {
        Q_UNUSED(onlyeditoutline)
    }
    //@}
protected:
    /** @name functions are intended to be overridden/specialised to extend basic functionality
        See documentation of the functions above*/
    //@{
    /** @brief Minimal handle activation respecting avoid redundants and continuous mode.*/
    void activated() override
    {
        avoidRedundants = sketchgui->AvoidRedundant.getValue()
            && sketchgui->Autoconstraints.getValue();

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher"
        );

        continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
    }

    /** @brief Default button pressing implementation, which redraws and moves to the next machine
     * state, until reaching end.
     *
     * If this behaviour is not acceptable, then the function must be specialised (or overloaded).*/
    virtual void onButtonPressed(Base::Vector2d onSketchPos)
    {
        this->updateDataAndDrawToPosition(onSketchPos);
        if (canGoToNextMode()) {
            this->moveToNextMode();
        }
    }

    virtual bool canGoToNextMode()
    {
        return true;
    }

    /** @brief Default behaviour that upon arriving to the End state of the state machine, the
     * command is finished. */
    bool onModeChanged() override
    {
        angleSnappingControl();
        // internally checks that state is SelectMode::End, and only finishes then.
        return !finish();
    };
    //@}

    /** @name Helper functions
        See documentation of the functions above*/
    //@{

    // TODO: Figure out and explain what it actually returns
    bool generateOneAutoConstraintFromSuggestion(
        const AutoConstraint& ac,
        int geoId1,
        Sketcher::PointPos posId1
    )
    {
        int geoId2 = ac.GeoId;
        Sketcher::PointPos posId2 = ac.PosId;

        static const auto isStartOrEnd = [](const Sketcher::PointPos posId) {
            return posId == Sketcher::PointPos::start || posId == Sketcher::PointPos::end;
        };


        switch (ac.Type) {
            case Sketcher::Coincident: {
                if (posId1 == Sketcher::PointPos::none) {
                    return true;
                }

                // find if there is already a matching tangency
                auto itOfTangentConstraint = AutoConstraints.end();
                if (isStartOrEnd(posId1) && isStartOrEnd(posId2)) {
                    itOfTangentConstraint = std::ranges::find(
                        AutoConstraints,
                        std::tuple {Sketcher::Tangent, geoId1, geoId2},
                        [](const auto& ace) {
                            return std::tuple {ace->Type, ace->First, ace->Second};
                        }
                    );
                }

                if (itOfTangentConstraint != AutoConstraints.end()) {
                    // modify tangency to endpoint-to-endpoint
                    (*itOfTangentConstraint)->FirstPos = posId1;
                    (*itOfTangentConstraint)->SecondPos = posId2;
                }
                else {
                    auto c = std::make_unique<Sketcher::Constraint>();
                    c->Type = Sketcher::Coincident;
                    c->First = geoId1;
                    c->FirstPos = posId1;
                    c->Second = geoId2;
                    c->SecondPos = posId2;
                    AutoConstraints.push_back(std::move(c));
                }
            } break;
            case Sketcher::PointOnObject: {
                if (posId1 == Sketcher::PointPos::none) {
                    // Auto constraining an edge so swap parameters
                    std::swap(geoId1, geoId2);
                    std::swap(posId1, posId2);
                }

                auto itOfTangentConstraint = AutoConstraints.end();
                if (isStartOrEnd(posId1)) {
                    itOfTangentConstraint = std::ranges::find_if(AutoConstraints, [&](const auto& ace) {
                        return ace->Type == Sketcher::Tangent && ace->involvesGeoId(geoId1)
                            && ace->involvesGeoId(geoId2);
                    });
                }

                // if tangency, convert to point-to-edge tangency
                if (itOfTangentConstraint != AutoConstraints.end()) {
                    if ((*itOfTangentConstraint)->First != geoId1) {
                        std::swap((*itOfTangentConstraint)->Second, (*itOfTangentConstraint)->First);
                    }

                    (*itOfTangentConstraint)->FirstPos = posId1;
                }
                else {
                    auto c = std::make_unique<Sketcher::Constraint>();
                    c->Type = Sketcher::PointOnObject;
                    c->First = geoId1;
                    c->FirstPos = posId1;
                    c->Second = geoId2;
                    AutoConstraints.push_back(std::move(c));
                }
            } break;
            case Sketcher::Symmetric: {
                auto c = std::make_unique<Sketcher::Constraint>();
                c->Type = Sketcher::Symmetric;
                c->First = geoId2;
                c->FirstPos = Sketcher::PointPos::start;
                c->Second = geoId2;
                c->SecondPos = Sketcher::PointPos::end;
                c->Third = geoId1;
                c->ThirdPos = posId1;
                AutoConstraints.push_back(std::move(c));
            } break;
            // In special case of Horizontal/Vertical constraint, geoId2 is normally
            // unused and should be 'Constraint::GeoUndef' However it can be used as a
            // way to require the function to apply these constraints on another
            // geometry In this case the caller as to set geoId2, then it will be used
            // as target instead of geoId2
            case Sketcher::Horizontal:
            case Sketcher::Vertical: {
                auto c = std::make_unique<Sketcher::Constraint>();
                c->Type = ac.Type;
                c->First = (geoId2 != Sketcher::GeoEnum::GeoUndef ? geoId2 : geoId1);
                AutoConstraints.push_back(std::move(c));
            } break;
            case Sketcher::Tangent: {
                Sketcher::SketchObject* Obj = sketchgui->getObject<Sketcher::SketchObject>();

                const Part::Geometry* geom1 = Obj->getGeometry(geoId1);
                const Part::Geometry* geom2 = Obj->getGeometry(geoId2);

                // ellipse tangency support using construction elements (lines)
                if (geom1 && geom2
                    && (geom1->is<Part::GeomEllipse>() || geom2->is<Part::GeomEllipse>())) {
                    if (!geom1->is<Part::GeomEllipse>()) {
                        std::swap(geoId1, geoId2);
                    }

                    // geoId1 is the ellipse
                    geom1 = Obj->getGeometry(geoId1);
                    geom2 = Obj->getGeometry(geoId2);

                    if (geom2->is<Part::GeomEllipse>() || geom2->is<Part::GeomArcOfEllipse>()
                        || geom2->is<Part::GeomCircle>() || geom2->is<Part::GeomArcOfCircle>()) {
                        // in all these cases an intermediate element is needed
                        // makeTangentToEllipseviaNewPoint(
                        //     Obj,
                        //     static_cast<const Part::GeomEllipse*>(geom1),
                        //     geom2,
                        //     geoId1,
                        //     geoId2);
                        // NOTE: Temporarily deactivated
                        return false;
                    }
                }

                // arc of ellipse tangency support using external elements
                if (geom1 && geom2
                    && (geom1->is<Part::GeomArcOfEllipse>() || geom2->is<Part::GeomArcOfEllipse>())) {
                    if (!geom1->is<Part::GeomArcOfEllipse>()) {
                        std::swap(geoId1, geoId2);
                    }

                    // geoId1 is the arc of ellipse
                    geom1 = Obj->getGeometry(geoId1);
                    geom2 = Obj->getGeometry(geoId2);

                    if (geom2->is<Part::GeomArcOfEllipse>() || geom2->is<Part::GeomCircle>()
                        || geom2->is<Part::GeomArcOfCircle>()) {
                        // in all these cases an intermediate element is needed
                        // makeTangentToArcOfEllipseviaNewPoint(
                        //     Obj,
                        //     static_cast<const Part::GeomArcOfEllipse*>(geom1),
                        //     geom2,
                        //     geoId1,
                        //     geoId2);
                        // NOTE: Temporarily deactivated
                        return false;
                    }
                }

                auto resultCoincident = std::ranges::find_if(AutoConstraints, [&](const auto& ace) {
                    return ace->Type == Sketcher::Coincident && ace->First == geoId1
                        && ace->Second == geoId2;
                });

                auto resultPointOnObject = std::ranges::find_if(AutoConstraints, [&](const auto& ace) {
                    return ace->Type == Sketcher::PointOnObject && ace->involvesGeoId(geoId1)
                        && ace->involvesGeoId(geoId2);
                });

                if (resultCoincident != AutoConstraints.end()
                    && isStartOrEnd((*resultCoincident)->FirstPos)
                    && isStartOrEnd((*resultCoincident)->SecondPos)) {
                    // endpoint-to-endpoint tangency
                    (*resultCoincident)->Type = Sketcher::Tangent;
                }
                else if (resultPointOnObject != AutoConstraints.end()
                         && isStartOrEnd((*resultPointOnObject)->FirstPos)) {
                    // endpoint-to-edge tangency
                    (*resultPointOnObject)->Type = Sketcher::Tangent;
                }
                else if (resultCoincident != AutoConstraints.end()
                         && (*resultCoincident)->FirstPos == Sketcher::PointPos::mid
                         && (*resultCoincident)->SecondPos == Sketcher::PointPos::mid && geom1 && geom2
                         && (geom1->is<Part::GeomCircle>() || geom1->is<Part::GeomArcOfCircle>())
                         && (geom2->is<Part::GeomCircle>() || geom2->is<Part::GeomArcOfCircle>())) {
                    // equality
                    auto c = std::make_unique<Sketcher::Constraint>();
                    c->Type = Sketcher::Equal;
                    c->First = geoId1;
                    c->Second = geoId2;
                    AutoConstraints.push_back(std::move(c));
                }
                else {  // regular edge to edge tangency
                    auto c = std::make_unique<Sketcher::Constraint>();
                    c->Type = Sketcher::Tangent;
                    c->First = geoId1;
                    c->Second = geoId2;
                    AutoConstraints.push_back(std::move(c));
                }
            } break;
            default:
                break;
        }

        return true;
    }

    /** @brief Convenience function to automatically generate and add to the AutoConstraints vector
     *  the suggested constraints.
     *
     * This generates actual Sketcher::Constraint which can be used for diagnostic before addition.
     */
    void generateAutoConstraintsOnElement(
        const std::vector<AutoConstraint>& autoConstrs,
        int geoId1,
        Sketcher::PointPos posId1
    )
    {
        if (!sketchgui->Autoconstraints.getValue()) {
            return;
        }

        for (auto& ac : autoConstrs) {
            if (!generateOneAutoConstraintFromSuggestion(ac, geoId1, posId1)) {
                return;
            }
        }
    }

    /** @brief Convenience function to automatically add to the SketchObjects (via Python command)
     * all the constraints stored in the AutoConstraints vector. */
    void createGeneratedAutoConstraints(bool owncommand)
    {
        try {
            // add auto-constraints
            if (owncommand) {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Auto-Constraints"));
            }

            tryAddAutoConstraints();

            if (owncommand) {
                Gui::Command::commitCommand();
            }
        }
        catch (const Base::PyException&) {
            if (owncommand) {
                Gui::Command::abortCommand();
            }
        }
    }

    /** @brief Convenience function to automatically add to the SketchObjects
     * all the constraints stored in the AutoConstraints vector. */
    void tryAddAutoConstraints()
    {
        auto autoConstraints = toPointerVector(AutoConstraints);

        Gui::Command::doCommand(
            Gui::Command::Doc,
            Sketcher::PythonConverter::convert(
                Gui::Command::getObjectCmd(sketchgui->getObject()),
                autoConstraints
            )
                .c_str()
        );
    }

    /** @brief Convenience function to remove redundant autoconstraints from the AutoConstraints
     * vector (before having added them to the SketchObject).
     *
     * @details The rationale is that constraints associated with the newly generated geometry SHALL
     * not be redundant by design. Then, the only source of redundancy lies with the
     * autoconstraints. This observation is also what motivates the separate treatment of both sets
     * of constraints.
     *
     * This approach avoids the previous issues with old style DSHs, that the autoremoval mechanism
     * sometimes removed a shape constraint, or a previously already existing constraint, instead of
     * an autoconstraint.
     */
    void removeRedundantAutoConstraints()
    {

        if (AutoConstraints.empty()) {
            return;
        }

        auto sketchobject = getSketchObject();

        auto autoConstraints = toPointerVector(AutoConstraints);

        // Allows a diagnose with the new autoconstraints as if they were part of the sketchobject,
        // but WITHOUT adding them to the sketchobject..
        sketchobject->diagnoseAdditionalConstraints(autoConstraints);

        if (sketchobject->getLastHasRedundancies()) {
            Base::Console().warning(
                QT_TRANSLATE_NOOP("Notifications", "Autoconstraints cause redundancy. Removing them") "\n"
            );

            auto lastsketchconstraintindex = sketchobject->Constraints.getSize() - 1;

            auto redundants = sketchobject->getLastRedundant();  // redundants is always sorted

            for (int index = redundants.size() - 1; index >= 0; index--) {
                int redundantconstraintindex = redundants[index] - 1;
                if (redundantconstraintindex > lastsketchconstraintindex) {
                    int removeindex = redundantconstraintindex - lastsketchconstraintindex - 1;
                    AutoConstraints.erase(std::next(AutoConstraints.begin(), removeindex));
                }
                else {
                    // This exception stops the procedure here, which means that:
                    // 1) Geometry (and constraints of the geometry in case of a multicurve shape)
                    // are created 2) No autoconstrains are actually added 3) No widget mandated
                    // constraints are added
                    THROWM(
                        Base::RuntimeError,
                        QT_TRANSLATE_NOOP(
                            "Notifications",
                            "Redundant constraint is not an autoconstraint. No autoconstraints "
                            "or additional constraints were added. Please report!"
                        ) "\n"
                    );
                }
            }

            // NOTE: If we removed all redundants in the list, then at this moment there are no
            // redundants anymore
        }

        // This can happen if OVP generated constraints and autoconstraints are conflicting
        // For instance : https://github.com/FreeCAD/FreeCAD/issues/17722
        if (sketchobject->getLastHasConflicts()) {
            auto lastsketchconstraintindex = sketchobject->Constraints.getSize() - 1;

            auto conflicting = sketchobject->getLastConflicting();

            for (int index = conflicting.size() - 1; index >= 0; index--) {
                int conflictingIndex = conflicting[index] - 1;
                if (conflictingIndex > lastsketchconstraintindex) {
                    int removeindex = conflictingIndex - lastsketchconstraintindex - 1;
                    AutoConstraints.erase(std::next(AutoConstraints.begin(), removeindex));
                }
            }
        }
    }

    /** @brief Function that performs a sketcher solver diagnose (determination of DoF and dependent
     * parameters), taking into account the suggested AutoConstraints.
     *
     * @details This function allows one to refresh solver information by taking into account any
     * added constraint, such as the ones introduced by a widget or on-screen parameters during the
     * execution of the DSH.
     *
     * Ultimately, it is intended to operate in combination with functions obtaining point/element
     * specific solver information, in order to decide whether to add constraints or not.
     */
    void diagnoseWithAutoConstraints()
    {
        auto sketchobject = getSketchObject();

        auto autoConstraints = toPointerVector(AutoConstraints);

        sketchobject->diagnoseAdditionalConstraints(autoConstraints);

        if (sketchobject->getLastHasRedundancies() || sketchobject->getLastHasConflicts()) {
            THROWM(
                Base::RuntimeError,
                QT_TRANSLATE_NOOP(
                    "Notifications",
                    "Unexpected Redundancy/Conflicting constraint. Check the "
                    "constraints and autoconstraints of this operation."
                ) "\n"
            );
        }
    }

    /** @brief Function to obtain detailed solver information on one point type geometric element.*/
    Sketcher::SolverGeometryExtension::PointParameterStatus getPointInfo(
        const Sketcher::GeoElementId& element
    )
    {
        if (element.isCurve()) {
            THROWM(Base::TypeError, "getPointInfo: Provided geometry element is not a point!")
        }

        auto sketchobject = getSketchObject();
        // it is important not to rely on Geometry attached extension from the solver, as geometry
        // is not updated during temporary diagnose of additional constraints
        const auto& solvedsketch = sketchobject->getSolvedSketch();

        auto solvext = solvedsketch.getSolverExtension(element.GeoId);

        if (solvext) {
            auto pointinfo = solvext->getPoint(element.Pos);

            return pointinfo;
        }

        THROWM(
            Base::ValueError,
            "Geometry element does not have solver information (possibly when trying to apply "
            "widget constraints)!"
        )
    }

    /** @brief Function to obtain detailed DoFs of one line type geometric element.*/
    int getLineDoFs(int geoid)
    {
        auto startpointinfo = getPointInfo(Sketcher::GeoElementId(geoid, Sketcher::PointPos::start));

        auto endpointinfo = getPointInfo(Sketcher::GeoElementId(geoid, Sketcher::PointPos::end));

        int DoFs = startpointinfo.getDoFs();
        DoFs += endpointinfo.getDoFs();

        return DoFs;
    }

    /** @brief Function to obtain detailed solver information on one edge.*/
    Sketcher::SolverGeometryExtension::EdgeParameterStatus getEdgeInfo(int geoid)
    {

        auto sketchobject = getSketchObject();
        // it is important not to rely on Geometry attached extension from the solver, as geometry
        // is not updated during temporary diagnose of additional constraints
        const auto& solvedsketch = sketchobject->getSolvedSketch();

        auto solvext = solvedsketch.getSolverExtension(geoid);

        if (solvext) {
            Sketcher::SolverGeometryExtension::EdgeParameterStatus edgeinfo
                = solvext->getEdgeParameters();

            return edgeinfo;
        }

        THROWM(
            Base::ValueError,
            "Geometry does not have solver extension when trying to apply widget constraints!"
        )
    }

    /** @brief Function to add shape inherent constraints (the ones that define the shape) to the
     * ShapeConstraints vector.
     *
     * @details These functions have the highest priority when adding the geometry as they are
     * inherent part of it and the shape would not go without them. Lower priority constraints are
     * AutoConstraints and constraints mandated by the widget/on-screen parameters.
     * .*/
    auto addToShapeConstraints(
        Sketcher::ConstraintType type,
        int first,
        Sketcher::PointPos firstPos = Sketcher::PointPos::none,
        int second = Sketcher::GeoEnum::GeoUndef,
        Sketcher::PointPos secondPos = Sketcher::PointPos::none,
        int third = Sketcher::GeoEnum::GeoUndef,
        Sketcher::PointPos thirdPos = Sketcher::PointPos::none
    )
    {
        auto constr = std::make_unique<Sketcher::Constraint>();
        constr->Type = type;
        constr->First = first;
        constr->FirstPos = firstPos;
        constr->Second = second;
        constr->SecondPos = secondPos;
        constr->Third = third;
        constr->ThirdPos = thirdPos;
        return ShapeConstraints.emplace_back(std::move(constr)).get();
    }

    /** @brief Function to add a line to the ShapeGeometry vector.*/
    auto addLineToShapeGeometry(Base::Vector3d p1, Base::Vector3d p2, bool constructionMode)
    {
        auto line = std::make_unique<Part::GeomLineSegment>();
        line->setPoints(p1, p2);
        Sketcher::GeometryFacade::setConstruction(line.get(), constructionMode);
        return static_cast<Part::GeomLineSegment*>(ShapeGeometry.emplace_back(std::move(line)).get());
    }

    /** @brief Function to add an arc to the ShapeGeometry vector.*/
    auto addArcToShapeGeometry(Base::Vector3d p1, double start, double end, double radius, bool constructionMode)
    {
        auto arc = std::make_unique<Part::GeomArcOfCircle>();
        arc->setCenter(p1);
        arc->setRange(start, end, true);
        arc->setRadius(radius);
        Sketcher::GeometryFacade::setConstruction(arc.get(), constructionMode);
        return static_cast<Part::GeomArcOfCircle*>(ShapeGeometry.emplace_back(std::move(arc)).get());
    }

    /** @brief Function to add a point to the ShapeGeometry vector.*/
    auto addPointToShapeGeometry(Base::Vector3d p1, bool constructionMode)
    {
        auto point = std::make_unique<Part::GeomPoint>();
        point->setPoint(p1);
        Sketcher::GeometryFacade::setConstruction(point.get(), constructionMode);
        return static_cast<Part::GeomPoint*>(ShapeGeometry.emplace_back(std::move(point)).get());
    }

    /** @brief Function to add an ellipse to the ShapeGeometry vector.*/
    auto addEllipseToShapeGeometry(
        Base::Vector3d centerPoint,
        Base::Vector3d majorAxisDirection,
        double majorRadius,
        double minorRadius,
        bool constructionMode
    )
    {
        auto ellipse = std::make_unique<Part::GeomEllipse>();
        ellipse->setMajorRadius(majorRadius);
        ellipse->setMinorRadius(minorRadius);
        ellipse->setMajorAxisDir(majorAxisDirection);
        ellipse->setCenter(centerPoint);
        Sketcher::GeometryFacade::setConstruction(ellipse.get(), constructionMode);
        return static_cast<Part::GeomEllipse*>(ShapeGeometry.emplace_back(std::move(ellipse)).get());
    }

    /** @brief Function to add a circle to the ShapeGeometry vector.*/
    auto addCircleToShapeGeometry(Base::Vector3d centerPoint, double radius, bool constructionMode)
    {
        auto circle = std::make_unique<Part::GeomCircle>();
        circle->setRadius(radius);
        circle->setCenter(centerPoint);
        Sketcher::GeometryFacade::setConstruction(circle.get(), constructionMode);
        return static_cast<Part::GeomCircle*>(ShapeGeometry.emplace_back(std::move(circle)).get());
    }

    /** @brief Function to add all the geometries and constraints in ShapeGeometry and
     * ShapeConstraints vectors to the SketchObject.*/
    void commandAddShapeGeometryAndConstraints()
    {
        auto shapeGeometry = toPointerVector(ShapeGeometry);
        std::string sketchObj = Gui::Command::getObjectCmd(sketchgui->getObject());
        Gui::Command::doCommand(Gui::Command::Doc, "ActiveSketch = %s\n", sketchObj.c_str());
        Gui::Command::doCommand(
            Gui::Command::Doc,
            Sketcher::PythonConverter::convert(
                sketchObj,
                shapeGeometry,
                Sketcher::PythonConverter::Mode::OmitInternalGeometry
            )
                .c_str()
        );

        size_t initialConstraintCount = sketchgui->getSketchObject()->Constraints.getSize();
        auto shapeConstraints = toPointerVector(ShapeConstraints);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            Sketcher::PythonConverter::convert(sketchObj, shapeConstraints).c_str()
        );

        reassignVirtualSpace(initialConstraintCount);
    }

    /** @brief Function to draw as an edit curve all the geometry in the ShapeGeometry vector.*/
    void DrawShapeGeometry()
    {
        drawEdit(toPointerVector(ShapeGeometry));
    }

    /** @brief Function to create a shape into ShapeGeometry vector and draw it.*/
    void CreateAndDrawShapeGeometry()
    {
        createShape(true);
        drawEdit(toPointerVector(ShapeGeometry));
    }

    //@}

private:
    // Reassign the correct virtual space index for the added constraints
    void reassignVirtualSpace(size_t startIndex)
    {
        if (ShapeConstraints.empty()) {
            return;
        }

        std::stringstream stream;
        bool hasConstraintsInVirtualSpace = false;
        for (size_t i = 0; i < ShapeConstraints.size(); ++i) {
            if (ShapeConstraints[i]->isInVirtualSpace) {
                if (hasConstraintsInVirtualSpace) {
                    stream << ",";
                }
                stream << i + startIndex;
                hasConstraintsInVirtualSpace = true;
            }
        }
        if (!hasConstraintsInVirtualSpace) {
            return;
        }

        try {
            Gui::cmdAppObjectArgs(
                sketchgui->getObject(),
                "setVirtualSpace([%s], True)",
                stream.str().c_str()
            );
        }
        catch (const Base::Exception& e) {
            Base::Console().error("%s\n", e.what());
        }
    }

protected:
    std::vector<std::vector<AutoConstraint>> sugConstraints;

    std::vector<std::unique_ptr<Part::Geometry>> ShapeGeometry;
    std::vector<std::unique_ptr<Sketcher::Constraint>> ShapeConstraints;
    std::vector<std::unique_ptr<Sketcher::Constraint>> AutoConstraints;

    bool avoidRedundants;
    bool continuousMode;
};

}  // namespace SketcherGui
