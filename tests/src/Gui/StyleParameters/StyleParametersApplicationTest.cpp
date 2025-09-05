// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <gtest/gtest.h>

#include "src/App/InitApplication.h"

#include <Gui/Application.h>
#include <Gui/StyleParameters/ParameterManager.h>

using namespace Gui;

class StyleParametersApplicationTest: public ::testing::Test
{
protected:
    static Application* app;

    static void SetUpTestSuite()
    {
        tests::initApplication();
        app = new Application(true);
    }

    void SetUp() override
    {
        auto styleParamManager = app->styleParameterManager();

        styleParamManager->addSource(new StyleParameters::InMemoryParameterSource(
            {
                {.name = "ColorPrimary", .value = "#ff0000"},
                {.name = "FontSize", .value = "12px"},
                {.name = "BoxWidth", .value = "100px"},
            },
            {.name = "Fixture Source"}));
    }
};

Application* StyleParametersApplicationTest::app = {};

// Test for replacing variables in QSS string
TEST_F(StyleParametersApplicationTest, ReplaceVariablesInQss)
{
    QString qss = "QWidget { color: @ColorPrimary; font-size: @FontSize; width: @BoxWidth; }";
    QString result = app->replaceVariablesInQss(qss);

    EXPECT_EQ(result.toStdString(), "QWidget { color: #ff0000; font-size: 12px; width: 100px; }");
}

// Test if unknown variables remain unchanged
TEST_F(StyleParametersApplicationTest, ReplaceVariablesInQssWithUnknownVariable)
{
    QString qss = "QWidget { color: @UnknownColor; margin: 10px; }";
    QString result = app->replaceVariablesInQss(qss);

    EXPECT_EQ(result.toStdString(), "QWidget { color: ; margin: 10px; }");
}

// Test with an empty QSS string
TEST_F(StyleParametersApplicationTest, ReplaceVariablesInQssWithEmptyString)
{
    QString qss = "";
    QString result = app->replaceVariablesInQss(qss);

    EXPECT_EQ(result.toStdString(), "");
}

// Test replacing multiple occurrences of the same variable
TEST_F(StyleParametersApplicationTest, ReplaceVariablesInQssWithMultipleOccurrences)
{
    QString qss = "QWidget { color: @ColorPrimary; background: @ColorPrimary; }";
    QString result = app->replaceVariablesInQss(qss);

    EXPECT_EQ(result.toStdString(), "QWidget { color: #ff0000; background: #ff0000; }");
}
