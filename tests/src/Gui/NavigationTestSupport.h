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

#pragma once

#include <gtest/gtest.h>

#include <QApplication>

#include <Gui/Application.h>
#include <Gui/MainWindow.h>

#include <memory>

namespace tests
{

class NavigationTestEnvironment final: public ::testing::Environment
{
public:
    void SetUp() override;
    void TearDown() override;

private:
    std::unique_ptr<QApplication> qtApplication;
    std::unique_ptr<Gui::Application> guiApplication;
    std::unique_ptr<Gui::MainWindow> mainWindow;
};

}  // namespace tests
