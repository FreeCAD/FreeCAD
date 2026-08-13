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
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                          *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "NavigationTestSupport.h"

#include <Inventor/SoDB.h>

#include <Gui/SoTouchEvents.h>

#include <src/App/InitApplication.h>

namespace tests
{

void NavigationTestEnvironment::SetUp()
{
    tests::initApplication();

    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
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
    if (SoGestureEvent::getClassTypeId().isBad()) {
        SoGestureEvent::initClass();
        SoGesturePanEvent::initClass();
        SoGesturePinchEvent::initClass();
    }
    guiApplication = std::make_unique<Gui::Application>(true);
    mainWindow = std::make_unique<Gui::MainWindow>();
}

void NavigationTestEnvironment::TearDown()
{
    mainWindow.reset();
    guiApplication.reset();
    qtApplication.reset();
}

}  // namespace tests

::testing::Environment* const navigationTestEnvironment = ::testing::AddGlobalTestEnvironment(
    new tests::NavigationTestEnvironment
);
