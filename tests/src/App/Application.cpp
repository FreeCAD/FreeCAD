// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <vector>
#define FC_OS_MACOSX 1
#include "App/Application.h"
#include "App/ProgramOptionsUtilities.h"
#include "Base/Sequencer.h"

#include <src/App/InitApplication.h>


using namespace App::Util;

using Spr = std::pair<std::string, std::string>;


class ApplicationTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

namespace
{

class RecordingSequencer: public Base::SequencerBase
{
public:
    std::string text;
    int lastProgress = -1;
    int checkAbortCalls = 0;
    bool throwOnCheckAbort = false;
    std::vector<int> progressUpdates;

protected:
    void setText(const char* pszTxt) override
    {
        text = pszTxt ? pszTxt : "";
    }

    void setProgress(size_t value) override
    {
        lastProgress = static_cast<int>(value);
        progressUpdates.push_back(lastProgress);
    }

    void checkAbort() override
    {
        ++checkAbortCalls;
        if (throwOnCheckAbort) {
            throw Base::AbortException("User aborted");
        }
    }
};

}  // namespace

TEST_F(ApplicationTest, fCustomSyntaxLookup)
{
    Spr res {customSyntax("-display")};
    Spr exp {"display", "null"};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxMac)
{
    Spr res {customSyntax("-psn_stuff")};
    Spr exp {"psn", "stuff"};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxWidgetCount)
{
    Spr res {customSyntax("-widgetcount")};
    Spr exp {"widgetcount", ""};
    EXPECT_EQ(res, exp);
}
TEST_F(ApplicationTest, fCustomSyntaxNotFound)
{
    Spr res {customSyntax("-displayx")};
    Spr exp {"", ""};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxAmpersand)
{
    Spr res {customSyntax("@freddie")};
    Spr exp {"response-file", "freddie"};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxEmptyIn)
{
    Spr res {customSyntax("")};
    Spr exp {"", ""};
    EXPECT_EQ(res, exp);
};

TEST_F(ApplicationTest, RecomputeProgressNestedScopesStayMonotonic)
{
    RecordingSequencer sequencer;
    App::RecomputeProgressHandle handle;

    auto outer = handle.makeScope("Outer");
    EXPECT_EQ(sequencer.text, "Outer");

    outer.setProgress(40);
    EXPECT_EQ(sequencer.lastProgress, 40);

    {
        auto inner = outer.makeScope("Inner");
        EXPECT_EQ(sequencer.text, "Inner");

        inner.setProgress(50);
        EXPECT_EQ(sequencer.lastProgress, 70);
    }

    EXPECT_EQ(sequencer.text, "Outer");
    EXPECT_EQ(sequencer.lastProgress, 70);

    outer.setProgress(30);
    EXPECT_EQ(sequencer.lastProgress, 70);

    outer.setProgress(90);
    EXPECT_EQ(sequencer.lastProgress, 90);
}

TEST_F(ApplicationTest, RecomputeProgressStepScopesPartitionParentRange)
{
    RecordingSequencer sequencer;
    App::RecomputeProgressHandle handle;

    auto outer = handle.makeScope("Outer");

    {
        auto firstStep = outer.makeStepScope(0, 4, "Step 1");
        firstStep.setProgress(100);
    }
    EXPECT_EQ(sequencer.lastProgress, 25);

    {
        auto secondStep = outer.makeStepScope(1, 4, "Step 2");
        secondStep.setProgress(50);
    }
    EXPECT_EQ(sequencer.lastProgress, 37);

    {
        auto lastStep = outer.makeStepScope(3, 4, "Step 4");
        lastStep.setProgress(100);
    }
    EXPECT_EQ(sequencer.lastProgress, 100);
}

TEST_F(ApplicationTest, RecomputeProgressFallsBackToParentText)
{
    RecordingSequencer sequencer;
    App::RecomputeProgressHandle handle;

    auto outer = handle.makeScope("Outer");
    EXPECT_EQ(sequencer.text, "Outer");

    {
        auto inner = outer.makeScope();
        inner.setProgress(10);
        EXPECT_EQ(sequencer.text, "Outer");

        inner.setText("Inner");
        EXPECT_EQ(sequencer.text, "Inner");
    }

    EXPECT_EQ(sequencer.text, "Outer");
}

TEST_F(ApplicationTest, RecomputeProgressCancelCheckpointUsesLegacySequencer)
{
    RecordingSequencer sequencer;

    EXPECT_NO_THROW(App::throwIfRecomputeCanceled());
    EXPECT_EQ(sequencer.checkAbortCalls, 1);

    sequencer.throwOnCheckAbort = true;
    EXPECT_THROW(App::throwIfRecomputeCanceled(), Base::AbortException);
    EXPECT_EQ(sequencer.checkAbortCalls, 2);
}

TEST_F(ApplicationTest, RecomputeProgressActivateClearsCancellationState)
{
    App::RecomputeProgressHandle handle;

    handle.cancel();
    EXPECT_TRUE(handle.wasCanceled());

    handle.activate();
    EXPECT_FALSE(handle.wasCanceled());
}

TEST_F(ApplicationTest, RecomputeProgressKeepsCompletedLeafTextOnScopeExit)
{
    RecordingSequencer sequencer;
    App::RecomputeProgressHandle handle;

    auto root = handle.makeScope("Transform");
    auto prepare = root.makeStepScope(0, 4, "Preparing transform...");
    prepare.setProgress(100);

    auto apply = root.makeStepScope(1, 4, "Applying transformed shape...");
    apply.setProgress(100);

    auto refine = root.makeStepScope(2, 4, "Refining transform...");
    refine.setProgress(100);

    {
        auto finalize = root.makeStepScope(3, 4, "Finalizing transform...");
        finalize.setProgress(100);
        EXPECT_EQ(sequencer.text, "Finalizing transform...");
        EXPECT_EQ(sequencer.lastProgress, 100);
    }

    EXPECT_EQ(sequencer.text, "Finalizing transform...");
    EXPECT_EQ(sequencer.lastProgress, 100);
}
