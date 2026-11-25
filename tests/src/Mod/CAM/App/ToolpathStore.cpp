// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <Base/Exception.h>
#include <Mod/CAM/App/Command.h>
#include <Mod/CAM/App/ToolpathStore.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
namespace
{
using Path::Command;
namespace Store = Path::ToolpathStore;

std::vector<Command> roundTrip(const std::vector<Command>& commands)
{
    std::vector<Command> result;
    Store::decode(Store::encode(commands), result);
    return result;
}

std::vector<Command> mixedCommands()
{
    return {
        Command("(a comment)", {}),
        Command("G90", {}),
        Command("M6", {{"T", 3.0}}, {{"tool_change_format", std::string("T_M6")}}),
        Command("G0", {{"X", 1.25}, {"Y", -2.5}, {"Z", 10.0}}),
        Command(
            "G1",
            {{"F", 16.6667}, {"X", 1.0 / 3.0}, {"Y", 2.0 / 3.0}, {"Z", -0.1234567891}},
            {{"type", std::string("linking")}, {"weight", 0.25}}
        ),
        Command("G2", {{"F", 10.0}, {"I", 2.5}, {"J", 0.0}, {"X", 5.0}, {"Y", 5.0}}),
        Command("G81", {{"F", 2.5}, {"R", 3.0}, {"X", 10.0}, {"Y", 10.0}, {"Z", -12.0}}),
        Command("G1", {{"A", 45.5}, {"B", -10.0}, {"X", -123456.654321}}),
        Command("G80", {}),
    };
}
}  // namespace

TEST(ToolpathStoreTest, emptyList)
{
    const std::string data = Store::encode({});
    EXPECT_TRUE(Store::isBinary(data));
    std::vector<Command> result {Command("G0", {})};
    Store::decode(data, result);
    EXPECT_TRUE(result.empty());
}

TEST(ToolpathStoreTest, textIsNotBinary)
{
    EXPECT_FALSE(Store::isBinary("G0 X1.000000\n"));
    EXPECT_FALSE(Store::isBinary(""));
    EXPECT_FALSE(Store::isBinary("FCT"));
}

TEST(ToolpathStoreTest, namesParametersAndAnnotationsComeBack)
{
    const auto commands = mixedCommands();
    const auto result = roundTrip(commands);
    ASSERT_EQ(result.size(), commands.size());
    for (std::size_t i = 0; i < commands.size(); ++i) {
        EXPECT_EQ(result[i].Name, commands[i].Name) << "command " << i;
        ASSERT_EQ(result[i].Parameters.size(), commands[i].Parameters.size()) << "command " << i;
        for (const auto& [key, value] : commands[i].Parameters) {
            ASSERT_TRUE(result[i].Parameters.contains(key)) << "command " << i << " " << key;
            EXPECT_NEAR(result[i].Parameters.at(key), value, 0.5 * Store::Resolution)
                << "command " << i << " " << key;
        }
        ASSERT_EQ(result[i].Annotations.size(), commands[i].Annotations.size()) << "command " << i;
    }
    EXPECT_EQ(std::get<std::string>(result[2].Annotations.at("tool_change_format")), "T_M6");
    EXPECT_EQ(std::get<std::string>(result[4].Annotations.at("type")), "linking");
    EXPECT_NEAR(std::get<double>(result[4].Annotations.at("weight")), 0.25, 1e-9);
}

TEST(ToolpathStoreTest, sameTextAsBefore)
{
    // The stored form holds the same information as the G-code text with six decimals.
    const auto commands = mixedCommands();
    const auto result = roundTrip(commands);
    ASSERT_EQ(result.size(), commands.size());
    for (std::size_t i = 0; i < commands.size(); ++i) {
        EXPECT_EQ(result[i].toGCode(), commands[i].toGCode());
    }
}

TEST(ToolpathStoreTest, roundingDoesNotAccumulate)
{
    std::vector<Command> commands;
    for (int i = 0; i < 5000; ++i) {
        commands.emplace_back("G1", std::map<std::string, double> {{"X", i * (1.0 / 3.0)}});
    }
    const auto result = roundTrip(commands);
    ASSERT_EQ(result.size(), commands.size());
    for (std::size_t i = 0; i < commands.size(); ++i) {
        EXPECT_NEAR(
            result[i].Parameters.at("X"),
            commands[i].Parameters.at("X"),
            0.5 * Store::Resolution + 1e-12
        );
    }
}

TEST(ToolpathStoreTest, valuesThatCannotBeRoundedStayExact)
{
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<Command> commands {
        Command("G1", {{"X", 1e15}, {"Y", inf}, {"Z", 0.1}}),
        Command("G1", {{"X", -3.0000001e13}, {"Y", nan}, {"Z", 0.2}}),
    };
    const auto result = roundTrip(commands);
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].Parameters.at("X"), 1e15);
    EXPECT_EQ(result[1].Parameters.at("X"), -3.0000001e13);
    EXPECT_EQ(result[0].Parameters.at("Y"), inf);
    EXPECT_TRUE(std::isnan(result[1].Parameters.at("Y")));
    EXPECT_NEAR(result[1].Parameters.at("Z"), 0.2, 1e-9);
}

TEST(ToolpathStoreTest, commentWithSemicolonComesBack)
{
    Command comment;
    comment.Name = "(a comment; with a semicolon)";
    const auto result = roundTrip({comment, Command("G0", {{"X", 1.0}})});
    ASSERT_EQ(result.size(), 2U);
    EXPECT_EQ(result[0].Name, "(a comment; with a semicolon)");
}

TEST(ToolpathStoreTest, damagedDataIsRefused)
{
    const std::string data = Store::encode(mixedCommands());
    std::vector<Command> result;

    EXPECT_THROW(Store::decode("G0 X1.000000\n", result), Base::BadFormatError);

    std::string wrongVersion = data;
    wrongVersion[4] = 99;
    EXPECT_THROW(Store::decode(wrongVersion, result), Base::BadFormatError);

    // Each truncated form must be refused; none may crash or return a partial list.
    for (std::size_t length = 0; length < data.size(); ++length) {
        result.clear();
        EXPECT_THROW(Store::decode(std::string_view(data).substr(0, length), result), Base::BadFormatError)
            << "length " << length;
        EXPECT_TRUE(result.empty()) << "length " << length;
    }
}

TEST(ToolpathStoreTest, changedBytesDoNotCrash)
{
    const std::string data = Store::encode(mixedCommands());
    for (std::size_t i = 5; i < data.size(); ++i) {
        for (const char value : {'\0', '\x7f', '\xff'}) {
            std::string changed = data;
            changed[i] = value;
            std::vector<Command> result;
            try {
                Store::decode(changed, result);
            }
            catch (const Base::BadFormatError&) {
            }
        }
    }
    SUCCEED();
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
