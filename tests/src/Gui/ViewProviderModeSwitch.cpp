// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Gui/Inventor/SoFCSwitch.h>
#include <Gui/SoFCDB.h>
#include <Gui/ViewProvider.h>

#include <src/App/InitApplication.h>

namespace
{

/// A view provider with three display modes and nothing else.
class ThreeModeViewProvider: public Gui::ViewProvider
{
public:
    ThreeModeViewProvider()
    {
        addDisplayMaskMode(new SoSeparator, "First");
        addDisplayMaskMode(new SoSeparator, "Second");
        addDisplayMaskMode(new SoSeparator, "Third");
    }

    std::vector<std::string> getDisplayModes() const override
    {
        return {"First", "Second", "Third"};
    }
};

}  // namespace

/// Covers the mode switch fallback that lets a hidden object draw on top: its
/// defaultChild has to keep tracking the mode the object would otherwise show.
class ViewProviderModeSwitchTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        if (Gui::ViewProvider::getClassTypeId().isBad()) {
            Gui::ViewProvider::init();
        }
        SoDB::init();
        if (!Gui::SoFCDB::isInitialized()) {
            Gui::SoFCDB::init();
        }
    }

    int whichChild() const
    {
        return provider.getModeSwitch()->whichChild.getValue();
    }

    int defaultChild() const
    {
        return static_cast<SoFCSwitch*>(provider.getModeSwitch())->defaultChild.getValue();
    }

    ThreeModeViewProvider provider;
};

TEST_F(ViewProviderModeSwitchTest, TheModeSwitchIsAnSoFCSwitch)
{
    // without this the fallback below silently does nothing
    EXPECT_TRUE(provider.getModeSwitch()->isOfType(SoFCSwitch::getClassTypeId()));
}

TEST_F(ViewProviderModeSwitchTest, DefaultChildFollowsTheShownMode)
{
    provider.setDisplayMaskMode("Second");

    EXPECT_EQ(whichChild(), 1);
    EXPECT_EQ(defaultChild(), 1);
}

TEST_F(ViewProviderModeSwitchTest, DefaultChildFollowsTheModeWhileHidden)
{
    provider.setDisplayMaskMode("Second");
    provider.hide();
    ASSERT_EQ(whichChild(), -1);

    provider.setDefaultMode(2);

    // still hidden, but the fallback now points at what it would show
    EXPECT_EQ(whichChild(), -1);
    EXPECT_EQ(defaultChild(), 2);
}

TEST_F(ViewProviderModeSwitchTest, DefaultChildFollowsAnOverrideModeWhileShown)
{
    provider.setDisplayMaskMode("First");
    provider.setOverrideMode("Third");

    EXPECT_EQ(whichChild(), 2);
    EXPECT_EQ(defaultChild(), 2);
}

TEST_F(ViewProviderModeSwitchTest, DefaultChildFollowsAnOverrideModeWhileHidden)
{
    provider.setDisplayMaskMode("First");
    provider.hide();
    ASSERT_EQ(whichChild(), -1);

    provider.setOverrideMode("Third");

    EXPECT_EQ(whichChild(), -1);
    EXPECT_EQ(defaultChild(), 2);
}

TEST_F(ViewProviderModeSwitchTest, DefaultChildReturnsToTheActualModeWhenTheOverrideIsDropped)
{
    provider.setDisplayMaskMode("Second");
    provider.hide();
    provider.setOverrideMode("Third");
    ASSERT_EQ(defaultChild(), 2);

    provider.setOverrideMode("As Is");

    EXPECT_EQ(whichChild(), -1);
    EXPECT_EQ(defaultChild(), 1);
}
