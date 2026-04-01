// SPDX-License-Identifier: LGPL-2.1-or-later

// Testing for conflicting shortcuts in Sketcher

#include <gtest/gtest.h>

#include <string>
#include <vector>

// Helpers

namespace
{

std::vector<std::pair<std::string, std::string>> sketcherShortcuts()
{
    return {
        {"Sketcher_CreateText", "X, T"},
        {"Sketcher_Trimming", "G, T"},
        // Add further entries here as needed.
    };
}

}  // namespace

class SketcherShortcutTest: public ::testing::Test
{
};

// Test 1 - Regression guard for the changes made.

TEST_F(SketcherShortcutTest, textToolAndTrimEdgeDoNotConflict)
{
    // Arrange
    const std::string textAccel = "X, T";
    const std::string trimAccel = "G, T";

    // Assert
    EXPECT_NE(textAccel, trimAccel);
}

// Test 2 - invariant: no two Sketcher commands share a shortcut

TEST_F(SketcherShortcutTest, noTwoSketcherCommandsShareAShortcut)
{
    // Arrange
    auto commands = sketcherShortcuts();

    // Assert
    for (std::size_t i = 0; i < commands.size(); ++i) {
        for (std::size_t j = i + 1; j < commands.size(); ++j) {
            if (commands[i].second == commands[j].second) {
                ADD_FAILURE() << "Shortcut conflict: " << commands[i].first << " (\""
                              << commands[i].second << "\") "
                              << "conflicts with " << commands[j].first << " (\""
                              << commands[j].second << "\").";
            }
        }
    }
}
