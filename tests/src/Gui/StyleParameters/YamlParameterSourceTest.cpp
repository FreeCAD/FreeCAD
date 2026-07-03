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

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include <Gui/Application.h>
#include <Gui/StyleParameters/ParameterManager.h>
#include <Gui/StyleParameters/Value.h>
#include <src/TempDirectory.h>

using namespace Gui::StyleParameters;

// Alias to avoid ambiguity with ParameterGrp::Manager() from Base/Parameter.h.
using StyleParameterManager = Gui::StyleParameters::ParameterManager;

class YamlParameterSourceTest: public ::testing::Test
{
protected:
    tests::TempDirectory tempDir {"fctest"};
    std::filesystem::path tempPath = tempDir.path() / "test_yaml_parameter_source.yaml";

    void TearDown() override
    {
        std::filesystem::remove(tempPath);
    }

    YamlParameterSource loadYaml(const std::string& content)
    {
        std::ofstream out(tempPath);
        out << content;
        out.close();
        return YamlParameterSource(tempPath.string());
    }

    StyleParameterManager managerWith(YamlParameterSource& source)
    {
        StyleParameterManager manager;
        manager.addSource(&source);
        return manager;
    }
};

TEST_F(YamlParameterSourceTest, ScalarStringIsLoadedAsExpression)
{
    auto source = loadYaml("BasePadding: 10px\n");
    const auto param = source.get("BasePadding");

    ASSERT_TRUE(param.has_value());
    EXPECT_EQ(param->value, "10px");
}

TEST_F(YamlParameterSourceTest, ScalarColorIsLoadedAsExpression)
{
    auto source = loadYaml("AccentColor: \"#ff0000\"\n");
    const auto param = source.get("AccentColor");

    ASSERT_TRUE(param.has_value());
    EXPECT_EQ(param->value, "#ff0000");
}

TEST_F(YamlParameterSourceTest, SequenceBecomesUnnamedTupleExpression)
{
    auto source = loadYaml("Corners: [10px, 20px, 10px, 20px]\n");
    const auto param = source.get("Corners");

    ASSERT_TRUE(param.has_value());
    EXPECT_EQ(param->value, "(10px, 20px, 10px, 20px)");
}

TEST_F(YamlParameterSourceTest, BlockSequenceBecomesUnnamedTupleExpression)
{
    auto source = loadYaml(
        "Padding:\n"
        "  - 10px\n"
        "  - 20px\n"
        "  - 5px\n"
        "  - 15px\n"
    );
    const auto param = source.get("Padding");

    ASSERT_TRUE(param.has_value());
    EXPECT_EQ(param->value, "(10px, 20px, 5px, 15px)");
}

TEST_F(YamlParameterSourceTest, MapBecomesNamedTupleExpression)
{
    auto source = loadYaml(
        "Insets:\n"
        "  top: 10px\n"
        "  right: 20px\n"
        "  bottom: 5px\n"
        "  left: 15px\n"
    );
    const auto param = source.get("Insets");

    ASSERT_TRUE(param.has_value());
    EXPECT_EQ(param->value, "(top: 10px, right: 20px, bottom: 5px, left: 15px)");
}

TEST_F(YamlParameterSourceTest, NestedSequenceBecomesNestedTuple)
{
    auto source = loadYaml(
        "Stops:\n"
        "  - [0, \"#ff0000\"]\n"
        "  - [1, \"#0000ff\"]\n"
    );
    const auto param = source.get("Stops");

    ASSERT_TRUE(param.has_value());
    EXPECT_EQ(param->value, "((0, #ff0000), (1, #0000ff))");
}

TEST_F(YamlParameterSourceTest, SequenceResolvesToTupleValue)
{
    auto source = loadYaml("Padding: [10px, 20px]\n");
    auto manager = managerWith(source);
    const auto result = manager.resolve("Padding");

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds<Tuple>());

    const auto& tuple = result->get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 20.0);
}

TEST_F(YamlParameterSourceTest, MapResolvesToNamedTupleValue)
{
    auto source = loadYaml(
        "Insets:\n"
        "  top: 10px\n"
        "  right: 20px\n"
    );
    auto manager = managerWith(source);
    const auto result = manager.resolve("Insets");

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds<Tuple>());

    const auto& tuple = result->get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);
    ASSERT_NE(tuple.find("top"), nullptr);
    EXPECT_DOUBLE_EQ(tuple.find("top")->get<Numeric>().value, 10.0);
    ASSERT_NE(tuple.find("right"), nullptr);
    EXPECT_DOUBLE_EQ(tuple.find("right")->get<Numeric>().value, 20.0);
}

TEST_F(YamlParameterSourceTest, MixedFileLoadsAllParameters)
{
    auto source = loadYaml(
        "BasePadding: 8px\n"
        "AccentColor: \"#ff0000\"\n"
        "Corners: [4px, 8px, 4px, 8px]\n"
        "Insets:\n"
        "  top: 10px\n"
        "  right: 20px\n"
    );

    const auto all = source.all();
    EXPECT_EQ(all.size(), 4);
    EXPECT_TRUE(source.get("BasePadding").has_value());
    EXPECT_TRUE(source.get("AccentColor").has_value());
    EXPECT_TRUE(source.get("Corners").has_value());
    EXPECT_TRUE(source.get("Insets").has_value());
}

TEST_F(YamlParameterSourceTest, FlushAndReloadPreservesSequenceValue)
{
    auto source = loadYaml("Padding: [10px, 20px]\n");
    source.flush();

    YamlParameterSource reloaded(tempPath.string());
    const auto param = reloaded.get("Padding");

    ASSERT_TRUE(param.has_value());

    // After flush the value is stored as a string expression — semantics are preserved
    auto manager = managerWith(reloaded);
    const auto result = manager.resolve("Padding");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds<Tuple>());
    EXPECT_EQ(result->get<Tuple>().size(), 2);
}
