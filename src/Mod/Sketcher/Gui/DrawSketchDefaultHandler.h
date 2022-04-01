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

#include "ViewProviderSketch.h"

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

/************************ List of Handlers/Commands ************************************/

enum class GeometryTools {
    Line,
    Rectangle,
    Frame,
    Polyline, // also known as LineSet
    Arc,
    Circle,
    Ellipse,
    ArcOfEllipse,
    ArcOfHyperbola,
    ArcOfParabola,
    BSpline,
    Point,
    Fillet,
    Trim,
    Extend,
    Split,
    Insert,
    External,
    CarbonCopy,
    Slot,
    ArcSlot,
    Polygon,
    Rotate,
    Scale,
    Offset,
    Translate
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
 * You should use DSHandlerDefaultWidget instead. However, both clases use the same interface, so if you derive from
 * this class when implementing your handler and then decide to use the tool widget, all you have to do is to change
 * the base class from DrawSketchGeometryHandler to DSHandlerDefaultWidget. Then you will have to implement the code that
 * is exclusively necessary for the default widget to work.
 */
template < auto PTool,         // The geometry tool for which the template is created (See GeometryTools above)
           typename SelectModeT,        // The state machine defining the states that the handle iterates
           int PInitEditCurveSize,      // The initial size of the EditCurve
           int PInitAutoConstraintSize> // The initial size of the AutoConstraint>
class DrawSketchGeometryHandler: public DrawSketchHandler, public StateMachine<SelectModeT>
{
public:
    DrawSketchGeometryHandler(): EditCurve(PInitEditCurveSize)
                                ,sugConstraints(PInitAutoConstraintSize)
    {
        applyCursor();
    }

    virtual ~DrawSketchGeometryHandler() = default;

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

/** @brief Template class intended for handlers that interact with SketcherToolDefaultWidget.
 *
 * @details
 * The template encapsulates a DSH and a nested class ToolWidgetManager which is responsible for
 * handling the interaction between the DSH and SketcherToolDefaultWidget.
 *
 * This class can be used:
 * 1. By instantiating it and specialising it (both NVI and ToolWidgetManager).
 * 2. By deriving from it to implement the DSH via the NVI interface and by specialising the nested
 * ToolWidgetManager class functions.
 *
 * Method 2 enables to add new data members.
 *
 * Template Types/Parameters:
 * PTool : Parameter to specialise behaviour to a specific tool
 * SelectModeT : The type of statemachine to be used (see namespace StateMachines above).
 * PInitEditCurveSize : Initial size of the EditCurve vector
 * PInitAutoConstraintSize : Initial size of the AutoConstraint vector
 * PNumToolwidgetparameters: The number of parameter spinboxes in the default widget
 * PNumToolwidgetCheckboxes: The number of checkboxes in the default widget
 *
 * Widget:
 * - Automatically initialises the parameters necessary
 * - Automatically connects boost signals of the widget to slots.
 * - Interface to modify point coordinates based on widget state.
 * - Interface to update Widget with value.
 *
 * Widget MUST specialise:
 * - configureToolWidget() for example to initialise widget labels.
 * - doUpdateDrawSketchHandlerValues() to update EditData when Widget values are changed.
 * - addConstraints() to add any constraint mandated by the Widget.
 *
 * Widget MAY need to specialise, if default behaviour is not appropriate:
 * - doChangeDrawSketchHandlerMode()
 * - onHandlerModeChanged()
 * - updateVisualValues()
 * - doOverrideSketchPosition()
 *
 * Handler provides:
 * - generic initialisation
 * - structured command finalisation
 * - handling of continuous creation mode
 * - interaction with widget
 *
 * Handler MUST specialise:
 * - getToolName                            => provide the string name of the tool
 * - getCrosshairCursorString               => provide the string for the svg icon of the cursor
 * - updateEditDataAndDrawToPositionImpl    => function to update the EditData structure and draw a temporal curve
 * - executeCommands                        => execution of commands to create the geometry
 * - createAutoConstraints                  => execution of commands to create autoconstraints (widget mandated constraints are called BEFORE this)
 *
 * Question: Do I need to use this handler or derive from this handler to make a new hander using the default tool widget?
 *
 * No, you do not NEED to. But you are encouraged to. Structuring a handler following this NVI, apart
 * from substantial savings in amount of code typed, enables a much easier and less verbose implementation of a handler
 * using a default widget (toolwidget), and will lead to easier to maintain code.
 */

template< GeometryTools PTool,          // The geometry tool for which the template is created (See GeometryTools above)
          typename SelectModeT,         // The state machine defining the states that the handle iterates
          int PEditCurveSize,           // The initial size of the EditCurve
          int PAutoConstraintSize,      // The initial size of the AutoConstraint
          int PNumToolwidgetparameters, // The number of parameter spinboxes in the default widget
          int PNumToolwidgetCheckboxes, // The number of checkboxes in the default widget
          int PNumToolwidgetComboboxes > // The number of comboboxes in the default widget
class DSHandlerDefaultWidget: public DrawSketchGeometryHandler<PTool, SelectModeT, PEditCurveSize, PAutoConstraintSize>
{
    using DSGeometryHandler = DrawSketchGeometryHandler<PTool, SelectModeT, PEditCurveSize, PAutoConstraintSize>;

private:
    class ToolWidgetManager {
        int nParameter = PNumToolwidgetparameters;
        int nCheckbox = PNumToolwidgetCheckboxes;
        int nCombobox = PNumToolwidgetComboboxes;

        SketcherToolDefaultWidget* toolWidget;
        DSHandlerDefaultWidget * handler;

        using Connection = boost::signals2::connection;

        Connection connectionParameterValueChanged;
        Connection connectionCheckboxCheckedChanged;
        Connection connectionComboboxSelectionChanged;

        Base::Vector2d prevCursorPosition;

        using WParameter = SketcherToolDefaultWidget::Parameter;
        using WCheckbox  = SketcherToolDefaultWidget::Checkbox;
        using WCombobox = SketcherToolDefaultWidget::Combobox;
        using SelectMode = SelectModeT;

    public:
        ToolWidgetManager(DSHandlerDefaultWidget * dshandler):handler(dshandler){}

        ~ToolWidgetManager(){
            connectionParameterValueChanged.disconnect();
            connectionCheckboxCheckedChanged.disconnect();
            connectionComboboxSelectionChanged.disconnect();
        }

        /** @name functions NOT intended for specialisation */
        //@{
        void initWidget(QWidget* widget) {
            toolWidget = static_cast<SketcherToolDefaultWidget*>(widget);

            connectionParameterValueChanged = toolWidget->registerParameterValueChanged(boost::bind(&ToolWidgetManager::parameterValueChanged, this, bp::_1, bp::_2));

            connectionCheckboxCheckedChanged = toolWidget->registerCheckboxCheckedChanged(boost::bind(&ToolWidgetManager::checkboxCheckedChanged, this, bp::_1, bp::_2));

            connectionComboboxSelectionChanged = toolWidget->registerComboboxSelectionChanged(boost::bind(&ToolWidgetManager::comboboxSelectionChanged, this, bp::_1, bp::_2));

            reset();
        }

        void reset() {

            boost::signals2::shared_connection_block parameter_block(connectionParameterValueChanged);
            boost::signals2::shared_connection_block checkbox_block(connectionCheckboxCheckedChanged);
            boost::signals2::shared_connection_block combobox_block(connectionComboboxSelectionChanged);

            toolWidget->initNParameters(nParameter);
            toolWidget->initNCheckboxes(nCheckbox);
            toolWidget->initNComboboxes(nCombobox);

            setComboBoxesElements();

            configureToolWidget();
        }

        void overrideSketchPosition(Base::Vector2d &onSketchPos)
        {
            prevCursorPosition = onSketchPos;

            doOverrideSketchPosition(onSketchPos);
        }

        /** boost slot triggering when a parameter has changed in the widget
         * It is intended to remote control the DSHandlerDefaultWidget
         */
        void parameterValueChanged(int parameterindex, double value)
        {
            adaptDrawingToParameterChange(parameterindex, value);

            doOverrideSketchPosition(prevCursorPosition); //Correct prevCursorPosition as it's modified by the new parameter

            doChangeDrawSketchHandlerMode();

        }

        /** boost slot triggering when a checkbox has changed in the widget
         * It is intended to remote control the DSHandlerDefaultWidget
         */
        void checkboxCheckedChanged(int checkboxindex, bool value) {
            adaptDrawingToCheckboxChange(checkboxindex, value);

            doOverrideSketchPosition(prevCursorPosition); //Correct prevCursorPosition as it's modified by the new parameter

            doChangeDrawSketchHandlerMode();
        }

        /** boost slot triggering when a combobox has changed in the widget
         * It is intended to remote control the DSHandlerDefaultWidget
         */
        void comboboxSelectionChanged(int comboboxindex, int value) {
            adaptDrawingToComboboxChange(comboboxindex, value);

            doOverrideSketchPosition(prevCursorPosition); //Correct prevCursorPosition as it's modified by the new parameter

            doChangeDrawSketchHandlerMode();
        }

        //@}

        /** @name functions which MUST be specialised */
        //@{
        /// Change DSH to reflect a value entered in the widget
        void adaptDrawingToParameterChange(int parameterindex, double value) {Q_UNUSED(parameterindex);Q_UNUSED(value);}

        /// Change DSH to reflect a checkbox changed in the widget
        void adaptDrawingToCheckboxChange(int checkboxindex, bool value) {Q_UNUSED(checkboxindex);Q_UNUSED(value);}

        /// Change DSH to reflect a comboBox changed in the widget
        void adaptDrawingToComboboxChange(int comboboxindex, int value) { Q_UNUSED(comboboxindex); Q_UNUSED(value); }

        /// function to create constraints based on widget information.
        void addConstraints() {}
        void setComboBoxesElements() {}
        //@}

        /** @name functions which MAY need to be specialised */
        //@{
        /// Function to specialise to set the correct widget strings and commands
        void configureToolWidget() {
            if constexpr (std::is_same_v<StateMachines::OneSeekEnd, SelectMode>) {
                toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of point"));
                toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of point"));
            }
            else if constexpr (std::is_same_v<StateMachines::TwoSeekEnd, SelectMode>) {
                toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
                toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
                toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
                toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
            }
            else if constexpr (std::is_same_v<StateMachines::ThreeSeekEnd, SelectMode>) {
                toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
                toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
                toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
                toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
                toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of 3rd point"));
                toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of 3rd point"));
            }
            else if constexpr (std::is_same_v<StateMachines::FourSeekEnd, SelectMode>) {
                toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
                toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
                toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
                toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
                toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of 3rd point"));
                toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of 3rd point"));
            }
        }

        /** Change DSH to reflect the SelectMode it should be in based on values entered in the widget
        *
        * This is just a default implementation for common stateMachines, that may
        * or may not do what you expect. It assumes two parameters per seek state.
        *
        * It MUST be specialised otherwise
        */
        void doChangeDrawSketchHandlerMode() {
            if constexpr (std::is_same_v<StateMachines::OneSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                {
                    if (toolWidget->isParameterSet(WParameter::First) &&
                        toolWidget->isParameterSet(WParameter::Second)) {

                        handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                        handler->setState(SelectMode::End);
                        handler->finish();
                    }
                }
                break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::TwoSeekEnd, SelectMode>) {
                switch(handler->state()) {
                    case SelectMode::SeekFirst:
                    {
                        if (toolWidget->isParameterSet(WParameter::First) &&
                            toolWidget->isParameterSet(WParameter::Second)) {

                            handler->setState(SelectMode::SeekSecond);

                            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints
                        }
                    }
                    break;
                    case SelectMode::SeekSecond:
                    {
                        if (toolWidget->isParameterSet(WParameter::Third) ||
                            toolWidget->isParameterSet(WParameter::Fourth)) {

                            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                            if(toolWidget->isParameterSet(WParameter::Third) &&
                                toolWidget->isParameterSet(WParameter::Fourth)) {

                                    handler->setState(SelectMode::End);
                                    handler->finish();
                                }
                        }
                    }
                    break;
                    default:
                        break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::ThreeSeekEnd, SelectMode>) {
               switch(handler->state()) {
                    case SelectMode::SeekFirst:
                    {
                        if (toolWidget->isParameterSet(WParameter::First) &&
                            toolWidget->isParameterSet(WParameter::Second)) {

                            handler->setState(SelectMode::SeekSecond);

                            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints
                        }
                    }
                    break;
                    case SelectMode::SeekSecond:
                    {
                        if (toolWidget->isParameterSet(WParameter::Third) ||
                            toolWidget->isParameterSet(WParameter::Fourth)) {

                            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                            if(toolWidget->isParameterSet(WParameter::Third) &&
                                toolWidget->isParameterSet(WParameter::Fourth)) {

                                    handler->setState(SelectMode::SeekThird);
                            }
                        }
                    }
                    break;
                    case SelectMode::SeekThird:
                    {
                        if (toolWidget->isParameterSet(WParameter::Fifth) ||
                            toolWidget->isParameterSet(WParameter::Sixth)) {

                            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                            if(toolWidget->isParameterSet(WParameter::Fifth) &&
                                toolWidget->isParameterSet(WParameter::Sixth)) {

                                    handler->setState(SelectMode::End);
                                    handler->finish();
                            }
                        }
                    }
                    break;
                    default:
                        break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::FourSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                {
                    if (toolWidget->isParameterSet(WParameter::First) &&
                        toolWidget->isParameterSet(WParameter::Second)) {

                        handler->setState(SelectMode::SeekSecond);

                        handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints
                    }
                }
                break;
                case SelectMode::SeekSecond:
                {
                    if (toolWidget->isParameterSet(WParameter::Third) ||
                        toolWidget->isParameterSet(WParameter::Fourth)) {

                        handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                        if (toolWidget->isParameterSet(WParameter::Third) &&
                            toolWidget->isParameterSet(WParameter::Fourth)) {

                            handler->setState(SelectMode::SeekThird);
                        }
                    }
                }
                break;
                case SelectMode::SeekThird:
                {
                    if (toolWidget->isParameterSet(WParameter::Fifth)) {

                        handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                        handler->setState(SelectMode::SeekFourth);
                    }
                }
                break;
                case SelectMode::SeekFourth:
                {
                    if (toolWidget->isParameterSet(WParameter::Sixth)) {

                        handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

                        handler->setState(SelectMode::End);
                        handler->finish();
                    }
                }
                break;
                default:
                    break;
                }
            }
        }

        /** function that is called by the handler when the selection mode changed
        *
        * This is just a default implementation for common stateMachines, that may
        * or may not do what you expect. It assumes two parameters per seek state.
        *
        * It MUST be specialised otherwise
        */
        void onHandlerModeChanged() {
            if constexpr (std::is_same_v<StateMachines::OneSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                    toolWidget->setParameterFocus(WParameter::First);
                    break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::TwoSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                    toolWidget->setParameterFocus(WParameter::First);
                    break;
                case SelectMode::SeekSecond:
                    toolWidget->setParameterFocus(WParameter::Third);
                    break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::ThreeSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                    toolWidget->setParameterFocus(WParameter::First);
                    break;
                case SelectMode::SeekSecond:
                    toolWidget->setParameterFocus(WParameter::Third);
                    break;
                case SelectMode::SeekThird:
                    toolWidget->setParameterFocus(WParameter::Fifth);
                    break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::FourSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                    toolWidget->setParameterFocus(WParameter::First);
                    break;
                case SelectMode::SeekSecond:
                    toolWidget->setParameterFocus(WParameter::Third);
                    break;
                case SelectMode::SeekThird:
                    toolWidget->setParameterFocus(WParameter::Fifth);
                    break;
                case SelectMode::SeekFourth:
                    toolWidget->setParameterFocus(WParameter::Sixth);
                    break;
                default:
                    break;
                }
            }
        }

        /** function that is called by the handler with a Vector2d position to update the widget
        *
        * This is just a default implementation for common stateMachines, that may
        * or may not do what you expect. It assumes two parameters per seek state.
        *
        * It MUST be specialised if the states correspond to different parameters
        */
        void updateVisualValues(Base::Vector2d onSketchPos) {
            if constexpr (std::is_same_v<StateMachines::OneSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                {
                    if (!toolWidget->isParameterSet(WParameter::First))
                        toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Second))
                        toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
                }
                break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::TwoSeekEnd, SelectMode>) {
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
                    if (!toolWidget->isParameterSet(WParameter::Third))
                        toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Fourth))
                        toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
                }
                break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::ThreeSeekEnd, SelectMode>) {
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
                    if (!toolWidget->isParameterSet(WParameter::Third))
                        toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Fourth))
                        toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
                }
                break;
                case SelectMode::SeekThird:
                {
                    if (!toolWidget->isParameterSet(WParameter::Fifth))
                        toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Sixth))
                        toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
                }
                break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::FourSeekEnd, SelectMode>) {
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
                    if (!toolWidget->isParameterSet(WParameter::Third))
                        toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Fourth))
                        toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
                }
                break;
                case SelectMode::SeekThird:
                {
                    if (!toolWidget->isParameterSet(WParameter::Fifth))
                        toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Sixth))
                        toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
                }
                break;
                case SelectMode::SeekFourth:
                {
                    if (!toolWidget->isParameterSet(WParameter::Fifth))
                        toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

                    if (!toolWidget->isParameterSet(WParameter::Sixth))
                        toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
                }
                break;
                default:
                    break;
                }
            }
        }

        /** function that is called by the handler with a mouse position, enabling the
        * widget to override it having regard to the widget information.
        *
        * This is just a default implementation for common stateMachines, that may
        * or may not do what you expect. It assumes two parameters per seek state.
        *
        * It MUST be specialised if the states correspond to different parameters
        */
        void doOverrideSketchPosition(Base::Vector2d &onSketchPos) {
            if constexpr (std::is_same_v<StateMachines::OneSeekEnd, SelectMode>) {
                switch (handler->state()) {
                case SelectMode::SeekFirst:
                {
                    if (toolWidget->isParameterSet(WParameter::First))
                        onSketchPos.x = toolWidget->getParameter(WParameter::First);

                    if (toolWidget->isParameterSet(WParameter::Second))
                        onSketchPos.y = toolWidget->getParameter(WParameter::Second);
                }
                break;
                default:
                    break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::TwoSeekEnd, SelectMode>) {
                switch(handler->state()) {
                    case SelectMode::SeekFirst:
                    {
                        if (toolWidget->isParameterSet(WParameter::First))
                            onSketchPos.x = toolWidget->getParameter(WParameter::First);

                        if(toolWidget->isParameterSet(WParameter::Second))
                            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
                    }
                    break;
                    case SelectMode::SeekSecond:
                    {
                        if (toolWidget->isParameterSet(WParameter::Third))
                            onSketchPos.x = toolWidget->getParameter(WParameter::Third);

                        if(toolWidget->isParameterSet(WParameter::Fourth))
                            onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
                    }
                    break;
                    default:
                        break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::ThreeSeekEnd, SelectMode>) {
                switch(handler->state()) {
                    case SelectMode::SeekFirst:
                    {
                        if (toolWidget->isParameterSet(WParameter::First))
                            onSketchPos.x = toolWidget->getParameter(WParameter::First);

                        if(toolWidget->isParameterSet(WParameter::Second))
                            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
                    }
                    break;
                    case SelectMode::SeekSecond:
                    {
                        if (toolWidget->isParameterSet(WParameter::Third))
                            onSketchPos.x = toolWidget->getParameter(WParameter::Third);

                        if(toolWidget->isParameterSet(WParameter::Fourth))
                            onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
                    }
                    break;
                    case SelectMode::SeekThird:
                    {
                        if (toolWidget->isParameterSet(WParameter::Fifth))
                            onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

                        if(toolWidget->isParameterSet(WParameter::Sixth))
                            onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
                    }
                    break;
                    default:
                        break;
                }
            }
            else if constexpr (std::is_same_v<StateMachines::FourSeekEnd, SelectMode>) {
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
                    if (toolWidget->isParameterSet(WParameter::Third))
                        onSketchPos.x = toolWidget->getParameter(WParameter::Third);

                    if (toolWidget->isParameterSet(WParameter::Fourth))
                        onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
                }
                break;
                case SelectMode::SeekThird:
                {
                    if (toolWidget->isParameterSet(WParameter::Fifth))
                        onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

                    if (toolWidget->isParameterSet(WParameter::Sixth))
                        onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
                }
                break;
                case SelectMode::SeekFourth:
                {
                    //nothing. It has to be reimplemented.
                }
                break;
                default:
                    break;
                }
            }

            prevCursorPosition = onSketchPos;//Register custor position after modifying it.
        }
        //@}

     };

public:
    DSHandlerDefaultWidget():toolWidgetManager(this) {}
    virtual ~DSHandlerDefaultWidget() = default;

    /** @name functions NOT intended for specialisation */
    //@{
    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        toolWidgetManager.overrideSketchPosition(onSketchPos);
        toolWidgetManager.updateVisualValues(onSketchPos);
        updateDataAndDrawToPosition (onSketchPos);
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        toolWidgetManager.overrideSketchPosition(onSketchPos);

        onButtonPressed(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override {
        Q_UNUSED(onSketchPos);
        DSGeometryHandler::finish();
        return true;
    }
    //@}


protected:
    /** @name functions requiring specialisation */
    //@{
    virtual std::string getToolName() const override { return DrawSketchHandler::getToolName();}
    virtual QString getCrosshairCursorString() const override { return DrawSketchHandler::getCrosshairCursorString();}
    //@}

private:
    /** @name functions requiring specialisation */
    //@{
    // For every machine state, it updates the EditData temporary
    // curve, and draws the temporary curve during edit mode.
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {Q_UNUSED(onSketchPos)};

    virtual void executeCommands() override {};
    virtual void createAutoConstraints() override {};
    //@}

    /** @name functions which MAY require specialisation*/
    //@{
    /** Default implementation is that on every mouse click the mode is changed to the next seek
        On the last seek, it changes to SelectMode::End
        If this behaviour is not acceptable, then the function must be specialised.*/
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        DSGeometryHandler::onButtonPressed(onSketchPos);
    }

    virtual void beforeCreateAutoConstraints() override
    {
        toolWidgetManager.addConstraints();
    }

    virtual void onWidgetChanged() override {
        toolWidgetManager.initWidget(DSGeometryHandler::toolwidget);
    }

    virtual void onReset() override {
        toolWidgetManager.reset();
    }

    virtual void onModeChanged() override {
        toolWidgetManager.onHandlerModeChanged();
    };
    //@}

private:
    ToolWidgetManager toolWidgetManager;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchDefaultHandler_H

