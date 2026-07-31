// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU         *
 *   Lesser General Public License for more details.                          *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses    *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/draggers/SoTrackballDragger.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoGroup.h>

#include <Gui/Navigation/GestureNavigationStyle.h>
#include <Gui/Navigation/NavigationInputState.h>
#include <Gui/SoTouchEvents.h>
#include <Gui/View3DInventorViewer.h>

#include "NavigationTestSupport.h"

#include <memory>
#include <string>
#include <vector>

namespace
{

using NavigationStyle = Gui::NavigationStyle;

struct GestureObservation
{
    std::string eventType;
    NavigationStyle::ViewerMode mode;
    bool processed;
    std::size_t postponedEvents;
};

struct ForwardedMouseEvent
{
    SoMouseButtonEvent::Button button;
    SoButtonEvent::State state;
};

class GestureNavigationTest: public ::testing::Test
{
};

class GestureProbe: public Gui::GestureNavigationStyle
{
public:
    bool popupOpened = false;
    std::vector<GestureObservation> observations;

    bool send(const SoEvent& event)
    {
        const bool processed = processSoEvent(&event);
        observations.push_back({
            event.getTypeId().getName().getString(),
            static_cast<NavigationStyle::ViewerMode>(getViewingMode()),
            processed,
            postponedEvents.size(),
        });
        return processed;
    }

    std::size_t postponedEventCount() const
    {
        return postponedEvents.size();
    }

    bool isSelectingForTest() const
    {
        return isSelecting();
    }

    void stopSelectionForTest()
    {
        stopSelection();
    }

    void simulateMissingRightRelease()
    {
        button2down = true;
    }

    bool draggerAt(const SbVec2s position) const
    {
        return isDraggerUnderCursor(position);
    }

protected:
    void openPopupMenu(const SbVec2s&) override
    {
        popupOpened = true;
    }
};

void configureStyle(GestureProbe& style, Gui::View3DInventorViewer& viewer)
{
    style.setViewer(&viewer);
    style.setPopupMenuEnabled(false);
}

bool sendMouse(
    GestureProbe& style,
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

bool sendMotion(GestureProbe& style, const SbVec2s position, const double time)
{
    SoLocation2Event event;
    event.setPosition(position);
    event.setTime(SbTime(time));
    return style.send(event);
}

bool sendMotion(GestureProbe& style, const SbVec2s position, const double time, const unsigned int modifiers)
{
    SoLocation2Event event;
    event.setPosition(position);
    event.setTime(SbTime(time));
    event.setCtrlDown((modifiers & Gui::NavigationInputState::CtrlDown) != 0U);
    event.setShiftDown((modifiers & Gui::NavigationInputState::ShiftDown) != 0U);
    event.setAltDown((modifiers & Gui::NavigationInputState::AltDown) != 0U);
    return style.send(event);
}

bool sendGesturePan(
    GestureProbe& style,
    const SoGestureEvent::SbGestureState state,
    const SbVec2f delta,
    const double time
)
{
    SoGesturePanEvent event;
    event.state = state;
    event.deltaOffset = delta;
    event.setPosition(SbVec2s(140, 130));
    event.setTime(SbTime(time));
    return style.send(event);
}

bool sendGesturePinch(
    GestureProbe& style,
    const SoGestureEvent::SbGestureState state,
    const double deltaZoom,
    const double time
)
{
    SoGesturePinchEvent event;
    event.state = state;
    event.deltaCenter = {4, 3};
    event.curCenter = {150, 140};
    event.deltaZoom = deltaZoom;
    event.setPosition(SbVec2s(140, 130));
    event.setTime(SbTime(time));
    return style.send(event);
}

void handleKeyboardEvent(void* userData, SoEventCallback* callback)
{
    auto* handled = static_cast<bool*>(userData);
    *handled = true;
    callback->setHandled();
}

void handleMouseButtonEvent(void* userData, SoEventCallback* callback)
{
    auto* events = static_cast<std::vector<ForwardedMouseEvent>*>(userData);
    const auto* event = static_cast<const SoMouseButtonEvent*>(callback->getEvent());
    events->push_back({event->getButton(), event->getState()});
    callback->setHandled();
}

}  // namespace

TEST_F(GestureNavigationTest, leftClickIsPostponedAndReplayedOnRelease)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(
        sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, SbVec2s(100, 100), 1.0)
    );
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    EXPECT_EQ(style.postponedEventCount(), 1U);

    EXPECT_FALSE(
        sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, SbVec2s(102, 102), 1.1)
    );
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    EXPECT_EQ(style.postponedEventCount(), 0U);
    ASSERT_EQ(style.observations.size(), 2U);
    EXPECT_EQ(style.observations[0].postponedEvents, 1U);
    EXPECT_EQ(style.observations[1].postponedEvents, 0U);
    EXPECT_TRUE(style.observations[0].processed);
    EXPECT_FALSE(style.observations[1].processed);
}

TEST_F(GestureNavigationTest, middlePressReplaysPostponedEventsBeforeItself)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    std::vector<ForwardedMouseEvent> forwarded;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoMouseButtonEvent::getClassTypeId(), handleMouseButtonEvent, &forwarded);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON3, SoButtonEvent::DOWN, {101, 101}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    EXPECT_EQ(style.postponedEventCount(), 0U);

    ASSERT_EQ(forwarded.size(), 2U);
    EXPECT_EQ(forwarded[0].button, SoMouseButtonEvent::BUTTON1);
    EXPECT_EQ(forwarded[0].state, SoButtonEvent::DOWN);
    EXPECT_EQ(forwarded[1].button, SoMouseButtonEvent::BUTTON3);
    EXPECT_EQ(forwarded[1].state, SoButtonEvent::DOWN);
    root->removeChild(callback);
}

TEST_F(GestureNavigationTest, leftDragEntersRotationAfterMoveThreshold)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_EQ(style.postponedEventCount(), 1U);

    EXPECT_TRUE(sendMotion(style, {110, 108}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
    EXPECT_EQ(style.postponedEventCount(), 0U);

    EXPECT_TRUE(sendMotion(style, {125, 118}, 1.2));
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {125, 118}, 1.3));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, altLmbDoesNotRotateInNormal3D)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        2.0,
        Gui::NavigationInputState::AltDown
    );
    EXPECT_FALSE(sendMotion(style, {110, 108}, 2.1, Gui::NavigationInputState::AltDown));
    EXPECT_NE(style.getViewingMode(), NavigationStyle::DRAGGING);
    EXPECT_FALSE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::UP,
        {110, 108},
        2.2,
        Gui::NavigationInputState::AltDown
    ));
}

TEST_F(GestureNavigationTest, editingLmbDragSelectsButAltLmbRotates)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);
    viewer.setEditing(true);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    sendMotion(style, {110, 108}, 1.1);
    EXPECT_NE(style.getViewingMode(), NavigationStyle::DRAGGING);
    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {110, 108}, 1.2);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        2.0,
        Gui::NavigationInputState::AltDown
    );
    EXPECT_TRUE(sendMotion(style, {110, 108}, 2.1, Gui::NavigationInputState::AltDown));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
    EXPECT_TRUE(sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::UP,
        {110, 108},
        2.2,
        Gui::NavigationInputState::AltDown
    ));
    viewer.setEditing(false);
}

TEST_F(GestureNavigationTest, shiftLmbDragStartsBoxSelection)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::DOWN,
        {100, 100},
        1.0,
        Gui::NavigationInputState::ShiftDown
    );
    EXPECT_TRUE(sendMotion(style, {130, 125}, 1.1, Gui::NavigationInputState::ShiftDown));
    EXPECT_TRUE(style.isSelectingForTest());
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);

    sendMouse(
        style,
        SoMouseButtonEvent::BUTTON1,
        SoButtonEvent::UP,
        {130, 125},
        1.2,
        Gui::NavigationInputState::ShiftDown
    );
    style.stopSelectionForTest();
}

TEST_F(GestureNavigationTest, lmbPressOnDraggerEntersInteract)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    auto* dragger = new SoTrackballDragger;
    root->addChild(dragger);

    ASSERT_TRUE(style.draggerAt({320, 240}));
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {320, 240}, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::INTERACT);
    EXPECT_FALSE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {320, 240}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);

    root->removeChild(dragger);
}

TEST_F(GestureNavigationTest, stickyPanExitRepairsMissingRightRelease)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_TRUE(sendMotion(style, {110, 108}, 2.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);

    style.simulateMissingRightRelease();
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {110, 108}, 2.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, rightDragEntersPan)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    EXPECT_EQ(style.postponedEventCount(), 1U);

    EXPECT_TRUE(sendMotion(style, {110, 108}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_EQ(style.postponedEventCount(), 0U);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::UP, {120, 115}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, tapAndHoldEntersStickyPan)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_TRUE(sendMotion(style, {110, 108}, 2.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_EQ(style.postponedEventCount(), 0U);

    EXPECT_TRUE(sendMotion(style, {125, 118}, 2.1));
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {125, 118}, 2.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, twoButtonDragEntersTiltAndReturnsThroughPan)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::DOWN, {101, 101}, 1.05);
    EXPECT_EQ(style.postponedEventCount(), 2U);

    EXPECT_TRUE(sendMotion(style, {112, 110}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::DRAGGING);
    EXPECT_EQ(style.postponedEventCount(), 0U);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {120, 115}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::UP, {120, 115}, 1.3));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, gesturePanStartsUpdatesAndEnds)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSStart, {0, 0}, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSUpdate, {8, 5}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSEnd, {0, 0}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, gesturePinchUpdatesCameraAndEnds)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    EXPECT_TRUE(sendGesturePinch(style, SoGestureEvent::SbGSStart, 1.0, 1.0));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendGesturePinch(style, SoGestureEvent::SbGSUpdate, 1.1, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendGesturePinch(style, SoGestureEvent::SbGSEnd, 1.0, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, gestureEndRepairsMissingMouseReleaseState)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSStart, {0, 0}, 1.1));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::PANNING);
    EXPECT_TRUE(sendGesturePan(style, SoGestureEvent::SbGSEnd, {0, 0}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);

    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {100, 100}, 1.3));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    ASSERT_EQ(style.observations.size(), 4U);
    EXPECT_EQ(style.observations[0].postponedEvents, 1U);
    EXPECT_EQ(style.observations[1].postponedEvents, 0U);
    EXPECT_EQ(style.observations[2].postponedEvents, 0U);
    EXPECT_EQ(style.observations[3].postponedEvents, 0U);
    for (const auto& observation : style.observations) {
        EXPECT_TRUE(observation.processed);
    }
}

TEST_F(GestureNavigationTest, rollSequenceEndsInAwaitingReleaseThenIdle)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::DOWN, {101, 101}, 1.1);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {101, 101}, 1.2));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON2, SoButtonEvent::UP, {101, 101}, 1.3));
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
    ASSERT_EQ(style.observations.size(), 4U);
    EXPECT_EQ(style.observations[0].postponedEvents, 1U);
    EXPECT_EQ(style.observations[1].postponedEvents, 2U);
    EXPECT_EQ(style.observations[2].postponedEvents, 0U);
    EXPECT_EQ(style.observations[3].postponedEvents, 0U);
}

TEST_F(GestureNavigationTest, longClickOpensPopupMenu)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);
    style.setPopupMenuEnabled(true);

    sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::DOWN, {100, 100}, 1.0);
    EXPECT_TRUE(sendMouse(style, SoMouseButtonEvent::BUTTON1, SoButtonEvent::UP, {100, 100}, 2.0));
    EXPECT_TRUE(style.popupOpened);
    EXPECT_EQ(style.getViewingMode(), NavigationStyle::IDLE);
}

TEST_F(GestureNavigationTest, unhandledKeyboardEventsAreForwarded)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    GestureProbe style;
    configureStyle(style, viewer);

    bool handledByScene = false;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoKeyboardEvent::getClassTypeId(), handleKeyboardEvent, &handledByScene);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    SoKeyboardEvent event;
    event.setKey(SoKeyboardEvent::A);
    event.setState(SoButtonEvent::DOWN);
    event.setPosition({100, 100});
    event.setTime(SbTime(1.0));

    EXPECT_TRUE(style.send(event));
    EXPECT_TRUE(handledByScene);

    root->removeChild(callback);
}
