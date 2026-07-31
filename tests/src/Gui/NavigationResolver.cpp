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

#include <Gui/Navigation/NavigationResolver.h>
#include <Gui/Navigation/NavigationStyle.h>

#include <src/App/InitApplication.h>

namespace
{

using Gui::NavigationInputState;
using Gui::NavigationStyle;

constexpr NavigationInputState inputState(
    const bool left,
    const bool middle,
    const bool right,
    const bool ctrl = false,
    const bool shift = false,
    const bool alt = false
)
{
    return {.left = left, .middle = middle, .right = right, .ctrl = ctrl, .shift = shift, .alt = alt};
}

Gui::ResolutionResult resolveInput(
    const Gui::NavigationProfile& profile,
    const NavigationStyle::ViewerMode currentMode,
    const unsigned int chord,
    const std::optional<Gui::GestureOwnership>& activeGesture = std::nullopt,
    const std::optional<NavigationStyle::ViewerMode>& requestedMode = std::nullopt
)
{
    return Gui::resolveNavigation(
        profile,
        {
            .currentMode = currentMode,
            .chord = chord,
            .requestedMode = requestedMode,
            .activeGesture = activeGesture,
        }
    );
}

template<typename Style>
class StyleProbe: public Style
{
public:
    const Gui::NavigationProfile& profileForTest() const
    {
        return this->profile();
    }
};

template<typename Style>
void expectStandardProfileRulesResolveExactly(const char* styleName)
{
    tests::initApplication();
    StyleProbe<Style> style;
    const Gui::NavigationProfile& profile = style.profileForTest();

    for (const Gui::NavigationRule& rule : profile.rules) {
        SCOPED_TRACE(styleName);
        SCOPED_TRACE(rule.chord);
        const NavigationStyle::ViewerMode currentMode = rule.fromMode.value_or(NavigationStyle::IDLE);
        const Gui::ResolutionResult result = resolveInput(profile, currentMode, rule.chord);

        EXPECT_EQ(result.mode, rule.toMode);
        EXPECT_EQ(result.activeGesture.has_value(), rule.ownership.buttons != 0U);
        if (result.activeGesture) {
            EXPECT_EQ(result.activeGesture->buttons, rule.ownership.buttons);
            EXPECT_EQ(result.activeGesture->ownerMatch, rule.ownership.ownerMatch);
        }
    }
}

template<typename Style>
void expectStandardProfileInvariants(const char* styleName)
{
    tests::initApplication();
    StyleProbe<Style> style;
    const Gui::NavigationProfile& profile = style.profileForTest();

    SCOPED_TRACE(styleName);
    EXPECT_NE(profile.selectionDescription, nullptr);
    EXPECT_NE(profile.panDescription, nullptr);
    EXPECT_NE(profile.rotateDescription, nullptr);
    EXPECT_NE(profile.zoomDescription, nullptr);

    for (std::size_t i = 0; i < profile.rules.size(); ++i) {
        const Gui::NavigationRule& rule = profile.rules[i];
        SCOPED_TRACE(rule.chord);

        EXPECT_EQ(rule.ownership.buttons & ~NavigationInputState::ButtonMask, 0U);
        if (rule.ownership.ownerMatch == Gui::OwnerMatch::AnyHeld) {
            EXPECT_NE(rule.ownership.buttons, 0U);
            EXPECT_NE(rule.ownership.buttons & (rule.ownership.buttons - 1U), 0U);
        }

        for (std::size_t previous = 0; previous < i; ++previous) {
            const Gui::NavigationRule& previousRule = profile.rules[previous];
            EXPECT_FALSE(rule.fromMode == previousRule.fromMode && rule.chord == previousRule.chord);
        }
    }
}

TEST(NavigationResolverTest, unmappedInputUsesResolverFallbackRules)
{
    const Gui::ResolutionResult endsMotion = Gui::resolveNavigation(
        {},
        {
            .currentMode = NavigationStyle::DRAGGING,
            .chord = NavigationInputState::RightDown,
            .requestedMode = std::nullopt,
            .activeGesture = std::nullopt,
        }
    );
    EXPECT_EQ(endsMotion.mode, NavigationStyle::IDLE);

    constexpr Gui::NavigationProfile preservedProfile {
        .rules = {},
        .preserveModeOnUnmappedInput = true,
    };
    const Gui::ResolutionResult preservesMotion = Gui::resolveNavigation(
        preservedProfile,
        {
            .currentMode = NavigationStyle::DRAGGING,
            .chord = NavigationInputState::RightDown,
            .requestedMode = std::nullopt,
            .activeGesture = std::nullopt,
        }
    );
    EXPECT_EQ(preservesMotion.mode, NavigationStyle::DRAGGING);
}

TEST(NavigationResolverTest, profileRulesResolveExactly)
{
    static constexpr Gui::NavigationRule rules[] {
        {std::nullopt, NavigationInputState::LeftDown, NavigationStyle::SELECTION},
        {std::nullopt,
         NavigationInputState::MiddleDown,
         NavigationStyle::PANNING,
         {.buttons = NavigationInputState::MiddleDown}},
        {std::nullopt,
         NavigationInputState::ShiftDown | NavigationInputState::MiddleDown,
         NavigationStyle::DRAGGING,
         {.buttons = NavigationInputState::MiddleDown}},
        {NavigationStyle::PANNING,
         NavigationInputState::MiddleDown | NavigationInputState::RightDown,
         NavigationStyle::DRAGGING,
         {.buttons = NavigationInputState::MiddleDown | NavigationInputState::RightDown,
          .ownerMatch = Gui::OwnerMatch::AnyHeld}},
    };
    constexpr Gui::NavigationProfile profile {rules};

    for (const Gui::NavigationRule& rule : profile.rules) {
        SCOPED_TRACE(rule.chord);
        const NavigationStyle::ViewerMode currentMode = rule.fromMode.value_or(NavigationStyle::IDLE);
        const Gui::ResolutionResult result = resolveInput(profile, currentMode, rule.chord);

        EXPECT_EQ(result.mode, rule.toMode);
        EXPECT_EQ(result.activeGesture.has_value(), rule.ownership.buttons != 0U);
        if (result.activeGesture) {
            EXPECT_EQ(result.activeGesture->buttons, rule.ownership.buttons);
            EXPECT_EQ(result.activeGesture->ownerMatch, rule.ownership.ownerMatch);
        }
    }
}

TEST(NavigationResolverTest, standardProfilesResolveEveryRuleExactly)
{
    expectStandardProfileRulesResolveExactly<Gui::BlenderNavigationStyle>("Blender");
    expectStandardProfileRulesResolveExactly<Gui::RevitNavigationStyle>("Revit");
    expectStandardProfileRulesResolveExactly<Gui::SolidWorksNavigationStyle>("SolidWorks");
    expectStandardProfileRulesResolveExactly<Gui::OpenCascadeNavigationStyle>("OpenCascade");
    expectStandardProfileRulesResolveExactly<Gui::InventorNavigationStyle>("Inventor");
    expectStandardProfileRulesResolveExactly<Gui::TouchpadNavigationStyle>("Touchpad");
    expectStandardProfileRulesResolveExactly<Gui::TinkerCADNavigationStyle>("TinkerCAD");
    expectStandardProfileRulesResolveExactly<Gui::OpenSCADNavigationStyle>("OpenSCAD");
    expectStandardProfileRulesResolveExactly<Gui::CADNavigationStyle>("CAD");
}

TEST(NavigationResolverTest, standardProfilesSatisfyConfigurationInvariants)
{
    expectStandardProfileInvariants<Gui::BlenderNavigationStyle>("Blender");
    expectStandardProfileInvariants<Gui::RevitNavigationStyle>("Revit");
    expectStandardProfileInvariants<Gui::SolidWorksNavigationStyle>("SolidWorks");
    expectStandardProfileInvariants<Gui::OpenCascadeNavigationStyle>("OpenCascade");
    expectStandardProfileInvariants<Gui::InventorNavigationStyle>("Inventor");
    expectStandardProfileInvariants<Gui::TouchpadNavigationStyle>("Touchpad");
    expectStandardProfileInvariants<Gui::TinkerCADNavigationStyle>("TinkerCAD");
    expectStandardProfileInvariants<Gui::OpenSCADNavigationStyle>("OpenSCAD");
    expectStandardProfileInvariants<Gui::CADNavigationStyle>("CAD");
}

TEST(NavigationResolverTest, allHeldGestureEndsForEveryOwnerRelease)
{
    static constexpr Gui::NavigationRule bindings[] {
        {std::nullopt,
         NavigationInputState::MiddleDown | NavigationInputState::RightDown,
         NavigationStyle::DRAGGING,
         {.buttons = NavigationInputState::MiddleDown | NavigationInputState::RightDown}},
    };
    constexpr Gui::NavigationProfile profile {bindings};

    const Gui::ResolutionResult started
        = resolveInput(profile, NavigationStyle::IDLE, inputState(false, true, true).chord());
    ASSERT_TRUE(started.activeGesture);

    const struct
    {
        NavigationInputState input;
        bool continues;
    } cases[] {
        {inputState(false, true, true), true},
        {inputState(false, true, false), false},
        {inputState(false, false, true), false},
        {inputState(false, false, false), false},
    };

    for (const auto& testCase : cases) {
        const Gui::ResolutionResult result = resolveInput(
            profile,
            NavigationStyle::DRAGGING,
            testCase.input.chord(),
            started.activeGesture
        );
        EXPECT_EQ(result.mode, testCase.continues ? NavigationStyle::DRAGGING : NavigationStyle::IDLE);
        EXPECT_EQ(result.activeGesture.has_value(), testCase.continues);
    }
}

TEST(NavigationResolverTest, modeSpecificRuleTakesPriorityOverGlobalRule)
{
    static constexpr Gui::NavigationRule rules[] {
        {std::nullopt,
         NavigationInputState::MiddleDown | NavigationInputState::RightDown,
         NavigationStyle::PANNING},
        {NavigationStyle::PANNING,
         NavigationInputState::MiddleDown | NavigationInputState::RightDown,
         NavigationStyle::DRAGGING},
    };
    constexpr Gui::NavigationProfile profile {rules};

    const Gui::ResolutionResult result
        = resolveInput(profile, NavigationStyle::PANNING, inputState(false, true, true).chord());

    EXPECT_EQ(result.mode, NavigationStyle::DRAGGING);
}

TEST(NavigationResolverTest, transitionOwnershipContinuesWithEitherButton)
{
    constexpr Gui::GestureOwnership ownership {
        .buttons = NavigationInputState::MiddleDown | NavigationInputState::RightDown,
        .ownerMatch = Gui::OwnerMatch::AnyHeld,
    };
    static constexpr Gui::NavigationRule transitions[] {
        {NavigationStyle::PANNING,
         NavigationInputState::MiddleDown | NavigationInputState::RightDown,
         NavigationStyle::DRAGGING,
         ownership},
    };
    constexpr Gui::NavigationProfile profile {transitions};

    const Gui::ResolutionResult started = Gui::resolveNavigation(
        profile,
        {
            .currentMode = NavigationStyle::PANNING,
            .chord = inputState(false, true, true).chord(),
            .requestedMode = std::nullopt,
            .activeGesture = std::nullopt,
        }
    );
    ASSERT_TRUE(started.activeGesture);

    const Gui::ResolutionResult continued = Gui::resolveNavigation(
        profile,
        {
            .currentMode = NavigationStyle::DRAGGING,
            .chord = inputState(false, false, true).chord(),
            .requestedMode = std::nullopt,
            .activeGesture = started.activeGesture,
        }
    );

    EXPECT_EQ(continued.mode, NavigationStyle::DRAGGING);
    EXPECT_TRUE(continued.activeGesture);

    const Gui::ResolutionResult continuedWithMiddle = resolveInput(
        profile,
        NavigationStyle::DRAGGING,
        inputState(false, true, false).chord(),
        started.activeGesture
    );
    EXPECT_EQ(continuedWithMiddle.mode, NavigationStyle::DRAGGING);
    EXPECT_TRUE(continuedWithMiddle.activeGesture);

    const Gui::ResolutionResult ended = Gui::resolveNavigation(
        profile,
        {
            .currentMode = NavigationStyle::DRAGGING,
            .chord = inputState(false, false, false).chord(),
            .requestedMode = std::nullopt,
            .activeGesture = continued.activeGesture,
        }
    );

    EXPECT_EQ(ended.mode, NavigationStyle::IDLE);
    EXPECT_FALSE(ended.activeGesture);
}

TEST(NavigationResolverTest, requestedModeResolvesWithoutRule)
{
    const Gui::ResolutionResult result = Gui::resolveNavigation(
        {},
        {
            .currentMode = NavigationStyle::PANNING,
            .chord = inputState(false, true, true).chord(),
            .requestedMode = NavigationStyle::DRAGGING,
            .activeGesture = std::nullopt,
        }
    );

    EXPECT_EQ(result.mode, NavigationStyle::DRAGGING);
}

TEST(NavigationResolverTest, requestedModeOverridesGlobalRule)
{
    static constexpr Gui::NavigationRule rules[] {
        {std::nullopt, NavigationInputState::LeftDown, NavigationStyle::SELECTION},
    };
    constexpr Gui::NavigationProfile profile {rules};

    const Gui::ResolutionResult result = Gui::resolveNavigation(
        profile,
        {
            .currentMode = NavigationStyle::SELECTION,
            .chord = NavigationInputState::LeftDown,
            .requestedMode = NavigationStyle::DRAGGING,
            .activeGesture = std::nullopt,
        }
    );

    EXPECT_EQ(result.mode, NavigationStyle::DRAGGING);
}

TEST(NavigationResolverTest, modeSpecificRulePrecedesRequestedMode)
{
    static constexpr Gui::NavigationRule transitions[] {
        {NavigationStyle::PANNING,
         NavigationInputState::MiddleDown | NavigationInputState::RightDown,
         NavigationStyle::DRAGGING,
         {}},
    };
    constexpr Gui::NavigationProfile profile {transitions};

    const Gui::ResolutionResult result = Gui::resolveNavigation(
        profile,
        {
            .currentMode = NavigationStyle::PANNING,
            .chord = inputState(false, true, true).chord(),
            .requestedMode = NavigationStyle::IDLE,
            .activeGesture = std::nullopt,
        }
    );

    EXPECT_EQ(result.mode, NavigationStyle::DRAGGING);
}

}  // namespace
