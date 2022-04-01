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

template< typename HandlerT,          // The geometry tool for which the template is created (See GeometryTools above)
          typename SelectModeT,         // The state machine defining the states that the handle iterates
          int PEditCurveSize,           // The initial size of the EditCurve
          int PAutoConstraintSize,      // The initial size of the AutoConstraint
          int PNumToolwidgetparameters, // The number of parameter spinboxes in the default widget
          int PNumToolwidgetCheckboxes, // The number of checkboxes in the default widget
          int PNumToolwidgetComboboxes > // The number of comboboxes in the default widget
class DrawSketchDefaultWidgetHandler: public DrawSketchDefaultHandler<HandlerT, SelectModeT, PEditCurveSize, PAutoConstraintSize>
{
    using DSDefaultHandler = DrawSketchDefaultHandler<HandlerT, SelectModeT, PEditCurveSize, PAutoConstraintSize>;

private:
    class ToolWidgetManager {
        int nParameter = PNumToolwidgetparameters;
        int nCheckbox = PNumToolwidgetCheckboxes;
        int nCombobox = PNumToolwidgetComboboxes;

        SketcherToolDefaultWidget* toolWidget;
        DrawSketchDefaultWidgetHandler * handler; // used to access private implementations
        HandlerT * dhandler; // real derived type

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
        ToolWidgetManager(DrawSketchDefaultWidgetHandler * dshandler):handler(dshandler), dhandler(static_cast<HandlerT *>(dshandler)){}

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
         * It is intended to remote control the DrawSketchDefaultWidgetHandler
         */
        void parameterValueChanged(int parameterindex, double value)
        {
            adaptDrawingToParameterChange(parameterindex, value);

            doOverrideSketchPosition(prevCursorPosition); //Correct prevCursorPosition as it's modified by the new parameter

            doChangeDrawSketchHandlerMode();

        }

        /** boost slot triggering when a checkbox has changed in the widget
         * It is intended to remote control the DrawSketchDefaultWidgetHandler
         */
        void checkboxCheckedChanged(int checkboxindex, bool value) {
            adaptDrawingToCheckboxChange(checkboxindex, value);

            doOverrideSketchPosition(prevCursorPosition); //Correct prevCursorPosition as it's modified by the new parameter

            doChangeDrawSketchHandlerMode();
        }

        /** boost slot triggering when a combobox has changed in the widget
         * It is intended to remote control the DrawSketchDefaultWidgetHandler
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
    DrawSketchDefaultWidgetHandler():toolWidgetManager(this) {}
    virtual ~DrawSketchDefaultWidgetHandler() = default;

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
        DSDefaultHandler::finish();
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
    };
    //@}

private:
    ToolWidgetManager toolWidgetManager;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerDefaultWidget_H

