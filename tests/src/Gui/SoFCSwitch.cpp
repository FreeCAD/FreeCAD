// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoPath.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSeparator.h>

#include <Gui/Inventor/SoFCSwitch.h>
#include <Gui/SoFCDB.h>

#include <src/App/InitApplication.h>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

namespace
{

constexpr float SmallChild = 2.0F;
constexpr float LargeChild = 6.0F;

SoNode* makeCube(float width)
{
    auto* cube = new SoCube;
    cube->width = width;
    cube->height = width;
    cube->depth = width;
    return cube;
}

}  // namespace

/// Covers SoFCSwitch, which lets a hidden object render on top for a tree
/// preselection without changing its visibility.
class SoFCSwitchTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        SoDB::init();
        if (!Gui::SoFCDB::isInitialized()) {
            Gui::SoFCDB::init();
        }
    }

    void SetUp() override
    {
        root = new SoSeparator;
        root->ref();

        node = new SoFCSwitch;
        node->addChild(makeCube(SmallChild));
        node->addChild(makeCube(LargeChild));
        root->addChild(node);

        path = new SoPath(root);
        path->ref();
        path->append(node);
    }

    void TearDown() override
    {
        path->unref();
        root->unref();
    }

    /// Width of whatever a bounding box traversal from the root actually reached.
    float traversedWidth() const
    {
        SoGetBoundingBoxAction action(SbViewportRegion(100, 100));
        action.apply(root);
        const SbBox3f box = action.getBoundingBox();
        if (box.isEmpty()) {
            return 0.0F;
        }
        return box.getMax()[0] - box.getMin()[0];
    }

    SoSeparator* root = nullptr;
    SoFCSwitch* node = nullptr;
    SoPath* path = nullptr;
};

TEST_F(SoFCSwitchTest, HonorsWhichChildWithoutAnOverride)
{
    node->whichChild = 1;
    node->defaultChild = 0;

    EXPECT_FLOAT_EQ(traversedWidth(), LargeChild);
}

TEST_F(SoFCSwitchTest, DrawsNothingWhenOffWithoutAnOverride)
{
    // the whole point of defaultChild is that it stays dormant until scoped
    node->whichChild = SO_SWITCH_NONE;
    node->defaultChild = 0;

    EXPECT_FLOAT_EQ(traversedWidth(), 0.0F);
}

TEST_F(SoFCSwitchTest, FallsBackToTheDefaultChildUnderAnOverride)
{
    node->whichChild = SO_SWITCH_NONE;
    node->defaultChild = 0;

    SoFCSwitch::OverrideScope scope(path);

    EXPECT_FLOAT_EQ(traversedWidth(), SmallChild);
}

TEST_F(SoFCSwitchTest, KeepsTheShownChildUnderAnOverride)
{
    node->whichChild = 1;
    node->defaultChild = 0;

    SoFCSwitch::OverrideScope scope(path);

    EXPECT_FLOAT_EQ(traversedWidth(), LargeChild);
}

TEST_F(SoFCSwitchTest, LeavesSwitchesOffThePathAlone)
{
    node->whichChild = SO_SWITCH_NONE;
    node->defaultChild = 0;

    auto* other = new SoFCSwitch;
    other->addChild(makeCube(LargeChild));
    other->whichChild = SO_SWITCH_NONE;
    other->defaultChild = 0;
    root->addChild(other);

    SoFCSwitch::OverrideScope scope(path);

    // only the scoped switch falls back; the other stays hidden
    EXPECT_FLOAT_EQ(traversedWidth(), SmallChild);
}

TEST_F(SoFCSwitchTest, DrawsNothingWhenTheDefaultChildIsOutOfRange)
{
    node->whichChild = SO_SWITCH_NONE;
    node->defaultChild = 7;

    SoFCSwitch::OverrideScope scope(path);

    EXPECT_FLOAT_EQ(traversedWidth(), 0.0F);
}

TEST_F(SoFCSwitchTest, ANullScopeSuppressesAnOuterOverride)
{
    // callers pass `condition ? path : nullptr`, so a null scope must switch off
    node->whichChild = SO_SWITCH_NONE;
    node->defaultChild = 0;

    SoFCSwitch::OverrideScope outer(path);
    ASSERT_FLOAT_EQ(traversedWidth(), SmallChild);

    {
        SoFCSwitch::OverrideScope inner(nullptr);
        EXPECT_FLOAT_EQ(traversedWidth(), 0.0F);
    }

    EXPECT_FLOAT_EQ(traversedWidth(), SmallChild);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
