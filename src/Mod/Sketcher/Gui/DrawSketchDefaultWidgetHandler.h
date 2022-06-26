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


#ifndef SKETCHERGUI_DrawSketchHandlerDefaultWidget_H
#define SKETCHERGUI_DrawSketchHandlerDefaultWidget_H

#include "DrawSketchDefaultHandler.h"

#include "SketcherToolDefaultWidget.h"

#include <Base/Tools.h>

#include <QApplication>

namespace bp = boost::placeholders;

namespace SketcherGui {

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
 * WidgetParametersT: The number of parameter spinboxes in the default widget
 * WidgetCheckboxesT: The number of checkboxes in the default widget
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
 * - adaptWidgetParameters()
 * - doEnforceWidgetParameters()
 *
 * Handler provides:
 * - generic initialisation
 * - structured command finalisation
 * - handling of continuous creation mode
 * - interaction with widget
 *
 * Handler MUST specialise:
 * - getToolName                            => provide the string name of the tool
 * - getCrosshairCursorSVGName               => provide the string for the svg icon of the cursor
 * - updateEditDataAndDrawToPositionImpl    => function to update the EditData structure and draw a temporal curve
 * - executeCommands                        => execution of commands to create the geometry
 * - createAutoConstraints                  => execution of commands to create autoconstraints (widget mandated constraints are called BEFORE this)
 *
 * Question: Do I need to use this handler or derive from this handler to make a new handler using the default tool widget?
 *
 * No, you do not NEED to. But you are encouraged to. Structuring a handler following this NVI, apart
 * from substantial savings in amount of code typed, enables a much easier and less verbose implementation of a handler
 * using a default widget (toolwidget), and will lead to easier to maintain code.
 */

template< int... sizes> // Initial sizes for each mode
class WidgetInitialValues {
public:
    template<typename constructionT>
    static constexpr int size(constructionT constructionmethod) {
        auto modeint = static_cast<int>(constructionmethod);

        return constructionMethodParameters[modeint];
    }

    static constexpr int defaultMethodSize() {
        return size(0);
    }
private:
    static constexpr std::array<int,sizeof...(sizes)> constructionMethodParameters = {{sizes...}};
};

template< int... sizes> // Initial sizes for each mode
class WidgetParameters : public WidgetInitialValues<sizes...> {
};

template< int... sizes> // Initial sizes for each mode
class WidgetCheckboxes : public WidgetInitialValues<sizes...> {
};

template< int... sizes> // Initial sizes for each mode
class WidgetComboboxes : public WidgetInitialValues<sizes...> {
};

template< typename HandlerT,          // The geometry tool for which the template is created (See GeometryTools above)
          typename SelectModeT,         // The state machine defining the states that the handle iterates
          int PEditCurveSize,           // The initial size of the EditCurve
          int PAutoConstraintSize,      // The initial size of the AutoConstraint
          typename WidgetParametersT, // The number of parameter spinboxes in the default widget
          typename WidgetCheckboxesT, // The number of checkboxes in the default widget
          typename WidgetComboboxesT, // The number of comboboxes in the default widget
          typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod,
          bool PFirstComboboxIsConstructionMethod = false>
class DrawSketchDefaultWidgetHandler: public DrawSketchDefaultHandler<HandlerT, SelectModeT, PEditCurveSize, PAutoConstraintSize, ConstructionMethodT>
{
    using DSDefaultHandler = DrawSketchDefaultHandler<HandlerT, SelectModeT, PEditCurveSize, PAutoConstraintSize, ConstructionMethodT>;
    using ConstructionMachine = ConstructionMethodMachine<ConstructionMethodT>;

private:
    class ToolWidgetManager {
        int nParameter = WidgetParametersT::defaultMethodSize();
        int nCheckbox = WidgetCheckboxesT::defaultMethodSize();
        int nCombobox = WidgetComboboxesT::defaultMethodSize();

        //std::array<int,ConstructionMachine::ConstructionMethodsCount()> constructionMethodParameters;

        SketcherToolDefaultWidget* toolWidget;
        DrawSketchDefaultWidgetHandler * handler; // used to access private implementations
        HandlerT * dHandler; // real derived type
        bool init = false; // returns true if the widget has been configured

        using Connection = boost::signals2::connection;

        Connection connectionParameterValueChanged;
        Connection connectionCheckboxCheckedChanged;
        Connection connectionComboboxSelectionChanged;

        Base::Vector2d prevCursorPosition;
        Base::Vector2d lastWidgetEnforcedPosition;

        using WParameter = SketcherToolDefaultWidget::Parameter;
        using WCheckbox  = SketcherToolDefaultWidget::Checkbox;
        using WCombobox = SketcherToolDefaultWidget::Combobox;
        using SelectMode = SelectModeT;

    public:
        ToolWidgetManager(DrawSketchDefaultWidgetHandler * dshandler):handler(dshandler), dHandler(static_cast<HandlerT *>(dshandler)){}

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
            init = true;
        }

        void reset() {

            boost::signals2::shared_connection_block parameter_block(connectionParameterValueChanged);
            boost::signals2::shared_connection_block checkbox_block(connectionCheckboxCheckedChanged);
            boost::signals2::shared_connection_block combobox_block(connectionComboboxSelectionChanged);

            toolWidget->initNParameters(nParameter);
            toolWidget->initNCheckboxes(nCheckbox);
            toolWidget->initNComboboxes(nCombobox);

            configureToolWidget();
        }

        void enforceWidgetParameters(Base::Vector2d &onSketchPos)
        {
            prevCursorPosition = onSketchPos;

            doEnforceWidgetParameters(onSketchPos);

            lastWidgetEnforcedPosition = onSketchPos; // store enforced cursor position.
        }

        /** boost slot triggering when a parameter has changed in the widget
         * It is intended to remote control the DrawSketchDefaultWidgetHandler
         */
        void parameterValueChanged(int parameterindex, double value)
        {
            // -> A machine does not forward to a next state when adapting the parameter (though it may forward to
            //    a next state if all the parameters are fulfiled, see doChangeDrawSketchHandlerMode). This ensures
            //    that the geometry has been defined (either by mouse clicking or by widget). Autoconstraints on point
            //    should be picked when the state is reached upon machine state advancement.
            //
            // -> A machine goes back to a previous state if a parameter of a previous state is modified. This ensures
            //    that appropriate autoconstraints are picked.
            if(isParameterOfPreviousMode(parameterindex)) {
                // change to previous state
                handler->setState(getState(parameterindex));
            }

            enforceWidgetParametersOnPreviousCursorPosition();

            adaptDrawingToParameterChange(parameterindex, value);

            finishWidgetChanged();
        }

        /** boost slot triggering when a checkbox has changed in the widget
         * It is intended to remote control the DrawSketchDefaultWidgetHandler
         */
        void checkboxCheckedChanged(int checkboxindex, bool value) {
            enforceWidgetParametersOnPreviousCursorPosition();

            adaptDrawingToCheckboxChange(checkboxindex, value);

            onHandlerModeChanged(); //re-focus/select spinbox

            finishWidgetChanged();
        }

        /** boost slot triggering when a combobox has changed in the widget
         * It is intended to remote control the DrawSketchDefaultWidgetHandler
         */
        void comboboxSelectionChanged(int comboboxindex, int value) {
            enforceWidgetParametersOnPreviousCursorPosition();

            adaptDrawingToComboboxChange(comboboxindex, value);

            finishWidgetChanged();
        }

        void adaptWidgetParameters() {
            adaptWidgetParameters(lastWidgetEnforcedPosition);
        }

        //@}

        /** @name functions which MUST be specialised */
        //@{
        /// Change DSH to reflect a value entered in the widget
        void adaptDrawingToParameterChange(int parameterindex, double value) {Q_UNUSED(parameterindex);Q_UNUSED(value);}

        /// Change DSH to reflect a checkbox changed in the widget
        void adaptDrawingToCheckboxChange(int checkboxindex, bool value) {Q_UNUSED(checkboxindex);Q_UNUSED(value);}

        /// Change DSH to reflect a comboBox changed in the widget
        void adaptDrawingToComboboxChange(int comboboxindex, int value) {
            Q_UNUSED(comboboxindex); Q_UNUSED(value);

            if constexpr (PFirstComboboxIsConstructionMethod == true) {

                if (comboboxindex == WCombobox::FirstCombo && handler->ConstructionMethodsCount() > 1) {
                    handler->iterateToNextConstructionMethod();
                }

            }
        }

        /** Returns the state to which the widget parameter corresponds in the current construction method
        */
        auto getState(int parameterindex) const {
            Q_UNUSED(parameterindex);
            return handler->getFirstState();
        }

        /// function to create constraints based on widget information.
        void addConstraints() {}
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
                        }
                    }
                    break;
                    case SelectMode::SeekSecond:
                    {
                        if (toolWidget->isParameterSet(WParameter::Third) ||
                            toolWidget->isParameterSet(WParameter::Fourth)) {

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
                        }
                    }
                    break;
                    case SelectMode::SeekSecond:
                    {
                        if (toolWidget->isParameterSet(WParameter::Third) ||
                            toolWidget->isParameterSet(WParameter::Fourth)) {

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
                    }
                }
                break;
                case SelectMode::SeekSecond:
                {
                    if (toolWidget->isParameterSet(WParameter::Third) ||
                        toolWidget->isParameterSet(WParameter::Fourth)) {

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

                        handler->setState(SelectMode::SeekFourth);
                    }
                }
                break;
                case SelectMode::SeekFourth:
                {
                    if (toolWidget->isParameterSet(WParameter::Sixth)) {

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

        /** function that is called by the handler when the construction mode changed
        *
        * This is just a default implementation for common stateMachines, that may
        * or may not do what you expect. It assumes two parameters per seek state.
        *
        * It MUST be specialised otherwise
        */
        void onConstructionMethodChanged() {

            nParameter = WidgetParametersT::size(handler->constructionMethod());
            nCheckbox = WidgetCheckboxesT::size(handler->constructionMethod());
            nCombobox = WidgetComboboxesT::size(handler->constructionMethod());

            // update the combobox only if necessary (if the change was not triggered by the combobox)
            if constexpr (PFirstComboboxIsConstructionMethod == true) {
                auto currentindex = toolWidget->getComboboxIndex(WCombobox::FirstCombo);
                auto methodint = static_cast<int>(handler->constructionMethod());

                if (currentindex != methodint) {
                    // avoid triggering of method change
                    boost::signals2::shared_connection_block combobox_block(connectionComboboxSelectionChanged);
                    toolWidget->setComboboxIndex(WCombobox::FirstCombo, methodint);
                }
            }

            dHandler->updateCursor();

            dHandler->reset(); //reset of handler to restart.
        }

        /** function that is called by the handler with a Vector2d position to update the widget
        *
        * This is just a default implementation for common stateMachines, that may
        * or may not do what you expect. It assumes two parameters per seek state.
        *
        * It MUST be specialised if the states correspond to different parameters
        */
        void adaptWidgetParameters(Base::Vector2d onSketchPos) {
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
        void doEnforceWidgetParameters(Base::Vector2d &onSketchPos) {
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
        }

        /** on first shortcut, it toggles the first checkbox if there is go. Must be specialised if this is not intended */
        void firstKeyShortcut() {
            if(nCheckbox >= 1) {
                auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
                toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
            }
        }

        void secondKeyShortcut() {
            if(nCheckbox >= 2) {
                auto secondchecked = toolWidget->getCheckboxChecked(WCheckbox::SecondBox);
                toolWidget->setCheckboxChecked(WCheckbox::SecondBox, !secondchecked);
            }
        }
        //@}

    private:
        /** @name helper functions */
        //@{
            /// function to assist in adaptDrawingToComboboxChange specialisation
            /// assigns the modevalue to the modeenum and updates the number of parameters according to map
            /// it also triggers an update of the cursor
            /*template <typename T>
            void setModeAndAdaptParameters(T & modeenum, int modevalue, const std::vector<int> & parametersmap) {
                if (modevalue < static_cast<int>(parametersmap.size())) {
                    auto mode = static_cast<T>(modevalue);

                    nParameter = WidgetParametersT::constructionMethodParameters[modevalue];
                    nCheckbox = WidgetCheckboxesT::constructionMethodParameters[modevalue];
                    nCombobox = WidgetComboboxesT::constructionMethodParameters[modevalue];

                    modeenum = mode;

                    dHandler->updateCursor();

                    reset(); //reset the widget to take into account the change of nparameter
                    dHandler->reset(); //reset of handler to restart.
                }
            }*/

            /// function to assist in adaptDrawingToComboboxChange specialisation
            /// assigns the modevalue to the modeenum
            /// it also triggers an update of the cursor
            template <typename T>
            void setMode(T & modeenum, int modevalue) {
                auto mode = static_cast<T>(modevalue);

                modeenum = mode;

                dHandler->updateCursor();

                dHandler->reset(); //reset of handler to restart.
            }

            /// function to redraw before and after any eventual mode change in reaction to a widget change
            void finishWidgetChanged() {

                //handler->moveCursorToSketchPoint(lastWidgetEnforcedPosition);

                auto currentstate = handler->state();
                // ensure that object at point is preselected, so that autoconstraints are generated
                handler->preselectAtPoint(lastWidgetEnforcedPosition);
                // ensure drawing in the previous mode
                handler->updateDataAndDrawToPosition(lastWidgetEnforcedPosition);

                doChangeDrawSketchHandlerMode();

                // if the state changed and is not the last state (End)
                if(!handler->isLastState() && handler->state() != currentstate) {
                    // mode has changed, so reprocess the previous position to the new widget state
                    enforceWidgetParametersOnPreviousCursorPosition();

                    // update the widget if state changed
                    adaptWidgetParameters(lastWidgetEnforcedPosition);

                    // ensure drawing in the next mode
                    handler->updateDataAndDrawToPosition(lastWidgetEnforcedPosition);
                }
            }

            void enforceWidgetParametersOnPreviousCursorPosition() {
                auto simulatedCursorPosition = prevCursorPosition; // ensure prevCursorPosition is preserved

                doEnforceWidgetParameters(simulatedCursorPosition); // updates lastWidgetEnforcedPosition with new widget state

                lastWidgetEnforcedPosition = simulatedCursorPosition; // store enforced cursor position.
            }

            /// returns the status to which the handler was updated
            bool syncHandlerToCheckbox(int checkboxindex, bool & handlerboolean) {
                bool status = toolWidget->getCheckboxChecked(checkboxindex);
                handlerboolean = status;

                return status;
            }

            /// returns true if checkbox was changed, and false if no sync was necessary
            bool syncCheckboxToHandler(int checkboxindex, bool handlerboolean) {
                bool status = toolWidget->getCheckboxChecked(checkboxindex);
                if(handlerboolean != status) {
                    toolWidget->setCheckboxChecked(checkboxindex, handlerboolean);
                    return true;
                }

                return false;
            }

            void syncHandlerToConstructionMethodCombobox() {

                if constexpr (PFirstComboboxIsConstructionMethod == true) {
                    auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

                    handler->initConstructionMethod(static_cast<ConstructionMethodT>(constructionmethod));
                }
            }
            void syncConstructionMethodComboboxToHandler() {

                if constexpr (PFirstComboboxIsConstructionMethod == true) {
                    auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

                    auto actualconstructionmethod = static_cast<int>(handler->constructionMethod());

                    if(constructionmethod != actualconstructionmethod)
                        toolWidget->setComboboxIndex(WCombobox::FirstCombo, actualconstructionmethod);
                }
            }

            bool isParameterOfCurrentMode(int parameterindex) const {
                return getState(parameterindex) == handler->state();
            }

            bool isParameterOfPreviousMode(int parameterindex) const {
                return getState(parameterindex) < handler->state();
            }
        //@}
     };

public:
    DrawSketchDefaultWidgetHandler(ConstructionMethodT constructionmethod = static_cast<ConstructionMethodT>(0)):
         DSDefaultHandler(constructionmethod)
        ,toolWidgetManager(this) {}
    virtual ~DrawSketchDefaultWidgetHandler() = default;

    /** @name functions NOT intended for specialisation */
    //@{
    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        toolWidgetManager.enforceWidgetParameters(onSketchPos);
        toolWidgetManager.adaptWidgetParameters(onSketchPos);
        updateDataAndDrawToPosition (onSketchPos);
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        toolWidgetManager.enforceWidgetParameters(onSketchPos);

        onButtonPressed(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override {
        Q_UNUSED(onSketchPos);
        DSDefaultHandler::finish();
        return true;
    }
    //@}


protected:
    /** @name functions requiring specialisation */
    //@{
    virtual std::string getToolName() const override { return DrawSketchHandler::getToolName();}
    virtual QString getCrosshairCursorSVGName() const override { return DrawSketchHandler::getCrosshairCursorSVGName();}
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
        DSDefaultHandler::onButtonPressed(onSketchPos);
    }

    virtual void beforeCreateAutoConstraints() override
    {
        toolWidgetManager.addConstraints();
    }

    virtual void onWidgetChanged() override {
        toolWidgetManager.initWidget(DSDefaultHandler::toolwidget);
    }

    virtual void onReset() override {
        toolWidgetManager.reset();
    }

    virtual void onModeChanged() override {
        toolWidgetManager.onHandlerModeChanged();
        DSDefaultHandler::onModeChanged();
    }

    virtual void onConstructionMethodChanged() override {
        toolWidgetManager.onConstructionMethodChanged();
    }

    virtual void registerPressedKey(bool pressed, int key) override {
        DSDefaultHandler::registerPressedKey(pressed, key);

        if (key == SoKeyboardEvent::U && !pressed && !this->isLastState())
            toolWidgetManager.firstKeyShortcut();

        if (key == SoKeyboardEvent::J && !pressed && !this->isLastState())
            toolWidgetManager.secondKeyShortcut();
    }
    //@}

protected:
    ToolWidgetManager toolWidgetManager;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerDefaultWidget_H

