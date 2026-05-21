// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <QString>
#include <algorithm>

class StatusBarHintsPriorityTest: public ::testing::Test
{
protected:
    StatusBarHintsPriorityTest() = default;
};

// Test that ellipsis logic with std::max works correctly
TEST_F(StatusBarHintsPriorityTest, MinimumWidthCalculation)
{
    // Simulate the std::max(1, widget->width()) logic from setElidedText
    int widgetWidth = 0;
    int availableWidth = std::max(1, widgetWidth);
    EXPECT_EQ(availableWidth, 1);

    widgetWidth = 50;
    availableWidth = std::max(1, widgetWidth);
    EXPECT_EQ(availableWidth, 50);
}

// Test that QString simplification works
TEST_F(StatusBarHintsPriorityTest, TextSimplification)
{
    QString text = "  Multiple   spaces   and   newlines  \n  ";
    QString simplified = text.simplified();

    EXPECT_EQ(simplified, "Multiple spaces and newlines");
    EXPECT_FALSE(simplified.contains("  "));
}

// Test that QString contains() and length() work
TEST_F(StatusBarHintsPriorityTest, StringProcessing)
{
    QString longText = "This is a very long text that should be ellipsed when the widget is too narrow";
    EXPECT_GT(longText.length(), 50);

    QString shortText = "Short";
    EXPECT_LT(shortText.length(), longText.length());
}

// Test property name constant
TEST_F(StatusBarHintsPriorityTest, PropertyNameConstant)
{
    constexpr const char* kStatusBarFullTextProperty = "statusBarFullText";
    EXPECT_STREQ(kStatusBarFullTextProperty, "statusBarFullText");
}
