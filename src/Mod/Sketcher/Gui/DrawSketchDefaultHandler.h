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


#ifndef SKETCHERGUI_DrawSketchDefaultHandler_H
#define SKETCHERGUI_DrawSketchDefaultHandler_H

#include "DrawSketchHandler.h"

#include "Utils.h"

namespace bp = boost::placeholders;

namespace SketcherGui {

/*********************** Ancillary classes for DrawSketch Hierarchy *******************************/

namespace StateMachines {

enum class OneSeekEnd {
    SeekFirst,
    End // MUST be the last one
};

enum class TwoSeekEnd {
    SeekFirst,
    SeekSecond,
    End // MUST be the last one
};

enum class ThreeSeekEnd {
    SeekFirst,
    SeekSecond,
    SeekThird,
    End // MUST be the last one
};

enum class FourSeekEnd {
    SeekFirst,
    SeekSecond,
    SeekThird,
    SeekFourth,
    End // MUST be the last one
};

enum class TwoSeekDoEnd {
    SeekFirst,
    SeekSecond,
    Do,
    End // MUST be the last one
};

} // namespace StateMachines

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
 * NOTE: The machine provided MUST include a last state named End.
 */
template <typename SelectModeT>
class StateMachine
{
public:
    StateMachine():Mode(static_cast<SelectModeT>(0)) {}
    virtual ~StateMachine(){}

protected:
    void setState(SelectModeT mode) {
        Mode = mode;
        onModeChanged();
    }

    SelectModeT state() {
        return Mode;
    }

    bool isState(SelectModeT state) { return Mode == state;}

    SelectModeT getNextMode() {
        auto modeint = static_cast<int>(state());


        if(modeint < maxMode) {
            auto newmode = static_cast<SelectModeT>(modeint+1);
            return newmode;
        }
        else {
            return SelectModeT::End;
        }
    }

    void moveToNextMode() {
        setState(getNextMode());
    }

    void reset() {
        setState(static_cast<SelectModeT>(0));
    }

    virtual void onModeChanged() {};

private:
    SelectModeT Mode;
    static const constexpr int maxMode = static_cast<int>(SelectModeT::End);

};

/** @brief A state machine DrawSketchHandler for geometry.
 *
 * @details
 * A state machine DrawSketchHandler defining a EditCurve and AutoConstraints, providing:
 * - generic initialisation including setting the cursor
 * - structured command finalisation
 * - handling of continuous creation mode
 *
 * Two ways of using it:
 * 1. By instanting and specialising functions.
 * 2. By creating a new class deriving from this one
 *
 * You need way 2 if you must add additional data members to your class.
 *
 * This class provides an NVI interface for extension. Alternatively, specialisation of those functions
 * may be effected if it is opted to instantiate and specialise.
 *
 * Template Types/Parameters:
 * PTool : Parameter to specialise behaviour to a specific tool
 * SelectModeT : The type of statemachine to be used (see namespace StateMachines above).
 * PInitEditCurveSize : Initial size of the EditCurve vector
 * PInitAutoConstraintSize : Initial size of the AutoConstraint vector
 *
 * Question 1: Do I need to use this handler or derive from this handler to make a new hander?
 *
 * No, you do not NEED to. But you are encouraged to. Structuring a handler following this NVI, apart
 * from savings in amount of code typed, enables a much easier and less verbose implementation of a handler
 * using a default widget (toolwidget).
 *
 * For handlers using a custom widget it will also help by structuring the code in a way consistent with other handlers.
 * It will result in an easier to maintain code.
 *
 * Question 2: I want to use the default widget, do I need to use this handler or derive from this handler?
 *
 * You should use DrawSketchDefaultWidgetHandler instead. However, both clases use the same interface, so if you derive from
 * this class when implementing your handler and then decide to use the tool widget, all you have to do is to change
 * the base class from DrawSketchDefaultHandler to DrawSketchDefaultWidgetHandler. Then you will have to implement the code that
 * is exclusively necessary for the default widget to work.
 */
template < typename PHandler,         // The geometry tool for which the template is created (See GeometryTools above)
           typename SelectModeT,        // The state machine defining the states that the handle iterates
           int PInitEditCurveSize,      // The initial size of the EditCurve
           int PInitAutoConstraintSize> // The initial size of the AutoConstraint>
class DrawSketchDefaultHandler: public DrawSketchHandler, public StateMachine<SelectModeT>
{
public:
    DrawSketchDefaultHandler(): EditCurve(PInitEditCurveSize)
                                ,sugConstraints(PInitAutoConstraintSize)
    {
        applyCursor();
    }

    virtual ~DrawSketchDefaultHandler() = default;

    /** @name public DrawSketchHandler interface
     * NOTE: Not intended to be specialised. It calls some functions intended to be
     * overriden/specialised instead.
     */
    //@{
    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        updateDataAndDrawToPosition(onSketchPos);
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {

        onButtonPressed(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override {
        Q_UNUSED(onSketchPos);
        finish();
        return true;
    }
    //@}


protected:
    using SelectMode = SelectModeT;
    using ModeStateMachine = StateMachine<SelectModeT>;


    /** @name functions NOT intended for specialisation or to be hidden
        These functions define a predefined structure and are extendable using NVI.
        1. Call them from your handle
        2. Do not hide them or otherwise redefine them UNLESS YOU HAVE A REALLY GOOD REASON TO
        3. EXTEND their functionality, if needed, using the NVI interface (or if you do not need to derive, by specialising these functions).*/
    //@{
    /** @brief This function finalises the creation operation. It works only if the state machine is in state End.
    *
    * @details
    * The functionality need to be provided by extending these virtual private functions:
    * 1. executeCommands() : Must be provided with the Commands to create the geometry
    * 2. beforeCreateAutoConstraints() : Enables derived clases to define specific actions before executeCommands and createAutoConstraints (optional).
    * 3. createAutoConstraints() : Must be provided with the commands to create autoconstraints
    *
    * It recomputes if not solves and handles continuous mode automatically
    */
    void finish() {

        if(this->isState(SelectMode::End)) {
            unsetCursor();
            resetPositionText();

            executeCommands();

            beforeCreateAutoConstraints();

            createAutoConstraints();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            handleContinuousMode();
        }
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
    void reset() {
        size_t sizeOfEditCurve = EditCurve.size();
        EditCurve.clear();
        drawEdit(EditCurve);

        ModeStateMachine::reset();
        EditCurve.resize(sizeOfEditCurve);
        for(auto & ac : sugConstraints)
            ac.clear();

        onReset();
        applyCursor();
    }

    /** @brief This function handles the geometry continuous mode.
    *
    * @details
    * The functionality can be extended using the virtual private function called from reset(), namely:
    * 1. onReset() : Any further initialisation applicable to your handler
    *
    * It performs all the operations in reset().
    */
    void handleContinuousMode() {

        if(continuousMode){
            // This code enables the continuous creation mode.
            reset();
            // It is ok not to call to purgeHandler in continuous creation mode because the
            // handler is destroyed by the quit() method on pressing the right button of the mouse
        }
        else{
            sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
        }
    }
    //@}

private:
    /** @name functions are intended to be overriden/specialised to extend basic functionality
        NVI interface. See documentation of the functions above.*/
    //@{
    virtual void onReset() { }
    virtual void executeCommands() {}
    virtual void beforeCreateAutoConstraints() {}
    virtual void createAutoConstraints() {}

    virtual void onModeChanged() override { };

    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) {Q_UNUSED(onSketchPos)};
    //@}
protected:
    /** @name functions are intended to be overriden/specialised to extend basic functionality
        See documentation of the functions above*/
    //@{
    /// Handles avoid redundants and continuous mode, if overriden the base class must be called!
    virtual void activated() override {
        avoidRedundants = sketchgui->AvoidRedundant.getValue()  && sketchgui->Autoconstraints.getValue();

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

        continuousMode = hGrp->GetBool("ContinuousCreationMode",true);
    }

    // Default implementation is that on every mouse click it redraws and the mode is changed to the next seek
    // On the last seek, it changes to SelectMode::End
    // If this behaviour is not acceptable, then the function must be specialised (or overloaded).
    virtual void onButtonPressed(Base::Vector2d onSketchPos) {
        this->updateDataAndDrawToPosition(onSketchPos);
        this->moveToNextMode();
    }
    //@}

protected:
    std::vector<Base::Vector2d> EditCurve;
    std::vector<std::vector<AutoConstraint>> sugConstraints;

    bool avoidRedundants;
    bool continuousMode;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchDefaultHandler_H

