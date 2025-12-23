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


#ifndef SKETCHERGUI_DrawSketchControllableHandler_H
#define SKETCHERGUI_DrawSketchControllableHandler_H

#include <type_traits>

#include "DrawSketchDefaultHandler.h"
#include "SnapManager.h"

namespace SketcherGui
{

/** @brief Template class intended for handlers controllable by a DrawSketchController
 *
 * @details
 * The template encapsulates a minimal interaction between a DrawSketchDefaultHandler and a
 * a DrawSketchController, and it is intended to facilitate the creation of enhanced DSHs having
 * controlling entities such as widgets and on-screen controls.
 */
template<typename ControllerT>
class DrawSketchControllableHandler: public DrawSketchDefaultHandler<
                                         typename ControllerT::HandlerType,
                                         typename ControllerT::SelectModeType,
                                         ControllerT::AutoConstraintInitialSize,
                                         typename ControllerT::ContructionMethodType>
{
    /** @name Meta-programming definitions and members */
    //@{
    using HandlerType = typename ControllerT::HandlerType;
    using SelectModeType = typename ControllerT::SelectModeType;
    using ConstructionMethodType = typename ControllerT::ContructionMethodType;
    //@}

    /** @name Convenience definitions */
    //@{
    using DSDefaultHandler
        = DrawSketchDefaultHandler<HandlerType, SelectModeType, ControllerT::AutoConstraintInitialSize, ConstructionMethodType>;

    using ConstructionMachine = ConstructionMethodMachine<ConstructionMethodType>;
    //@}

    // Enable access to the actual controller provided
    friend ControllerT;
    // Enable access to the parent controller (if any).
    // Non-derived controllers shall define ControllerBase as void to interoperate with this class.
    friend typename ControllerT::ControllerBase;

public:
    DrawSketchControllableHandler(
        ConstructionMethodType constructionmethod = static_cast<ConstructionMethodType>(0)
    )
        : DSDefaultHandler(constructionmethod)
        , toolWidgetManager(static_cast<HandlerType*>(this))
    {}

    ~DrawSketchControllableHandler() override = default;

    /** @name functions NOT intended for specialisation or further overriding */
    //@{
    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        Base::Vector2d onSketchPos = snapHandle.compute();
        toolWidgetManager.mouseMoved(onSketchPos);

        toolWidgetManager.enforceControlParameters(onSketchPos);
        updateDataAndDrawToPosition(onSketchPos);
        toolWidgetManager.adaptParameters(onSketchPos);
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        // ensure controller state is initialized even if no mouseMove occurred
        // ie. when a modal dialog blocks input before the first click
        toolWidgetManager.mouseMoved(onSketchPos);
        toolWidgetManager.enforceControlParameters(onSketchPos);
        updateDataAndDrawToPosition(onSketchPos);
        toolWidgetManager.adaptParameters(onSketchPos);

        onButtonPressed(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        DSDefaultHandler::finish();
        return true;
    }
    //@}


protected:
    /** @name functions requiring specialisation */
    //@{
    std::string getToolName() const override
    {
        return DrawSketchHandler::getToolName();
    }
    QString getCrosshairCursorSVGName() const override
    {
        return DrawSketchHandler::getCrosshairCursorSVGName();
    }
    //@}

private:
    /** @name functions requiring specialisation */
    //@{
    // For every machine state, it updates the EditData temporary
    // curve, and draws the temporary curve during edit mode.
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos)
    };

    void executeCommands() override {};
    void createAutoConstraints() override {};
    //@}

    /** @name functions which MAY require specialisation*/
    //@{
    /** Default implementation is that on every mouse click the mode is changed to the next seek
        On the last seek, it changes to SelectMode::End
        If this behaviour is not acceptable, then the function must be specialised.*/
    void onButtonPressed(Base::Vector2d onSketchPos) override
    {
        DSDefaultHandler::onButtonPressed(onSketchPos);
    }

    void beforeCreateAutoConstraints() override
    {
        toolWidgetManager.addConstraints();
    }

    void onWidgetChanged() override
    {
        toolWidgetManager.initControls(DSDefaultHandler::toolwidget);
    }

    void onReset() override
    {
        toolWidgetManager.resetControls();
    }

    bool onModeChanged() override
    {
        DrawSketchHandler::resetPositionText();
        DrawSketchHandler::updateHint();

        toolWidgetManager.onHandlerModeChanged();

        if (DSDefaultHandler::onModeChanged()) {
            // If onModeChanged returns false, then the handler has been purged.
            toolWidgetManager.afterHandlerModeChanged();
        }
        return true;
    }

    void onConstructionMethodChanged() override
    {
        toolWidgetManager.onConstructionMethodChanged();
    }

    void registerPressedKey(bool pressed, int key) override
    {
        DSDefaultHandler::registerPressedKey(pressed, key);

        if (key == SoKeyboardEvent::U && !pressed && !this->isLastState()) {
            toolWidgetManager.firstKeyShortcut();
        }

        if (key == SoKeyboardEvent::J && !pressed && !this->isLastState()) {
            toolWidgetManager.secondKeyShortcut();
        }

        if (key == SoKeyboardEvent::R && !pressed && !this->isLastState()) {
            toolWidgetManager.thirdKeyShortcut();
        }

        if (key == SoKeyboardEvent::F && !pressed && !this->isLastState()) {
            toolWidgetManager.fourthKeyShortcut();
        }

        if (key == SoKeyboardEvent::TAB && !pressed) {
            toolWidgetManager.tabShortcut();
        }
    }
    //@}

protected:
    ControllerT toolWidgetManager;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchControllableHandler_H
