#include <gtest/gtest.h>
#include "WorkbenchManager.h"

using namespace Gui;

// ============================================================================
// Test Case 1: The Initial State (Empty History)
// Goal: Verify the app doesn't crash when history is empty.
// ============================================================================
TEST(WorkbenchManagerTest, TestCase1_InitialState)
{
    WorkbenchManager manager;

    // Result: The previous and next lists must be completely empty.
    EXPECT_TRUE(manager.getPreviousWorkbenchList().empty());
    EXPECT_TRUE(manager.getNextWorkbenchList().empty());
    EXPECT_EQ(manager.getPreviousWorkbench(), nullptr);
    EXPECT_EQ(manager.getNextWorkbench(), nullptr);
}

// ============================================================================
// Test Case 2 Linear Walk
// Goal: Verify basic LIFO stack movement.
// ============================================================================
TEST(WorkbenchManagerTest, TestCase2_LinearWalk)
{
    WorkbenchManager manager;

    // Action: Change workbenches in order: Part -> Draft -> Sketcher -> TechDraw.
    manager.activate("Part", "Part");
    manager.activate("Draft", "Draft");
    manager.activate("Sketcher", "Sketcher");
    manager.activate("TechDraw", "TechDraw");

    // Action: Click the "Previous" button 3 times.
    manager.goToPreviousWorkbench();
    manager.activate("Sketcher", "Sketcher");
    manager.goToPreviousWorkbench();
    manager.activate("Draft", "Draft");
    manager.goToPreviousWorkbench();
    manager.activate("Part", "Part");

    // Result: You should be back on Part.
    EXPECT_EQ(manager.activeName(), "Part");

    // Result: "Next" list should have Draft, Sketcher, and TechDraw (Size 3).
    EXPECT_EQ(manager.getNextWorkbenchList().size(), 3);
}

// ============================================================================
// Test Case 3
// Goal: Verify the "Forward" stack is cleared when navigating to a new branch.
// ============================================================================
TEST(WorkbenchManagerTest, TestCase3_TheBranch)
{
    WorkbenchManager manager;

    // Setup: We are on Part, and Next list has Draft.
    manager.activate("Part", "Part");
    manager.activate("Draft", "Draft");
    manager.goToPreviousWorkbench();
    manager.activate("Part", "Part");

    // Verify setup is correct
    ASSERT_FALSE(manager.getNextWorkbenchList().empty());

    // Action: Select BIM (a brand new branch)
    manager.activate("BIM", "BIM");

    // Result: Active is BIM
    EXPECT_EQ(manager.activeName(), "BIM");

    // Result: CRITICAL - The "Next" history must be completely empty.
    EXPECT_TRUE(manager.getNextWorkbenchList().empty());
}

// ============================================================================
// Test Case 4:Over-clicking bounds
// Goal: Verify pointer safety when hitting the end of the lists.
// ============================================================================
TEST(WorkbenchManagerTest, TestCase4_TheWall)
{
    WorkbenchManager manager;
    manager.activate("Part", "Part");

    // Action: Try to go back when at the first workbench.
    // Result: getPreviousWorkbench returns nullptr. No crash.
    EXPECT_EQ(manager.getPreviousWorkbench(), nullptr);

    // Action: Try to go forward when at the most recent workbench.
    // Result: getNextWorkbench returns nullptr. No crash.
    EXPECT_EQ(manager.getNextWorkbench(), nullptr);
}

// ============================================================================
// Test Cases 5: Dropdown Jump Logic
// Goal: Verify jumpInHistory correctly moves multiple items between stacks.
// ============================================================================
TEST(WorkbenchManagerTest, TestCase5_JumpsAndBranches)
{
    WorkbenchManager manager;

    // Setup: Build history PartDesign -> Sketcher -> Draft -> TechDraw
    manager.activate("PartDesign", "PartDesign");
    manager.activate("Sketcher", "Sketcher");
    manager.activate("Draft", "Draft");
    manager.activate("TechDraw", "TechDraw");

    // Action: Jump to PartDesign (bottom of history)
    manager.jumpInHistory("PartDesign", "backwards");
    manager.activate("PartDesign", "PartDesign");

    EXPECT_EQ(manager.activeName(), "PartDesign");
    EXPECT_EQ(manager.getNextWorkbenchList().size(), 3);  // Sketcher, Draft, TechDraw are in the future

    // Action: Jump forward to TechDraw (top of Next stack)
    manager.jumpInHistory("TechDraw", "forward");
    manager.activate("TechDraw", "TechDraw");

    EXPECT_EQ(manager.activeName(), "TechDraw");
    EXPECT_TRUE(manager.getNextWorkbenchList().empty());      // No more future
    EXPECT_EQ(manager.getPreviousWorkbenchList().size(), 3);  // Past is restored

    // Action: Jump backwards by only 1 to Draft
    manager.jumpInHistory("Draft", "backwards");
    manager.activate("Draft", "Draft");

    EXPECT_EQ(manager.activeName(), "Draft");
    EXPECT_EQ(manager.getNextWorkbenchList().size(), 1);  // Only TechDraw is in the future

    // Action: User is on Draft, but activates BIM instead of going forward.
    manager.activate("BIM", "BIM");

    EXPECT_EQ(manager.activeName(), "BIM");
    EXPECT_TRUE(manager.getNextWorkbenchList().empty());
}

// ============================================================================
// Test Case 6: Duplicate Self-Navigation
// Goal: Ensure navigating to the same workbench doesn't create broken loops.
// ============================================================================
TEST(WorkbenchManagerTest, TestCase6_DuplicateSelfNavigation)
{
    WorkbenchManager manager;

    // Setup: Activate Sketcher
    manager.activate("Sketcher", "Sketcher");
    size_t historySize = manager.getPreviousWorkbenchList().size();

    // Action: Activate Sketcher AGAIN
    manager.activate("Sketcher", "Sketcher");

    // Result: History size should NOT grow (no duplicate push)
    EXPECT_EQ(manager.getPreviousWorkbenchList().size(), historySize);
}
