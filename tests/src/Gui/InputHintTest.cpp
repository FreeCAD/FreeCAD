// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Gui/InputHint.h"

using namespace Gui;

class InputHintTest: public ::testing::Test
{
protected:
    enum class State : std::uint8_t
    {
        SeekFirst,
        SeekSecond
    };

    enum class Method : std::uint8_t
    {
        FirstMethod,
        SecondMethod
    };

    using StateMethod = std::pair<State, Method>;

    // Constants for hints
    static const InputHint firstHint;
    static const InputHint secondHint;
    static const InputHint firstMethodHint;
    static const InputHint secondMethodHint;
    static const InputHint thirdMethodHint;
    static const InputHint fourthMethodHint;
};

// Define the constants
const InputHint InputHintTest::firstHint = {QString("First hint"), {{Qt::Key_Space}}};
const InputHint InputHintTest::secondHint = {QString("Second hint"), {{Qt::Key_Enter}}};
const InputHint InputHintTest::firstMethodHint = {QString("First method hint"), {{Qt::Key_A}}};
const InputHint InputHintTest::secondMethodHint = {QString("Second method hint"), {{Qt::Key_B}}};
const InputHint InputHintTest::thirdMethodHint = {QString("Third method hint"), {{Qt::Key_C}}};
const InputHint InputHintTest::fourthMethodHint = {QString("Fourth method hint"), {{Qt::Key_D}}};

TEST_F(InputHintTest, LookupHintsSimpleState)
{
    // Arrange
    std::list<InputHint> hintsSeekFirst = {firstHint};
    std::list<InputHint> hintsSeekSecond = {secondHint};

    HintTable<State> table = {
        {.state = State::SeekFirst, .hints = hintsSeekFirst},
        {.state = State::SeekSecond, .hints = hintsSeekSecond}
    };

    // Act
    auto resultFirst = lookupHints(State::SeekFirst, table);
    auto resultSecond = lookupHints(State::SeekSecond, table);

    // Assert
    EXPECT_EQ(resultFirst, hintsSeekFirst);
    EXPECT_EQ(resultSecond, hintsSeekSecond);
}

TEST_F(InputHintTest, LookupHintsPairState)
{
    // Arrange
    std::list<InputHint> firstFirstHints = {firstMethodHint};
    std::list<InputHint> firstSecondHints = {secondMethodHint};
    std::list<InputHint> secondFirstHints = {thirdMethodHint};
    std::list<InputHint> secondSecondHints = {fourthMethodHint};

    HintTable<StateMethod> table = {
        {.state = {State::SeekFirst, Method::FirstMethod}, .hints = firstFirstHints},
        {.state = {State::SeekFirst, Method::SecondMethod}, .hints = firstSecondHints},
        {.state = {State::SeekSecond, Method::FirstMethod}, .hints = secondFirstHints},
        {.state = {State::SeekSecond, Method::SecondMethod}, .hints = secondSecondHints}
    };

    // Act
    auto resultFirstFirst = lookupHints({State::SeekFirst, Method::FirstMethod}, table);
    auto resultFirstSecond = lookupHints({State::SeekFirst, Method::SecondMethod}, table);
    auto resultSecondFirst = lookupHints({State::SeekSecond, Method::FirstMethod}, table);
    auto resultSecondSecond = lookupHints({State::SeekSecond, Method::SecondMethod}, table);

    // Assert
    EXPECT_EQ(resultFirstFirst, firstFirstHints);
    EXPECT_EQ(resultFirstSecond, firstSecondHints);
    EXPECT_EQ(resultSecondFirst, secondFirstHints);
    EXPECT_EQ(resultSecondSecond, secondSecondHints);
}
