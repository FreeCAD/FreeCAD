// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoGroup.h>

#include <Gui/Navigation/MayaGestureNavigationStyle.h>
#include <Gui/Navigation/NavigationInputState.h>
#include <Gui/SoTouchEvents.h>
#include <Gui/View3DInventorViewer.h>

#include "NavigationTestSupport.h"

#include <string>
#include <vector>

namespace
{

using NavigationStyle = Gui::NavigationStyle;

class MayaGestureProbe: public Gui::MayaGestureNavigationStyle
{
public:
    using MayaGestureNavigationStyle::processSoEvent;

    bool popupOpened = false;

    bool send(const SoEvent& event)
    {
        return processSoEvent(&event);
    }

    bool isSelectingForTest() const
    {
        return isSelecting();
    }

protected:
    void openPopupMenu(const SbVec2s&) override
    {
        popupOpened = true;
    }
};

void configureStyle(MayaGestureProbe& style, Gui::View3DInventorViewer& viewer)
{
    style.setViewer(&viewer);
    style.setPopupMenuEnabled(false);
}

bool sendMouse(
    MayaGestureProbe& style,
    const SoMouseButtonEvent::Button button,
    const SoButtonEvent::State state,
    const SbVec2s position,
    const double time,
    const unsigned int modifiers = 0
)
{
    SoMouseButtonEvent event;
    event.setButton(button);
    event.setState(state);
    event.setPosition(position);
    event.setTime(SbTime(time));
    event.setCtrlDown((modifiers & Gui::NavigationInputState::CtrlDown) != 0U);
    event.setShiftDown((modifiers & Gui::NavigationInputState::ShiftDown) != 0U);
    event.setAltDown((modifiers & Gui::NavigationInputState::AltDown) != 0U);
    return style.send(event);
}

bool sendMotion(
    MayaGestureProbe& style,
    const SbVec2s position,
    const double time,
    const unsigned int modifiers = 0
)
{
    SoLocation2Event event;
    event.setPosition(position);
    event.setTime(SbTime(time));
    event.setCtrlDown((modifiers & Gui::NavigationInputState::CtrlDown) != 0U);
    event.setShiftDown((modifiers & Gui::NavigationInputState::ShiftDown) != 0U);
    event.setAltDown((modifiers & Gui::NavigationInputState::AltDown) != 0U);
    return style.send(event);
}

bool sendKey(
    MayaGestureProbe& style,
    const SoKeyboardEvent::Key key,
    const SoButtonEvent::State state,
    const SbVec2s position,
    const double time
)
{
    SoKeyboardEvent event;
    event.setKey(key);
    event.setState(state);
    event.setPosition(position);
    event.setTime(SbTime(time));
    return style.send(event);
}

bool sendGesturePan(
    MayaGestureProbe& style,
    const SoGestureEvent::SbGestureState state,
    const SbVec2f delta,
    const double time
)
{
    SoGesturePanEvent event;
    event.state = state;
    event.deltaOffset = delta;
    event.setPosition({140, 130});
    event.setTime(SbTime(time));
    return style.send(event);
}

bool sendGesturePinch(
    MayaGestureProbe& style,
    const SoGestureEvent::SbGestureState state,
    const double deltaZoom,
    const double deltaAngle,
    const double time
)
{
    SoGesturePinchEvent event;
    event.state = state;
    event.deltaCenter = {4, 3};
    event.curCenter = {150, 140};
    event.deltaZoom = deltaZoom;
    event.deltaAngle = deltaAngle;
    event.setPosition({140, 130});
    event.setTime(SbTime(time));
    return style.send(event);
}

struct ForwardedMouseEvent
{
    SoMouseButtonEvent::Button button;
    SoButtonEvent::State state;
};

void handleMouseButtonEvent(void* userData, SoEventCallback* callback)
{
    auto* events = static_cast<std::vector<ForwardedMouseEvent>*>(userData);
    const auto* event = static_cast<const SoMouseButtonEvent*>(callback->getEvent());
    events->push_back({event->getButton(), event->getState()});
    callback->setHandled();
}

void handleKeyboardEvent(void* userData, SoEventCallback* callback)
{
    auto* forwarded = static_cast<bool*>(userData);
    *forwarded = true;
    callback->setHandled();
}

}  // namespace

TEST(MayaGestureNavigationTest, leftClickIsDeferredAndReplayedOnRelease)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    std::vector<ForwardedMouseEvent> forwarded;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoMouseButtonEvent::getClassTypeId(), handleMouseButtonEvent, &forwarded);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {102, 102}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    ASSERT_EQ(forwarded.size(), 2U);
    EXPECT_EQ(forwarded[0].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[0].state, SoButtonEvent::DOWN);
    EXPECT_EQ(forwarded[1].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[1].state, SoButtonEvent::UP);
    root->removeChild(callback);
}

TEST(MayaGestureNavigationTest, deferredClickSurvivesGestureNavigation)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    std::vector<ForwardedMouseEvent> forwarded;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoMouseButtonEvent::getClassTypeId(), handleMouseButtonEvent, &forwarded);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0));
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSStart, {0, 0}, 1.1));
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSUpdate, {4, 3}, 1.2));
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSEnd, {0, 0}, 1.3));
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {102, 102}, 1.4));

    ASSERT_EQ(forwarded.size(), 2U);
    EXPECT_EQ(forwarded[0].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[0].state, SoButtonEvent::DOWN);
    EXPECT_EQ(forwarded[1].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[1].state, SoButtonEvent::UP);
    root->removeChild(callback);
}

TEST(MayaGestureNavigationTest, rightClickOpensPopupWithoutNavigation)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);
    style.setPopupMenuEnabled(true);

    sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::UP, {102, 102}, 1.1));
    EXPECT_TRUE(style.popupOpened);
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, altLeftDragEntersRotationAfterThreshold)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::AltDown
    );
    EXPECT_TRUE(sendMotion(style, {140, 140}, 1.1, Gui::NavigationInputState::AltDown));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);

    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::UP,
        {140, 140},
        1.2,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, altRightDragEntersZoom)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON2,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::AltDown
    );
    EXPECT_TRUE(sendMotion(style, {140, 140}, 1.1, Gui::NavigationInputState::AltDown));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::ZOOMING);
    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON2,
        SoButtonEvent::UP,
        {140, 140},
        1.2,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, altMiddleButtonPans)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON3,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendMotion(style, {120, 120}, 1.1, Gui::NavigationInputState::AltDown));
    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON3,
        SoButtonEvent::UP,
        {120, 120},
        1.2,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, plainMiddleClickRecenters)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON3, SoButtonEvent::DOWN, {100, 100}, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, shiftLeftDragStartsBoxSelection)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);
    viewer.setEditing(false);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::ShiftDown
    );
    EXPECT_TRUE(sendMotion(style, {140, 140}, 1.1, Gui::NavigationInputState::ShiftDown));
    EXPECT_TRUE(style.isSelectingForTest());
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, editingLmbSelectionIsProtectedButAltOverridesIt)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);
    viewer.setEditing(true);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_FALSE(sendMotion(style, {140, 140}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::SELECTION);

    EXPECT_FALSE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {140, 140}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        2.0,
        Gui::NavigationInputState::AltDown
    );
    EXPECT_TRUE(sendMotion(style, {140, 140}, 2.1, Gui::NavigationInputState::AltDown));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
}

TEST(MayaGestureNavigationTest, twoButtonRotationReturnsToRemainingButtonMode)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::AltDown
    );
    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON2,
        SoButtonEvent::DOWN,
        {102, 102},
        1.1,
        Gui::NavigationInputState::AltDown
    );
    sendMotion(style, {140, 140}, 1.2, Gui::NavigationInputState::AltDown);
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::UP,
        {140, 140},
        1.3,
        Gui::NavigationInputState::AltDown
    );
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON2,
        SoButtonEvent::UP,
        {140, 140},
        1.4,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, twoButtonRotationReturnsToRotationWhenRightReleasedFirst)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::AltDown
    );
    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON2,
        SoButtonEvent::DOWN,
        {102, 102},
        1.1,
        Gui::NavigationInputState::AltDown
    );
    sendMotion(style, {140, 140}, 1.2, Gui::NavigationInputState::AltDown);
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);

    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON2,
        SoButtonEvent::UP,
        {140, 140},
        1.3,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::UP,
        {140, 140},
        1.4,
        Gui::NavigationInputState::AltDown
    ));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST(MayaGestureNavigationTest, gesturePanStartsUpdatesAndEndsInSelection)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSStart, {0, 0}, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSUpdate, {4, 3}, 1.1));
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSEnd, {0, 0}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::SELECTION);
}

TEST(MayaGestureNavigationTest, gesturePinchUpdatesZoomAndRotation)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendGesturePinch(style, SoGestureEvent::SbGSStart, 1.0, 0.0, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
    EXPECT_TRUE(sendGesturePinch(style, SoGestureEvent::SbGSUpdate, 1.1, 0.25, 1.1));
    EXPECT_TRUE(sendGesturePinch(style, SoGestureEvent::SbGSEnd, 1.0, 0.0, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::SELECTION);
}

TEST(MayaGestureNavigationTest, mouseInputCancelsActiveGestureNavigation)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    sendGesturePan(style, SoGestureEvent::SbGSStart, {0, 0}, 1.0);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
}

TEST(MayaGestureNavigationTest, mouseCancellationDiscardsDeferredClick)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    std::vector<ForwardedMouseEvent> forwarded;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoMouseButtonEvent::getClassTypeId(), handleMouseButtonEvent, &forwarded);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0));
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSStart, {0, 0}, 1.1));
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {102, 102}, 1.2));

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {120, 120}, 2.0));
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {122, 122}, 2.1));

    ASSERT_EQ(forwarded.size(), 2U);
    EXPECT_EQ(forwarded[0].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[0].state, SoButtonEvent::DOWN);
    EXPECT_EQ(forwarded[1].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[1].state, SoButtonEvent::UP);
    root->removeChild(callback);
}

TEST(MayaGestureNavigationTest, pageKeysZoomAndUnhandledKeysForward)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendKey(style, SoKeyboardEvent::PAGE_UP, SoButtonEvent::DOWN, {100, 100}, 1.0));
    EXPECT_TRUE(sendKey(style, SoKeyboardEvent::PAGE_DOWN, SoButtonEvent::DOWN, {100, 100}, 1.1));

    bool forwarded = false;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoKeyboardEvent::getClassTypeId(), handleKeyboardEvent, &forwarded);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    SoKeyboardEvent event;
    event.setKey(SoKeyboardEvent::S);
    event.setState(SoButtonEvent::DOWN);
    event.setPosition({100, 100});
    event.setTime(SbTime(1.2));
    EXPECT_TRUE(style.send(event));
    EXPECT_TRUE(forwarded);
    root->removeChild(callback);
}

TEST(MayaGestureNavigationTest, interruptedAnimationReturnsToSelection)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    MayaGestureProbe style;
    configureStyle(style, viewer);
    style.setViewingMode(NavigationStyle::SPINNING);

    EXPECT_FALSE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::SELECTION);
}
