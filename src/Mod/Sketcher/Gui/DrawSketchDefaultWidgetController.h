// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/Tools.h>
#include <Gui/EditableDatumLabel.h>

#include "DrawSketchController.h"

#include "DrawSketchKeyboardManager.h"

namespace SketcherGui
{

/** @brief Type encapsulating the number of parameters in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetParameters: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of checkboxes in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetCheckboxes: public ControlAmount<sizes...>
{
};

/** @brief Type encapsulating the number of comboboxes in the widget*/
template<int... sizes>  // Initial sizes for each mode
class WidgetComboboxes: public ControlAmount<sizes...>
{
};

namespace sp = std::placeholders;

/** @brief Class defining a handler controller making use of parameters provided by a widget of type
 * SketcherToolDefaultWidget.
 *
 * @details
 * This class derives from DrawSketchController and thus provides on-view parameters too. In
 * addition it manages a SketcherToolDefaultWidget in which a number of spinboxes, checkboxes and
 * comboboxes are provided.
 *
 * It also provides automatic handling of change of creation method.
 *
 * This class is not intended to control based on a custom widget.
 */
template<
    typename HandlerT,           // The name of the actual handler of the tool
    typename SelectModeT,        // The state machine defining the working of the tool
    int PAutoConstraintSize,     // The initial size of the AutoConstraint vector
    typename OnViewParametersT,  // The number of parameter spinboxes in the 3D view
    typename WidgetParametersT,  // The number of parameter spinboxes in the default widget
    typename WidgetCheckboxesT,  // The number of checkboxes in the default widget
    typename WidgetComboboxesT,  // The number of comboboxes in the default widget
    typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod,
    bool PFirstComboboxIsConstructionMethod = false>  // The handler template or class having this
                                                      // as inner class
class DrawSketchDefaultWidgetController
    : public DrawSketchController<HandlerT, SelectModeT, PAutoConstraintSize, OnViewParametersT, ConstructionMethodT>
{
public:
    /** @name Meta-programming definitions and members */
    //@{
    using ControllerBase
        = DrawSketchController<HandlerT, SelectModeT, PAutoConstraintSize, OnViewParametersT, ConstructionMethodT>;
    //@}

private:
    int nParameter = WidgetParametersT::defaultMethodSize();
    int nCheckbox = WidgetCheckboxesT::defaultMethodSize();
    int nCombobox = WidgetComboboxesT::defaultMethodSize();

    SketcherToolDefaultWidget* toolWidget;

    using Connection = fastsignals::advanced_connection;

    Connection connectionParameterTabOrEnterPressed;
    Connection connectionParameterValueChanged;
    Connection connectionCheckboxCheckedChanged;
    Connection connectionComboboxSelectionChanged;

    /** @name Named indices for controls of the default widget (SketcherToolDefaultWidget) */
    //@{
    using WParameter = SketcherToolDefaultWidget::Parameter;
    using WCheckbox = SketcherToolDefaultWidget::Checkbox;
    using WCombobox = SketcherToolDefaultWidget::Combobox;
    //@}

    using SelectMode = SelectModeT;
    using ControllerBase::handler;

public:
    explicit DrawSketchDefaultWidgetController(HandlerT* dshandler)
        : ControllerBase(dshandler)
        , toolWidget(nullptr)
    {}

    DrawSketchDefaultWidgetController(const DrawSketchDefaultWidgetController&) = delete;
    DrawSketchDefaultWidgetController(DrawSketchDefaultWidgetController&&) = delete;
    DrawSketchDefaultWidgetController& operator=(const DrawSketchDefaultWidgetController&) = delete;
    DrawSketchDefaultWidgetController& operator=(DrawSketchDefaultWidgetController&&) = delete;

    ~DrawSketchDefaultWidgetController() override
    {
        connectionParameterTabOrEnterPressed.disconnect();
        connectionParameterValueChanged.disconnect();
        connectionCheckboxCheckedChanged.disconnect();
        connectionComboboxSelectionChanged.disconnect();
    }

    /** @name functions NOT intended for specialisation offering specialisation interface for
     * extension */
    /** These functions offer a specialisation interface to ensure the order on initialisation. It
     * is heavily encouraged to extend functionality using the specialisation interface.
     */
    //@{
    /** boost slot triggering when a parameter has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void parameterValueChanged(int parameterindex, double value)
    {
        adaptDrawingToParameterChange(parameterindex, value);  // specialisation interface

        // we block the auto passing of focus back to OVP that would occur in mouseMove
        // triggered in finishControlsChanged
        ControllerBase::focusAutoPassing = false;

        ControllerBase::finishControlsChanged();

        ControllerBase::focusAutoPassing = true;
    }

    void parameterTabOrEnterPressed(int parameterindex)
    {
        Q_UNUSED(parameterindex);
        passFocusToNextParameter();
    }

    /** boost slot triggering when a checkbox has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void checkboxCheckedChanged(int checkboxindex, bool value)
    {
        adaptDrawingToCheckboxChange(checkboxindex, value);  // specialisation interface

        ControllerBase::finishControlsChanged();
    }

    /** boost slot triggering when a combobox has changed in the widget
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void comboboxSelectionChanged(int comboboxindex, int value)
    {
        adaptDrawingToComboboxChange(comboboxindex, value);  // specialisation interface

        ControllerBase::finishControlsChanged();
    }
    //@}

    /** @name Specialisation Interface */
    /** These functions offer a specialisation interface. Non-virtual functions are specific to
     * this controller. Virtual functions may depend on input from a derived controller, and thus
     * the specialisation needs to be of an overridden version (so as to be able to access members
     * of the derived controller).
     */
    /// Change DSH to reflect a value entered in the widget
    void adaptDrawingToParameterChange(int parameterindex, double value)
    {
        Q_UNUSED(parameterindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a checkbox changed in the widget
    void adaptDrawingToCheckboxChange(int checkboxindex, bool value)
    {
        Q_UNUSED(checkboxindex);
        Q_UNUSED(value);
    }

    /// Change DSH to reflect a comboBox changed in the widget
    void adaptDrawingToComboboxChange(int comboboxindex, [[maybe_unused]] int value)
    {
        Q_UNUSED(comboboxindex);

        if constexpr (PFirstComboboxIsConstructionMethod == true) {

            if (comboboxindex == WCombobox::FirstCombo && handler->ConstructionMethodsCount() > 1) {
                handler->setConstructionMethod(static_cast<ConstructionMethodT>(value));
            }
        }
    }

    /// function to create constraints based on widget information.
    void addConstraints() override
    {}

    /// function to configure the default widget.
    void configureToolWidget()
    {}

    /** function that is called by the handler with a Vector2d position to update the widget*/
    void computeNextDrawSketchHandlerMode() override
    {}

    /** function that is called by the handler with a Vector2d position to update the widget */
    void adaptParameters(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos)
    }

    /** on first shortcut, it toggles the first checkbox if there is one. Must be specialised if
     * this is not intended */
    void firstKeyShortcut() override
    {
        if (nCheckbox >= 1) {
            auto firstchecked = toolWidget->getCheckboxChecked(WCheckbox::FirstBox);
            toolWidget->setCheckboxChecked(WCheckbox::FirstBox, !firstchecked);
        }
    }

    /** on second shortcut, it toggles the second checkbox if there is one. Must be specialised if
     * this is not intended */
    void secondKeyShortcut() override
    {
        if (nCheckbox >= 2) {
            auto secondchecked = toolWidget->getCheckboxChecked(WCheckbox::SecondBox);
            toolWidget->setCheckboxChecked(WCheckbox::SecondBox, !secondchecked);
        }
    }

    /** on third shortcut, it toggles the third checkbox if there is one. Must be specialised if
     * this is not intended */
    void thirdKeyShortcut() override
    {
        if (nCheckbox >= 3) {
            auto thirdchecked = toolWidget->getCheckboxChecked(WCheckbox::ThirdBox);
            toolWidget->setCheckboxChecked(WCheckbox::ThirdBox, !thirdchecked);
        }
    }

    /** on fourth shortcut, it toggles the fourth checkbox if there is one. Must be specialised if
     * this is not intended */
    void fourthKeyShortcut() override
    {
        if (nCheckbox >= 4) {
            auto fourthchecked = toolWidget->getCheckboxChecked(WCheckbox::FourthBox);
            toolWidget->setCheckboxChecked(WCheckbox::FourthBox, !fourthchecked);
        }
    }

    /** on tab, we cycle through OVP and widget parameters */
    void tabShortcut() override
    {
        passFocusToNextParameter();
    }

    //@}

protected:
    /** @name DrawSketchController NVI */
    //@{
    /// Initialises widget control
    void doInitControls(QWidget* widget) override
    {
        initDefaultWidget(widget);
        ControllerBase::doInitControls(widget);
    }

    /// Resets widget controls
    void doResetControls() override
    {
        ControllerBase::doResetControls();
        resetDefaultWidget();
    }

    /// Automatic default method update in combobox
    void doConstructionMethodChanged() override
    {}

    /// here we can pass focus to either OVP or widget parameters.
    void setFocusToParameter(unsigned int parameterindex)
    {
        // To be able to cycle through OVP and widget, we use a parameter index that goes from
        // 0 to (onViewParameters.size() + nParameter)
        if (!ControllerBase::setFocusToOnViewParameter(parameterindex)) {
            parameterindex = parameterindex - ControllerBase::onViewParameters.size();

            if (parameterindex < static_cast<unsigned int>(nParameter)) {
                toolWidget->setParameterFocus(parameterindex);
                ControllerBase::parameterWithFocus = ControllerBase::onViewParameters.size()
                    + parameterindex;
            }
        }
    }

    /// Here we can pass focus to either OVP or widget parameters.
    void passFocusToNextParameter()
    {
        unsigned int index = ControllerBase::parameterWithFocus + 1;

        if (index >= ControllerBase::onViewParameters.size() + nParameter) {
            index = 0;
        }

        auto trySetFocus = [this](unsigned int& idx) -> bool {
            while (idx < ControllerBase::onViewParameters.size()) {
                if (ControllerBase::isOnViewParameterOfCurrentMode(idx)
                    && ControllerBase::isOnViewParameterVisible(idx)) {
                    setFocusToParameter(idx);
                    return true;
                }
                idx++;
            }
            if (idx < ControllerBase::onViewParameters.size() + nParameter) {
                setFocusToParameter(idx);
                return true;
            }
            return false;
        };

        if (!trySetFocus(index)) {
            // We have not found a parameter in this mode after current.
            // So we go back to start and retry.
            index = 0;
            trySetFocus(index);
        }

        // At that point if no onViewParameter is found, there is none.
    }
    //@}

private:
    /// Initialisation of the widget
    void initDefaultWidget(QWidget* widget)
    {
        toolWidget = static_cast<SketcherToolDefaultWidget*>(widget);  // NOLINT

        connectionParameterTabOrEnterPressed = toolWidget->registerParameterTabOrEnterPressed(
            std::bind(&DrawSketchDefaultWidgetController::parameterTabOrEnterPressed, this, sp::_1)
        );

        connectionParameterValueChanged = toolWidget->registerParameterValueChanged(
            std::bind(&DrawSketchDefaultWidgetController::parameterValueChanged, this, sp::_1, sp::_2)
        );

        connectionCheckboxCheckedChanged = toolWidget->registerCheckboxCheckedChanged(
            std::bind(&DrawSketchDefaultWidgetController::checkboxCheckedChanged, this, sp::_1, sp::_2)
        );

        connectionComboboxSelectionChanged = toolWidget->registerComboboxSelectionChanged(
            std::bind(&DrawSketchDefaultWidgetController::comboboxSelectionChanged, this, sp::_1, sp::_2)
        );
    }

    /// Resets the widget
    void resetDefaultWidget()
    {
        fastsignals::shared_connection_block parameter_focus_block(
            connectionParameterTabOrEnterPressed
        );
        fastsignals::shared_connection_block parameter_block(connectionParameterValueChanged);
        fastsignals::shared_connection_block checkbox_block(connectionCheckboxCheckedChanged);
        fastsignals::shared_connection_block combobox_block(connectionComboboxSelectionChanged);

        nParameter = WidgetParametersT::size(handler->constructionMethod());
        nCheckbox = WidgetCheckboxesT::size(handler->constructionMethod());
        nCombobox = WidgetComboboxesT::size(handler->constructionMethod());

        toolWidget->initNParameters(nParameter, ControllerBase::getKeyManager());
        toolWidget->initNCheckboxes(nCheckbox);
        toolWidget->initNComboboxes(nCombobox);

        configureToolWidget();

        // update the combobox only if necessary (if the change was not triggered by the
        // combobox)
        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto currentindex = toolWidget->getComboboxIndex(WCombobox::FirstCombo);
            auto methodint = static_cast<int>(handler->constructionMethod());

            if (currentindex != methodint) {
                // avoid triggering of method change
                fastsignals::shared_connection_block combobox_block(connectionComboboxSelectionChanged);
                toolWidget->setComboboxIndex(WCombobox::FirstCombo, methodint);
            }
        }
    }

private:
    /** @name helper functions */
    //@{
    /// returns the status to which the handler was updated
    bool syncHandlerToCheckbox(int checkboxindex, bool& handlerboolean)
    {
        bool status = toolWidget->getCheckboxChecked(checkboxindex);
        handlerboolean = status;

        return status;
    }

    /// returns true if checkbox was changed, and false if no sync was necessary
    bool syncCheckboxToHandler(int checkboxindex, bool handlerboolean)
    {
        bool status = toolWidget->getCheckboxChecked(checkboxindex);
        if (handlerboolean != status) {
            toolWidget->setCheckboxChecked(checkboxindex, handlerboolean);
            return true;
        }

        return false;
    }

    /// Syncs the handler to the construction method selection in the combobox
    void syncHandlerToConstructionMethodCombobox()
    {

        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

            handler->initConstructionMethod(static_cast<ConstructionMethodT>(constructionmethod));
        }
    }
    /// Syncs the construction method selection in the combobox to the handler selection
    void syncConstructionMethodComboboxToHandler()
    {
        if constexpr (PFirstComboboxIsConstructionMethod == true) {
            auto constructionmethod = toolWidget->getComboboxIndex(WCombobox::FirstCombo);

            auto actualconstructionmethod = static_cast<int>(handler->constructionMethod());

            if (constructionmethod != actualconstructionmethod) {
                fastsignals::shared_connection_block combobox_block(connectionComboboxSelectionChanged);
                toolWidget->setComboboxIndex(WCombobox::FirstCombo, actualconstructionmethod);
            }
        }
    }
    //@}
};


}  // namespace SketcherGui
