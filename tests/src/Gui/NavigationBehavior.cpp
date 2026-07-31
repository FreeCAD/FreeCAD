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
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <Inventor/SoDB.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoGroup.h>
#include <QApplication>

#include <Gui/Navigation/NavigationInputState.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/Navigation/MappedNavigationStyle.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Quarter/Quarter.h>
#include <Gui/SoFCDB.h>
#include <Gui/View3DInventorViewer.h>

#include <src/App/InitApplication.h>

#include <memory>

namespace
{

enum class EventType
{
    MousePress,
    MouseRelease,
    KeyPress,
    KeyRelease,
    PointerMotion
};

enum class MouseButton
{
    None,
    Left,
    Middle,
    Right
};

using ModifierFlags = unsigned int;
using NavigationStyle = Gui::NavigationStyle;

constexpr ModifierFlags CtrlDown = Gui::NavigationInputState::CtrlDown;
constexpr ModifierFlags ShiftDown = Gui::NavigationInputState::ShiftDown;
constexpr ModifierFlags AltDown = Gui::NavigationInputState::AltDown;

struct NavigationStep
{
    EventType event;
    MouseButton button;
    ModifierFlags modifiers;
    NavigationStyle::ViewerMode expectedMode;
    bool expectedProcessed;
    double timestamp = -1.0;
    SoKeyboardEvent::Key key = SoKeyboardEvent::LEFT_SHIFT;
};

const char* eventTypeName(const EventType event)
{
    switch (event) {
        case EventType::MousePress:
            return "mouse press";
        case EventType::MouseRelease:
            return "mouse release";
        case EventType::KeyPress:
            return "key press";
        case EventType::KeyRelease:
            return "key release";
        case EventType::PointerMotion:
            return "pointer motion";
    }
    return "unknown event";
}

const char* mouseButtonName(const MouseButton button)
{
    switch (button) {
        case MouseButton::None:
            return "none";
        case MouseButton::Left:
            return "left";
        case MouseButton::Middle:
            return "middle";
        case MouseButton::Right:
            return "right";
    }
    return "unknown button";
}

class NavigationStyleTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();

        if (!QApplication::instance()) {
            static int argc = 1;
            static char appName[] = "Gui_navigation_tests";
            static char* argv[] = {appName, nullptr};
            qtApplication = std::make_unique<QApplication>(argc, argv);
        }

        Gui::Application::initApplication();
        if (!SoDB::isInitialized()) {
            Gui::Application::initOpenInventor();
        }
        guiApplication = std::make_unique<Gui::Application>(true);
        mainWindow = std::make_unique<Gui::MainWindow>();
    }

    static void TearDownTestSuite()
    {
        // The GUI singleton and Qt widget tree intentionally live until process exit. Their
        // normal static destruction order conflicts with Qt's accessibility teardown.
        mainWindow.release();
        guiApplication.release();
        qtApplication.release();
    }

    static inline std::unique_ptr<QApplication> qtApplication;
    static inline std::unique_ptr<Gui::Application> guiApplication;
    static inline std::unique_ptr<Gui::MainWindow> mainWindow;
};

template<typename Style>
class StyleProbe: public Style
{
public:
    using Style::processSoEvent;

    bool hasPannedFlag() const
    {
        return this->hasPanned;
    }

    bool hasDraggedFlag() const
    {
        return this->hasDragged;
    }

    bool hasZoomedFlag() const
    {
        return this->hasZoomed;
    }

    double centerTimeValue() const
    {
        return this->centerTime.getValue();
    }

    bool popupOpened = false;

protected:
    void openPopupMenu(const SbVec2s&) override
    {
        popupOpened = true;
    }
};

constexpr Gui::NavigationRule testRules[] {
    {std::nullopt, Gui::NavigationInputState::LeftDown, NavigationStyle::SELECTION},
    {std::nullopt,
     Gui::NavigationInputState::CtrlDown | Gui::NavigationInputState::LeftDown,
     NavigationStyle::SELECTION},
    {std::nullopt,
     Gui::NavigationInputState::MiddleDown,
     NavigationStyle::PANNING,
     Gui::ownedBy(Gui::NavigationInputState::MiddleDown)},
    {std::nullopt,
     Gui::NavigationInputState::ShiftDown | Gui::NavigationInputState::MiddleDown,
     NavigationStyle::DRAGGING,
     Gui::ownedBy(Gui::NavigationInputState::MiddleDown)},
    {std::nullopt,
     Gui::NavigationInputState::CtrlDown | Gui::NavigationInputState::MiddleDown,
     NavigationStyle::ZOOMING,
     Gui::ownedBy(Gui::NavigationInputState::MiddleDown)},
    {std::nullopt,
     Gui::NavigationInputState::CtrlDown | Gui::NavigationInputState::ShiftDown
         | Gui::NavigationInputState::RightDown,
     NavigationStyle::ZOOMING,
     Gui::ownedBy(Gui::NavigationInputState::RightDown)},
    {std::nullopt,
     Gui::NavigationInputState::LeftDown | Gui::NavigationInputState::RightDown,
     NavigationStyle::PANNING,
     Gui::ownedBy(Gui::NavigationInputState::LeftDown | Gui::NavigationInputState::RightDown)},
};

constexpr Gui::NavigationProfile testProfile {testRules};

class TestMappedNavigationStyle: public Gui::MappedNavigationStyle
{
public:
    using MappedNavigationStyle::processSoEvent;

    const char* mouseButtons(ViewerMode) override
    {
        return "test";
    }

    int styleButtonEventCount = 0;
    bool popupOpened = false;

    bool hasPannedFlag() const
    {
        return hasPanned;
    }

    bool hasDraggedFlag() const
    {
        return hasDragged;
    }

    bool hasZoomedFlag() const
    {
        return hasZoomed;
    }

    double centerTimeValue() const
    {
        return centerTime.getValue();
    }

protected:
    const Gui::NavigationProfile& profile() const override
    {
        return testProfile;
    }

    void processStyleButtonEvent(EventContext&) override
    {
        ++styleButtonEventCount;
    }

    void openPopupMenu(const SbVec2s&) override
    {
        popupOpened = true;
    }
};

SoMouseButtonEvent::Button coinButton(const MouseButton button)
{
    switch (button) {
        case MouseButton::Left:
            return SoMouseButtonEvent::BUTTON1;
        case MouseButton::Right:
            return SoMouseButtonEvent::BUTTON2;
        case MouseButton::Middle:
            return SoMouseButtonEvent::BUTTON3;
        case MouseButton::None:
            return SoMouseButtonEvent::ANY;
    }
    return SoMouseButtonEvent::ANY;
}

template<typename Style>
void runSequence(Style& style, std::initializer_list<NavigationStep> steps)
{
    SbVec2s position(100, 100);
    double time = 1.0;
    std::size_t stepIndex = 0;
    for (const NavigationStep& step : steps) {
        SCOPED_TRACE(
            ::testing::Message() << "step=" << stepIndex << " event=" << eventTypeName(step.event)
                                 << " button=" << mouseButtonName(step.button) << " modifiers=0x"
                                 << std::hex << step.modifiers
        );

        position += SbVec2s(7, 5);
        const double eventTime = step.timestamp >= 0.0 ? step.timestamp : time;

        bool processed = false;
        if (step.event == EventType::PointerMotion) {
            SoLocation2Event event;
            event.setPosition(position);
            event.setTime(SbTime(eventTime));
            event.setCtrlDown((step.modifiers & CtrlDown) != 0);
            event.setShiftDown((step.modifiers & ShiftDown) != 0);
            event.setAltDown((step.modifiers & AltDown) != 0);
            processed = style.processSoEvent(&event);
        }
        else if (step.event == EventType::KeyPress || step.event == EventType::KeyRelease) {
            SoKeyboardEvent event;
            event.setKey(step.key);
            event.setState(step.event == EventType::KeyPress ? SoButtonEvent::DOWN : SoButtonEvent::UP);
            event.setPosition(position);
            event.setTime(SbTime(eventTime));
            event.setCtrlDown((step.modifiers & CtrlDown) != 0);
            event.setShiftDown((step.modifiers & ShiftDown) != 0);
            event.setAltDown((step.modifiers & AltDown) != 0);
            processed = style.processSoEvent(&event);
        }
        else {
            SoMouseButtonEvent event;
            event.setButton(coinButton(step.button));
            event.setState(
                step.event == EventType::MousePress ? SoButtonEvent::DOWN : SoButtonEvent::UP
            );
            event.setPosition(position);
            event.setTime(SbTime(eventTime));
            event.setCtrlDown((step.modifiers & CtrlDown) != 0);
            event.setShiftDown((step.modifiers & ShiftDown) != 0);
            event.setAltDown((step.modifiers & AltDown) != 0);
            processed = style.processSoEvent(&event);
        }

        EXPECT_EQ(static_cast<NavigationStyle::ViewerMode>(style.getViewingMode()), step.expectedMode)
            << "at event " << eventTime;
        EXPECT_EQ(static_cast<bool>(processed), step.expectedProcessed) << "at event " << eventTime;
        time += 1.0;
        ++stepIndex;
    }
}

template<typename Style>
void configureStyle(Style& style, Gui::View3DInventorViewer& viewer)
{
    style.setViewer(&viewer);
    style.setPopupMenuEnabled(false);
}

void handleKeyboardEvent(void* userData, SoEventCallback* callback)
{
    auto* handled = static_cast<bool*>(userData);
    *handled = true;
    callback->setHandled();
}

}  // namespace

TEST_F(NavigationStyleTest, mappedStyleForcesRotationWhenButtonsAreAdded)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    TestMappedNavigationStyle style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Middle, 0, NavigationStyle::PANNING, false},
         {EventType::MousePress, MouseButton::Left, 0, NavigationStyle::DRAGGING, true},
         {EventType::MouseRelease, MouseButton::Left, 0, NavigationStyle::PANNING, true},
         {EventType::MouseRelease, MouseButton::Middle, 0, NavigationStyle::IDLE, true}}
    );
}

TEST_F(NavigationStyleTest, mappedStyleTerminatesWhenMiddleIsReleasedWithModifier)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    TestMappedNavigationStyle style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Middle, ShiftDown, NavigationStyle::DRAGGING, false},
         {EventType::MouseRelease, MouseButton::Middle, ShiftDown, NavigationStyle::IDLE, false},
         {EventType::MousePress, MouseButton::Middle, CtrlDown, NavigationStyle::ZOOMING, false},
         {EventType::MouseRelease, MouseButton::Middle, CtrlDown, NavigationStyle::IDLE, false}}
    );
}

TEST_F(NavigationStyleTest, mappedStyleUsesSharedProcessingPipeline)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    TestMappedNavigationStyle style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Middle, 0, NavigationStyle::PANNING, false},
         {EventType::PointerMotion, MouseButton::None, 0, NavigationStyle::PANNING, true},
         {EventType::MouseRelease, MouseButton::Middle, 0, NavigationStyle::IDLE, false},
         {EventType::MousePress, MouseButton::Middle, ShiftDown, NavigationStyle::DRAGGING, false},
         {EventType::MouseRelease, MouseButton::Middle, 0, NavigationStyle::IDLE, false},
         {EventType::MousePress, MouseButton::Middle, CtrlDown, NavigationStyle::ZOOMING, false},
         {EventType::MouseRelease, MouseButton::Middle, 0, NavigationStyle::IDLE, false},
         {EventType::MousePress, MouseButton::Left, 0, NavigationStyle::SELECTION, false},
         {EventType::MousePress, MouseButton::Right, 0, NavigationStyle::PANNING, true},
         {EventType::MouseRelease, MouseButton::Right, 0, NavigationStyle::IDLE, false},
         {EventType::MouseRelease, MouseButton::Left, 0, NavigationStyle::IDLE, true}}
    );

    EXPECT_EQ(style.styleButtonEventCount, 10);
}

TEST_F(NavigationStyleTest, mappedStylePreparesMiddleButtonPress)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    TestMappedNavigationStyle style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Middle, ShiftDown, NavigationStyle::DRAGGING, false, 2.5}}
    );

    EXPECT_DOUBLE_EQ(style.centerTimeValue(), 2.5);
}

TEST_F(NavigationStyleTest, mappedStyleForwardsUnhandledEvents)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    TestMappedNavigationStyle style;
    configureStyle(style, viewer);

    bool handledByScene = false;
    auto* callback = new SoEventCallback;
    callback->addEventCallback(SoKeyboardEvent::getClassTypeId(), handleKeyboardEvent, &handledByScene);
    auto* root = static_cast<SoGroup*>(viewer.getSoRenderManager()->getSceneGraph());
    root->addChild(callback);

    SoKeyboardEvent event;
    event.setKey(SoKeyboardEvent::A);
    event.setState(SoButtonEvent::DOWN);
    event.setPosition(SbVec2s(100, 100));
    event.setTime(SbTime::getTimeOfDay());

    EXPECT_TRUE(style.processSoEvent(&event));
    EXPECT_TRUE(handledByScene);

    root->removeChild(callback);
}

TEST_F(NavigationStyleTest, tinkerCADKeepsSimpleRightClickPopup)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::TinkerCADNavigationStyle> style;
    configureStyle(style, viewer);
    style.setPopupMenuEnabled(true);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Right, 0, NavigationStyle::DRAGGING, true},
         {EventType::MouseRelease, MouseButton::Right, 0, NavigationStyle::IDLE, true}}
    );

    EXPECT_TRUE(style.popupOpened);
}

TEST_F(NavigationStyleTest, tinkerCADKeepsPanningWhenRightButtonIsAdded)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::TinkerCADNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Middle, 0, NavigationStyle::PANNING, false},
         {EventType::MousePress, MouseButton::Right, 0, NavigationStyle::PANNING, false},
         {EventType::MouseRelease, MouseButton::Right, 0, NavigationStyle::PANNING, true},
         {EventType::MouseRelease, MouseButton::Middle, 0, NavigationStyle::IDLE, true}}
    );
}

TEST_F(NavigationStyleTest, openCascadeCtrlLmbMotionEntersZoom)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::OpenCascadeNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Left, 0, NavigationStyle::SELECTION, false},
         {EventType::PointerMotion, MouseButton::None, CtrlDown, NavigationStyle::ZOOMING, true},
         {EventType::MouseRelease, MouseButton::Left, 0, NavigationStyle::IDLE, true}}
    );
}

TEST_F(NavigationStyleTest, openCascadePreservesRotationAcrossModifierChanges)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::OpenCascadeNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Right, CtrlDown, NavigationStyle::DRAGGING, false},
         {EventType::KeyPress, MouseButton::None, CtrlDown | ShiftDown, NavigationStyle::DRAGGING, false},
         {EventType::KeyRelease, MouseButton::None, CtrlDown, NavigationStyle::DRAGGING, false},
         {EventType::MouseRelease, MouseButton::Right, 0, NavigationStyle::IDLE, true}}
    );
}

TEST_F(NavigationStyleTest, openCascadeSuppressesCtrlSelectionDrag)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::OpenCascadeNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Left, 0, NavigationStyle::SELECTION, false},
         {EventType::KeyPress, MouseButton::None, CtrlDown, NavigationStyle::SELECTION, false},
         {EventType::KeyPress, MouseButton::None, CtrlDown | ShiftDown, NavigationStyle::SELECTION, false},
         {EventType::PointerMotion,
          MouseButton::None,
          CtrlDown | ShiftDown,
          NavigationStyle::SELECTION,
          false}}
    );
}

TEST_F(NavigationStyleTest, touchpadNavigationModes)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::TouchpadNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::KeyPress, MouseButton::None, ShiftDown, NavigationStyle::PANNING, false},
         {EventType::PointerMotion, MouseButton::None, ShiftDown, NavigationStyle::PANNING, true},
         {EventType::KeyRelease, MouseButton::None, 0, NavigationStyle::IDLE, false},
         {EventType::KeyPress,
          MouseButton::None,
          AltDown,
          NavigationStyle::DRAGGING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::MousePress, MouseButton::Left, AltDown, NavigationStyle::DRAGGING, true},
         {EventType::MouseRelease, MouseButton::Left, AltDown, NavigationStyle::DRAGGING, false},
         {EventType::KeyRelease,
          MouseButton::None,
          0,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyPress,
          MouseButton::None,
          CtrlDown,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_CONTROL},
         {EventType::KeyPress, MouseButton::None, CtrlDown | ShiftDown, NavigationStyle::ZOOMING, false},
         {EventType::KeyRelease, MouseButton::None, CtrlDown, NavigationStyle::IDLE, false},
         {EventType::KeyRelease,
          MouseButton::None,
          0,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_CONTROL},
         {EventType::MousePress, MouseButton::Middle, 0, NavigationStyle::IDLE, false},
         {EventType::MouseRelease, MouseButton::Middle, 0, NavigationStyle::IDLE, false}}
    );
}

TEST_F(NavigationStyleTest, touchpadPreservesModeForUnmappedModifierCombinations)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::TouchpadNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::KeyPress,
          MouseButton::None,
          AltDown,
          NavigationStyle::DRAGGING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyPress,
          MouseButton::None,
          AltDown | ShiftDown,
          NavigationStyle::DRAGGING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_SHIFT},
         {EventType::KeyRelease,
          MouseButton::None,
          AltDown,
          NavigationStyle::DRAGGING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_SHIFT},
         {EventType::KeyRelease,
          MouseButton::None,
          0,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyPress,
          MouseButton::None,
          ShiftDown,
          NavigationStyle::PANNING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_SHIFT},
         {EventType::KeyPress,
          MouseButton::None,
          ShiftDown | AltDown,
          NavigationStyle::PANNING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyRelease,
          MouseButton::None,
          ShiftDown,
          NavigationStyle::PANNING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyRelease,
          MouseButton::None,
          0,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_SHIFT},
         {EventType::KeyPress,
          MouseButton::None,
          CtrlDown,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_CONTROL},
         {EventType::KeyPress,
          MouseButton::None,
          CtrlDown | ShiftDown,
          NavigationStyle::ZOOMING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_SHIFT},
         {EventType::KeyPress,
          MouseButton::None,
          CtrlDown | ShiftDown | AltDown,
          NavigationStyle::ZOOMING,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyRelease,
          MouseButton::None,
          CtrlDown | ShiftDown,
          NavigationStyle::ZOOMING,
          true,
          -1.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::KeyRelease,
          MouseButton::None,
          CtrlDown,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_SHIFT},
         {EventType::KeyRelease,
          MouseButton::None,
          0,
          NavigationStyle::IDLE,
          false,
          -1.0,
          SoKeyboardEvent::LEFT_CONTROL}}
    );
}

TEST_F(NavigationStyleTest, touchpadAltRotationInitializesCursorAnchor)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::TouchpadNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::KeyPress,
          MouseButton::None,
          AltDown,
          NavigationStyle::DRAGGING,
          false,
          4.0,
          SoKeyboardEvent::LEFT_ALT},
         {EventType::PointerMotion, MouseButton::None, AltDown, NavigationStyle::DRAGGING, true, 4.1}}
    );

    EXPECT_DOUBLE_EQ(style.centerTimeValue(), 4.0);
}

TEST_F(NavigationStyleTest, openSCADSelectionMotionStartsRotation)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::OpenSCADNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Left, 0, NavigationStyle::SELECTION, false},
         {EventType::PointerMotion, MouseButton::None, 0, NavigationStyle::DRAGGING, false},
         {EventType::MouseRelease, MouseButton::Left, 0, NavigationStyle::IDLE, true}}
    );
}

TEST_F(NavigationStyleTest, openSCADRotationInitializesCursorOnce)
{
    Gui::View3DInventorViewer viewer(nullptr);
    viewer.resize(640, 480);
    StyleProbe<Gui::OpenSCADNavigationStyle> style;
    configureStyle(style, viewer);

    runSequence(
        style,
        {{EventType::MousePress, MouseButton::Left, 0, NavigationStyle::SELECTION, false, 2.0},
         {EventType::PointerMotion, MouseButton::None, 0, NavigationStyle::DRAGGING, false, 3.0}}
    );

    EXPECT_DOUBLE_EQ(style.centerTimeValue(), 3.0);
}
