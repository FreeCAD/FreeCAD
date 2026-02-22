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

#include <Base/Console.h>
#include <Base/Tools2D.h>
#include <Gui/EditableDatumLabel.h>

#include "DrawSketchDefaultHandler.h"
#include "SketcherToolDefaultWidget.h"

#include "DrawSketchKeyboardManager.h"

namespace SketcherGui
{

/** @brief template class for creating a type encapsulating an int value associated to each of
    the possible construction modes supported by the tool.

    @details Different construction modes of a DSH may use different types of controls. This class
    allows one to instantiate a handler template class to provide such construction mode specific
    controls.

    Each different type of control is a template class deriving from this.
    */
template<int... sizes>  // Initial sizes for each mode
class ControlAmount
{
public:
    template<typename constructionT>
    static constexpr int size(constructionT constructionmethod)
    {
        auto modeint = static_cast<int>(constructionmethod);

        return constructionMethodParameters[modeint];
    }

    static constexpr int defaultMethodSize()
    {
        return size(0);
    }

private:
    static constexpr std::array<int, sizeof...(sizes)> constructionMethodParameters = {{sizes...}};
};

/** @brief Type encapsulating the number of On view parameters*/
template<int... sizes>  // Initial sizes for each mode
class OnViewParameters: public ControlAmount<sizes...>
{
};

namespace sp = std::placeholders;

/** @brief Class defining a generic handler controller operable with a DrawSketchControllableHandler
 *
 * @details
 * This class is intended as a parent for controller classes. This controller class provides the
 * essential controller functionalities, including on-view parameters. This controller class does
 * NOT control based on (taskbox) widgets.
 *
 * For an example of a tool using directly this class see DrawSketchHandlerPoint.
 *
 * Controls based on taskbox widgets may derive from this class and add on top any widget mandated
 * functionality. For the default widget (SketcherToolDefaultWidget), see
 * DrawSketchDefaultWidgetController. For custom widgets, an appropriate class, preferably deriving
 * from this controller needs to be provided.
 */
template<
    typename HandlerT,           // The name of the actual handler of the tool
    typename SelectModeT,        // The state machine defining the working of the tool
    int PAutoConstraintSize,     // The initial size of the AutoConstraint vector
    typename OnViewParametersT,  // The number of parameter spinboxes in the 3D view (one
                                 // value per construction mode)
    typename ConstructionMethodT = ConstructionMethods::DefaultConstructionMethod>  // The enum
                                                                                    // comprising
                                                                                    // all the
                                                                                    // supported
                                                                                    // construction
                                                                                    // methods
class DrawSketchController
{
public:
    /** @name Meta-programming definitions and members */
    //@{
    using ControllerBase = void;  // No base controller for parent class.

    using HandlerType = HandlerT;
    using SelectModeType = SelectModeT;
    using ContructionMethodType = ConstructionMethodT;
    static constexpr const int AutoConstraintInitialSize = PAutoConstraintSize;
    //@}

    /** @name Convenience definitions */
    //@{
    using DSDefaultHandler
        = DrawSketchDefaultHandler<HandlerT, SelectModeT, PAutoConstraintSize, ConstructionMethodT>;
    using ConstructionMachine = ConstructionMethodMachine<ConstructionMethodT>;

    using ConstructionMethod = ConstructionMethodT;
    using SelectMode = SelectModeT;
    //@}

protected:
    // NOLINTBEGIN
    HandlerT* handler;  // real derived type
    std::vector<std::unique_ptr<Gui::EditableDatumLabel>> onViewParameters;
    // NOLINTEND

    bool init = false;           // true if the controls have been initialised.
    int parameterWithFocus = 0;  // track the index of the parameter having the focus

    /** @name Named indices for controlling on-view controls */
    //@{
    enum OnViewParameter
    {
        First,
        Second,
        Third,
        Fourth,
        Fifth,
        Sixth,
        Seventh,
        Eighth,
        Ninth,
        Tenth,
        nOnViewParameters  // Must Always be the last one
    };
    //@}

    Base::Vector2d prevCursorPosition;

private:
    Base::Vector2d lastControlEnforcedPosition;

    int nOnViewParameter = OnViewParametersT::defaultMethodSize();

    /// Class to keep track of colors used by the on-view parameters
    class ColorManager
    {
    public:
        SbColor dimConstrColor, dimConstrDeactivatedColor;

        ColorManager()
        {
            init();
        }

    private:
        void init()
        {
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/View"
            );

            dimConstrColor = SbColor(1.0f, 0.149f, 0.0f);           // NOLINT
            dimConstrDeactivatedColor = SbColor(0.5f, 0.5f, 0.5f);  // NOLINT

            float transparency = 0.f;
            unsigned long color = (unsigned long)(dimConstrColor.getPackedValue());
            color = hGrp->GetUnsigned("ConstrainedDimColor", color);
            dimConstrColor.setPackedValue((uint32_t)color, transparency);

            color = (unsigned long)(dimConstrDeactivatedColor.getPackedValue());
            color = hGrp->GetUnsigned("DeactivatedConstrDimColor", color);
            dimConstrDeactivatedColor.setPackedValue((uint32_t)color, transparency);
        }
    };

    class OnViewParameterVisibilityManager
    {
    public:
        enum class OnViewParameterVisibility
        {
            Hidden = 0,
            OnlyDimensional = 1,
            ShowAll = 2
        };

        OnViewParameterVisibilityManager()
        {
            init();
        }

        OnViewParameterVisibility visibility() const
        {
            return onViewParameterVisibility;
        }

        bool isVisibility(OnViewParameterVisibility visibility) const
        {
            return onViewParameterVisibility == visibility;
        }

        bool isVisible(Gui::EditableDatumLabel* ovp) const
        {
            if (ovp->getFunction() == Gui::EditableDatumLabel::Function::Forced) {
                return true;
            }

            switch (onViewParameterVisibility) {

                case OnViewParameterVisibility::Hidden:
                    return dynamicOverride;
                case OnViewParameterVisibility::OnlyDimensional: {
                    auto isDimensional
                        = (ovp->getFunction() == Gui::EditableDatumLabel::Function::Dimensioning);

                    return isDimensional != dynamicOverride;
                }
                case OnViewParameterVisibility::ShowAll:
                    return !dynamicOverride;
            }

            return false;
        }

        void toggleDynamicOverride()
        {
            dynamicOverride = !dynamicOverride;
        }

        void resetDynamicOverride()
        {
            dynamicOverride = false;
        }


    private:
        void init()
        {
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher/Tools"
            );

            onViewParameterVisibility = static_cast<OnViewParameterVisibility>(
                hGrp->GetInt("OnViewParameterVisibility", 1)
            );
        }

        OnViewParameterVisibility onViewParameterVisibility;
        bool dynamicOverride = false;
    };

public:
    /** Creates the controller.
     *  @param dshandler a controllable DSH handler
     */
    explicit DrawSketchController(HandlerT* dshandler)
        : handler(dshandler)
        , keymanager(std::make_unique<DrawSketchKeyboardManager>())
    {}

    DrawSketchController(const DrawSketchController&) = delete;
    DrawSketchController(DrawSketchController&&) = delete;
    DrawSketchController& operator=(const DrawSketchController&) = delete;
    DrawSketchController& operator=(DrawSketchController&&) = delete;

    virtual ~DrawSketchController()
    {}

    /** @name functions NOT intended for specialisation offering a NVI for extension */
    /** These functions offer a NVI to ensure the order on initialisation. It is heavily encouraged
     * to extend functionality using the NVI interface (by overriding the NVI functions).
     */
    //@{

    /** @brief Initialises controls, such as the widget and the on-view parameters via NVI.*/
    void initControls(QWidget* widget)
    {
        doInitControls(widget);  // NVI

        resetControls();
        init = true;
    }

    /** @brief Resets the controls, such as the widget and the on-view parameters */
    void resetControls()
    {
        // Make sure we do not loose focus if next methode does not have OVP that take focus.
        handler->ensureFocus();

        doResetControls();  // NVI

        firstMoveInit = false;
    }

    /** @brief function triggered by the handler when the mouse has been moved */
    void mouseMoved(Base::Vector2d originalSketchPosition)
    {
        onMouseMoved(originalSketchPosition);  // NVI

        if (!firstMoveInit) {
            firstMoveInit = true;
        }
    }

    /** @brief function triggered by the handler to ensure its operating position takes into
     * account widget mandated parameters */
    void enforceControlParameters(Base::Vector2d& onSketchPos)
    {
        prevCursorPosition = onSketchPos;

        doEnforceControlParameters(onSketchPos);  // specialisation interface

        lastControlEnforcedPosition = onSketchPos;  // store enforced cursor position.

        afterEnforceControlParameters();  // NVI
    }

    /** function that is called by the handler when the construction mode changed */
    void onConstructionMethodChanged()
    {
        doConstructionMethodChanged();  // NVI

        handler->updateCursor();

        if (resetOnConstructionMethodeChanged()) {
            handler->reset();  // reset of handler to restart.
        }

        auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, prevCursorPosition);
        handler->mouseMove(*snapHandle);
    }
    //@}

    /** function that define if the handler should be reset on construction methode change */
    virtual bool resetOnConstructionMethodeChanged()
    {
        return true;
    }
    //@}

    /** @name functions NOT intended for specialisation offering specialisation interface for
     * extension */
    /** These functions offer a specialisation interface to ensure the order on initialisation. It
     * is heavily encouraged to extend functionality using the specialisation interface (by
     * specialising the NVI functions).
     */
    //@{

    /** slot triggering when a on view parameter has changed
     * It is intended to remote control the DrawSketchDefaultWidgetHandler
     */
    void onViewValueChanged(int onviewparameterindex, double value)
    {
        // Since this method is used as a Qt slot we must handle
        // all exceptions here
        try {
            tryViewValueChanged(onviewparameterindex, value);
        }
        catch (const Base::Exception& e) {
            e.reportException();
        }
        catch (const std::exception& e) {
            Base::Console().error("C++ exception in onViewValueChanged: %s\n", e.what());
        }
    }

    void finishEditingOnAllOVPs()
    {
        // we call this on a current OnViewParameter when pressed CTRL+ENTER to accept
        // input on all visible ovps of current mode

        // we check for initial state, since `onViewValueChanged` can process to next mode
        // if we set hasFinishedEditing on current mode
        auto initialState = handler->state();
        for (size_t i = 0; i < onViewParameters.size(); i++) {
            if (isOnViewParameterOfCurrentMode(i) && isOnViewParameterVisible(i)
                && initialState == getState(static_cast<int>(i))) {
                onViewParameters[i]->isSet = true;
                onViewParameters[i]->hasFinishedEditing = true;

                double currentValue = onViewParameters[i]->getValue();
                onViewValueChanged(static_cast<int>(i), currentValue);
            }
        }
    }

    void tryViewValueChanged(int onviewparameterindex, double value)
    {
        // go to next label in circular manner if user has currently pressed enter on current one
        if (onViewParameters[onviewparameterindex]->hasFinishedEditing) {
            // find the first parameter of the current mode that is not locked to start the cycle
            auto findNextUnlockedParameter = [this](size_t startIndex) -> int {
                for (size_t i = startIndex; i < onViewParameters.size(); i++) {
                    if (isOnViewParameterOfCurrentMode(i)
                        && !onViewParameters[i]->hasFinishedEditing) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            };

            // find first unlocked parameter (for cycling back)
            int firstOfCurrentMode = findNextUnlockedParameter(0);

            // find next unlocked parameter after current one
            int nextUnlockedIndex = findNextUnlockedParameter(onviewparameterindex + 1);

            // if no next parameter found, cycle back to first of current mode
            if (nextUnlockedIndex != -1) {
                setFocusToOnViewParameter(nextUnlockedIndex);
            }
            else if (firstOfCurrentMode != -1) {
                setFocusToOnViewParameter(firstOfCurrentMode);
            }
        }

        /* That is not supported with on-view parameters.
        // -> A machine does not forward to a next state when adapting the parameter (though it
        // may forward to
        //    a next state if all the parameters are fulfilled, see
        //    computeNextDrawSketchHandlerMode). This ensures that the geometry has been defined
        //    (either by mouse clicking or by widget). Autoconstraints on point should be picked
        //    when the state is reached upon machine state advancement.
        //
        // -> A machine goes back to a previous state if a parameter of a previous state is
        // modified. This ensures
        //    that appropriate autoconstraints are picked.
        if (isOnViewParameterOfPreviousMode(onviewparameterindex)) {
            // change to previous state
            handler->setState(getState(onviewparameterindex));
        }*/

        adaptDrawingToOnViewParameterChange(
            onviewparameterindex,
            value
        );  // specialisation interface

        finishControlsChanged();
    }

    void adaptParameters()
    {
        adaptParameters(lastControlEnforcedPosition);  // specialisation interface
    }
    //@}

    /** @name Specialisation Interface */
    /** These functions offer a specialisation interface. Non-virtual functions are specific to
     * this controller. Virtual functions may depend on input from a derived controller, and thus
     * the specialisation needs to be of an overridden version (so as to be able to access members
     * of the derived controller).
     */
    //@{
    /// Change DSH to reflect a value entered in the view
    void adaptDrawingToOnViewParameterChange(int onviewparameterindex, double value)
    {
        Q_UNUSED(onviewparameterindex);
        Q_UNUSED(value);
    }

    /** Returns the state to which the on-view parameter corresponds in the current construction
     * method. */
    auto getState(int parameterindex) const
    {
        Q_UNUSED(parameterindex);
        return handler->getFirstState();
    }

    /// function to create constraints based on control information.
    virtual void addConstraints()
    {}

    /// Configures on-view parameters
    void configureOnViewParameters()
    {}

    /** Change DSH to reflect the SelectMode it should be in based on values entered in the
     * controls
     */
    virtual void computeNextDrawSketchHandlerMode()
    {}

    /** function that is called by the handler when the selection mode changed */
    void onHandlerModeChanged()
    {
        setModeOnViewParameters();
    }

    /** function that is called by the handler with a Vector2d position to update the widget
     *
     * It MUST be specialised if you want the parameters to update on mouseMove
     */
    virtual void adaptParameters(Base::Vector2d onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    /** function that is called by the handler with a mouse position, enabling the
     * widget to override it having regard to the widget information.
     *
     * It MUST be specialised if you want to override mouse position based on parameters.
     */
    void doEnforceControlParameters(Base::Vector2d& onSketchPos)
    {
        Q_UNUSED(onSketchPos)
    }

    /** on first shortcut, it toggles the first checkbox if there is go. Must be specialised if
     * this is not intended */
    virtual void firstKeyShortcut()
    {}

    virtual void secondKeyShortcut()
    {}

    virtual void thirdKeyShortcut()
    {}

    virtual void fourthKeyShortcut()
    {}

    virtual void tabShortcut()
    {
        passFocusToNextOnViewParameter();
    }
    //@}


    /// triggered by the controllable DSH after a mode change has been effected
    virtual void afterHandlerModeChanged()
    {
        if (handler && (!handler->isState(SelectModeT::End) || handler->continuousMode)) {
            auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, prevCursorPosition);
            handler->mouseMove(*snapHandle);
        }
    }

    void drawPositionAtCursor(const Base::Vector2d& position)
    {
        if (shouldDrawPositionAtCursor()) {
            handler->drawPositionAtCursor(position);
        }
    }

    void drawDirectionAtCursor(const Base::Vector2d& position, const Base::Vector2d& origin)
    {
        if (shouldDrawDimensionsAtCursor()) {
            handler->drawDirectionAtCursor(position, origin);
        }
    }

    void drawWidthHeightAtCursor(const Base::Vector2d& position, const double val1, const double val2)
    {
        if (shouldDrawDimensionsAtCursor()) {
            handler->drawWidthHeightAtCursor(position, val1, val2);
        }
    }

    void drawDoubleAtCursor(
        const Base::Vector2d& position,
        const double radius,
        Base::Unit unit = Base::Unit::Length
    )
    {
        if (shouldDrawDimensionsAtCursor()) {
            handler->drawDoubleAtCursor(position, radius, unit);
        }
    }

protected:
    /** @name NVI for extension of controller functionality in derived classes */
    //@{
    virtual void doInitControls(QWidget* widget)
    {
        Q_UNUSED(widget)
    }

    virtual void doResetControls()
    {
        resetOnViewParameters();
    }
    virtual void onMouseMoved(Base::Vector2d originalSketchPosition)
    {
        Q_UNUSED(originalSketchPosition)

        if (!firstMoveInit) {
            setModeOnViewParameters();
        }
    }

    virtual void afterEnforceControlParameters()
    {
        // Give focus to current on-view parameter. In case user interacted outside of 3dview.
        if (focusAutoPassing && parameterWithFocus >= 0) {
            setFocusToOnViewParameter(parameterWithFocus);
        }
    }

    virtual void doConstructionMethodChanged()
    {}
    //@}

protected:
    /** @name helper functions */
    //@{
    /// function to assist in adaptDrawingToComboboxChange specialisation
    /// assigns the modevalue to the modeenum
    /// it also triggers an update of the cursor
    template<typename T>
    void setMode(T& modeenum, int modevalue)
    {
        auto mode = static_cast<T>(modevalue);

        modeenum = mode;

        handler->updateCursor();

        handler->resetControls();  // resetControls of handler to restart.
    }

    /// function to redraw before and after any eventual mode change in reaction to a control
    /// change
    void finishControlsChanged()
    {
        auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, prevCursorPosition);
        handler->mouseMove(*snapHandle);

        auto currentstate = handler->state();
        // ensure that object at point is preselected, so that autoconstraints are generated
        handler->preselectAtPoint(lastControlEnforcedPosition);
        // We have to redo an update to regenerate the correct autoconstraints after the
        // preselectAtPoint.
        handler->updateDataAndDrawToPosition(lastControlEnforcedPosition);

        computeNextDrawSketchHandlerMode();

        auto nextState = handler->getNextState();
        bool shouldProcessLastPosWithNextState = nextState && nextState != SelectMode::End
            && nextState != currentstate && firstMoveInit;
        // the handler will be destroyed in applyNextState if the nextState is End
        handler->applyNextState();

        // if the state changed and is not the last state (End). And is init (ie tool has not
        // reset)
        if (shouldProcessLastPosWithNextState) {
            // mode has changed, so reprocess the previous position to the new widget state
            auto snapHandle = std::make_unique<SnapManager::SnapHandle>(nullptr, prevCursorPosition);
            handler->mouseMove(*snapHandle);
        }
    }

    /** @brief Initialises on-screen parameters */
    void initNOnViewParameters(int n)
    {
        Gui::View3DInventorViewer* viewer = handler->getViewer();

        auto doc = Gui::Application::Instance->editDocument();
        auto placement = Base::Placement(doc->getEditingTransform());

        onViewParameters.clear();

        for (int i = 0; i < n; i++) {

            // the returned is a naked pointer
            auto parameter = onViewParameters
                                 .emplace_back(
                                     std::make_unique<Gui::EditableDatumLabel>(
                                         viewer,
                                         placement,
                                         colorManager.dimConstrDeactivatedColor,
                                         /*autoDistance = */ true,
                                         /*avoidMouseCursor = */ true
                                     )
                                 )
                                 .get();

            QObject::connect(
                parameter,
                &Gui::EditableDatumLabel::valueChanged,
                [this, parameter, i](double value) {
                    parameter->setColor(colorManager.dimConstrColor);
                    onViewValueChanged(i, value);
                }
            );

            // this gets triggered whenever user deletes content in OVP, we remove the
            // constraints and unset everything to give user another change to select stuff
            // with mouse
            QObject::connect(parameter, &Gui::EditableDatumLabel::parameterUnset, [this, parameter]() {
                unsetOnViewParameter(parameter);
                finishControlsChanged();
            });

            // Connect Ctrl+Enter signal to apply values to all visible OVPs in current stage
            QObject::connect(parameter, &Gui::EditableDatumLabel::finishEditingOnAllOVPs, [this]() {
                finishEditingOnAllOVPs();
            });
        }
    }

    /// Allows a on-view parameter to take any mouse mandated value (as opposed to enforce one)
    void unsetOnViewParameter(Gui::EditableDatumLabel* onViewParameter)
    {
        onViewParameter->isSet = false;
        onViewParameter->hasFinishedEditing = false;
        onViewParameter->setColor(colorManager.dimConstrDeactivatedColor);
        onViewParameter->setLockedAppearance(false);
    }

    void setOnViewParameterValue(
        OnViewParameter index,
        double val,
        const Base::Unit& unit = Base::Unit::Length
    )
    {
        bool visible = isOnViewParameterVisible(index);

        if (visible) {
            onViewParameters[index]->setSpinboxValue(val, unit);
        }
    }

    /** Activates the correct set of on-view parameters corresponding to current
     * mode. It may be specialized if necessary.*/
    void setModeOnViewParameters()
    {
        // before each mode change we reset the dynamic override
        ovpVisibilityManager.resetDynamicOverride();

        bool firstOfMode = true;
        parameterWithFocus = -1;

        for (size_t i = 0; i < onViewParameters.size(); i++) {

            if (!isOnViewParameterOfCurrentMode(i)) {
                onViewParameters[i]->stopEdit();
                if (!onViewParameters[i]->isSet || handler->state() == SelectMode::End) {
                    onViewParameters[i]->deactivate();
                }
            }
            else {

                if (firstOfMode) {
                    parameterWithFocus = static_cast<int>(i);
                    firstOfMode = false;
                }

                bool visible = isOnViewParameterVisible(i);

                if (visible) {
                    onViewParameters[i]->activate();

                    // points/value will be overridden by the mouseMove triggered by the mode
                    // change.
                    onViewParameters[i]->setPoints(Base::Vector3d(), Base::Vector3d());
                    onViewParameters[i]->startEdit(0.0, keymanager.get());
                }
            }
        }
    }

    /// This function gives the focus to a spinbox and tracks the focus.
    bool setFocusToOnViewParameter(unsigned int onviewparameterindex)
    {
        if (onviewparameterindex < onViewParameters.size()) {

            bool visible = isOnViewParameterVisible(onviewparameterindex);

            if (visible) {
                onViewParameters[onviewparameterindex]->setFocusToSpinbox();
                parameterWithFocus = static_cast<int>(onviewparameterindex);
                return true;
            }
        }
        return false;
    }

    /// Switches focus to the next parameter in the current state machine.
    void passFocusToNextOnViewParameter()
    {
        unsigned int index = parameterWithFocus + 1;

        if (index >= onViewParameters.size()) {
            index = 0;
        }

        auto trySetFocus = [this](unsigned int& idx) -> bool {
            while (idx < onViewParameters.size()) {
                if (isOnViewParameterOfCurrentMode(idx) && isOnViewParameterVisible(idx)) {
                    setFocusToOnViewParameter(idx);
                    return true;
                }
                idx++;
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

    /** Returns whether the provided on-view parameter index belongs to the current state of the
     * state machine */
    bool isOnViewParameterOfCurrentMode(unsigned int onviewparameterindex) const
    {
        return onviewparameterindex < onViewParameters.size()
            && getState(onviewparameterindex) == handler->state();
    }

    /** Returns whether the provided on-view parameter index belongs to the previous state of the
     * state machine */
    bool isOnViewParameterOfPreviousMode(unsigned int onviewparameterindex) const
    {
        return onviewparameterindex < onViewParameters.size()
            && getState(onviewparameterindex) < handler->state();
    }

    bool isOnViewParameterVisible(unsigned int onviewparameterindex)
    {
        return ovpVisibilityManager.isVisible(onViewParameters[onviewparameterindex].get());
    }

    /** Resets the on-view parameter controls */
    void resetOnViewParameters()
    {
        nOnViewParameter = OnViewParametersT::size(handler->constructionMethod());
        initNOnViewParameters(nOnViewParameter);
        parameterWithFocus = 0;

        configureOnViewParameters();
    }
    //@}

    // makes keymanager available to derived classes so that they install it as event handler
    // where necessary, without allowing them to manage resource ownership
    DrawSketchKeyboardManager* getKeyManager()
    {
        return keymanager.get();
    }

    bool focusAutoPassing = true;

private:
    /** @name helper functions */
    //@{
    bool shouldDrawPositionAtCursor() const
    {
        return !(ovpVisibilityManager.isVisibility(
            OnViewParameterVisibilityManager::OnViewParameterVisibility::ShowAll
        ));
    }

    bool shouldDrawDimensionsAtCursor() const
    {
        return (ovpVisibilityManager.isVisibility(
            OnViewParameterVisibilityManager::OnViewParameterVisibility::Hidden
        ));
    }
    //@}

private:
    OnViewParameterVisibilityManager ovpVisibilityManager;
    ColorManager colorManager;
    std::unique_ptr<DrawSketchKeyboardManager> keymanager;

    bool firstMoveInit = false;  // true if first mouse movement not yet performed (resets)
};


}  // namespace SketcherGui
